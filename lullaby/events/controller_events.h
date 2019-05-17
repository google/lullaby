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

#ifndef LULLABY_EVENTS_CONTROLLER_EVENTS_H_
#define LULLABY_EVENTS_CONTROLLER_EVENTS_H_

#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Invokes ControllerSystem::ShowControls()
struct ShowControllerModelEvent {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Invokes ControllerSystem::HideControls()
struct HideControllerModelEvent {
  template <typename Archive>
  void Serialize(Archive archive) {}
};

// Invokes ControllerSystem::ShowLaser()
struct ShowControllerLaserEvent {
  ShowControllerLaserEvent() {}
  ShowControllerLaserEvent(InputManager::DeviceType device)
      : controller_type(device) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&controller_type, ConstHash("controller_type"));
  }

  // The controller that bounds with the laser.
  InputManager::DeviceType controller_type;
};

// Invokes ControllerSystem::HideCLaser()
struct HideControllerLaserEvent {
  HideControllerLaserEvent() {}
  HideControllerLaserEvent(InputManager::DeviceType device)
      : controller_type(device) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&controller_type, ConstHash("controller_type"));
  }

  // The controller that bounds with the laser.
  InputManager::DeviceType controller_type;
};

// Invokes ControllerSystem::SetLaserFadePoints()
struct SetLaserFadePointsEvent {
  SetLaserFadePointsEvent() {}
  SetLaserFadePointsEvent(InputManager::DeviceType device,
                          const mathfu::vec4& fade_points)
      : controller_type(device), fade_points(fade_points) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&controller_type, ConstHash("controller_type"));
    archive(&fade_points, ConstHash("fade_points"));
  }

  // The controller that bounds with the laser.
  InputManager::DeviceType controller_type;
  // The new fade points of the laser.
  mathfu::vec4 fade_points;
};

// Invokes ControllerSystem::ResetLaserFadePoints()
struct ResetLaserFadePointsEvent {
  ResetLaserFadePointsEvent() {}
  ResetLaserFadePointsEvent(InputManager::DeviceType device)
      : controller_type(device) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&controller_type, ConstHash("controller_type"));
  }

  // The controller that bounds with the laser.
  InputManager::DeviceType controller_type;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ShowControllerModelEvent);
LULLABY_SETUP_TYPEID(lull::HideControllerModelEvent);
LULLABY_SETUP_TYPEID(lull::ShowControllerLaserEvent);
LULLABY_SETUP_TYPEID(lull::HideControllerLaserEvent);
LULLABY_SETUP_TYPEID(lull::SetLaserFadePointsEvent);
LULLABY_SETUP_TYPEID(lull::ResetLaserFadePointsEvent);

#endif  // LULLABY_EVENTS_CONTROLLER_EVENTS_H_
