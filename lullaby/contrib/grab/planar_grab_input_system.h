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

#ifndef LULLABY_CONTRIB_GRAB_PLANAR_GRAB_INPUT_SYSTEM_H_
#define LULLABY_CONTRIB_GRAB_PLANAR_GRAB_INPUT_SYSTEM_H_

#include <set>
#include <unordered_map>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/contrib/grab/grab_system.h"

namespace lull {

/// The PlanarGrabInputSystem is set up to intersect a reticle's collision ray
/// with a plane in an Entity's local space, and allow the entity to be dragged
/// along that plane.
class PlanarGrabInputSystem : public System, GrabSystem::GrabInputInterface {
 public:
  explicit PlanarGrabInputSystem(Registry* registry);
  ~PlanarGrabInputSystem() override = default;

  void Create(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;

  /// See GrabSystem::GrabInputInterface.
  bool StartGrab(Entity entity, InputManager::DeviceType device) override;
  Sqt UpdateGrab(Entity entity, InputManager::DeviceType device,
                 const Sqt& original_sqt) override;
  bool ShouldCancel(Entity entity, InputManager::DeviceType device) override;
  void EndGrab(Entity entity, InputManager::DeviceType device) override;

 private:
  struct Handler {
    // The normal of the plane to drag the entity along, in world space.
    // Set during StartGrab.
    mathfu::vec3 plane_normal = -mathfu::kAxisZ3f;

    // Angle in radians that the controller can diverge from the entity before
    // the grab is canceled.
    float break_angle = 30.0f;

    // The offset from the reticle to the grabbed entity's origin.
    // Set during StartGrab.
    mathfu::vec3 grab_offset = mathfu::kZeros3f;

    // The offset between the ideal grab_offset (based on position at press)
    // and the actual grab_offset (based on position at dragStart).
    mathfu::vec3 initial_offset = mathfu::kZeros3f;
  };

  std::unordered_map<Entity, Handler> handlers_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::PlanarGrabInputSystem);

#endif  // LULLABY_CONTRIB_GRAB_PLANAR_GRAB_INPUT_SYSTEM_H_
