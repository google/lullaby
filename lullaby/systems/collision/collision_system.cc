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

#include "lullaby/systems/collision/collision_system.h"

#include "lullaby/generated/collision_def_generated.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"

namespace lull {

namespace {
const HashValue kCollisionDefHash = Hash("CollisionDef");
const HashValue kClipBoundsDefHash = Hash("CollisionClipBoundsDef");
}  // namespace

CollisionSystem::CollisionSystem(Registry* registry)
    : System(registry),
      transform_system_(nullptr),
      collision_flag_(TransformSystem::kInvalidFlag),
      on_exit_flag_(TransformSystem::kInvalidFlag),
      interaction_flag_(TransformSystem::kInvalidFlag),
      default_interaction_flag_(TransformSystem::kInvalidFlag),
      clip_flag_(TransformSystem::kInvalidFlag) {
  RegisterDef(this, kCollisionDefHash);
  RegisterDef(this, kClipBoundsDefHash);
  RegisterDependency<TransformSystem>(this);
}

void CollisionSystem::Initialize() {
  transform_system_ = registry_->Get<TransformSystem>();
  on_exit_flag_ = transform_system_->RequestFlag();
  collision_flag_ = transform_system_->RequestFlag();
  interaction_flag_ = transform_system_->RequestFlag();
  default_interaction_flag_ = transform_system_->RequestFlag();
  clip_flag_ = transform_system_->RequestFlag();
}

void CollisionSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type == kClipBoundsDefHash) {
    const CollisionClipBoundsDef* data =
        ConvertDef<CollisionClipBoundsDef>(def);
    Aabb aabb;
    AabbFromFbAabb(data->aabb(), &aabb);
    clip_bounds_.emplace(entity, aabb);
  } else if (type == kCollisionDefHash) {
    const CollisionDef* data = ConvertDef<CollisionDef>(def);

    transform_system_->SetFlag(entity, collision_flag_);
    if (data->interactive()) {
      transform_system_->SetFlag(entity, interaction_flag_);
      transform_system_->SetFlag(entity, default_interaction_flag_);
    }
    if (data->collision_on_exit()) {
      transform_system_->SetFlag(entity, on_exit_flag_);
    }
    if (data->clip_outside_bounds()) {
      transform_system_->SetFlag(entity, clip_flag_);
    }
  } else {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting CollisionDef or "
                << "CollisionClipBoundsDef!";
  }
}

void CollisionSystem::Destroy(Entity entity) {
  clip_bounds_.erase(entity);
  transform_system_->ClearFlag(entity, collision_flag_);
  transform_system_->ClearFlag(entity, on_exit_flag_);
  transform_system_->ClearFlag(entity, interaction_flag_);
  transform_system_->ClearFlag(entity, default_interaction_flag_);
  transform_system_->ClearFlag(entity, clip_flag_);
}

CollisionSystem::CollisionResult CollisionSystem::CheckForCollision(
    const Ray& ray) {
  CollisionResult result = {kNullEntity, kNoHitDistance};
  std::vector<CollisionResult> clipped_results;

  transform_system_->ForAll([&](Entity entity,
                                const mathfu::mat4& world_from_entity_mat,
                                const Aabb& box, Bits flags) {
    if (!CheckBit(flags, collision_flag_)) {
      return;
    }

    const bool check_exit = CheckBit(flags, on_exit_flag_);
    const float distance =
        CheckRayOBBCollision(ray, world_from_entity_mat, box, check_exit);
    if (distance == kNoHitDistance) {
      return;
    }

    if (result.entity != kNullEntity && distance >= result.distance) {
      return;
    }

    const bool clip_outside_bounds = CheckBit(flags, clip_flag_);
    if (clip_outside_bounds &&
        IsCollisionClipped(entity, ray.GetPointAt(distance))) {
      return;
    }

    result.entity = entity;
    result.distance = distance;
  });

  return result;
}

std::vector<Entity> CollisionSystem::CheckForPointCollisions(
    const mathfu::vec3& point) {
  std::vector<Entity> collisions;

  transform_system_->ForAll([&](Entity entity,
                                const mathfu::mat4& world_from_entity_mat,
                                const Aabb& box, Bits flags) {
    if (CheckBit(flags, collision_flag_)) {
      if (CheckPointOBBCollision(point, world_from_entity_mat, box)) {
        collisions.push_back(entity);
      }
    }
  });
  return collisions;
}

void CollisionSystem::DisableCollision(Entity entity) {
  transform_system_->ClearFlag(entity, collision_flag_);
}

void CollisionSystem::EnableCollision(Entity entity) {
  transform_system_->SetFlag(entity, collision_flag_);
}

bool CollisionSystem::IsCollisionEnabled(Entity entity) const {
  return transform_system_->HasFlag(entity, collision_flag_);
}

void CollisionSystem::DisableInteraction(Entity entity) {
  transform_system_->ClearFlag(entity, interaction_flag_);
  SendEvent(registry_, entity, OnInteractionDisabledEvent(entity));
}

void CollisionSystem::EnableInteraction(Entity entity) {
  transform_system_->SetFlag(entity, interaction_flag_);
  SendEvent(registry_, entity, OnInteractionEnabledEvent(entity));
}

void CollisionSystem::DisableDefaultInteraction(Entity entity) {
  transform_system_->ClearFlag(entity, default_interaction_flag_);
}

void CollisionSystem::EnableDefaultInteraction(Entity entity) {
  transform_system_->SetFlag(entity, default_interaction_flag_);
}

bool CollisionSystem::IsInteractionEnabled(Entity entity) const {
  return transform_system_->HasFlag(entity, interaction_flag_);
}

void CollisionSystem::RestoreInteraction(Entity entity) {
  if (transform_system_->HasFlag(entity, default_interaction_flag_)) {
    transform_system_->SetFlag(entity, interaction_flag_);
    SendEvent(registry_, entity, OnInteractionEnabledEvent(entity));
  } else {
    transform_system_->ClearFlag(entity, interaction_flag_);
    SendEvent(registry_, entity, OnInteractionDisabledEvent(entity));
  }
}

void CollisionSystem::DisableInteractionDescendants(Entity entity) {
  transform_system_->ForAllDescendants(entity, [this](Entity child) {
    transform_system_->ClearFlag(child, interaction_flag_);
    SendEvent(registry_, child, OnInteractionDisabledEvent(child));
  });
}

void CollisionSystem::RestoreInteractionDescendants(Entity entity) {
  transform_system_->ForAllDescendants(entity, [this](Entity child) {
    if (transform_system_->HasFlag(child, default_interaction_flag_)) {
      transform_system_->SetFlag(child, interaction_flag_);
      SendEvent(registry_, child, OnInteractionEnabledEvent(child));
    } else {
      transform_system_->ClearFlag(child, interaction_flag_);
      SendEvent(registry_, child, OnInteractionDisabledEvent(child));
    }
  });
}

void CollisionSystem::DisableClipping(Entity entity) {
  transform_system_->ClearFlag(entity, clip_flag_);
}

void CollisionSystem::EnableClipping(Entity entity) {
  transform_system_->SetFlag(entity, clip_flag_);
}

Entity CollisionSystem::GetContainingBounds(Entity entity) const {
  Entity parent = transform_system_->GetParent(entity);
  while (parent != kNullEntity) {
    if (clip_bounds_.find(parent) != clip_bounds_.end()) {
      break;
    }
    parent = transform_system_->GetParent(parent);
  }
  return parent;
}

bool CollisionSystem::IsCollisionClipped(Entity entity,
                                         const mathfu::vec3& point) const {
  const Entity bounds_entity = GetContainingBounds(entity);
  if (bounds_entity == kNullEntity) {
    return false;
  }

  const auto clip_bounds = clip_bounds_.find(bounds_entity);
  if (clip_bounds == clip_bounds_.end()) {
    return false;
  }

  const mathfu::mat4* world_from_bounds_matrix =
      transform_system_->GetWorldFromEntityMatrix(clip_bounds->first);
  if (!world_from_bounds_matrix) {
    return false;
  }

  return !CheckPointOBBCollision(point, *world_from_bounds_matrix,
                                 clip_bounds->second);
}

}  // namespace lull
