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

#ifndef LULLABY_MODULES_CAMERA_MUTABLE_CAMERA_H_
#define LULLABY_MODULES_CAMERA_MUTABLE_CAMERA_H_

#include "lullaby/modules/camera/camera.h"
#include "lullaby/modules/camera/camera_manager.h"

namespace lull {

/// Various setters and utility functions on top of the default Camera class.
/// This class should be used for implementing custom camera setups.
class MutableCamera : public Camera {
 public:
  explicit MutableCamera(Registry* registry);

  /// Sets the camera's starting position.  Automatically called by
  /// camera_manager if the camera is attached to an entity.
  void SetWorldFromSensorStart(
      const mathfu::mat4& world_from_sensor_start) override;

  /// Sets the pose of the camera in sensor_start space.
  void SetSensorPose(const mathfu::vec3& sensor_pos_local,
                     const mathfu::quat& sensor_rot_local);

  /// Sets the offset from sensor pos to camera pos (i.e. eye_from_head).
  void SetCameraFromSensor(const mathfu::mat4& camera_from_sensor);

  /// Sets the projection matrix.  Note: this value will be over written if
  /// SetupDisplay, SetClipPlanes, or SetClipScale are called.
  void SetClipFromCamera(const mathfu::mat4& clip_from_camera);

  /// Sets the display rotation / device orientation (Landscape, Portrait, etc).
  void SetDisplayRotation(Rotation rotation);

  /// Sets the clip scale and recalculates the projection matrix. See
  /// Camera::ClipScale() for more details.
  void SetClipScale(const mathfu::vec3& clip_scale);

  /// Sets the near and far clip planes and recalculates the projection matrix.
  void SetClipPlanes(float near, float far);

  /// Recalculates the projection matrix based on clip planes, a vertical field
  /// of view, and a viewport.
  void SetupDisplay(float near, float far, float vertical_fov_radians,
                    const mathfu::recti& viewport);

  /// Recalculates the projection matrix based on clip planes, a field of view
  /// rectangle (left, right, bottom, top), and a viewport.
  void SetupDisplay(float near_clip, float far_clip, const mathfu::rectf& fov,
                    const mathfu::recti& viewport);

  /// Set the viewport without recalculating any values.  Use if you are
  /// directly setting the ClipFromCamera matrix.
  void SetViewport(const mathfu::recti& viewport);

  /// Recalculates ClipFromCamera using the clip planes, the field of view, and
  /// the clip scale as a perspective projection matrix.  This is automatically
  /// called when SetupDisplay, SetClipPlanes, or SetClipScale are called.
  void RecalculatePerspectiveProjection();

  /// Recalculates derived matrices based on SensorStart space, Sensor Pose,
  /// SensorFromCamera, and ClipFromCamera.  This is automatically called when
  /// any of those are changed using the setters above.
  void RecalculateClipFromWorld();

  /// Set the tracking state.
  void SetIsCameraTracking(bool tracking);

  // Functions to update the InputManager's Hmd from a camera.  These are
  // primarily for supporting legacy apps that rely on the InputManager to
  // populate render views.
  void UpdateInputManagerHmdPose();
  void UpdateInputManagerEye(int32_t eye);

  /// Create a field of view rect from a vertical fov and a viewport.
  /// The rect is composed of radian angles out from the center of view, in the
  /// order (left, right, bottom, top).
  static mathfu::rectf MakeFovRect(float vertical_fov_radians,
                                   const mathfu::recti& viewport);

 protected:
  /// Create a ClipFromCamera matrix using the clip planes, the field of view,
  /// and the clip scale.
  mathfu::mat4 MakePerspectiveProjection() const;
  Registry* registry_;
};
}  // namespace lull

#endif  // LULLABY_MODULES_CAMERA_MUTABLE_CAMERA_H_
