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

#include "lullaby/util/color.h"

#include "gtest/gtest.h"

namespace lull {

TEST(ColorTest, DefaultCtor) {
  Color4f color;
  EXPECT_EQ(color.r, 1.0f);
  EXPECT_EQ(color.g, 1.0f);
  EXPECT_EQ(color.b, 1.0f);
  EXPECT_EQ(color.a, 1.0f);
}

TEST(ColorTest, ScalarCtor) {
  Color4f color1(1.0f);
  EXPECT_EQ(color1.r, 1.0f);
  EXPECT_EQ(color1.g, 1.0f);
  EXPECT_EQ(color1.b, 1.0f);
  EXPECT_EQ(color1.a, 1.0f);

  Color4f color2(0.5f);
  EXPECT_EQ(color2.r, 0.5f);
  EXPECT_EQ(color2.g, 0.5f);
  EXPECT_EQ(color2.b, 0.5f);
  EXPECT_EQ(color2.a, 0.5f);
}

TEST(ColorTest, RGBACtor) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  EXPECT_EQ(color.r, 1.0f);
  EXPECT_EQ(color.g, 2.0f);
  EXPECT_EQ(color.b, 3.0f);
  EXPECT_EQ(color.a, 0.5f);
}

TEST(ColorTest, CopyCtor) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f copy(color);
  EXPECT_EQ(copy.r, 1.0f);
  EXPECT_EQ(copy.g, 2.0f);
  EXPECT_EQ(copy.b, 3.0f);
  EXPECT_EQ(copy.a, 0.5f);

  Color4f copy_2 = color;
  EXPECT_EQ(copy_2.r, 1.0f);
  EXPECT_EQ(copy_2.g, 2.0f);
  EXPECT_EQ(copy_2.b, 3.0f);
  EXPECT_EQ(copy_2.a, 0.5f);
}

TEST(ColorTest, OperatorEquals) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color2(25.0f, 25.0f, 41.0f, 78.0f);

  EXPECT_TRUE(color0 == color1);
  EXPECT_FALSE(color0 == color2);
}

TEST(ColorTest, OperatorNotEqual) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color2(25.0f, 25.0f, 41.0f, 78.0f);

  EXPECT_FALSE(color0 != color1);
  EXPECT_TRUE(color0 != color2);
}

TEST(ColorTest, FromVec4) {
  mathfu::vec4 vec(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color = Color4f::FromVec4(vec);

  EXPECT_EQ(color, Color4f(1.0f, 2.0f, 3.0f, 0.5f));
}

TEST(ColorTest, ToVec4) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  mathfu::vec4 vec = Color4f::ToVec4(color);

  EXPECT_EQ(vec, mathfu::vec4(1.0f, 2.0f, 3.0f, 0.5f));
}

TEST(ColorTest, OperatorUnaryMinus) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1 = -color0;

  EXPECT_EQ(color1, Color4f(-1.0f, -2.0f, -3.0f, -0.5f));
}

TEST(ColorTest, OperatorMultiplyColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 3.0f, 0.5f, 4.0f);

  EXPECT_EQ(color0 * color1, Color4f(2.0f, 6.0f, 1.5f, 2.0f));
}

TEST(ColorTest, OperatorDivideColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 1.0f, 0.5f, 4.0f);

  EXPECT_EQ(color0 / color1, Color4f(0.5f, 2.0f, 6.0f, 0.125f));
}

TEST(ColorTest, OperatorAddColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 3.0f, 0.5f, 4.0f);

  EXPECT_EQ(color0 + color1, Color4f(3.0f, 5.0f, 3.5f, 4.5f));
}

TEST(ColorTest, OperatorSubtractColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 1.0f, 0.5f, 4.0f);

  EXPECT_EQ(color0 - color1, Color4f(-1.0f, 1.0f, 2.5f, -3.5f));
}

TEST(ColorTest, OperatorMultiplyScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);

  EXPECT_EQ(color * 2.0f, Color4f(2.0f, 4.0f, 6.0f, 1.0f));
  EXPECT_EQ(2.0f * color, Color4f(2.0f, 4.0f, 6.0f, 1.0f));
}

TEST(ColorTest, OperatorDivideScalar) {
  Color4f color(1.0f, 2.0f, 4.0f, 0.5f);

  EXPECT_EQ(color / 2.0f, Color4f(0.5f, 1.0f, 2.0f, 0.25f));
  EXPECT_EQ(2.0f / color, Color4f(2.0f, 1.0f, 0.5f, 4.0f));
}

TEST(ColorTest, OperatorAddScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);

  EXPECT_EQ(color + 5.0f, Color4f(6.0f, 7.0f, 8.0f, 5.5f));
  EXPECT_EQ(5.0f + color, Color4f(6.0f, 7.0f, 8.0f, 5.5f));
}

TEST(ColorTest, OperatorSubtractScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);

  EXPECT_EQ(color - 1.0f, Color4f(0.0f, 1.0f, 2.0f, -0.5f));
  EXPECT_EQ(1.0f - color, Color4f(0.0f, -1.0f, -2.0f, 0.5f));
}

TEST(ColorTest, OperatorAssignmentMultiplyColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 3.0f, 0.5f, 4.0f);
  color0 *= color1;

  EXPECT_EQ(color0, Color4f(2.0f, 6.0f, 1.5f, 2.0f));
}

TEST(ColorTest, OperatorAssignmentDivideColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 1.0f, 0.5f, 4.0f);
  color0 /= color1;

  EXPECT_EQ(color0, Color4f(0.5f, 2.0f, 6.0f, 0.125f));
}

TEST(ColorTest, OperatorAssignmentAddColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 3.0f, 0.5f, 4.0f);
  color0 += color1;

  EXPECT_EQ(color0, Color4f(3.0f, 5.0f, 3.5f, 4.5f));
}

TEST(ColorTest, OperatorAssignmentSubtractColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 1.0f, 0.5f, 4.0f);
  color0 -= color1;

  EXPECT_EQ(color0, Color4f(-1.0f, 1.0f, 2.5f, -3.5f));
}

TEST(ColorTest, OperatorAssignmentMultiplyScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  color *= 2.0f;

  EXPECT_EQ(color, Color4f(2.0f, 4.0f, 6.0f, 1.0f));
}

TEST(ColorTest, OperatorAssignmentDivideScalar) {
  Color4f color(1.0f, 2.0f, 4.0f, 0.5f);
  color /= 2.0f;

  EXPECT_EQ(color, Color4f(0.5f, 1.0f, 2.0f, 0.25f));
}

TEST(ColorTest, OperatorAssignmentAddScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  color += 5.0f;

  EXPECT_EQ(color, Color4f(6.0f, 7.0f, 8.0f, 5.5f));
}

TEST(ColorTest, OperatorAssignmentSubtractScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  color -= 1.0f;

  EXPECT_EQ(color, Color4f(0.0f, 1.0f, 2.0f, -0.5f));
}

TEST(ColorTest, Min) {
  Color4f color0(1.0f, 5.0f, 7.0f, 20.5f);
  Color4f color1(2.0f, 3.0f, 10.5f, 15.0f);

  EXPECT_EQ(Color4f::Min(color0, color1), Color4f(1.0f, 3.0f, 7.0f, 15.0f));
}

TEST(ColorTest, Max) {
  Color4f color0(1.0f, 5.0f, 7.0f, 20.5f);
  Color4f color1(2.0f, 3.0f, 10.5f, 15.0f);

  EXPECT_EQ(Color4f::Max(color0, color1), Color4f(2.0f, 5.0f, 10.5f, 20.5f));
}

TEST(ColorTest, Lerp) {
  Color4f color0(0.0f, 0.0f, 0.0f, 1.0f);
  Color4f color1(1.0f);

  EXPECT_EQ(Color4f::Lerp(color0, color1, 0.0f),
            Color4f(0.0f, 0.0f, 0.0f, 1.0f));
  EXPECT_EQ(Color4f::Lerp(color0, color1, 0.5f),
            Color4f(0.5f, 0.5f, 0.5f, 1.0f));
  EXPECT_EQ(Color4f::Lerp(color0, color1, 1.0f),
            Color4f(1.0f, 1.0f, 1.0f, 1.0f));
}
}  // namespace lull
