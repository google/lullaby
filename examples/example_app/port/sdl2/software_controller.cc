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

#include "lullaby/examples/example_app/port/sdl2/software_controller.h"

#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/math.h"

namespace lull {

SoftwareController::SoftwareController(Registry* registry)
      : registry_(registry),
        rotation_(0.f, 0.f, 0.f) {
  InputManager::DeviceParams params;
  params.has_rotation_dof = true;
  params.has_position_dof = true;
  params.num_buttons = 1;

  InputManager* input_manager = registry_->Get<InputManager>();
  input_manager->ConnectDevice(InputManager::kController, params);
}

SoftwareController::~SoftwareController() {
  InputManager* input_manager = registry_->Get<InputManager>();
  input_manager->DisconnectDevice(InputManager::kController);
}

void SoftwareController::Update() {
  InputManager* input_manager = registry_->Get<InputManager>();
  input_manager->UpdatePosition(InputManager::kController, mathfu::kZeros3f);
  input_manager->UpdateRotation(
      InputManager::kController, mathfu::quat::FromEulerAngles(rotation_));
  input_manager->UpdateButton(InputManager::kController,
                              InputManager::kPrimaryButton, pressed_, false);
}

void SoftwareController::OnMouseMotion(const mathfu::vec2i& delta) {
  static const float kRotationSensitivity = 0.25f * kDegreesToRadians;
  rotation_.y -= delta.x * kRotationSensitivity;
  rotation_.x -= delta.y * kRotationSensitivity;
}

void SoftwareController::OnButtonDown() {
  pressed_ = true;
}

void SoftwareController::OnButtonUp() {
  pressed_ = false;
}

}  // namespace lull
