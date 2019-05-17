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

#include "lullaby/modules/camera/camera_manager.h"

#include "lullaby/modules/camera/mutable_camera.h"

#include "gtest/gtest.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using testing::NearMathfuVec3;

constexpr float kEpsilon = 1e-5f;
constexpr float kNear = 0.01f;
constexpr float kFar = 200.0f;

TEST(CameraManagerTest, CameraRegistration) {
  CameraManager::RenderTargetId target =
      CameraManager::kDefaultScreenRenderTarget;
  CameraManager manager;
  auto camera = std::make_shared<MutableCamera>(nullptr);
  auto camera2 = std::make_shared<MutableCamera>(nullptr);

  manager.RegisterScreenCamera(camera);
  EXPECT_EQ(manager.GetNumCamerasForTarget(target), 1);
  EXPECT_EQ((*manager.GetCameras(target))[0].get(), camera.get());
  manager.RegisterScreenCamera(camera2);
  EXPECT_EQ(manager.GetNumCamerasForTarget(target), 2);
  EXPECT_EQ((*manager.GetCameras(target))[0].get(), camera.get());
  EXPECT_EQ((*manager.GetCameras(target))[1].get(), camera2.get());
  manager.UnregisterScreenCamera(camera);
  EXPECT_EQ(manager.GetNumCamerasForTarget(target), 1);
  EXPECT_EQ((*manager.GetCameras(target))[0].get(), camera2.get());
  manager.UnregisterScreenCamera(camera2);
  EXPECT_EQ(manager.GetNumCamerasForTarget(target), 0);

  // Unregistering multiple times shouldn't error.
  manager.UnregisterScreenCamera(camera);
  manager.UnregisterScreenCamera(camera2);
}

TEST(CameraManagerTest, DuplicateCameraRegistration) {
  CameraManager::RenderTargetId target =
      CameraManager::kDefaultScreenRenderTarget;
  CameraManager manager;
  auto camera = std::make_shared<MutableCamera>(nullptr);
  auto camera2 = std::make_shared<MutableCamera>(nullptr);

  // Registering multiple times should crash in debug, have no effect in
  // release.
  manager.RegisterScreenCamera(camera);
  PORT_EXPECT_DEBUG_DEATH(manager.RegisterScreenCamera(camera), "");
  EXPECT_EQ(manager.GetNumCamerasForTarget(target), 1);
  manager.RegisterScreenCamera(camera2);
  PORT_EXPECT_DEBUG_DEATH(manager.RegisterScreenCamera(camera), "");
  PORT_EXPECT_DEBUG_DEATH(manager.RegisterScreenCamera(camera2), "");
  EXPECT_EQ(manager.GetNumCamerasForTarget(target), 2);
}

TEST(CameraManagerTest, ScreenRenderTarget) {
  CameraManager::RenderTargetId target1 =
      CameraManager::kDefaultScreenRenderTarget;
  CameraManager::RenderTargetId target2 = ConstHash("target2");

  CameraManager manager;
  EXPECT_EQ(manager.GetNumCamerasForScreen(), 0);
  auto camera = std::make_shared<MutableCamera>(nullptr);
  manager.RegisterScreenCamera(camera);
  EXPECT_EQ(manager.GetNumCamerasForScreen(), 1);

  manager.SetScreenRenderTarget(target2);
  EXPECT_EQ(manager.GetNumCamerasForScreen(), 0);

  auto camera2 = std::make_shared<MutableCamera>(nullptr);
  manager.RegisterScreenCamera(camera2);
  EXPECT_EQ(manager.GetNumCamerasForScreen(), 1);
  EXPECT_EQ((*manager.GetCameras(target2))[0].get(), camera2.get());

  manager.RegisterCamera(camera2, target1);
  EXPECT_EQ(manager.GetNumCamerasForScreen(), 1);
  EXPECT_EQ(manager.GetNumCamerasForTarget(target1), 2);

  manager.UnregisterCamera(camera2, target2);
  EXPECT_EQ(manager.GetNumCamerasForScreen(), 0);

  manager.SetScreenRenderTarget(target1);
  EXPECT_EQ(manager.GetNumCamerasForScreen(), 2);
  EXPECT_EQ((*manager.GetCameras(target1))[0].get(), camera.get());
  EXPECT_EQ((*manager.GetCameras(target1))[1].get(), camera2.get());
}

TEST(CameraManagerTest, GetCameraByPixel) {
  const float kFov = 90.0f;
  const mathfu::recti viewport1(0, 0, 100, 200);
  const mathfu::recti viewport2(100, 0, 100, 200);
  CameraManager::RenderTargetId target2 = ConstHash("target2");

  CameraManager manager;
  auto camera1 = std::make_shared<MutableCamera>(nullptr);
  auto camera2 = std::make_shared<MutableCamera>(nullptr);
  manager.RegisterScreenCamera(camera1);
  manager.RegisterScreenCamera(camera2);
  camera1->SetupDisplay(kNear, kFar, kFov, viewport1);
  camera2->SetupDisplay(kNear, kFar, kFov, viewport2);

  EXPECT_EQ(manager.GetCameraByScreenPixel(mathfu::vec2(50.0f, 200.0f)),
            camera1.get());
  EXPECT_EQ(manager.GetCameraByScreenPixel(mathfu::vec2(150.0f, 0.0f)),
            camera2.get());
  EXPECT_EQ(manager.GetCameraByScreenPixel(mathfu::vec2(150.0f, 200.1f)),
            nullptr);

  manager.RegisterCamera(camera1, target2);
  EXPECT_EQ(
      manager.GetCameraByTargetPixel(target2, mathfu::vec2(50.0f, 100.0f)),
      camera1.get());
  EXPECT_EQ(
      manager.GetCameraByTargetPixel(target2, mathfu::vec2(150.0f, 100.0f)),
      nullptr);
}

TEST(CameraManagerTest, WorldRayFromPixel) {
  const float kFov = 90.0f;
  const mathfu::recti viewport1(0, 0, 100, 200);
  const mathfu::recti viewport2(100, 0, 100, 200);
  CameraManager::RenderTargetId target2 = ConstHash("target2");

  CameraManager manager;
  auto camera1 = std::make_shared<MutableCamera>(nullptr);
  auto camera2 = std::make_shared<MutableCamera>(nullptr);
  manager.RegisterScreenCamera(camera1);
  manager.RegisterScreenCamera(camera2);
  camera1->SetupDisplay(kNear, kFar, kFov, viewport1);
  camera2->SetupDisplay(kNear, kFar, kFov, viewport2);
  camera2->SetSensorPose(
      mathfu::vec3(1.0f, 2.0f, 3.0f),
      mathfu::quat::FromEulerAngles(0.0f, M_PI / 2.0f, 0.0f));

  Ray ray =
      manager.WorldRayFromScreenPixel(mathfu::vec2(50.0f, 100.0f)).value();
  EXPECT_EQ(ray.origin, mathfu::kZeros3f);
  EXPECT_THAT(ray.direction,
              NearMathfuVec3(-1.0f * mathfu::kAxisZ3f, kEpsilon));
  ray = manager.WorldRayFromScreenPixel(mathfu::vec2(150.0f, 100.0f)).value();
  EXPECT_EQ(ray.origin, mathfu::vec3(1.0f, 2.0f, 3.0f));
  EXPECT_THAT(ray.direction,
              NearMathfuVec3(-1.0f * mathfu::kAxisX3f, kEpsilon));
  EXPECT_FALSE(manager.WorldRayFromScreenPixel(mathfu::vec2(150.0f, 200.1f)));

  manager.RegisterCamera(camera1, target2);
  ray = manager.WorldRayFromTargetPixel(target2, mathfu::vec2(50.0f, 100.0f))
            .value();
  EXPECT_EQ(ray.origin, mathfu::kZeros3f);
  EXPECT_THAT(ray.direction,
              NearMathfuVec3(-1.0f * mathfu::kAxisZ3f, kEpsilon));
  EXPECT_FALSE(
      manager.WorldRayFromTargetPixel(target2, mathfu::vec2(150.0f, 100.0f)));
}

}  // namespace
}  // namespace lull
