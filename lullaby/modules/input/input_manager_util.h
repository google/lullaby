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

#ifndef LULLABY_UTIL_INPUT_MANAGER_UTIL_H_
#define LULLABY_UTIL_INPUT_MANAGER_UTIL_H_

#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"

namespace lull {

// Get the head pose of the HMD.
Sqt GetHmdSqt(Registry* registry);

// Calculates the selection ray from a device with rotation
Ray CalculateDeviceSelectionRay(Registry* registry,
                                InputManager::DeviceType device);

// Gets the ButtonId of the first |type| button on the given |profile|.
// If the device does not have a button of the given |type|, this will return
// kInvalidButton.
// This is useful for buttons that don't have standardized IDs in the
// InputManager.
InputManager::ButtonId GetButtonByType(const DeviceProfile* profile,
                                       DeviceProfile::Button::Type type);

}  // namespace lull

#endif  // LULLABY_UTIL_INPUT_MANAGER_UTIL_H_
