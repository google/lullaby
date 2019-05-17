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

#include "lullaby/modules/camera/camera.h"
#include "mathfu/io.h"

namespace lull {
const mathfu::mat4& Camera::ClipFromWorld() const { return clip_from_world_; }
const mathfu::mat4& Camera::WorldFromClip() const { return world_from_clip_; }
const mathfu::mat4& Camera::ClipFromCamera() const { return clip_from_camera_; }
const mathfu::mat4& Camera::CameraFromClip() const { return camera_from_clip_; }
const mathfu::mat4& Camera::CameraFromWorld() const {
  return camera_from_world_;
}
const mathfu::mat4& Camera::WorldFromCamera() const {
  return world_from_camera_;
}
const mathfu::mat4& Camera::WorldFromSensorStart() const {
  return world_from_sensor_start_;
}
const mathfu::mat4& Camera::SensorStartFromWorld() const {
  return sensor_start_from_world_;
}
const mathfu::vec3& Camera::LocalPosition() const { return sensor_pos_local_; }
const mathfu::quat& Camera::LocalRotation() const { return sensor_rot_local_; }
mathfu::vec3 Camera::WorldPosition() const {
  return world_from_camera_.TranslationVector3D();
}
mathfu::quat Camera::WorldRotation() const {
  return mathfu::quat::FromMatrix(world_from_camera_);
}
const mathfu::mat4& Camera::CameraFromSensor() const {
  return camera_from_sensor_;
}
const mathfu::mat4& Camera::SensorFromCamera() const {
  return sensor_from_camera_;
}
float Camera::NearClip() const { return near_clip_; }
float Camera::FarClip() const { return far_clip_; }
const mathfu::rectf& Camera::Fov() const { return fov_; }
const mathfu::recti& Camera::Viewport() const { return viewport_; }
const mathfu::vec3& Camera::ClipScale() const { return clip_scale_; }
Camera::Rotation Camera::DisplayRotation() const { return display_rotation_; }
bool Camera::IsCameraTracking() const { return tracking_; }
int Camera::Width() const { return viewport_.size.x; }
int Camera::Height() const { return viewport_.size.y; }

DeviceOrientation Camera::Orientation() const {
  switch (display_rotation_) {
    case kRotation0:
      return DeviceOrientation::kPortrait;
    case kRotation90:
      return DeviceOrientation::kLandscape;
    case kRotation270:
      return DeviceOrientation::kReverseLandscape;
    case kRotation180:
    default:
      return DeviceOrientation::kUnknown;
  }
}

Camera::Rotation Camera::ToDisplayRotation(DeviceOrientation orientation) {
  switch (orientation) {
    case DeviceOrientation::kPortrait:
      // android.view.Surface.ROTATION_0
      return kRotation0;
    case DeviceOrientation::kLandscape:
      // android.view.Surface.ROTATION_90
      return kRotation90;
    case DeviceOrientation::kReverseLandscape:
      // android.view.Surfce.ROTATION_270
      return kRotation270;
    case DeviceOrientation::kUnknown:
    default:
      // Default to no rotation for unexpected and unknown.
      return kRotation0;
  }
}

void Camera::PopulateRenderView(RenderView* view) const {
  view->viewport = viewport_.pos;
  view->dimensions = viewport_.size;
  view->world_from_eye_matrix = world_from_camera_;
  view->eye_from_world_matrix = camera_from_world_;
  view->clip_from_eye_matrix = clip_from_camera_;
  view->clip_from_world_matrix = clip_from_world_;
}

Ray Camera::WorldRayFromClipPoint(const mathfu::vec3& clip_point) const {
  return CalculateRayFromCamera(WorldPosition(), world_from_clip_,
                                clip_point.xy());
}

Ray Camera::WorldRayFromUV(const mathfu::vec2& uv) const {
  return WorldRayFromClipPoint(ClipFromUV(uv));
}

Optional<Ray> Camera::WorldRayFromPixel(const mathfu::vec2& pixel) const {
  Optional<mathfu::vec3> clip = ClipFromPixel(pixel);
  return clip ? WorldRayFromClipPoint(clip.value()) : Optional<Ray>();
}

Optional<mathfu::vec2> Camera::PixelFromWorldPoint(
    const mathfu::vec3& world_point) const {
  return PixelFromClip(ClipFromWorldPoint(world_point));
}

mathfu::vec3 Camera::WorldPointFromClip(const mathfu::vec3& clip_point) const {
  return world_from_clip_ * clip_point;
}

mathfu::vec3 Camera::ClipFromWorldPoint(const mathfu::vec3& world_point) const {
  return clip_from_world_ * world_point;
}

mathfu::vec2 Camera::UVFromWorldPoint(const mathfu::vec3& world_point) const {
  return UVFromClip(ClipFromWorldPoint(world_point));
}

Optional<mathfu::vec3> Camera::ClipFromPixel(const mathfu::vec2& pixel) const {
  Optional<mathfu::vec2> uv = UVFromPixel(pixel);
  return uv ? ClipFromUV(uv.value()) : Optional<mathfu::vec3>();
}

Optional<mathfu::vec2> Camera::PixelFromClip(const mathfu::vec3& clip_point) const {
  return PixelFromUV(UVFromClip(clip_point));
}

Optional<mathfu::vec2> Camera::UVFromPixel(const mathfu::vec2& pixel) const {
  if (viewport_.size.x <= 0 || viewport_.size.y <= 0) {
    return Optional<mathfu::vec2>();
  }
  // Convert pixel to [0,1].
  return mathfu::vec2((pixel.x - viewport_.pos.x) / viewport_.size.x,
                      (pixel.y - viewport_.pos.y) / viewport_.size.y);
}

Optional<mathfu::vec2> Camera::PixelFromUV(const mathfu::vec2& uv) const {
  if (viewport_.size.x <= 0 || viewport_.size.y <= 0) {
    return Optional<mathfu::vec2>();
  }
  // Convert from [0,1] to [pos, pos+size].
  return mathfu::vec2(viewport_.pos.x + uv.x * viewport_.size.x,
                      viewport_.pos.y + uv.y * viewport_.size.y);
}

mathfu::vec3 Camera::ClipFromUV(const mathfu::vec2& uv) {
  // Convert to [-1,1].  Also flip y, so that +y is up.
  return mathfu::vec3(2.0f * (uv.x - 0.5f), -2.0f * (uv.y - 0.5f), 0.0f);
}

mathfu::vec2 Camera::UVFromClip(const mathfu::vec3& clip_point) {
  // Convert from [-1,1] to [0,1].
  // Also flip y, so that +y is down (0,0 is top left pixel)
  return mathfu::vec2(0.5f + (clip_point.x * 0.5f),
                      0.5f - (clip_point.y * 0.5f));
}

}  // namespace lull
