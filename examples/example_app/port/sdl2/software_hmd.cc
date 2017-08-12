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

#include "lullaby/examples/example_app/port/sdl2/software_hmd.h"

#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/math.h"

namespace lull {

SoftwareHmd::SoftwareHmd(Registry* registry, size_t width, size_t height,
                         bool stereo)
      : registry_(registry),
        width_(width),
        height_(height),
        num_eyes_(stereo ? 2 : 1),
        eye_offset_(stereo ? 0.031f : 0.0f),
        translation_(0.f, 0.f, 0.f),
        rotation_(0.f, 0.f, 0.f) {
  const float kFovAngle = 45.f * kDegreesToRadians;
  const size_t viewport_width = stereo ? width / 2 : width;
  const float aspect_ratio = static_cast<float>(viewport_width) /
                             static_cast<float>(height);
  const float hfov = atanf(tanf(kFovAngle) * aspect_ratio);
  fov_[0] = mathfu::rectf(hfov, hfov, kFovAngle, kFovAngle);
  fov_[1] = mathfu::rectf(hfov, hfov, kFovAngle, kFovAngle);
  viewport_[0] = mathfu::recti(0, 0, viewport_width, height);
  viewport_[1] = mathfu::recti(viewport_width, 0, viewport_width, height);

  InputManager::DeviceParams params;
  params.num_eyes = num_eyes_;
  params.has_rotation_dof = true;
  params.has_position_dof = true;
  params.num_buttons = 1;

  InputManager* input_manager = registry_->Get<InputManager>();
  input_manager->ConnectDevice(InputManager::kHmd, params);
}

SoftwareHmd::~SoftwareHmd() {
  InputManager* input_manager = registry_->Get<InputManager>();
  input_manager->DisconnectDevice(InputManager::kHmd);
}

void SoftwareHmd::Update() {
  InputManager* input_manager = registry_->Get<InputManager>();
  input_manager->UpdatePosition(InputManager::kHmd, -translation_);
  input_manager->UpdateRotation(
      InputManager::kHmd, mathfu::quat::FromEulerAngles(rotation_));

  for (int i = 0; i < num_eyes_; ++i) {
    const float offset = (i == 0) ? -eye_offset_ : eye_offset_;
    const mathfu::vec3 translation(offset, 0.0f, 0.0f);
    const mathfu::mat4 eye_from_head =
        mathfu::mat4::FromTranslationVector(translation);
    input_manager->UpdateEye(InputManager::kHmd, i, eye_from_head, fov_[i],
                             viewport_[i]);
  }
}

void SoftwareHmd::OnMouseMotion(const mathfu::vec2i& delta, ControlMode mode) {
  static constexpr float kTranslationSensitivity = 0.01f;
  static constexpr float kRotationSensitivity = 0.25f * kDegreesToRadians;
  switch (mode) {
    case kRotatePitchYaw: {
      rotation_.y += delta.x * kRotationSensitivity;
      rotation_.x += delta.y * kRotationSensitivity;
      break;
    }
    case kRotateRoll: {
      rotation_.z += delta.y * kRotationSensitivity;
      break;
    }
    case kTranslateXY: {
      const mathfu::vec3 delta_xy(static_cast<float>(delta.x),
                                  -static_cast<float>(delta.y), 0.0f);
      const mathfu::mat4 yaw = mathfu::quat::FromAngleAxis(
          -rotation_.y, mathfu::kAxisY3f).ToMatrix4();
      translation_ += yaw * delta_xy * kTranslationSensitivity;
      break;
    }
    case kTranslateXZ: {
      const mathfu::vec3 delta_xz(static_cast<float>(delta.x), 0.0f,
                                  static_cast<float>(delta.y));
      const mathfu::mat4 yaw = mathfu::quat::FromAngleAxis(
          -rotation_.y, mathfu::kAxisY3f).ToMatrix4();
      translation_ += yaw * delta_xz * kTranslationSensitivity;
      break;
    }
  }
}

}  // namespace lull
