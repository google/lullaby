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

#ifndef LULLABY_SYSTEMS_INPUT_BEHAVIOR_INPUT_BEHAVIOR_SYSTEM_H_
#define LULLABY_SYSTEMS_INPUT_BEHAVIOR_INPUT_BEHAVIOR_SYSTEM_H_

#include <deque>

#include "lullaby/generated/input_behavior_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

/// Handles grouping entities together so that all input events are sent to a
/// single parent.  Also handles deadzones, which prevent focus from starting on
/// an entity at the edges of its collision volume.
class InputBehaviorSystem : public System {
 public:
  explicit InputBehaviorSystem(Registry* registry);

  void Create(Entity entity, HashValue type, const Def* def) override;

  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  void Destroy(Entity entity) override;

  void UpdateInputFocus(InputFocus* focus) const;

  /// Sets the cursor collision behavior for |entity|.
  void SetFocusBehavior(Entity entity, InputBehaviorType behavior_type);

  /// Set to true to make the Entity receive Drag events.
  void SetDraggable(Entity entity, bool draggable);
  bool IsDraggable(Entity entity) const;

 private:
  struct InputBehavior : Component {
    explicit InputBehavior(Entity entity)
        : Component(entity),
          focus_start_dead_zone(mathfu::kZeros3f),
          behavior_type(InputBehaviorType_HandleAlone),
          draggable(false) {}

    // The amount to shrink this entity's Aabb by when starting to focus on it.
    // The dead zone is applied on both sides.
    mathfu::vec3 focus_start_dead_zone;

    // How this entity should handle collisions.
    InputBehaviorType behavior_type;

    // If this entity should receive drag events.
    bool draggable;
  };

  // If the target has a ReticleBehavior, apply that behavior and return the
  // actual target.
  Entity HandleBehavior(Entity entity) const;

  // Checks if |collided_entity| has a hover start dead zone. If it does and the
  // cursor is currently within the dead zone, return true. Otherwise, return
  // false.
  bool IsInsideEntityDeadZone(Entity entity, const Ray& collision_ray) const;

  ComponentPool<InputBehavior> input_behaviors_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::InputBehaviorSystem);

#endif  // LULLABY_SYSTEMS_INPUT_BEHAVIOR_INPUT_BEHAVIOR_SYSTEM_H_
