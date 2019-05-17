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

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

TEST(MathfuFbConversions, MathfuVec2FromFbVec2) {
  std::unique_ptr<Vec2> in(new Vec2(1, 2));
  std::unique_ptr<mathfu::vec2> out(new mathfu::vec2());
  MathfuVec2FromFbVec2(in.get(), out.get());
  EXPECT_EQ(1, out->x);
  EXPECT_EQ(2, out->y);
}

TEST(MathfuFbConversions, MathfuVec2iFromFbVec2i) {
  std::unique_ptr<Vec2i> in(new Vec2i(1, 2));
  std::unique_ptr<mathfu::vec2i> out(new mathfu::vec2i());
  MathfuVec2iFromFbVec2i(in.get(), out.get());
  EXPECT_EQ(1, out->x);
  EXPECT_EQ(2, out->y);
}

TEST(MathfuFbConversions, MathfuVec3FromFbVec3) {
  std::unique_ptr<Vec3> in(new Vec3(1, 2, 3));
  std::unique_ptr<mathfu::vec3> out(new mathfu::vec3());
  MathfuVec3FromFbVec3(in.get(), out.get());
  EXPECT_EQ(1, out->x);
  EXPECT_EQ(2, out->y);
  EXPECT_EQ(3, out->z);
}

TEST(MathfuFbConversions, MathfuVec4FromFbVec4) {
  std::unique_ptr<Vec4> in(new Vec4(1, 2, 3, 4));
  std::unique_ptr<mathfu::vec4> out(new mathfu::vec4());
  MathfuVec4FromFbVec4(in.get(), out.get());
  EXPECT_EQ(1, out->x);
  EXPECT_EQ(2, out->y);
  EXPECT_EQ(3, out->z);
  EXPECT_EQ(4, out->w);
}

TEST(MathfuFbConversions, MathfuQuatFromFbQuat) {
  // In lull, scalar is first; in fb, it is last.
  const mathfu::quat value = mathfu::quat(1, 2, 3, 4).Normalized();
  std::unique_ptr<Quat> in(new Quat(value[1], value[2], value[3], value[0]));
  std::unique_ptr<mathfu::quat> out(new mathfu::quat());
  MathfuQuatFromFbQuat(in.get(), out.get());
  EXPECT_EQ(value.scalar(), out->scalar());
  EXPECT_EQ(value.vector().x, out->vector().x);
  EXPECT_EQ(value.vector().y, out->vector().y);
  EXPECT_EQ(value.vector().z, out->vector().z);
}

TEST(MathfuFbConversions, MathfuQuatFromFbVec3) {
  // In lull, scalar is first; in fb, it is last.
  std::unique_ptr<Vec3> in(new Vec3(1, 2, 3));
  std::unique_ptr<mathfu::quat> out(new mathfu::quat());
  MathfuQuatFromFbVec3(in.get(), out.get());
  EXPECT_NEAR(0.999471, out->scalar(), kDefaultEpsilon);
  EXPECT_NEAR(0.00826538, out->vector().x, kDefaultEpsilon);
  EXPECT_NEAR(0.0176742, out->vector().y, kDefaultEpsilon);
  EXPECT_NEAR(0.0260197, out->vector().z, kDefaultEpsilon);
}

TEST(MathfuFbConversions, MathfuQuatFromFbVec4) {
  // In lull, scalar is first; in fb, it is last.
  std::unique_ptr<Vec4> in(new Vec4(1, 2, 3, 4));
  std::unique_ptr<mathfu::quat> out(new mathfu::quat());
  MathfuQuatFromFbVec4(in.get(), out.get());
  EXPECT_EQ(4, out->scalar());
  EXPECT_EQ(1, out->vector().x);
  EXPECT_EQ(2, out->vector().y);
  EXPECT_EQ(3, out->vector().z);
}

TEST(MathfuFbConversions, MathfuVec4FromFbColor) {
  std::unique_ptr<Color> in(new Color(1, 2, 3, 4));
  std::unique_ptr<mathfu::vec4> out(new mathfu::vec4());
  MathfuVec4FromFbColor(in.get(), out.get());
  EXPECT_EQ(1, out->x);
  EXPECT_EQ(2, out->y);
  EXPECT_EQ(3, out->z);
  EXPECT_EQ(4, out->w);
}

TEST(MathfuFbConversions, MathfuVec4FromFbColorHex) {
  std::unique_ptr<mathfu::vec4> out(new mathfu::vec4());

  MathfuVec4FromFbColorHex("#336699", out.get());
  EXPECT_NEAR(0.2f, out->x, kDefaultEpsilon);
  EXPECT_NEAR(0.4f, out->y, kDefaultEpsilon);
  EXPECT_NEAR(0.6f, out->z, kDefaultEpsilon);
  EXPECT_NEAR(1.0f, out->w, kDefaultEpsilon);

  MathfuVec4FromFbColorHex("#33669900", out.get());
  EXPECT_NEAR(0.2f, out->x, kDefaultEpsilon);
  EXPECT_NEAR(0.4f, out->y, kDefaultEpsilon);
  EXPECT_NEAR(0.6f, out->z, kDefaultEpsilon);
  EXPECT_NEAR(0.0f, out->w, kDefaultEpsilon);
}

TEST(MathfuFbConversions, AabbFromFbAabb) {
  std::unique_ptr<AabbDef> in(new AabbDef(Vec3(1, 2, 3), Vec3(4, 5, 6)));
  std::unique_ptr<Aabb> out(new Aabb());
  AabbFromFbAabb(in.get(), out.get());
  EXPECT_EQ(1, out->min.x);
  EXPECT_EQ(2, out->min.y);
  EXPECT_EQ(3, out->min.z);
  EXPECT_EQ(4, out->max.x);
  EXPECT_EQ(5, out->max.y);
  EXPECT_EQ(6, out->max.z);
}

TEST(MathfuFbConversions, AabbFromFbRect) {
  Rect in(1, 2, 3, 4);
  Aabb out;
  AabbFromFbRect(&in, &out);
  EXPECT_EQ(1, out.min.x);
  EXPECT_EQ(2, out.min.y);
  EXPECT_EQ(0, out.min.z);
  EXPECT_EQ(4, out.max.x);
  EXPECT_EQ(6, out.max.y);
  EXPECT_EQ(0, out.max.z);
}

TEST(MathfuFbConversions, Color4ubFromFbColor) {
  std::unique_ptr<Color> in(
      new Color(0, 128.0f / 255.0f, 196.0f / 255.0f, 1.0f));
  std::unique_ptr<Color4ub> out(new Color4ub());
  Color4ubFromFbColor(in.get(), out.get());
  EXPECT_EQ(0, out->r);
  EXPECT_EQ(128, out->g);
  EXPECT_EQ(196, out->b);
  EXPECT_EQ(255, out->a);
}

}  // namespace
}  // namespace lull
