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

#ifndef LULLABY_CONTRIB_GRAB_SPHERICAL_GRAB_INPUT_SYSTEM_H_
#define LULLABY_CONTRIB_GRAB_SPHERICAL_GRAB_INPUT_SYSTEM_H_

#include <set>
#include <unordered_map>

#include "lullaby/contrib/grab/grab_system.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_manager.h"

namespace lull {

/// The SphericalGrabInputSystem is set up to allow an entity to be dragged
/// along a sphere, which keeps a fixed distance from the entity's origin to the
/// sphere center during grab.
class SphericalGrabInputSystem : public System, GrabSystem::GrabInputInterface {
 public:
  explicit SphericalGrabInputSystem(Registry* registry);
  ~SphericalGrabInputSystem() override = default;

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
    Sphere grab_sphere;
    bool keep_grab_offset = true;
    bool move_with_hmd = false;
    bool hide_cursor = false;
    bool hide_laser = false;

    // The last collision position of the controller collision ray and the grab
    // sphere in world space.
    mathfu::vec3 last_collision_position;
    // The cursor state before starting the grab.
    bool cursor_enabled_before_grab = true;
    bool laser_enabled_before_grab = true;
  };

  std::unordered_map<Entity, Handler> handlers_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::SphericalGrabInputSystem);

#endif  // LULLABY_CONTRIB_GRAB_SPHERICAL_GRAB_INPUT_SYSTEM_H_
