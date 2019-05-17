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

#include "lullaby/contrib/shader_clip/shader_clip_system.h"

#include "lullaby/generated/shader_clip_def_generated.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"

namespace lull {
namespace {
// Specifies the number of elements to store in contiguous memory before
// allocating a new "page" for more elements.
constexpr size_t kRegionComponentPoolPageSize = 1;
constexpr size_t kTargetComponentPoolPageSize = 64;
constexpr HashValue kShaderClipDefHash = ConstHash("ShaderClipDef");
constexpr HashValue kShaderClipTargetDefHash = ConstHash("ShaderClipTargetDef");
constexpr char kMinInClipRegionSpace[] = "min_in_clip_region_space";
constexpr char kMaxInClipRegionSpace[] = "max_in_clip_region_space";
constexpr char kClipRegionFromModelSpaceMatrix[] =
    "clip_region_from_model_space_matrix";
}  // namespace

ShaderClipSystem::ClipRegion::ClipRegion(Entity entity)
    : Component(entity) {}

ShaderClipSystem::ClipTarget::ClipTarget(Entity entity)
    : Component(entity), region(kNullEntity), manually_enabled(false) {}

ShaderClipSystem::ShaderClipSystem(Registry* registry)
    : System(registry),
      clip_regions_(kRegionComponentPoolPageSize),
      clip_targets_(kTargetComponentPoolPageSize),
      disabled_clip_targets_(kTargetComponentPoolPageSize) {
  RegisterDef<ShaderClipDefT>(this);
  RegisterDef<ShaderClipTargetDefT>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<RenderSystem>(this);

  // Attach to the immediate parent changed event since this has render
  // implications which don't want to be delayed a frame.
  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const ParentChangedImmediateEvent& event) {
    OnParentChanged(event);
  });
  dispatcher->Connect(this, [this](const OnDisabledEvent& event) {
    OnDisabled(event.target);
  });
  dispatcher->Connect(this, [this](const OnEnabledEvent& event) {
    OnEnabled(event.target);
  });
}

ShaderClipSystem::~ShaderClipSystem() {
  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Disconnect<ParentChangedImmediateEvent>(this);
  dispatcher->Disconnect<OnDisabledEvent>(this);
  dispatcher->Disconnect<OnEnabledEvent>(this);
}

void ShaderClipSystem::AddTarget(Entity region, Entity target) {
  AddTargetRecursive(region, target);
  GetTarget(target)->manually_enabled = true;
}

void ShaderClipSystem::AddTargetRecursive(Entity region, Entity target) {
  auto* transform_system = registry_->Get<TransformSystem>();
  auto* render_system = registry_->Get<RenderSystem>();
  ClipTarget* clip_target = GetTarget(target);
  // Only create the target if it doesn't exist already.
  if (!clip_target) {
    if (transform_system->IsEnabled(target)) {
      clip_target = clip_targets_.Emplace(target);
    } else {
      clip_target = disabled_clip_targets_.Emplace(target);
    }

    // Make sure this uniform is set at least once since Update() uses hashed
    // version of SetUniform().
    const std::vector<float> data(16, 0.f);
    render_system->SetUniform(target, kClipRegionFromModelSpaceMatrix,
                              data.data(), 16 /* dimension */);
  }

  // If there is no region don't continue.
  if (region == kNullEntity) {
    ResetUniforms(target);
    return;
  }

  // If the new region is the same as the old, return early.
  if (clip_target->region == region) {
    return;
  }

  const ClipRegion* clip_region = clip_regions_.Get(region);
  if (!clip_region) {
    LOG(ERROR) << "Could not find region in ShaderClipSystem";
    return;
  }

  clip_target->region = region;

  // Set shader uniforms.
  render_system->SetUniform(target, kMinInClipRegionSpace,
                            &clip_region->min_in_clip_region_space[0],
                            3 /* dimension */);
  render_system->SetUniform(target, kMaxInClipRegionSpace,
                            &clip_region->max_in_clip_region_space[0],
                            3 /* dimension */);
  // Reset the cache so that it will be changed in Update().
  clip_target->world_from_model_matrix = mathfu::mat4::Identity();

  for (Entity child : *transform_system->GetChildren(target)) {
    AddTargetRecursive(region, child);
  }
}

void ShaderClipSystem::TryAddTargetRecursive(Entity region, Entity target) {
  const auto* clip_target = GetTarget(target);
  if (clip_target && clip_target->region == kNullEntity) {
    AddTargetRecursive(region, target);
  } else {
    auto* transform_system = registry_->Get<TransformSystem>();
    for (Entity child : *transform_system->GetChildren(target)) {
      TryAddTargetRecursive(region, child);
    }
  }
}

void ShaderClipSystem::Update() {
  // Store the inverse of the world from clip region matrix.
  const auto* transform_system = registry_->Get<TransformSystem>();
  clip_regions_.ForEach([transform_system](ClipRegion& region) {
    const mathfu::mat4* world_from_clip_region_matrix =
        transform_system->GetWorldFromEntityMatrix(region.GetEntity());
    if (!world_from_clip_region_matrix) {
      return;
    }
    region.world_from_clip_region_matrix_changed =
        region.world_from_clip_region_matrix != *world_from_clip_region_matrix;
    if (region.world_from_clip_region_matrix_changed) {
      region.world_from_clip_region_matrix = *world_from_clip_region_matrix;
      region.clip_region_from_world_matrix =
          world_from_clip_region_matrix->Inverse();
    }
  });

  // Set the uniform for all targets.
  auto* render_system = registry_->Get<RenderSystem>();
  clip_targets_.ForEach(
      [this, transform_system, render_system](ClipTarget& target) {
        if (target.region == kNullEntity) return;

        const ClipRegion* region = clip_regions_.Get(target.region);
        if (!region) {
          LOG(ERROR) << "Clip Target's Region not found.";
          return;
        }

        const mathfu::mat4* world_from_model_matrix =
            transform_system->GetWorldFromEntityMatrix(target.GetEntity());
        if (!world_from_model_matrix) {
          return;
        }
        if (region->world_from_clip_region_matrix_changed ||
            target.world_from_model_matrix != *world_from_model_matrix) {
          target.world_from_model_matrix = *world_from_model_matrix;
          const mathfu::mat4 clip_region_from_model_space_matrix =
              region->clip_region_from_world_matrix *
              (*world_from_model_matrix);
          render_system->SetUniform(target.GetEntity(),
                                    kClipRegionFromModelSpaceMatrix,
                                    &clip_region_from_model_space_matrix(0, 0),
                                    16 /* dimension */);
        }
  });
}

void ShaderClipSystem::Create(Entity entity, HashValue type,
                              const Def* def) {
  if (type == kShaderClipDefHash) {
    auto* data = ConvertDef<ShaderClipDef>(def);
    ClipRegion* region = clip_regions_.Emplace(entity);
    MathfuVec3FromFbVec3(data->min_in_clip_region_space(),
                         &region->min_in_clip_region_space);
    MathfuVec3FromFbVec3(data->max_in_clip_region_space(),
                         &region->max_in_clip_region_space);
  } else if (type == kShaderClipTargetDefHash) {
    AddTarget(GetContainingRegion(entity), entity);
  } else {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting ShaderClipDef or "
                << "ShaderClipTargetDef!";
  }
}

void ShaderClipSystem::Destroy(Entity entity) {
  // Check if |entity| is a target and delete it if so.
  if (GetTarget(entity)) {
    ResetUniforms(entity);
    DestroyTarget(entity);
  }

  // Check if |entity| is a region. If so, delete the region and all the targets
  // connected to the region.
  if (const ClipRegion* region = clip_regions_.Get(entity)) {
    std::vector<Entity> targets_to_be_destroyed;
    clip_targets_.ForEach([&](const ClipTarget& target) {
        if (target.region == entity) {
          targets_to_be_destroyed.push_back(target.GetEntity());
        }
      });
    disabled_clip_targets_.ForEach([&](const ClipTarget& target) {
        if (target.region == entity) {
          targets_to_be_destroyed.push_back(target.GetEntity());
        }
      });

    for (Entity target : targets_to_be_destroyed) {
      ResetUniforms(target);
      DestroyTarget(target);
    }

    clip_regions_.Destroy(entity);
  }
}

ShaderClipSystem::ClipTarget* ShaderClipSystem::GetTarget(Entity entity) {
  auto* target = clip_targets_.Get(entity);
  if (!target) {
    target = disabled_clip_targets_.Get(entity);
  }
  return target;
}

const ShaderClipSystem::ClipTarget* ShaderClipSystem::GetTarget(
    Entity entity) const {
  const auto* target = clip_targets_.Get(entity);
  if (!target) {
    target = disabled_clip_targets_.Get(entity);
  }
  return target;
}

Entity ShaderClipSystem::GetContainingRegion(Entity entity) const {
  const auto* transform_system = registry_->Get<TransformSystem>();
  while (entity != kNullEntity) {
    if (clip_regions_.Get(entity)) {
      return entity;
    }
    entity = transform_system->GetParent(entity);
  }
  return kNullEntity;
}

void ShaderClipSystem::OnDisabled(Entity entity) {
  auto* target = clip_targets_.Get(entity);
  if (target) {
    disabled_clip_targets_.Emplace(*target);
    clip_targets_.Destroy(entity);
  }
}

void ShaderClipSystem::OnEnabled(Entity entity) {
  auto* target = disabled_clip_targets_.Get(entity);
  if (target) {
    clip_targets_.Emplace(*target);
    disabled_clip_targets_.Destroy(entity);
  }
}

void ShaderClipSystem::OnParentChanged(
    const ParentChangedImmediateEvent& event) {
  Entity new_containing_region = GetContainingRegion(event.new_parent);
  const ClipTarget* new_target = GetTarget(event.new_parent);
  const ClipTarget* target = GetTarget(event.target);
  if (target && target->region != kNullEntity && !new_target) {
    // If the child target had a region and now does not, remove it.
    RemoveTarget(event.target);
  } else if (new_target) {
    // If the new parent is a target, automatically enable the child.
    AddTargetRecursive(new_target->region, event.target);
  } else if (new_containing_region != kNullEntity) {
    // At this point, the child could be a target with null region, or not a
    // target. It is also possible that a farther down descendant has a target
    // with null region.
    // Since the new_parent has a region, we should check all children and add
    // them if they have a null region.
    TryAddTargetRecursive(new_containing_region, event.target);
  }
}

void ShaderClipSystem::RemoveTarget(Entity entity) {
  const ClipTarget* target = GetTarget(entity);
  // Don't automatically disable targets that were manually enabled.
  if (!target || target->manually_enabled) {
    return;
  }

  ResetUniforms(entity);
  DestroyTarget(entity);

  const auto* transform_system = registry_->Get<TransformSystem>();
  const std::vector<Entity>* children = transform_system->GetChildren(entity);
  if (children != nullptr) {
    for (auto& child : *children) {
      RemoveTarget(child);
    }
  }
}

void ShaderClipSystem::ResetUniforms(Entity entity) {
  // Reset the uniforms to values that will never clip.
  const mathfu::vec3 min = -mathfu::kOnes3f;
  const mathfu::vec3 max = mathfu::kOnes3f;
  const std::vector<float> data(16, 0.f);
  RenderSystem* render_system = registry_->Get<RenderSystem>();
  render_system->SetUniform(entity, kMinInClipRegionSpace, &min[0],
                            3 /* dimension */);
  render_system->SetUniform(entity, kMaxInClipRegionSpace, &max[0],
                            3 /* dimension */);
  render_system->SetUniform(entity, kClipRegionFromModelSpaceMatrix,
                            data.data(), 16 /* dimension */);
}

void ShaderClipSystem::DestroyTarget(Entity entity) {
  clip_targets_.Destroy(entity);
  disabled_clip_targets_.Destroy(entity);
}

}  // namespace lull
