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

#ifndef LULLABY_EVENTS_BATTERY_STATE_CHANGED_H_
#define LULLABY_EVENTS_BATTERY_STATE_CHANGED_H_

#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Event used for notifying systems when the battery state changes.
struct BatteryStateChangedEvent {
  static const int kInvalidBatteryLevel = -1;

  BatteryStateChangedEvent()
      : level(kInvalidBatteryLevel),
        status(InputManager::BatteryState::kUnknown),
        device(InputManager::DeviceType::kHmd) {}

  BatteryStateChangedEvent(int level, InputManager::BatteryState status)
      : level(level), status(status), device(InputManager::DeviceType::kHmd) {}

  BatteryStateChangedEvent(int level, InputManager::BatteryState status,
                           InputManager::DeviceType device)
      : level(level), status(status), device(device) {}

  bool operator==(const BatteryStateChangedEvent& other) const {
    return level == other.level && status == other.status &&
           device == other.device;
  }

  /// Battery level ranging from 0 to 100.
  int level;
  /// Battery status.
  InputManager::BatteryState status;
  /// Device whose battery state we are describing
  InputManager::DeviceType device;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::BatteryStateChangedEvent);

#endif  // LULLABY_EVENTS_BATTERY_STATE_CHANGED_H_
