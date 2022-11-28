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
#include "redux/modules/flatbuffers/math.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatNear;

constexpr float kEps = 1.0e-6f;

TEST(FbsMath, Vec2i) {
  const fbs::Vec2i in(1, 2);
  const vec2i out = ReadFbs(in);
  EXPECT_THAT(out.x, Eq(1));
  EXPECT_THAT(out.y, Eq(2));
}

TEST(FbsMath, Vec2f) {
  const fbs::Vec2f in(1.1f, 2.2f);
  const vec2 out = ReadFbs(in);
  EXPECT_THAT(out.x, Eq(1.1f));
  EXPECT_THAT(out.y, Eq(2.2f));
}

TEST(FbsMath, Vec3i) {
  const fbs::Vec3i in(1, 2, 3);
  const vec3i out = ReadFbs(in);
  EXPECT_THAT(out.x, Eq(1));
  EXPECT_THAT(out.y, Eq(2));
  EXPECT_THAT(out.z, Eq(3));
}

TEST(FbsMath, Vec3f) {
  const fbs::Vec3f in(1.1f, 2.2f, 3.3f);
  const vec3 out = ReadFbs(in);
  EXPECT_THAT(out.x, Eq(1.1f));
  EXPECT_THAT(out.y, Eq(2.2f));
  EXPECT_THAT(out.z, Eq(3.3f));
}

TEST(FbsMath, Vec4i) {
  const fbs::Vec4i in(1, 2, 3, 4);
  const vec4i out = ReadFbs(in);
  EXPECT_THAT(out.x, Eq(1));
  EXPECT_THAT(out.y, Eq(2));
  EXPECT_THAT(out.z, Eq(3));
  EXPECT_THAT(out.w, Eq(4));
}

TEST(FbsMath, Vec4f) {
  const fbs::Vec4f in(1.1f, 2.2f, 3.3f, 4.4f);
  const vec4 out = ReadFbs(in);
  EXPECT_THAT(out.x, Eq(1.1f));
  EXPECT_THAT(out.y, Eq(2.2f));
  EXPECT_THAT(out.z, Eq(3.3f));
  EXPECT_THAT(out.w, Eq(4.4f));
}

TEST(FbsMath, Quatf) {
  const fbs::Quatf in(1.1f, 2.2f, 3.3f, 4.4f);
  const quat out = ReadFbs(in);

  // Quaternions are automatically normalized.
  const auto n = vec4(1.1f, 2.2f, 3.3f, 4.4f).Normalized();
  EXPECT_THAT(out.w, FloatNear(n.w, kEps));
  EXPECT_THAT(out.x, FloatNear(n.x, kEps));
  EXPECT_THAT(out.y, FloatNear(n.y, kEps));
  EXPECT_THAT(out.z, FloatNear(n.z, kEps));
}

TEST(FbsMath, Recti) {
  const fbs::Recti in(1, 2, 3, 4);
  const Bounds2i out = ReadFbs(in);
  EXPECT_THAT(out.min.x, Eq(1));
  EXPECT_THAT(out.min.y, Eq(2));
  EXPECT_THAT(out.max.x, Eq(4));
  EXPECT_THAT(out.max.y, Eq(6));
}

TEST(FbsMath, Rectf) {
  const fbs::Rectf in(1.1f, 2.2f, 3.3f, 4.4f);
  const Bounds2f out = ReadFbs(in);
  EXPECT_THAT(out.min.x, Eq(1.1f));
  EXPECT_THAT(out.min.y, Eq(2.2f));
  EXPECT_THAT(out.max.x, FloatNear(4.4f, kEps));
  EXPECT_THAT(out.max.y, FloatNear(6.6f, kEps));
}

TEST(FbsMath, Mat4x4f) {
  const fbs::Vec4f c0(1, 2, 3, 4);
  const fbs::Vec4f c1(5, 6, 7, 8);
  const fbs::Vec4f c2(9, 10, 11, 12);
  const fbs::Vec4f c3(13, 14, 15, 16);
  const fbs::Mat4x4f in(c0, c1, c2, c3);
  const mat4 out = ReadFbs(in);
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      EXPECT_THAT(out(i, j), Eq(i + (j * 4) + 1));
    }
  }
}

TEST(FbsMath, Boxf) {
  const fbs::Vec3f min(-1.1f, -2.2f, -3.3f);
  const fbs::Vec3f max(4.4f, 5.5f, 6.6f);
  const fbs::Boxf in(min, max);
  const Box out = ReadFbs(in);
  EXPECT_THAT(out.min.x, Eq(-1.1f));
  EXPECT_THAT(out.min.y, Eq(-2.2f));
  EXPECT_THAT(out.min.z, Eq(-3.3f));
  EXPECT_THAT(out.max.x, Eq(4.4f));
  EXPECT_THAT(out.max.y, Eq(5.5f));
  EXPECT_THAT(out.max.z, Eq(6.6f));
}

}  // namespace
}  // namespace redux
