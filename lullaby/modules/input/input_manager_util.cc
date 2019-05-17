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

#include "lullaby/modules/input/input_manager_util.h"

namespace lull {
Ray CalculateDeviceSelectionRay(Registry* registry,
                                InputManager::DeviceType device) {
  auto* input = registry->Get<InputManager>();

  // If the device has a default local space ray, use that instead.
  const Variant selection_ray = registry->Get<InputManager>()->GetDeviceInfo(
      device, ConstHash("SelectionRay"));
  Ray result = selection_ray.ValueOr(Ray(mathfu::kZeros3f, -mathfu::kAxisZ3f));

  if (input && input->HasRotationDof(device)) {
    mathfu::quat rotation = input->GetDofRotation(device);
    result.origin = rotation * result.origin;

    if (input->HasPositionDof(device)) {
      result.origin += input->GetDofPosition(device);
    }
    result.direction = rotation * result.direction;
  }
  return result;
}

Sqt GetHmdSqt(Registry* registry) {
  auto* input_manager = registry->Get<InputManager>();
  return Sqt(input_manager->GetDofPosition(InputManager::kHmd),
             input_manager->GetDofRotation(InputManager::kHmd),
             mathfu::kOnes3f);
}

InputManager::ButtonId GetButtonByType(const DeviceProfile* profile,
                                       DeviceProfile::Button::Type type) {
  if (profile == nullptr) {
    return InputManager::kInvalidButton;
  }

  // The ButtonId is the index of the button in the DeviceProfile.
  for (int i = 0; i < profile->buttons.size(); ++i) {
    if (profile->buttons[i].type == type) {
      return static_cast<InputManager::ButtonId>(i);
    }
  }
  return InputManager::kInvalidButton;
}

}  // namespace lull
