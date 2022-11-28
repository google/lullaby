/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/graphics/camera_ops.h"
#include "redux/modules/math/math.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(CameraOpsTest, WorldRayFromPixel) {
  const float fov = DegreesToRadians(90.f);
  const Bounds2i viewport({0, 0}, {100, 200});
  const mat4 matrix = PerspectiveMatrix(fov, 0.5f, 1.f, 100.f);
  CameraOps ops(vec3::Zero(), quat::Identity(), matrix, viewport);

  Ray ray = ops.WorldRayFromPixel(vec2(50.0f, 100.0f)).value();
  EXPECT_THAT(ray.origin, Eq(vec3::Zero()));
  EXPECT_TRUE(AreNearlyEqual(ray.direction, -1.0f * vec3::ZAxis()));
}

}  // namespace
}  // namespace redux
