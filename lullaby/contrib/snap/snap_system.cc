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

#include "lullaby/contrib/snap/snap_system.h"

#include <limits>

#include "lullaby/generated/snap_def_generated.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/logging.h"
#include "lullaby/modules/animation_channels/transform_channels.h"

namespace lull {

namespace {
const HashValue kSnappableDefHash = ConstHash("SnappableDef");
const HashValue kSnapTargetDefHash = ConstHash("SnapTargetDef");

const HashValue kDefaultGroup = ConstHash("default");
}  // namespace

SnapSystem::Snappable::Snappable(Entity e)
    : Component(e), time_ms(-1), target_group(kDefaultGroup) {}

SnapSystem::SnapTarget::SnapTarget(Entity e)
    : Component(e), group(kDefaultGroup) {}

SnapSystem::SnapSystem(Registry* registry)
    : System(registry),
      default_anim_time_ms_(200),
      snappables_(16),
      targets_(16) {
  RegisterDef<SnappableDefT>(this);
  RegisterDef<SnapTargetDefT>(this);
  RegisterDependency<AnimationSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void SnapSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type == kSnapTargetDefHash) {
    SnapTarget* target = targets_.Emplace(entity);

    auto* data = ConvertDef<SnapTargetDef>(def);
    if (data->group() != nullptr) {
      target->group = Hash(data->group()->c_str());
    }

    targets_by_group_[target->group].insert(entity);
  } else if (type == kSnappableDefHash) {
    Snappable snappable(entity);

    auto* data = ConvertDef<SnappableDef>(def);
    snappable.time_ms = data->time_ms();
    if (data->target_group() != nullptr) {
      snappable.target_group = Hash(data->target_group()->c_str());
    }
    snappables_.insert({entity, snappable});
  }
}

void SnapSystem::Destroy(Entity entity) {
  snappables_.erase(entity);

  SnapTarget* target = targets_.Get(entity);
  if (target != nullptr) {
    auto group = targets_by_group_.find(target->group);
    if (group != targets_by_group_.end()) {
      group->second.erase(entity);

      // If this is the last element in the given group, remove the group.
      if (group->second.empty()) {
        targets_by_group_.erase(target->group);
      }
    }
    targets_.Destroy(entity);
  }
}

void SnapSystem::SetDefaultAnimationTime(int time_ms) {
  if (time_ms < 0.0f) {
    LOG(DFATAL) << "Cannot specify negative default interpolation time";
  }
  default_anim_time_ms_ = time_ms;
}

AnimationId SnapSystem::SnapToClosest(Entity entity, int time_ms) {
  HashValue group = kDefaultGroup;

  // Override group with info from SnappableDef, if present.
  auto found = snappables_.find(entity);
  if (found != snappables_.end()) {
    group = found->second.target_group;
  }

  return SnapToClosestInGroup(entity, group, time_ms);
}

AnimationId SnapSystem::SnapToClosestInGroup(Entity entity, HashValue group,
                                             int time_ms) {
  if (time_ms < 0.0f) {
    time_ms = default_anim_time_ms_;

    // Override default time if entity has a snappable component.
    auto found = snappables_.find(entity);
    if (found != snappables_.end() && found->second.time_ms > 0) {
      time_ms = found->second.time_ms;
    }
  }

  // Find the target position.
  const Entity target = GetClosestTarget(entity, group);
  if (target == kNullEntity) {
    return kNullAnimation;
  }

  // Create the animation, return the animation id.
  return CreateAnimationToTarget(entity, target, time_ms);
}

void SnapSystem::SnapHardToClosest(Entity entity) {
  HashValue group = kDefaultGroup;

  // Override group with info from SnappableDef, if present.
  auto found = snappables_.find(entity);
  if (found != snappables_.end()) {
    group = found->second.target_group;
  }

  SnapHardToClosestInGroup(entity, group);
}

void SnapSystem::SnapHardToClosestInGroup(Entity entity, HashValue group) {
  Entity target = GetClosestTarget(entity, group);
  if (target == kNullEntity) {
    return;
  }

  // Get target's position.
  const mathfu::vec3 target_position = GetEntityWorldPosition(target);

  // Move entity to target's position.
  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::mat4* world_from_entity_matrix =
      transform_system->GetWorldFromEntityMatrix(entity);
  mathfu::mat4 updated_world_from_entity_matrix(*world_from_entity_matrix);
  updated_world_from_entity_matrix.GetColumn(3) =
      mathfu::vec4(target_position, 1.0f);
  transform_system->SetWorldFromEntityMatrix(entity,
                                             updated_world_from_entity_matrix);
}

Entity SnapSystem::GetClosestTarget(Entity entity, HashValue group) {
  auto target_group = targets_by_group_.find(group);
  if (target_group == targets_by_group_.end()) {
    return kNullEntity;  // group not found
  }

  // Get entity's current position.
  const mathfu::vec3 entity_position = GetEntityWorldPosition(entity);

  const auto& targets = target_group->second;
  Entity closest_target = kNullEntity;
  float closest_distance = std::numeric_limits<float>::max();
  for (const auto& target : targets) {
    const mathfu::vec3 target_position = GetEntityWorldPosition(target);
    const float distance = (target_position - entity_position).LengthSquared();
    if (distance < closest_distance) {
      closest_target = target;
      closest_distance = distance;
    }
  }
  return closest_target;
}

AnimationId SnapSystem::CreateAnimationToTarget(Entity entity, Entity target,
                                                int time_ms) {
  if (target == kNullEntity) {
    return kNullAnimation;
  }

  mathfu::vec3 target_position = GetEntityWorldPosition(target);

  // If entity has a parent, transform the target position in the parent's
  // coordinate space.
  auto* transform_system = registry_->Get<TransformSystem>();
  lull::Entity parent = transform_system->GetParent(entity);
  if (parent != lull::kNullEntity) {
    const mathfu::mat4* world_from_parent_matrix =
        transform_system->GetWorldFromEntityMatrix(parent);
    const mathfu::mat4 parent_from_world_matrix =
        world_from_parent_matrix->Inverse();
    target_position = parent_from_world_matrix * target_position;
  }

  const auto duration_ms = std::chrono::milliseconds(time_ms);

  auto* animation_system = registry_->Get<AnimationSystem>();
  return animation_system->SetTarget(entity, PositionChannel::kChannelName,
                                     &target_position[0], 3, duration_ms);
}

mathfu::vec3 SnapSystem::GetEntityWorldPosition(Entity entity) {
  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::mat4* matrix =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (matrix != nullptr) {
    return matrix->TranslationVector3D();
  }
  return mathfu::kZeros3f;
}

}  // namespace lull
