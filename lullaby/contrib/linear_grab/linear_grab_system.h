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

#ifndef LULLABY_CONTRIB_LINEAR_GRAB_LINEAR_GRAB_SYSTEM_H_
#define LULLABY_CONTRIB_LINEAR_GRAB_LINEAR_GRAB_SYSTEM_H_

#include <unordered_map>

#include "lullaby/events/input_events.h"
#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/reticle/input_focus_locker.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/math.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/typeid.h"
#include "mathfu/constants.h"

namespace lull {

/// Event sent when an entity if first grabbed.
struct LinearGrabEvent {
  LinearGrabEvent() {}
  LinearGrabEvent(Entity e, const mathfu::vec3& location)
      : entity(e), location(location) {}
  // The entity being grabbed.
  Entity entity = kNullEntity;
  // Location of the grab in local coordinates of the entity.
  mathfu::vec3 location = mathfu::kZeros3f;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&location, ConstHash("location"));
  }
};

/// Event sent when a grabbed entity is released.
struct LinearGrabReleasedEvent {
  LinearGrabReleasedEvent() {}
  explicit LinearGrabReleasedEvent(Entity e) : entity(e) {}
  // The entity being released.
  Entity entity = kNullEntity;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
  }
};

/// The LinearGrabSystem allows entities to be moved along a line via the
/// controller. Grabbable entities must have a LinearGrabbableDef, a
/// TransformDef, and a CollisionDef.
class LinearGrabSystem : public System {
 public:
  explicit LinearGrabSystem(Registry* registry);
  ~LinearGrabSystem() override = default;

  void Create(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;

  void AdvanceFrame(const Clock::duration& delta_time);

  /// Enable grabbing on the |entity|. This is a no-op if the |entity| does not
  /// have a LinearGrabbable component.
  void Enable(Entity entity);

  /// Disable grabbing on the |entity|. This is a no-op if the |entity| does not
  /// have a LinearGrabbable component. If the |entity| is currently grabbed,
  /// this will immediately release the entity and trigger a
  /// LinearGrabReleasedEvent to be sent.
  void Disable(Entity entity);

  /// Get the line in which the given |entity| is being constrained.
  /// Optional will be unset if the entity is not grabbable.
  Optional<Line> GetGrabLine(Entity entity);

 private:
  struct Grabbable : Component {
    explicit Grabbable(Entity entity) : Component(entity) {}

    /// Whether grabbing this entity is enabled.
    bool enabled = true;

    /// Vector defining the direction of the line used to constrain the object's
    /// movement. The origin on the line will be set at runtime as the point at
    /// which the object is grabbed.
    mathfu::vec3 line_direction;

    /// Whether the line direction is defined in object-local or world space.
    bool local_orientation;
  };

  struct GrabData {
    explicit GrabData(Entity e, const mathfu::vec3& offset,
                      const mathfu::vec3& origin, const Line& line,
                      InputManager::DeviceType device)
        : entity(e),
          grab_local_offset(offset),
          grab_origin(origin),
          line(line),
          device(device) {}

    /// Entity being grabbed.
    Entity entity;

    /// Offset in local coordinates where the grab took place on the entity.
    mathfu::vec3 grab_local_offset;

    /// World-space position of the initial grab point.
    mathfu::vec3 grab_origin;

    /// Line in which the entity's movement is constrained. This is defined in
    /// world-space.
    Line line;

    /// Which device initiated the Grab.
    InputManager::DeviceType device;
  };

  void OnGrab(const EventWrapper& event);
  void OnGrabReleased(const EventWrapper& event);

  void Release(Entity entity);

  std::unordered_map<Entity, Grabbable> grabbables_;
  std::unordered_map<Entity, GrabData> grabbed_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::LinearGrabSystem);
LULLABY_SETUP_TYPEID(lull::LinearGrabEvent);
LULLABY_SETUP_TYPEID(lull::LinearGrabReleasedEvent);

#endif  // LULLABY_CONTRIB_LINEAR_GRAB_LINEAR_GRAB_SYSTEM_H_
