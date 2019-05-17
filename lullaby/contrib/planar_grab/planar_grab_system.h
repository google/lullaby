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

#ifndef LULLABY_CONTRIB_PLANAR_GRAB_PLANAR_GRAB_SYSTEM_H_
#define LULLABY_CONTRIB_PLANAR_GRAB_PLANAR_GRAB_SYSTEM_H_

#include <unordered_map>

#include "lullaby/events/input_events.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"
#include "lullaby/util/optional.h"

namespace lull {

/// The PlanarGrabSystem allows the controller to manipulate entities' position
/// inside a plane constraint.
/// Grabbable entities must have a PlanarGrabbableDef, a TransformDef, and a
/// CollisionDef.
class PlanarGrabSystem : public System {
 public:
  explicit PlanarGrabSystem(Registry* registry);
  ~PlanarGrabSystem() override = default;

  void Create(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;

  void AdvanceFrame(const Clock::duration& delta_time);

  /// Get the world-space position of the point at which the |entity| was
  /// grabbed. Optional will be unset if the entity is not grabbable.
  Optional<mathfu::vec3> GetGrabOrigin(Entity entity);

  /// Get the plane in which the given |entity| is being constrained.
  /// Optional will be unset if the entity is not grabbable.
  Optional<Plane> GetGrabPlane(Entity entity);

 private:
  struct Grabbable : Component {
    explicit Grabbable(Entity entity) : Component(entity) {}

    /// Normal which will define the orientation of the plane used to constrain
    /// the objects movement. The origin of the plane is defined dynamically as
    /// the location where the object is grabbed.
    mathfu::vec3 plane_normal;

    /// Whether the plane normal is defined in object-local or world space.
    bool local_orientation;
  };

  struct GrabData {
    explicit GrabData(Entity e, const mathfu::vec3& offset,
                      const mathfu::vec3& origin, const Plane& plane)
        : entity(e),
          grab_local_offset(offset),
          grab_origin(origin),
          plane(plane) {}

    /// Entity being grabbed.
    Entity entity;

    /// Offset in local coordinates where the grab took place on the entity.
    mathfu::vec3 grab_local_offset;

    /// World-space position of the initial grab point.
    mathfu::vec3 grab_origin;

    /// Plane in which the entity's movement is constrained. This is defined in
    /// world-space. If the entity's plane constraint is relative to its local
    /// space, the conversion will happen when the entity is grabbed. This value
    /// will be updated each frame to account for any movement of the entity
    /// due to other systems.
    Plane plane;
  };

  void OnGrab(const ClickEvent& event);
  void OnGrabReleased(const ClickReleasedEvent& event);

  std::unordered_map<Entity, Grabbable> grabbables_;
  std::unordered_map<Entity, GrabData> grabbed_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::PlanarGrabSystem);

#endif  // LULLABY_CONTRIB_PLANAR_GRAB_PLANAR_GRAB_SYSTEM_H_
