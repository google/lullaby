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
#include "redux/modules/math/transform.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(TransformTest, Default) {
  const Transform transform;
  EXPECT_THAT(transform.translation.x, Eq(vec3::Zero().x));
  EXPECT_THAT(transform.translation.y, Eq(vec3::Zero().y));
  EXPECT_THAT(transform.translation.z, Eq(vec3::Zero().z));
  EXPECT_THAT(transform.rotation.x, Eq(quat::Identity().x));
  EXPECT_THAT(transform.rotation.y, Eq(quat::Identity().y));
  EXPECT_THAT(transform.rotation.z, Eq(quat::Identity().z));
  EXPECT_THAT(transform.rotation.w, Eq(quat::Identity().w));
  EXPECT_THAT(transform.scale.x, Eq(vec3::One().x));
  EXPECT_THAT(transform.scale.y, Eq(vec3::One().y));
  EXPECT_THAT(transform.scale.z, Eq(vec3::One().z));
}

mat4 TranslationMatrix(const vec3& translation) {
  mat4 m = mat4::Identity();
  m(0, 3) = translation.x;
  m(1, 3) = translation.y;
  m(2, 3) = translation.z;
  return m;
}

mat4 ScaleMatrix(const vec3& scale) {
  mat4 m = mat4::Identity();
  m(0, 0) = scale.x;
  m(1, 1) = scale.y;
  m(2, 2) = scale.z;
  return m;
}

TEST(TransformTest, FromMat4) {
  const vec3 translation(1.8, 2.1, -3.5);
  const quat rotation = QuaternionFromEulerAngles(vec3(0.5, 1.2, 0.8));
  const vec3 scale(1, 2, 3);

  mat4 m = mat4::Identity();
  m *= TranslationMatrix(translation);
  m *= mat4(RotationMatrixFromQuaternion(rotation));
  m *= ScaleMatrix(scale);

  const Transform transform(m);
  EXPECT_TRUE(AreNearlyEqual(transform.translation, translation));
  EXPECT_TRUE(AreNearlyEqual(transform.rotation, rotation));
  EXPECT_TRUE(AreNearlyEqual(transform.scale, scale));
}

TEST(TransformTest, FromMat34) {
  const vec3 translation(1.8, 2.1, -3.5);
  const quat rotation = QuaternionFromEulerAngles(vec3(0.5, 1.2, 0.8));
  const vec3 scale(1, 2, 3);

  mat4 m = mat4::Identity();
  m *= TranslationMatrix(translation);
  m *= mat4(RotationMatrixFromQuaternion(rotation));
  m *= ScaleMatrix(scale);
  mat34 m1(m);

  const Transform transform(m1);
  EXPECT_TRUE(AreNearlyEqual(transform.translation, translation));
  EXPECT_TRUE(AreNearlyEqual(transform.rotation, rotation));
  EXPECT_TRUE(AreNearlyEqual(transform.scale, scale));
}

TEST(TransformTest, TransformMatrix) {
  Transform transform;
  transform.translation = vec3(1.8, 2.1, -3.5);
  transform.rotation = QuaternionFromEulerAngles(vec3(0.5, 1.2, 0.8));
  transform.scale = vec3(1, 2, 3);
  mat4 actual = TransformMatrix(transform);

  mat4 expect = mat4::Identity();
  expect *= TranslationMatrix(transform.translation);
  expect *= mat4(RotationMatrixFromQuaternion(transform.rotation));
  expect *= ScaleMatrix(transform.scale);

  EXPECT_TRUE(AreNearlyEqual(actual, expect));
}

}  // namespace
}  // namespace redux
