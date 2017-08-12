/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#include "lullaby/systems/scroll/scroll_content_layout_system.h"

#include "lullaby/generated/scroll_def_generated.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/systems/scroll/scroll_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"

namespace lull {
namespace {

const HashValue kScrollContentLayoutDefHash = Hash("ScrollContentLayoutDef");
}

ScrollContentLayoutSystem::ScrollContentLayoutSystem(Registry* registry)
    : System(registry), contents_(2) {
  RegisterDef(this, kScrollContentLayoutDefHash);
  RegisterDependency<TransformSystem>(this);

  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const ChildAddedEvent& event) {
    UpdateScrollContentBounds(event.target);
  });
  dispatcher->Connect(this, [this](const ChildRemovedEvent& event) {
    UpdateScrollContentBounds(event.target);
  });
  dispatcher->Connect(this, [this](const AabbChangedEvent& event) {
    TransformSystem* transform_system = registry_->Get<TransformSystem>();
    const Entity parent = transform_system->GetParent(event.target);
    UpdateScrollContentBounds(parent);
  });
}

ScrollContentLayoutSystem::~ScrollContentLayoutSystem() {
  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->DisconnectAll(this);
}

void ScrollContentLayoutSystem::Create(Entity entity, HashValue type,
                                       const Def* def) {
  if (type != kScrollContentLayoutDefHash) {
    LOG(DFATAL)
        << "Invalid type passed to Create. Expecting ScrollContentLayoutDef!";
    return;
  }
  const auto* data = ConvertDef<ScrollContentLayoutDef>(def);

  Content* content = contents_.Emplace(entity);
  content->min_padding =
      mathfu::vec3(data->left_padding(), data->bottom_padding(), 0.f);
  content->max_padding =
      mathfu::vec3(data->right_padding(), data->top_padding(), 0.f);
}

void ScrollContentLayoutSystem::PostCreateInit(Entity entity, HashValue type,
                                               const Def* def) {
  UpdateScrollContentBounds(entity);
}

void ScrollContentLayoutSystem::Destroy(Entity entity) {
  contents_.Destroy(entity);
}

void ScrollContentLayoutSystem::UpdateScrollContentBounds(Entity entity) {
  Content* content = contents_.Get(entity);
  if (content == nullptr) {
    return;
  }

  TransformSystem* transform_system = registry_->Get<TransformSystem>();
  const std::vector<Entity>* children = transform_system->GetChildren(entity);
  if (!children || children->empty()) {
    return;
  }

  Aabb bounds;
  bool bounds_set = false;
  for (const Entity child : *children) {
    const Sqt* sqt = transform_system->GetSqt(child);
    const Aabb* aabb = transform_system->GetAabb(child);
    if (!aabb || !sqt) {
      continue;
    }

    const Aabb transformed_aabb = TransformAabb(*sqt, *aabb);
    if (bounds_set) {
      bounds = MergeAabbs(bounds, transformed_aabb);
    } else {
      bounds = transformed_aabb;
      bounds_set = true;
    }
  }

  if (bounds_set) {
    // The current view offset is a translation that is already applied to all
    // child entities. It must be un-done, else it is permanently "applied" to
    // scroll layout.
    ScrollSystem* scroll_system = registry_->Get<ScrollSystem>();
    mathfu::vec2 view_offset = scroll_system->GetViewOffset(entity);
    Sqt offset_sqt;
    offset_sqt.translation = mathfu::vec3(view_offset.x, view_offset.y, 0.f);
    bounds = TransformAabb(offset_sqt, bounds);

    bounds.min += content->min_padding;
    bounds.max += content->max_padding;
    if (bounds.max.x < bounds.min.x) {
      bounds.min.x = bounds.max.x;
    }
    if (bounds.max.y < bounds.min.y) {
      bounds.min.y = bounds.max.y;
    }
    scroll_system->SetContentBounds(entity, bounds);
  }
}

}  // namespace lull
