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

#ifndef LULLABY_MODULES_CAMERA_CAMERA_H_
#define LULLABY_MODULES_CAMERA_CAMERA_H_

#include "lullaby/modules/render/render_view.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"

namespace lull {

// Device orientation for mobile.
enum class DeviceOrientation {
  kPortrait,
  kLandscape,
  kReverseLandscape,
  kUnknown
};

/// A class containing all of the information needed to go between screen space
/// (or render target space) and world space.
/// The Projection Matrix is the ClipFromCamera matrix.
/// The View matrix is created by combining the WorldFromSensorStart, Sensor
/// Pose, and CameraFromSensor transforms.
///
/// Spaces Explanation:
///
///
/// World Space: The root of the Entity Scene Graph.
/// SensorStart Space: The coordinates that the API controlling the camera
///   reports the pose in.  Often the same as World Space.
/// Sensor Space: The space of the device driving the camera.  On a smartphone
///   this will usually be the same as camera space.  On an HMD this will be
///   the head pose.
/// Camera Space:  The local space of this camera.  On an HMD this will be an
///   eye's space.
/// Clip Space:  The space after applying the perspective transform.  Values
///   should be in the range ([-1, 1], [-1, 1], [0,1]). +y is up, +x is right,
///   z = 0 is the near clip plane, and z = 1 is the far clip plane.  (0,0,0)
///   is the center of the space.
/// Camera Texture Space (or UV Space): the space of the texture the camera will
///   render into.  Values should be in the range ([0, 1), [0, 1)).  +y is down,
///   +x is right, 0,0 is the top left corner.
/// Pixel Space:  The Camera Texture Space scaled and translated into the
///   viewport.  Only available if the viewport has been set up.  Values should
///   be in the range ([x, x + width], [y, y+height]).  +y is down, +x is right,
///   0,0 is the top left corner.
class Camera {
 public:
  // Equivalents to android.view.Surface.ROTATION_* constants.
  enum Rotation : int {
    kRotation0 = 0,
    kRotation90 = 1,
    kRotation180 = 2,
    kRotation270 = 3,
  };

  Camera() {}
  virtual ~Camera() {}

  /// Returns the view projection matrix.
  const mathfu::mat4& ClipFromWorld() const;
  /// Returns the inverse view projection matrix.
  const mathfu::mat4& WorldFromClip() const;

  /// Returns the projection matrix.
  const mathfu::mat4& ClipFromCamera() const;
  /// Returns the inverse projection matrix.
  const mathfu::mat4& CameraFromClip() const;

  /// Returns the view matrix.
  const mathfu::mat4& CameraFromWorld() const;
  /// Returns the inverse view matrix.
  const mathfu::mat4& WorldFromCamera() const;

  /// Returns the first part of the view matrix.
  /// If an entity exists for this camera, this should be that entity's parent's
  /// transform.  Use this if your World space doesn't match the space that the
  /// camera reports its pose in.
  const mathfu::mat4& WorldFromSensorStart() const;
  const mathfu::mat4& SensorStartFromWorld() const;

  /// Returns the sensor's pose in SensorStart space.  This pose is used to
  /// calculate the second part of the view matrix.
  const mathfu::vec3& LocalPosition() const;
  const mathfu::quat& LocalRotation() const;

  /// Returns the camera's pose in world space.  This includes the SensorStart
  /// and CameraFromSensor transforms.
  mathfu::vec3 WorldPosition() const;
  mathfu::quat WorldRotation() const;

  /// Returns the third part of the view matrix, representing the transfrom
  /// between the sensor's pose and the actual camera position.  This is usually
  /// used to go from Head (Sensor) to Eye (Camera) when using a head mounted
  /// display.
  const mathfu::mat4& CameraFromSensor() const;
  const mathfu::mat4& SensorFromCamera() const;

  /// Camera parameters
  float NearClip() const;
  float FarClip() const;
  const mathfu::rectf& Fov() const;
  const mathfu::recti& Viewport() const;
  /// A scale transform applied to the projection matrix, default (1, 1, 1).
  /// For inward facing cameras the x scale should be -1. If an odd number of
  /// axis are reversed, clip space will become left handed (world space remains
  /// unchanged). One effect of this is that face culling for meshes will need
  /// to be the opposite direction compared to the default.
  const mathfu::vec3& ClipScale() const;
  /// The rotation applied between camera texture space and screen space.
  Rotation DisplayRotation() const;
  /// True if the camera is providing valid data.
  bool IsCameraTracking() const;

  /// Derived Camera parameters
  /// The width of the viewport, in pixels.
  int Width() const;
  /// The height of the viewport, in pixels.
  int Height() const;
  /// Named values for different rotations (Portrait, Landscape, etc).
  DeviceOrientation Orientation() const;

  /// Convert from DeviceOrientation to a DisplayRotation value.
  static Rotation ToDisplayRotation(DeviceOrientation orientation);

  /// Populates a render view to match this camera.
  virtual void PopulateRenderView(RenderView* view) const;

  /// Sets the camera's starting position.  Automatically called by
  /// camera_manager if the camera is attached to an entity.
  virtual void SetWorldFromSensorStart(
      const mathfu::mat4& world_from_sensor_start) = 0;

  // Conversion functions:
  /// Project a ray from a clip coordinate into world space. |clip_point|'s
  /// values should be in the range ([-1,1], [-1,1], [0,1]).
  Ray WorldRayFromClipPoint(const mathfu::vec3& clip_point) const;

  /// Project a ray from a camera texture coordinate into world space. |uv|
  /// should have values in the range [0,1).
  Ray WorldRayFromUV(const mathfu::vec2& uv) const;

  /// Project a ray from a pixel into world space. |pixel| should have values in
  /// the viewport.  Returns an unset Optional if the viewport is not set up.
  Optional<Ray> WorldRayFromPixel(const mathfu::vec2& pixel) const;

  /// Convert a point in world space to a pixel.  If the |world_point| is
  /// outside the view frustrum, returned pixel may have NaN values.  Returns an
  /// unset Optional if the viewport is not set up.
  Optional<mathfu::vec2> PixelFromWorldPoint(
      const mathfu::vec3& world_point) const;

  /// Convert a point in clip space to world space. |clip_point|'s values should
  /// be in the range ([-1,1], [-1,1], [0,1]) for a result in the view frustrum.
  mathfu::vec3 WorldPointFromClip(const mathfu::vec3& clip_point) const;

  /// Convert a point in world space to clip space.  If |world_point| is in the
  /// view frustrum, the result will be in the range ([-1,1], [-1,1], [0,1]).
  mathfu::vec3 ClipFromWorldPoint(const mathfu::vec3& world_point) const;

  /// Convert a point in world space to camera texture space.  If |world_point|
  /// is in the view frustrum, the result's values will be in the range [0,1].
  mathfu::vec2 UVFromWorldPoint(const mathfu::vec3& world_point) const;

  /// Convert a point in pixel space to clip space.  If |pixel|'s values
  /// are in the viewport, the result will be in the range ([-1,1], [-1,1], 0).
  /// Returns an unset Optional if the viewport is not set up.
  Optional<mathfu::vec3> ClipFromPixel(const mathfu::vec2& screen_pixel) const;

  /// Convert a point in clip space to a pixel.  If |clip_points|'s
  /// values are in the range ([-1,1], [-1,1], [0, 1]), the pixel will be in the
  /// viewport.  Returns an unset Optional if the viewport is not set up.
  Optional<mathfu::vec2> PixelFromClip(const mathfu::vec3& clip_point) const;

  /// Convert a pixel to a UV coordinate.  If the pixel is inside the viewport,
  /// the result will be in the range [0, 1).  Returns an unset Optional if the
  /// viewport is not set up.
  Optional<mathfu::vec2> UVFromPixel(const mathfu::vec2& pixel) const;

  /// Convert a uv point in camera texture space to a pixel.  If the uv
  /// coordinate is in the range [0, 1), the pixel will be inside the viewport.
  /// Returns an unset Optional if the viewport is not set up.
  Optional<mathfu::vec2> PixelFromUV(const mathfu::vec2& uv) const;

  /// Convert a uv point in camera texture space to clip space.  If the uv
  /// is in the range [0, 1), the result will be in the range ([-1,1], [-1,1],
  /// 0). This will flip the Y value, since texture space has y == 0 as the top,
  /// and clip space has y == 1 as the top.
  static mathfu::vec3 ClipFromUV(const mathfu::vec2& uv);
  /// Convert a point in clip space to camera texture space.  If the point is in
  /// the range ([-1,1], [-1,1], 0), the result will be in the range  [0, 1).
  /// This will flip the Y value, since texture space has y == 0 as the top, and
  /// clip space has y == 1 as the top.
  static mathfu::vec2 UVFromClip(const mathfu::vec3& clip_point);

 protected:
  mathfu::mat4 clip_from_world_ = mathfu::mat4::Identity();
  mathfu::mat4 world_from_clip_ = mathfu::mat4::Identity();
  mathfu::mat4 clip_from_camera_ = mathfu::mat4::Identity();
  mathfu::mat4 camera_from_clip_ = mathfu::mat4::Identity();
  mathfu::mat4 camera_from_world_ = mathfu::mat4::Identity();
  mathfu::mat4 world_from_camera_ = mathfu::mat4::Identity();
  mathfu::mat4 world_from_sensor_start_ = mathfu::mat4::Identity();
  mathfu::mat4 sensor_start_from_world_ = mathfu::mat4::Identity();
  mathfu::mat4 camera_from_sensor_ = mathfu::mat4::Identity();
  mathfu::mat4 sensor_from_camera_ = mathfu::mat4::Identity();

  mathfu::vec3 sensor_pos_local_ = mathfu::kZeros3f;
  mathfu::quat sensor_rot_local_ = mathfu::kQuatIdentityf;

  float near_clip_ = 0.01f;
  float far_clip_ = 200.0f;
  mathfu::rectf fov_ = mathfu::rectf(mathfu::kZeros4f);
  mathfu::recti viewport_ = mathfu::recti(mathfu::kZeros4i);
  mathfu::vec3 clip_scale_ = mathfu::kOnes3f;
  Rotation display_rotation_ = kRotation0;
  bool tracking_ = false;
};

using CameraPtr = std::shared_ptr<Camera>;
using CameraList = std::vector<CameraPtr>;

}  // namespace lull

#endif  // LULLABY_MODULES_CAMERA_CAMERA_H_
