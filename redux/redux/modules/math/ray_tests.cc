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
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/ray.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(RayTest, GetPointAt) {
  const Ray ray(vec3::Zero(), vec3::XAxis());
  EXPECT_THAT(ray.GetPointAt(0.0f), Eq(vec3::Zero()));
  EXPECT_THAT(ray.GetPointAt(0.5f), Eq(vec3(0.5, 0, 0)));
  EXPECT_THAT(ray.GetPointAt(1.0f), Eq(vec3::XAxis()));
}

TEST(RayTest, TransformRay) {
  const mat3 transform = RotationMatrixFromQuaternion(
      QuaternionFromAxisAngle(vec3::ZAxis(), kHalfPi));
  const Ray ray1 = Ray(vec3::Zero(), vec3::XAxis());
  const Ray ray2 = TransformRay(mat4(transform), ray1);
  EXPECT_TRUE(AreNearlyEqual(ray2.direction, vec3::YAxis()));
}

TEST(RayTest, ProjectPointOntoRay) {
  const vec3 point(0.5f, 1.f, 1.f);
  const Ray ray(vec3::Zero(), vec3::XAxis());
  const vec3 proj = ProjectPointOntoRay(point, ray);
  EXPECT_THAT(proj, Eq(vec3(0.5f, 0.f, 0.f)));
}

}  // namespace
}  // namespace redux
