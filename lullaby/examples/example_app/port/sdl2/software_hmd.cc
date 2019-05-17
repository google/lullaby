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

#include "lullaby/examples/example_app/port/sdl2/software_hmd.h"

#include "lullaby/modules/camera/camera_manager.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/device_util.h"
#include "lullaby/util/math.h"

namespace {
const float kStereoEyeOffset = 0.031f;
}  // namespace
namespace lull {
SoftwareHmd::SoftwareHmd(Registry* registry, size_t width, size_t height,
                         bool stereo, float near_clip_plane,
                         float far_clip_plane)
    : registry_(registry),
      width_(width),
      height_(height),
      num_eyes_(stereo ? 2 : 1),
      translation_(0.f, 0.f, 0.f),
      rotation_(0.f, 0.f, 0.f) {
  const float kFovAngle = 90.f * kDegreesToRadians;
  const size_t viewport_width = stereo ? width / 2 : width;

  // Configure the cameras:
  for (int i = 0; i < num_eyes_; ++i) {
    const float viewport_x = i * viewport_width;
    cameras_[i] = std::make_shared<MutableCamera>(registry_);
    cameras_[i]->SetupDisplay(
        near_clip_plane, far_clip_plane, kFovAngle,
        mathfu::recti(viewport_x, 0, viewport_width, height));

    if (stereo) {
      const float offset = (i == 0) ? kStereoEyeOffset : -kStereoEyeOffset;
      const mathfu::vec3 translation(offset, 0.0f, 0.0f);
      cameras_[i]->SetCameraFromSensor(
          mathfu::mat4::FromTranslationVector(translation));
    }
  }

  DeviceProfile profile = GetCardboardHeadsetProfile();
  profile.eyes.resize(num_eyes_);

  InputManager* input_manager = registry_->Get<InputManager>();
  input_manager->ConnectDevice(InputManager::kHmd, profile);

  CameraManager* camera_manager = registry_->Get<CameraManager>();
  camera_manager->RegisterScreenCamera(cameras_[0]);
  if (num_eyes_ > 1) {
    camera_manager->RegisterScreenCamera(cameras_[1]);
  }
}

SoftwareHmd::~SoftwareHmd() {
  InputManager* input_manager = registry_->Get<InputManager>();
  input_manager->DisconnectDevice(InputManager::kHmd);

  CameraManager* camera_manager = registry_->Get<CameraManager>();
  camera_manager->UnregisterScreenCamera(cameras_[0]);
  if (num_eyes_ > 1) {
    camera_manager->UnregisterScreenCamera(cameras_[1]);
  }
}

void SoftwareHmd::Update() {
  for (int i = 0; i < num_eyes_; ++i) {
    cameras_[i]->SetSensorPose(-translation_,
                               lull::FromEulerAnglesYXZ(rotation_));

    cameras_[i]->UpdateInputManagerEye(/* eye = */ i);
  }
  // Note: both cameras have the same sensor pose, so just use the 1st camera to
  // set the Hmd pose.
  cameras_[0]->UpdateInputManagerHmdPose();
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
      const mathfu::mat4 yaw =
          mathfu::quat::FromAngleAxis(rotation_.y, mathfu::kAxisY3f)
              .ToMatrix4();
      translation_ += yaw * delta_xy * kTranslationSensitivity;
      break;
    }
    case kTranslateXZ: {
      const mathfu::vec3 delta_xz(static_cast<float>(delta.x), 0.0f,
                                  static_cast<float>(delta.y));
      const mathfu::mat4 yaw =
          mathfu::quat::FromAngleAxis(rotation_.y, mathfu::kAxisY3f)
              .ToMatrix4();
      translation_ += yaw * delta_xz * kTranslationSensitivity;
      break;
    }
  }
}

}  // namespace lull
