/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lullaby/contrib/clip/clip_system.h"

#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"

namespace lull {
static constexpr HashValue kClipDefHash = ConstHash("ClipDef");
static constexpr int kDefaultAutomaticStencilStartValue = 81;
static constexpr int kMaxStencilValue = 256;

ClipSystem::ClipSystem(Registry* registry)
    : System(registry),
      regions_(16),
      targets_(16),
      next_stencil_value_(kDefaultAutomaticStencilStartValue),
      auto_stencil_start_value_(kDefaultAutomaticStencilStartValue) {
  RegisterDef<ClipDefT>(this);

  RegisterDependency<Dispatcher>(this);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

ClipSystem::~ClipSystem() {
  // The Dispatcher might be destroyed before the ClipSystem, so we need to
  // check the pointer first before using it.
  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->Disconnect<ParentChangedImmediateEvent>(this);
  }
}

void ClipSystem::Initialize() {
  // Attach to the immediate parent changed event since this has render
  // implications which don't want to be delayed a frame.
  auto* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const ParentChangedImmediateEvent& event) {
    OnParentChanged(event);
  });
}

void ClipSystem::Create(Entity e, HashValue type, const Def* def) {
  if (type != kClipDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting ClipDef!";
    return;
  }
  auto data = ConvertDef<ClipDef>(def);

  ClipRegion* region = regions_.Emplace(e);
  if (data->stencil_reference_value()) {
    region->stencil_value = data->stencil_reference_value();
    if (region->stencil_value >= auto_stencil_start_value_) {
      LOG(DFATAL) << "Cannot have stencil value >= than "
                  << auto_stencil_start_value_;
      region->stencil_value = kDefaultAutomaticStencilStartValue;
    }
  } else {
    region->stencil_value = next_stencil_value_;
    ++next_stencil_value_;
    if (next_stencil_value_ >= kMaxStencilValue) {
      LOG(DFATAL) << "Exceeded max stencil value!";
      next_stencil_value_ = kMaxStencilValue - 1;
    }
  }

  RenderSystem* render_system = registry_->Get<RenderSystem>();
  render_system->SetStencilMode(e, RenderStencilMode::kWrite,
                                region->stencil_value);
}

void ClipSystem::Create(Entity e) {
  ClipRegion* region = regions_.Emplace(e);
  if (!region) {
    // Clip region exists already for given entity e.
    return;
  }

  region->stencil_value = next_stencil_value_;
  ++next_stencil_value_;
  if (next_stencil_value_ >= kMaxStencilValue) {
    LOG(DFATAL) << "Exceeded max stencil value!";
    next_stencil_value_ = kMaxStencilValue - 1;
  }

  RenderSystem* render_system = registry_->Get<RenderSystem>();
  render_system->SetStencilMode(e, RenderStencilMode::kWrite,
                                region->stencil_value);
}

void ClipSystem::SetReferenceValue(Entity e, const int value) {
  ClipRegion* region = regions_.Get(e);
  if (!region) {
    return;
  }

  if (value < auto_stencil_start_value_) {
    region->stencil_value = value;
  } else {
    LOG(DFATAL) << "Cannot have stencil value >= than "
                << auto_stencil_start_value_;
    region->stencil_value = kDefaultAutomaticStencilStartValue;
  }

  // We've changed the stencil value; update all affected values in the render
  // system if this region's enabled.
  if (region->enabled) {
    RenderSystem* render_system = registry_->Get<RenderSystem>();
    render_system->SetStencilMode(e, RenderStencilMode::kWrite,
                                  region->stencil_value);

    targets_.ForEach([render_system, region](const ClipTarget& target) {
      if (target.region == region->GetEntity()) {
        render_system->SetStencilMode(target.GetEntity(),
                                      RenderStencilMode::kTest,
                                      region->stencil_value);
      }
    });
  }
}

void ClipSystem::PostCreateInit(Entity e, HashValue type, const Def* def) {
  if (type != kClipDefHash) {
    LOG(DFATAL) << "Invalid type passed to PostCreateInit. Expecting ClipDef!";
    return;
  }
  ClipRegion* region = regions_.Get(e);
  if (region) {
    TransformSystem* transform_system = registry_->Get<TransformSystem>();
    const std::vector<Entity>* children = transform_system->GetChildren(e);
    if (children != nullptr) {
      for (const auto& child : *children) {
        AddTarget(child, e);
      }
    }
  }
}

void ClipSystem::DisableStencil(Entity e) {
  ClipRegion* region = regions_.Get(e);
  if (region) {
    region->enabled = false;
    RenderSystem* render_system = registry_->Get<RenderSystem>();
    render_system->SetStencilMode(e, RenderStencilMode::kDisabled,
                                  region->stencil_value);
    targets_.ForEach([render_system, region](const ClipTarget& target) {
      if (target.region == region->GetEntity()) {
        render_system->SetStencilMode(target.GetEntity(),
                                      RenderStencilMode::kDisabled,
                                      region->stencil_value);
      }
    });
  }
}

void ClipSystem::EnableStencil(Entity e) {
  ClipRegion* region = regions_.Get(e);
  if (region) {
    region->enabled = true;
    RenderSystem* render_system = registry_->Get<RenderSystem>();
    render_system->SetStencilMode(e, RenderStencilMode::kWrite,
                                  region->stencil_value);
    targets_.ForEach([render_system, region](const ClipTarget& target) {
      if (target.region == region->GetEntity()) {
        render_system->SetStencilMode(target.GetEntity(),
                                      RenderStencilMode::kTest,
                                      region->stencil_value);
      }
    });
  }
}

void ClipSystem::Destroy(Entity e) {
  ClipRegion* region = regions_.Get(e);
  if (region) {
    TransformSystem* transform_system = registry_->Get<TransformSystem>();
    const std::vector<Entity>* children = transform_system->GetChildren(e);
    if (children != nullptr) {
      for (const auto& child : *children) {
        RemoveTarget(child);
      }
    }

    regions_.Destroy(e);
  }

  // Reset the stencil counter when we destroy all the clip regions.  This is to
  // prevent the case where we run out of values when the process remains alive
  // between runs.
  if (regions_.Size() == 0) {
    next_stencil_value_ = kDefaultAutomaticStencilStartValue;
  }
}

void ClipSystem::OnParentChanged(const ParentChangedImmediateEvent& event) {
  const Entity new_region = GetRegion(event.new_parent);
  const ClipTarget* target = targets_.Get(event.target);
  if (target && !new_region) {
    RemoveTarget(event.target);
  } else if (new_region) {
    AddTarget(event.target, new_region);
  }
}

void ClipSystem::AddTarget(Entity target_entity, Entity region_entity) {
  ClipTarget* target = targets_.Get(target_entity);
  if (!target) {
    target = targets_.Emplace(target_entity);
  }
  target->region = region_entity;

  ClipRegion* region = regions_.Get(region_entity);
  if (region == nullptr) {
    return;
  }
  if (region->enabled) {
    RenderSystem* render_system = registry_->Get<RenderSystem>();
    render_system->SetStencilMode(target_entity, RenderStencilMode::kTest,
                                  region->stencil_value);
  }

  TransformSystem* transform_system = registry_->Get<TransformSystem>();
  const std::vector<Entity>* children =
      transform_system->GetChildren(target_entity);
  if (children != nullptr) {
    for (auto& child : *children) {
      AddTarget(child, region_entity);
    }
  }
}

void ClipSystem::RemoveTarget(Entity e) {
  RenderSystem* render_system = registry_->Get<RenderSystem>();
  render_system->SetStencilMode(e, RenderStencilMode::kDisabled, 0);

  targets_.Destroy(e);

  TransformSystem* transform_system = registry_->Get<TransformSystem>();
  const std::vector<Entity>* children = transform_system->GetChildren(e);
  if (children != nullptr) {
    for (auto& child : *children) {
      RemoveTarget(child);
    }
  }
}

// TODO cull in AdvanceFrame

Entity ClipSystem::GetRegion(Entity e) const {
  const ClipRegion* region = regions_.Get(e);
  if (region) {
    return e;
  }
  const ClipTarget* target = targets_.Get(e);
  if (target) {
    return target->region;
  }
  return kNullEntity;
}

}  // namespace lull
