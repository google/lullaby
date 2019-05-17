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

#ifndef LULLABY_CONTRIB_SNAP_SNAP_SYSTEM_H_
#define LULLABY_CONTRIB_SNAP_SNAP_SYSTEM_H_

#include <unordered_map>
#include <unordered_set>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/util/entity.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/events/animation_events.h"
#include "lullaby/util/math.h"

namespace lull {

/// A system for animating entities to pre-defined snap targets.
///
/// A SnapTarget must have SnapTargetDef and TransformDef components. Snap
/// targets may be assigned to a group. All targets without an explicitly
/// specified group are said to belong to the "default" group.
///
/// Any entity with a TransformDef may be animated to a snap target. However,
/// a SnappableDef component may be used to specify in an entity's blueprint
/// which target group that entity should apply to.
///
/// Applications using this system must setup the PositionChannel animation
/// channel.
class SnapSystem : public System {
 public:
  static constexpr int kUseDefaultTime = -1;

  explicit SnapSystem(Registry* registry);

  void Create(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;

  /// Set the default interpolation time for snap animations to take.
  void SetDefaultAnimationTime(int time_ms);

  /// Animate the |entity| to the nearest snap target in the default group over
  /// |time_ms|. If the |entity| has a SnappableDef component, those values will
  /// override defaults. Lastly, if |time_ms| is specified, this will override
  /// both the default and SnappableDef-specified values.
  AnimationId SnapToClosest(Entity entity, int time_ms = kUseDefaultTime);

  /// Animate the |entity| to the nearest snap target in the given |group| over
  /// |time_ms|. The group is a hash of the group's name string.
  AnimationId SnapToClosestInGroup(Entity entity, HashValue group,
                                   int time_ms = kUseDefaultTime);

  /// Snap the |entity| to the nearest snap target in the default group
  /// immediately without animating.
  void SnapHardToClosest(Entity entity);

  /// Snap the |entity| to the nearest snap target in the given |group|
  /// immediately without animating.
  void SnapHardToClosestInGroup(Entity entity, HashValue group);

 private:
  struct Snappable : Component {
    explicit Snappable(Entity e);

    /// Time (in milliseconds) taken to animate to the target position. A value
    /// of -1 indicates that the default value should be used.
    int time_ms;

    /// Hashed name of the target group to restrict snapping to.
    HashValue target_group;
  };

  struct SnapTarget : Component {
    explicit SnapTarget(Entity e);

    /// Hashed name of the group to which this target belongs.
    HashValue group;
  };

  Entity GetClosestTarget(Entity entity, HashValue group);
  AnimationId CreateAnimationToTarget(Entity entity, Entity target,
                                      int time_ms);
  mathfu::vec3 GetEntityWorldPosition(Entity entity);

  /// Default time taken to interpolate a snap animation.
  int default_anim_time_ms_;

  std::unordered_map<Entity, Snappable> snappables_;
  ComponentPool<SnapTarget> targets_;

  /// Hash of group name to group of snap target entities.
  std::unordered_map<HashValue, std::unordered_set<Entity>> targets_by_group_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::SnapSystem);

#endif  // LULLABY_CONTRIB_SNAP_SNAP_SYSTEM_H_
