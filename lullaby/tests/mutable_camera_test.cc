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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using testing::NearMathfuMat4;
using testing::NearMathfuVec2;
using testing::NearMathfuVec3;

constexpr float kEpsilon = 1e-5f;
constexpr float kNearClip = 0.1f;
constexpr float kFarClip = 200.0f;
constexpr float kFov = 90.0f * kDegreesToRadians;
constexpr float kHalfFov = kFov / 2.0f;
const mathfu::recti kViewport(0, 0, 100, 100);
// Note: this fov rect only works for square viewports.
const mathfu::rectf kFovRect(kHalfFov, kHalfFov, kHalfFov, kHalfFov);

TEST(MutableCameraTest, FullMatrixStack) {
  MutableCamera camera(nullptr);

  // Setup a perspective projection:
  camera.SetupDisplay(kNearClip, kFarClip, kFov, mathfu::recti(0, 0, 100, 100));

  const mathfu::mat4 clip_from_camera =
      CalculatePerspectiveMatrixFromView(kFovRect, kNearClip, kFarClip);
  EXPECT_THAT(clip_from_camera,
              NearMathfuMat4(camera.ClipFromCamera(), kEpsilon));
  EXPECT_THAT(clip_from_camera.Inverse(),
              NearMathfuMat4(camera.CameraFromClip(), kEpsilon));

  mathfu::mat4 vp = clip_from_camera;
  // Check that ClipFromWorld is updated.
  EXPECT_THAT(clip_from_camera,
              NearMathfuMat4(camera.ClipFromWorld(), kEpsilon));
  EXPECT_THAT(clip_from_camera.Inverse(),
              NearMathfuMat4(camera.WorldFromClip(), kEpsilon));

  // Set the SensorStart:
  mathfu::mat4 world_from_sensor_start = CalculateTransformMatrix(
      mathfu::vec3(0.0, 1.0f, 0.0f), mathfu::kQuatIdentityf, mathfu::kOnes3f);
  camera.SetWorldFromSensorStart(world_from_sensor_start);
  EXPECT_THAT(world_from_sensor_start,
              NearMathfuMat4(camera.WorldFromSensorStart(), kEpsilon));
  EXPECT_THAT(world_from_sensor_start.Inverse(),
              NearMathfuMat4(camera.SensorStartFromWorld(), kEpsilon));
  // Check that WorldFromCamera is updated.
  mathfu::mat4 world_from_camera = world_from_sensor_start;
  EXPECT_THAT(world_from_camera,
              NearMathfuMat4(camera.WorldFromCamera(), kEpsilon));
  EXPECT_THAT(world_from_camera.Inverse(),
              NearMathfuMat4(camera.CameraFromWorld(), kEpsilon));
  // Check that ClipFromWorld is updated.
  vp = clip_from_camera * world_from_camera.Inverse();
  EXPECT_THAT(vp, NearMathfuMat4(camera.ClipFromWorld(), kEpsilon));
  EXPECT_THAT(vp.Inverse(), NearMathfuMat4(camera.WorldFromClip(), kEpsilon));

  EXPECT_EQ(mathfu::vec3(0.0f, 0.0f, 0.0f), camera.LocalPosition());
  EXPECT_EQ(mathfu::vec3(0.0f, 1.0f, 0.0f), camera.WorldPosition());

  // Set the sensor pose:
  const mathfu::vec3 sensor_pos(0.0f, 0.0f, 1.0f);
  const mathfu::quat sensor_rot = mathfu::kQuatIdentityf;
  camera.SetSensorPose(sensor_pos, sensor_rot);
  // Check that WorldFromCamera is updated.
  world_from_camera =
      world_from_sensor_start *
      CalculateTransformMatrix(sensor_pos, sensor_rot, mathfu::kOnes3f);
  EXPECT_THAT(world_from_camera,
              NearMathfuMat4(camera.WorldFromCamera(), kEpsilon));
  EXPECT_THAT(world_from_camera.Inverse(),
              NearMathfuMat4(camera.CameraFromWorld(), kEpsilon));
  // Check that ClipFromWorld is updated.
  vp = clip_from_camera * world_from_camera.Inverse();
  EXPECT_THAT(vp, NearMathfuMat4(camera.ClipFromWorld(), kEpsilon));
  EXPECT_THAT(vp.Inverse(), NearMathfuMat4(camera.WorldFromClip(), kEpsilon));

  EXPECT_EQ(mathfu::vec3(0.0f, 0.0f, 1.0f), camera.LocalPosition());
  EXPECT_EQ(mathfu::vec3(0.0f, 1.0f, 1.0f), camera.WorldPosition());

  // Set the Eye offset:
  mathfu::mat4 sensor_from_camera = CalculateTransformMatrix(
      mathfu::vec3(1.0, 0.0f, 0.0f), mathfu::kQuatIdentityf, mathfu::kOnes3f);
  camera.SetCameraFromSensor(sensor_from_camera.Inverse());
  EXPECT_THAT(sensor_from_camera,
              NearMathfuMat4(camera.SensorFromCamera(), kEpsilon));
  EXPECT_THAT(sensor_from_camera.Inverse(),
              NearMathfuMat4(camera.CameraFromSensor(), kEpsilon));
  // Check that WorldFromCamera is updated.
  world_from_camera = world_from_camera * sensor_from_camera;
  EXPECT_THAT(world_from_camera,
              NearMathfuMat4(camera.WorldFromCamera(), kEpsilon));
  EXPECT_THAT(world_from_camera.Inverse(),
              NearMathfuMat4(camera.CameraFromWorld(), kEpsilon));
  // Check that ClipFromWorld is updated.
  vp = clip_from_camera * world_from_camera.Inverse();
  EXPECT_THAT(vp, NearMathfuMat4(camera.ClipFromWorld(), kEpsilon));
  EXPECT_THAT(vp.Inverse(), NearMathfuMat4(camera.WorldFromClip(), kEpsilon));

  EXPECT_EQ(mathfu::vec3(0.0f, 0.0f, 1.0f), camera.LocalPosition());
  EXPECT_EQ(mathfu::vec3(1.0f, 1.0f, 1.0f), camera.WorldPosition());
}

TEST(MutableCameraTest, PopulateRenderView) {
  MutableCamera camera(nullptr);

  // Setup a perspective projection:
  camera.SetupDisplay(kNearClip, kFarClip, kFov, mathfu::recti(0, 0, 100, 100));

  const mathfu::mat4 clip_from_camera =
      CalculatePerspectiveMatrixFromView(kFovRect, kNearClip, kFarClip);
  EXPECT_EQ(clip_from_camera, camera.ClipFromCamera());
  EXPECT_EQ(clip_from_camera.Inverse(), camera.CameraFromClip());

  // Set the SensorStart:
  mathfu::mat4 world_from_sensor_start = CalculateTransformMatrix(
      mathfu::vec3(0.0, 1.0f, 0.0f), mathfu::kQuatIdentityf, mathfu::kOnes3f);
  camera.SetWorldFromSensorStart(world_from_sensor_start);
  // Set the sensor pose:
  const mathfu::vec3 sensor_pos(0.0f, 0.0f, 1.0f);
  const mathfu::quat sensor_rot = mathfu::kQuatIdentityf;
  camera.SetSensorPose(sensor_pos, sensor_rot);
  // Set the Eye offset:
  mathfu::mat4 sensor_from_camera = CalculateTransformMatrix(
      mathfu::vec3(1.0, 0.0f, 0.0f), mathfu::kQuatIdentityf, mathfu::kOnes3f);
  camera.SetCameraFromSensor(sensor_from_camera.Inverse());

  mathfu::mat4 world_from_camera =
      world_from_sensor_start *
      CalculateTransformMatrix(sensor_pos, sensor_rot, mathfu::kOnes3f) *
      sensor_from_camera;
  mathfu::mat4 clip_from_world = clip_from_camera * world_from_camera.Inverse();

  RenderView view;
  camera.PopulateRenderView(&view);
  EXPECT_EQ(view.viewport, kViewport.pos);
  EXPECT_EQ(view.dimensions, kViewport.size);
  EXPECT_EQ(view.world_from_eye_matrix, world_from_camera);
  EXPECT_EQ(view.eye_from_world_matrix, world_from_camera.Inverse());
  EXPECT_EQ(view.clip_from_eye_matrix, clip_from_camera);
  EXPECT_EQ(view.clip_from_world_matrix, clip_from_world);
}

TEST(MutableCameraTest, WorldRayFromClipPoint) {
  MutableCamera camera(nullptr);
  // Setup a perspective projection:
  camera.SetupDisplay(kNearClip, kFarClip, kFov, mathfu::recti(0, 0, 100, 100));

  const mathfu::vec3 sensor_pos(1.0f, 2.0f, 3.0f);
  const mathfu::quat sensor_rot =
      mathfu::quat::FromEulerAngles(mathfu::kAxisY3f * mathfu::kPi / 2.0f);
  camera.SetSensorPose(sensor_pos, sensor_rot);

  // Straight forward from center of camera
  Ray ray = camera.WorldRayFromClipPoint(mathfu::vec3(0.0f, 0.0f, 0.0f));
  EXPECT_EQ(ray.origin, sensor_pos);
  EXPECT_THAT(ray.direction,
              NearMathfuVec3(mathfu::vec3(-1.0f, 0.0f, 0.0f), kEpsilon));

  // Left edge of camera
  ray = camera.WorldRayFromClipPoint(mathfu::vec3(-1.0f, 0.0f, 0.0f));
  EXPECT_EQ(ray.origin, sensor_pos);
  EXPECT_THAT(
      ray.direction,
      NearMathfuVec3(mathfu::vec3(-1.0f, 0.0f, 1.0f).Normalized(), kEpsilon));
}

TEST(MutableCameraTest, WorldRayFromPixel) {
  MutableCamera camera(nullptr);
  // Setup a perspective projection:
  camera.SetupDisplay(kNearClip, kFarClip, kFov, mathfu::recti(0, 0, 100, 100));

  const mathfu::vec3 sensor_pos(1.0f, 2.0f, 3.0f);
  const mathfu::quat sensor_rot =
      mathfu::quat::FromEulerAngles(mathfu::kAxisY3f * mathfu::kPi / 2.0f);
  camera.SetSensorPose(sensor_pos, sensor_rot);

  // Straight forward from center of camera
  Optional<Ray> ray = camera.WorldRayFromPixel(mathfu::vec2(50.0f, 50.0f));
  EXPECT_TRUE(ray);
  EXPECT_EQ(ray.value().origin, sensor_pos);
  EXPECT_THAT(ray.value().direction,
              NearMathfuVec3(mathfu::vec3(-1.0f, 0.0f, 0.0f), kEpsilon));

  // Left edge of camera
  ray = camera.WorldRayFromPixel(mathfu::vec2(00.0f, 50.0f));
  EXPECT_TRUE(ray);
  EXPECT_EQ(ray.value().origin, sensor_pos);
  EXPECT_THAT(
      ray.value().direction,
      NearMathfuVec3(mathfu::vec3(-1.0f, 0.0f, 1.0f).Normalized(), kEpsilon));
}

TEST(MutableCameraTest, PixelFromWorldPoint) {
  MutableCamera camera(nullptr);

  // Calling PixelFromWorldPoint without setting a viewport should return an
  // unset optional.
  Optional<mathfu::vec2> pixel =
      camera.PixelFromWorldPoint(mathfu::vec3(0.0f, 0.0f, -1.0f));
  EXPECT_FALSE(pixel);

  // Setup a perspective projection:
  camera.SetupDisplay(kNearClip, kFarClip, kFov, kViewport);

  // Center
  pixel =
      camera.PixelFromWorldPoint(mathfu::vec3(0.0f, 0.0f, -1.0f));
  EXPECT_THAT(pixel.value(), NearMathfuVec2(mathfu::vec2(50.0f, 50.0f), kEpsilon));

  // Top Left
  pixel = camera.PixelFromWorldPoint(mathfu::vec3(-1.0f, 1.0f, -1.0f));
  EXPECT_THAT(pixel.value(), NearMathfuVec2(mathfu::vec2(0.0f, 0.0f), kEpsilon));

  // Bottom Right
  pixel = camera.PixelFromWorldPoint(mathfu::vec3(1.0f, -1.0f, -1.0f));
  EXPECT_THAT(pixel.value(), NearMathfuVec2(mathfu::vec2(100.0f, 100.0f), kEpsilon));

  // Pixel out of camera frustrum. - expect invalid values
  pixel = camera.PixelFromWorldPoint(mathfu::vec3(1.0f, 0.0f, 0.0f));
  EXPECT_TRUE(isnan(pixel.value().x) || isnan(pixel.value().y));

  const mathfu::vec3 sensor_pos(1.0f, 2.0f, 3.0f);
  const mathfu::quat sensor_rot =
      mathfu::quat::FromEulerAngles(mathfu::kAxisY3f * mathfu::kPi / 2.0f);
  camera.SetSensorPose(sensor_pos, sensor_rot);
}

TEST(MutableCameraTest, WorldPointFromClip) {
  MutableCamera camera(nullptr);
  // Setup a perspective projection:
  camera.SetupDisplay(kNearClip, kFarClip, kFov, mathfu::recti(0, 0, 100, 100));

  // const mathfu::vec3 sensor_pos(1.0f, 2.0f, 3.0f);
  const mathfu::vec3 sensor_pos(0.0f, 0.0f, 0.0f);
  const mathfu::quat sensor_rot =
      mathfu::quat::FromEulerAngles(mathfu::kZeros3f);
  // mathfu::quat::FromEulerAngles(mathfu::kAxisY3f * mathfu::kPi / 2.0f);
  camera.SetSensorPose(sensor_pos, sensor_rot);

  const mathfu::vec3 cam_forward = mathfu::kAxisZ3f * -1.0f;
  const mathfu::vec3 cam_right = mathfu::kAxisX3f;

  // in front of center of camera, near and far clip planes
  EXPECT_THAT(
      sensor_pos + cam_forward * kNearClip,
      NearMathfuVec3(camera.WorldPointFromClip(mathfu::vec3(0.0f, 0.0f, -1.0f)),
                     kEpsilon));
  // NOTE: calculations at the far plane have low accuracy, so using a lower
  // epsilon here.
  EXPECT_THAT(
      sensor_pos + cam_forward * kFarClip,
      NearMathfuVec3(camera.WorldPointFromClip(mathfu::vec3(0.0f, 0.0f, 1.0f)),
                     0.01));
  // Right edge of camera
  EXPECT_THAT(
      sensor_pos + cam_forward * kNearClip + cam_right * kNearClip,
      NearMathfuVec3(camera.WorldPointFromClip(mathfu::vec3(1.0f, 0.0f, -1.0f)),
                     kEpsilon));
}

TEST(MutableCameraTest, ClipFromWorldPoint) {
  MutableCamera camera(nullptr);
  // Setup a perspective projection:
  camera.SetupDisplay(kNearClip, kFarClip, kFov, mathfu::recti(0, 0, 100, 100));

  // const mathfu::vec3 sensor_pos(1.0f, 2.0f, 3.0f);
  const mathfu::vec3 sensor_pos(0.0f, 0.0f, 0.0f);
  const mathfu::quat sensor_rot =
      mathfu::quat::FromEulerAngles(mathfu::kZeros3f);
  // mathfu::quat::FromEulerAngles(mathfu::kAxisY3f * mathfu::kPi / 2.0f);
  camera.SetSensorPose(sensor_pos, sensor_rot);

  const mathfu::vec3 cam_forward = mathfu::kAxisZ3f * -1.0f;
  const mathfu::vec3 cam_right = mathfu::kAxisX3f;

  // in front of center of camera, near and far clip planes
  EXPECT_THAT(camera.ClipFromWorldPoint(sensor_pos + cam_forward * kNearClip),
              NearMathfuVec3(mathfu::vec3(0.0f, 0.0f, -1.0f), kEpsilon));
  EXPECT_THAT(camera.ClipFromWorldPoint(sensor_pos + cam_forward * kFarClip),
              NearMathfuVec3(mathfu::vec3(0.0f, 0.0f, 1.0f), kEpsilon));
  // Right edge of camera
  EXPECT_THAT(camera.ClipFromWorldPoint(sensor_pos + cam_forward * kNearClip +
                                        cam_right * kNearClip),
              NearMathfuVec3(mathfu::vec3(1.0f, 0.0f, -1.0f), kEpsilon));
}

TEST(MutableCameraTest, ClipFromPixel) {
  MutableCamera camera(nullptr);

  // Calling ClipFromPixel without setting a viewport should return an
  // unset optional.
  Optional<mathfu::vec3> clip =
      camera.ClipFromPixel(mathfu::vec2(50.0f, 50.0f));
  EXPECT_FALSE(clip);

  // Setup a perspective projection:
  camera.SetupDisplay(kNearClip, kFarClip, kFov, kViewport);

  EXPECT_EQ(camera.ClipFromPixel(mathfu::vec2(50.0f, 50.0f)).value(),
            mathfu::vec3(0.0f, 0.0f, 0.0f));
  EXPECT_EQ(camera.ClipFromPixel(mathfu::vec2(0.0f, 100.0f)).value(),
            mathfu::vec3(-1.0f, -1.0f, 0.0f));
  EXPECT_EQ(camera.ClipFromPixel(mathfu::vec2(100.0f, 0.0f)).value(),
            mathfu::vec3(1.0f, 1.0f, 0.0f));
}

TEST(MutableCameraTest, PixelFromClip) {
  MutableCamera camera(nullptr);

  // Calling PixelFromWorldPoint without setting a viewport should return an
  // unset optional.
  Optional<mathfu::vec2> pixel =
      camera.PixelFromClip(mathfu::vec3(0.0f, 0.0f, -1.0f));
  EXPECT_FALSE(pixel);

  // Setup a perspective projection:
  camera.SetupDisplay(kNearClip, kFarClip, kFov, kViewport);

  EXPECT_EQ(camera.PixelFromClip(mathfu::vec3(0.0f, 0.0f, 0.0f)).value(),
            mathfu::vec2(50.0f, 50.0f));
  EXPECT_EQ(camera.PixelFromClip(mathfu::vec3(-1.0f, -1.0f, 0.0f)).value(),
            mathfu::vec2(0.0f, 100.0f));
  EXPECT_EQ(camera.PixelFromClip(mathfu::vec3(1.0f, 1.0f, 0.0f)).value(),
            mathfu::vec2(100.0f, 0.0f));
}

TEST(MutableCameraTest, DisplayRotation) {
  MutableCamera camera(nullptr);

  EXPECT_EQ(Camera::kRotation0, camera.DisplayRotation());
  EXPECT_EQ(DeviceOrientation::kPortrait, camera.Orientation());

  camera.SetDisplayRotation(Camera::kRotation90);
  EXPECT_EQ(Camera::kRotation90, camera.DisplayRotation());
  EXPECT_EQ(DeviceOrientation::kLandscape, camera.Orientation());

  camera.SetDisplayRotation(Camera::kRotation180);
  EXPECT_EQ(Camera::kRotation180, camera.DisplayRotation());
  EXPECT_EQ(DeviceOrientation::kUnknown, camera.Orientation());

  camera.SetDisplayRotation(Camera::kRotation270);
  EXPECT_EQ(Camera::kRotation270, camera.DisplayRotation());
  EXPECT_EQ(DeviceOrientation::kReverseLandscape, camera.Orientation());
}

TEST(MutableCameraTest, ToDisplayRotation) {
  DeviceOrientation portrait = DeviceOrientation::kPortrait;
  EXPECT_EQ(Camera::kRotation0, Camera::ToDisplayRotation(portrait));

  DeviceOrientation landscape = DeviceOrientation::kLandscape;
  EXPECT_EQ(Camera::kRotation90, Camera::ToDisplayRotation(landscape));

  DeviceOrientation reverse_landscape = DeviceOrientation::kReverseLandscape;
  EXPECT_EQ(Camera::kRotation270, Camera::ToDisplayRotation(reverse_landscape));

  DeviceOrientation unknown = DeviceOrientation::kUnknown;
  EXPECT_EQ(Camera::kRotation0, Camera::ToDisplayRotation(unknown));
}

}  // namespace
}  // namespace lull
