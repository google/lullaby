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

#include "lullaby/contrib/input_behavior/input_behavior_system.h"

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "mathfu/constants.h"

#include "mathfu/io.h"

namespace lull {

const HashValue kInputBehaviorDef = ConstHash("InputBehaviorDef");

InputBehaviorSystem::InputBehaviorSystem(Registry* registry)
    : System(registry), input_behaviors_(16) {
  RegisterDef<InputBehaviorDefT>(this);
  RegisterDependency<InputProcessor>(this);
  RegisterDependency<TransformSystem>(this);
}

void InputBehaviorSystem::Create(Entity entity, HashValue type,
                                 const Def* def) {
  CHECK_NE(def, nullptr);
  if (type == kInputBehaviorDef) {
    const auto* data = ConvertDef<InputBehaviorDef>(def);
    auto* behavior = input_behaviors_.Emplace(entity);
    if (behavior == nullptr) {
      behavior = input_behaviors_.Get(entity);
    }
    if (data->focus_start_dead_zone()) {
      mathfu::vec3 out_vec;
      MathfuVec3FromFbVec3(data->focus_start_dead_zone(), &out_vec);
      behavior->focus_start_dead_zone = out_vec;
    }
    behavior->behavior_type = data->behavior_type();

    if (data->draggable() != OptionalBool::OptionalBool_Unset) {
      behavior->draggable =
          data->draggable() == OptionalBool::OptionalBool_True;
    }
  } else {
    DCHECK(false) << "Unsupported ComponentDef type: " << type;
  }
}

void InputBehaviorSystem::PostCreateInit(Entity entity, HashValue type,
                                         const Def* def) {
  if (type != kInputBehaviorDef) {
    return;
  }

  const auto* data = ConvertDef<InputBehaviorDef>(def);
  auto* behavior = input_behaviors_.Get(entity);

  if (behavior->behavior_type ==
          InputBehaviorType::InputBehaviorType_HandleDescendants &&
      data->interactive_if_handle_descendants()) {
    auto* collision_system = registry_->Get<CollisionSystem>();
    if (collision_system) {
      collision_system->EnableInteraction(entity);
      collision_system->EnableDefaultInteraction(entity);
    }
  }
}

void InputBehaviorSystem::Destroy(Entity entity) {
  input_behaviors_.Destroy(entity);
}

void InputBehaviorSystem::SetDraggable(Entity entity, bool draggable) {
  auto* behavior = input_behaviors_.Get(entity);
  if (behavior == nullptr) {
    behavior = input_behaviors_.Emplace(entity);
  }
  behavior->draggable = draggable;
}

bool InputBehaviorSystem::IsDraggable(Entity entity) const {
  auto* behavior = input_behaviors_.Get(entity);
  return behavior != nullptr ? behavior->draggable : false;
}

void InputBehaviorSystem::UpdateInputFocus(InputFocus* focus) const {
  // Finalize what entity to actually target
  const Entity original_target = focus->target;

  // If specified, attempt to find an ancestor that is designated to handle
  // reticle events for its descendants. If no such entity is found, the
  // collision will be handled by the original entity.
  focus->target = HandleBehavior(original_target);

  // Get the current target.
  const auto* input_processor = registry_->Get<InputProcessor>();
  const InputFocus* current_focus =
      input_processor->GetInputFocus(focus->device);
  if (current_focus != nullptr) {
    // If the target entity would change, check for deadzone.
    const bool changing_target =
        current_focus == nullptr || focus->target != current_focus->target;
    if (changing_target &&
        IsInsideEntityDeadZone(original_target, focus->collision_ray)) {
      focus->target = kNullEntity;
    }
  }

  if (focus->target != kNullEntity) {
    focus->draggable = IsDraggable(focus->target);
  }
}

bool InputBehaviorSystem::IsInsideEntityDeadZone(
    Entity entity, const Ray& collision_ray) const {
  auto* behavior = input_behaviors_.Get(entity);
  if (behavior == nullptr ||
      behavior->focus_start_dead_zone == mathfu::kZeros3f) {
    return false;
  }

  const auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::mat4* world_from_collided =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (!world_from_collided) {
    LOG(DFATAL)
        << "focus_start_dead_zone only works on entities with transforms";
    return false;
  }

  const Aabb* aabb = transform_system->GetAabb(entity);
  if (!aabb) {
    LOG(DFATAL) << "focus_start_dead_zone only works on entities with AABBs";
    return false;
  }

  const Aabb modified_aabb(aabb->min + behavior->focus_start_dead_zone,
                           aabb->max - behavior->focus_start_dead_zone);
  return !ComputeLocalRayOBBCollision(collision_ray, *world_from_collided,
                                      modified_aabb);
}

Entity InputBehaviorSystem::HandleBehavior(Entity entity) const {
  auto* behavior = input_behaviors_.Get(entity);
  if (behavior && behavior->behavior_type ==
                      InputBehaviorType::InputBehaviorType_FindAncestor) {
    auto transform_system = registry_->Get<TransformSystem>();
    Entity parent = transform_system->GetParent(entity);
    while (parent != kNullEntity) {
      behavior = input_behaviors_.Get(parent);
      if (behavior &&
          behavior->behavior_type ==
              InputBehaviorType::InputBehaviorType_HandleDescendants) {
        return parent;
      }
      parent = transform_system->GetParent(parent);
    }
    if (parent == kNullEntity) {
      LOG(DFATAL) << "Entity specified with FindAncestor focus behavior, "
                     "but no ancestor with HandleDescendants found.";
    }
  }
  return entity;
}

void InputBehaviorSystem::SetFocusBehavior(Entity entity,
                                           InputBehaviorType behavior_type) {
  auto* behavior = input_behaviors_.Get(entity);
  if (!behavior) {
    behavior = input_behaviors_.Emplace(entity);
  }
  behavior->behavior_type = behavior_type;
}

}  // namespace lull
