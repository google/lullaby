/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_MODULES_FLATBUFFERS_COMMON_FB_CONVERSIONS_H_
#define LULLABY_MODULES_FLATBUFFERS_COMMON_FB_CONVERSIONS_H_

#include "lullaby/generated/common_generated.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/logging.h"

namespace lull {

// ---------------------------------------------------------------------------
// Functions to convert from flatbuffer common types.
// ---------------------------------------------------------------------------
inline InputManager::DeviceType TranslateInputDeviceType(
    DeviceType device_type) {
  switch (device_type) {
    case DeviceType_Hmd:
      return InputManager::kHmd;
    case DeviceType_Mouse:
      return InputManager::kMouse;
    case DeviceType_Keyboard:
      return InputManager::kKeyboard;
    case DeviceType_Controller:
      return InputManager::kController;
    case DeviceType_Controller2:
      return InputManager::kController2;
    case DeviceType_Hand:
      return InputManager::kHand;
    default:
      DCHECK(false);
      return InputManager::kController;
  }
}

}  // namespace lull

#endif  // LULLABY_MODULES_FLATBUFFERS_COMMON_FB_CONVERSIONS_H_
