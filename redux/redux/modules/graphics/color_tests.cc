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
#include "redux/modules/graphics/color.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(ColorTest, DefaultCtor) {
  Color4f color;
  EXPECT_THAT(color.r, Eq(1.0f));
  EXPECT_THAT(color.g, Eq(1.0f));
  EXPECT_THAT(color.b, Eq(1.0f));
  EXPECT_THAT(color.a, Eq(1.0f));
}

TEST(ColorTest, ScalarCtor) {
  Color4f color1(1.0f);
  EXPECT_THAT(color1.r, Eq(1.0f));
  EXPECT_THAT(color1.g, Eq(1.0f));
  EXPECT_THAT(color1.b, Eq(1.0f));
  EXPECT_THAT(color1.a, Eq(1.0f));

  Color4f color2(0.5f);
  EXPECT_THAT(color2.r, Eq(0.5f));
  EXPECT_THAT(color2.g, Eq(0.5f));
  EXPECT_THAT(color2.b, Eq(0.5f));
  EXPECT_THAT(color2.a, Eq(0.5f));
}

TEST(ColorTest, RGBACtor) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  EXPECT_THAT(color.r, Eq(1.0f));
  EXPECT_THAT(color.g, Eq(2.0f));
  EXPECT_THAT(color.b, Eq(3.0f));
  EXPECT_THAT(color.a, Eq(0.5f));
}

TEST(ColorTest, CopyCtor) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f copy(color);
  EXPECT_THAT(copy.r, Eq(1.0f));
  EXPECT_THAT(copy.g, Eq(2.0f));
  EXPECT_THAT(copy.b, Eq(3.0f));
  EXPECT_THAT(copy.a, Eq(0.5f));

  Color4f copy_2 = color;
  EXPECT_THAT(copy_2.r, Eq(1.0f));
  EXPECT_THAT(copy_2.g, Eq(2.0f));
  EXPECT_THAT(copy_2.b, Eq(3.0f));
  EXPECT_THAT(copy_2.a, Eq(0.5f));
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
  vec4 vec(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color = Color4f::FromVec4(vec);

  EXPECT_THAT(color, Eq(Color4f(1.0f, 2.0f, 3.0f, 0.5f)));
}

TEST(ColorTest, ToVec4) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  vec4 vec = Color4f::ToVec4(color);

  EXPECT_THAT(vec, Eq(vec4(1.0f, 2.0f, 3.0f, 0.5f)));
}

TEST(ColorTest, OperatorUnaryMinus) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1 = -color0;

  EXPECT_THAT(color1, Eq(Color4f(-1.0f, -2.0f, -3.0f, -0.5f)));
}

TEST(ColorTest, OperatorMultiplyColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 3.0f, 0.5f, 4.0f);

  EXPECT_THAT(color0 * color1, Eq(Color4f(2.0f, 6.0f, 1.5f, 2.0f)));
}

TEST(ColorTest, OperatorDivideColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 1.0f, 0.5f, 4.0f);

  EXPECT_THAT(color0 / color1, Eq(Color4f(0.5f, 2.0f, 6.0f, 0.125f)));
}

TEST(ColorTest, OperatorAddColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 3.0f, 0.5f, 4.0f);

  EXPECT_THAT(color0 + color1, Eq(Color4f(3.0f, 5.0f, 3.5f, 4.5f)));
}

TEST(ColorTest, OperatorSubtractColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 1.0f, 0.5f, 4.0f);

  EXPECT_THAT(color0 - color1, Eq(Color4f(-1.0f, 1.0f, 2.5f, -3.5f)));
}

TEST(ColorTest, OperatorMultiplyScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);

  EXPECT_THAT(color * 2.0f, Eq(Color4f(2.0f, 4.0f, 6.0f, 1.0f)));
  EXPECT_THAT(2.0f * color, Eq(Color4f(2.0f, 4.0f, 6.0f, 1.0f)));
}

TEST(ColorTest, OperatorDivideScalar) {
  Color4f color(1.0f, 2.0f, 4.0f, 0.5f);

  EXPECT_THAT(color / 2.0f, Eq(Color4f(0.5f, 1.0f, 2.0f, 0.25f)));
  EXPECT_THAT(2.0f / color, Eq(Color4f(2.0f, 1.0f, 0.5f, 4.0f)));
}

TEST(ColorTest, OperatorAddScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);

  EXPECT_THAT(color + 5.0f, Eq(Color4f(6.0f, 7.0f, 8.0f, 5.5f)));
  EXPECT_THAT(5.0f + color, Eq(Color4f(6.0f, 7.0f, 8.0f, 5.5f)));
}

TEST(ColorTest, OperatorSubtractScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);

  EXPECT_THAT(color - 1.0f, Eq(Color4f(0.0f, 1.0f, 2.0f, -0.5f)));
  EXPECT_THAT(1.0f - color, Eq(Color4f(0.0f, -1.0f, -2.0f, 0.5f)));
}

TEST(ColorTest, OperatorAssignmentMultiplyColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 3.0f, 0.5f, 4.0f);
  color0 *= color1;

  EXPECT_THAT(color0, Eq(Color4f(2.0f, 6.0f, 1.5f, 2.0f)));
}

TEST(ColorTest, OperatorAssignmentDivideColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 1.0f, 0.5f, 4.0f);
  color0 /= color1;

  EXPECT_THAT(color0, Eq(Color4f(0.5f, 2.0f, 6.0f, 0.125f)));
}

TEST(ColorTest, OperatorAssignmentAddColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 3.0f, 0.5f, 4.0f);
  color0 += color1;

  EXPECT_THAT(color0, Eq(Color4f(3.0f, 5.0f, 3.5f, 4.5f)));
}

TEST(ColorTest, OperatorAssignmentSubtractColor) {
  Color4f color0(1.0f, 2.0f, 3.0f, 0.5f);
  Color4f color1(2.0f, 1.0f, 0.5f, 4.0f);
  color0 -= color1;

  EXPECT_THAT(color0, Eq(Color4f(-1.0f, 1.0f, 2.5f, -3.5f)));
}

TEST(ColorTest, OperatorAssignmentMultiplyScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  color *= 2.0f;

  EXPECT_THAT(color, Eq(Color4f(2.0f, 4.0f, 6.0f, 1.0f)));
}

TEST(ColorTest, OperatorAssignmentDivideScalar) {
  Color4f color(1.0f, 2.0f, 4.0f, 0.5f);
  color /= 2.0f;

  EXPECT_THAT(color, Eq(Color4f(0.5f, 1.0f, 2.0f, 0.25f)));
}

TEST(ColorTest, OperatorAssignmentAddScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  color += 5.0f;

  EXPECT_THAT(color, Eq(Color4f(6.0f, 7.0f, 8.0f, 5.5f)));
}

TEST(ColorTest, OperatorAssignmentSubtractScalar) {
  Color4f color(1.0f, 2.0f, 3.0f, 0.5f);
  color -= 1.0f;

  EXPECT_THAT(color, Eq(Color4f(0.0f, 1.0f, 2.0f, -0.5f)));
}

TEST(ColorTest, Min) {
  Color4f color0(1.0f, 5.0f, 7.0f, 20.5f);
  Color4f color1(2.0f, 3.0f, 10.5f, 15.0f);

  EXPECT_THAT(Color4f::Min(color0, color1),
              Eq(Color4f(1.0f, 3.0f, 7.0f, 15.0f)));
}

TEST(ColorTest, Max) {
  Color4f color0(1.0f, 5.0f, 7.0f, 20.5f);
  Color4f color1(2.0f, 3.0f, 10.5f, 15.0f);

  EXPECT_THAT(Color4f::Max(color0, color1),
              Eq(Color4f(2.0f, 5.0f, 10.5f, 20.5f)));
}

TEST(ColorTest, Lerp) {
  Color4f color0(0.0f, 0.0f, 0.0f, 1.0f);
  Color4f color1(1.0f);

  EXPECT_THAT(Color4f::Lerp(color0, color1, 0.0f),
              Eq(Color4f(0.0f, 0.0f, 0.0f, 1.0f)));
  EXPECT_THAT(Color4f::Lerp(color0, color1, 0.5f),
              Eq(Color4f(0.5f, 0.5f, 0.5f, 1.0f)));
  EXPECT_THAT(Color4f::Lerp(color0, color1, 1.0f),
              Eq(Color4f(1.0f, 1.0f, 1.0f, 1.0f)));
}

}  // namespace
}  // namespace redux
