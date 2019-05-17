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

#include "lullaby/modules/camera/mutable_camera.h"

namespace lull {

MutableCamera::MutableCamera(Registry* registry) : registry_(registry) {}

void MutableCamera::SetWorldFromSensorStart(
    const mathfu::mat4& world_from_sensor_start) {
  world_from_sensor_start_ = world_from_sensor_start;
  sensor_start_from_world_ = world_from_sensor_start.Inverse();
  RecalculateClipFromWorld();
}

void MutableCamera::SetSensorPose(const mathfu::vec3& sensor_pos_local,
                                  const mathfu::quat& sensor_rot_local) {
  sensor_pos_local_ = sensor_pos_local;
  sensor_rot_local_ = sensor_rot_local;
  RecalculateClipFromWorld();
}

void MutableCamera::SetCameraFromSensor(
    const mathfu::mat4& camera_from_sensor) {
  camera_from_sensor_ = camera_from_sensor;
  sensor_from_camera_ = camera_from_sensor_.Inverse();
  RecalculateClipFromWorld();
}

void MutableCamera::SetClipFromCamera(const mathfu::mat4& clip_from_camera) {
  clip_from_camera_ = clip_from_camera;
  camera_from_clip_ = clip_from_camera_.Inverse();
  RecalculateClipFromWorld();
}

void MutableCamera::SetDisplayRotation(Rotation rotation) {
  display_rotation_ = rotation;
}

void MutableCamera::SetClipScale(const mathfu::vec3& clip_scale) {
  clip_scale_ = clip_scale;
  RecalculatePerspectiveProjection();
}

void MutableCamera::SetClipPlanes(float near, float far) {
  SetupDisplay(near, far, fov_, viewport_);
}

void MutableCamera::SetupDisplay(float near, float far,
                                 float vertical_fov_radians,
                                 const mathfu::recti& viewport) {
  const mathfu::rectf fov = MakeFovRect(vertical_fov_radians, viewport);
  SetupDisplay(near, far, fov, viewport);
}

void MutableCamera::SetupDisplay(float near_clip, float far_clip,
                                 const mathfu::rectf& fov,
                                 const mathfu::recti& viewport) {
  near_clip_ = near_clip;
  far_clip_ = far_clip;
  fov_ = fov;
  viewport_ = viewport;
  RecalculatePerspectiveProjection();
}

void MutableCamera::SetViewport(const mathfu::recti& viewport) {
  viewport_ = viewport;
}

void MutableCamera::RecalculateClipFromWorld() {
  mathfu::mat4 sensor_start_from_sensor = CalculateTransformMatrix(
      LocalPosition(), LocalRotation(), mathfu::kOnes3f);

  world_from_camera_ =
      WorldFromSensorStart() * sensor_start_from_sensor * SensorFromCamera();
  camera_from_world_ = world_from_camera_.Inverse();
  world_from_clip_ = world_from_camera_ * camera_from_clip_;
  clip_from_world_ = clip_from_camera_ * camera_from_world_;
}

mathfu::mat4 MutableCamera::MakePerspectiveProjection() const {
  return CalculatePerspectiveMatrixFromView(fov_, near_clip_, far_clip_) *
         mathfu::mat4::FromScaleVector(clip_scale_);
}

void MutableCamera::RecalculatePerspectiveProjection() {
  clip_from_camera_ = MakePerspectiveProjection();
  camera_from_clip_ = clip_from_camera_.Inverse();
  world_from_clip_ = world_from_camera_ * camera_from_clip_;
  clip_from_world_ = clip_from_camera_ * camera_from_world_;
}

void MutableCamera::SetIsCameraTracking(bool tracking) { tracking_ = tracking; }

void MutableCamera::UpdateInputManagerHmdPose() {
  if (registry_) {
    InputManager* input_manager = registry_->Get<InputManager>();
    if (input_manager) {
      input_manager->UpdatePosition(InputManager::kHmd, LocalPosition());
      input_manager->UpdateRotation(InputManager::kHmd, LocalRotation());
    }
  }
}

void MutableCamera::UpdateInputManagerEye(int32_t eye) {
  if (registry_) {
    InputManager* input_manager = registry_->Get<InputManager>();
    if (input_manager) {
      input_manager->UpdateEye(InputManager::kHmd, eye, CameraFromSensor(),
                               ClipFromCamera(), Fov(), Viewport());
    }
  }
}

mathfu::rectf MutableCamera::MakeFovRect(float vertical_fov_radians,
                                         const mathfu::recti& viewport) {
  float half_vfov = vertical_fov_radians / 2.0f;
  const float aspect_ratio =
      static_cast<float>(viewport.size.x) / static_cast<float>(viewport.size.y);
  const float half_hfov = atanf(tanf(half_vfov) * aspect_ratio);
  return mathfu::rectf(half_hfov, half_hfov, half_vfov, half_vfov);
}

}  // namespace lull
