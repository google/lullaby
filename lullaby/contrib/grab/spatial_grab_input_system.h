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

#ifndef LULLABY_CONTRIB_GRAB_SPATIAL_GRAB_INPUT_SYSTEM_H_
#define LULLABY_CONTRIB_GRAB_SPATIAL_GRAB_INPUT_SYSTEM_H_

#include <set>
#include <unordered_map>

#include "lullaby/contrib/grab/grab_system.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_manager.h"

namespace lull {

// The SpatialGrabInputSystem is set up to allow an Entity to be dragged in 3D
// space while maintaining a fixed sqt offset from a device throughout the drag.
class SpatialGrabInputSystem : public System, GrabSystem::GrabInputInterface {
 public:
  explicit SpatialGrabInputSystem(Registry* registry);
  ~SpatialGrabInputSystem() override = default;

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
    // Angle in radians that the controller can diverge from the entity before
    // the grab is canceled.
    float break_angle = 30.0f;

    // Indicates whether to set |device_from_grabbed| on StartGrab based on the
    // actual sqt offset between device and grabbed entity, rather than the
    // |grab_offset| configured in the Def.
    bool set_grab_offset_on_start = true;

    // Transformation between device and grabbed entity to be maintained
    // throughout drag.
    mathfu::mat4 device_from_grabbed;
  };

  std::unordered_map<Entity, Handler> handlers_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::SpatialGrabInputSystem);

#endif  // LULLABY_CONTRIB_GRAB_SPATIAL_GRAB_INPUT_SYSTEM_H_
