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

#include <cmath>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/math/interpolation.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatEq;

TEST(InterpolationTest, Lerp) {
  EXPECT_THAT(Lerp(0.0f, 1.0f, 0.00f), Eq(0.00f));
  EXPECT_THAT(Lerp(0.0f, 1.0f, 0.25f), Eq(0.25f));
  EXPECT_THAT(Lerp(0.0f, 1.0f, 0.50f), Eq(0.50f));
  EXPECT_THAT(Lerp(0.0f, 1.0f, 0.75f), Eq(0.75f));
  EXPECT_THAT(Lerp(0.0f, 1.0f, 1.00f), Eq(1.00f));

  EXPECT_THAT(Lerp(0.0f, 2.0f, 0.00f), Eq(0.00f));
  EXPECT_THAT(Lerp(0.0f, 2.0f, 0.25f), Eq(0.50f));
  EXPECT_THAT(Lerp(0.0f, 2.0f, 0.50f), Eq(1.00f));
  EXPECT_THAT(Lerp(0.0f, 2.0f, 0.75f), Eq(1.50f));
  EXPECT_THAT(Lerp(0.0f, 2.0f, 1.00f), Eq(2.00f));

  EXPECT_THAT(Lerp(1.0f, 2.0f, 0.00f), Eq(1.00f));
  EXPECT_THAT(Lerp(1.0f, 2.0f, 0.25f), Eq(1.25f));
  EXPECT_THAT(Lerp(1.0f, 2.0f, 0.50f), Eq(1.50f));
  EXPECT_THAT(Lerp(1.0f, 2.0f, 0.75f), Eq(1.75f));
  EXPECT_THAT(Lerp(1.0f, 2.0f, 1.00f), Eq(2.00f));

  EXPECT_THAT(Lerp(1.0f, 0.0f, 0.00f), Eq(1.00f));
  EXPECT_THAT(Lerp(1.0f, 0.0f, 0.25f), Eq(0.75f));
  EXPECT_THAT(Lerp(1.0f, 0.0f, 0.50f), Eq(0.50f));
  EXPECT_THAT(Lerp(1.0f, 0.0f, 0.75f), Eq(0.25f));
}

TEST(InterpolationTest, FastOutSlowIn) {
  EXPECT_THAT(FastOutSlowIn(0.0f), 0.0f);
  EXPECT_THAT(FastOutSlowIn(1.0f), 1.0f);
  EXPECT_THAT(FastOutSlowIn(0.525252525f), 0.8f);
}

TEST(InterpolationTest, SineEaseIn) {
  auto fn = [](float t) { return 1.0f - std::cos((t * kPi) / 2.0f); };
  EXPECT_THAT(SineEaseIn(0.0f, 1.0f, 0.00f), FloatEq(fn(0.00f)));
  EXPECT_THAT(SineEaseIn(0.0f, 1.0f, 0.25f), FloatEq(fn(0.25f)));
  EXPECT_THAT(SineEaseIn(0.0f, 1.0f, 0.50f), FloatEq(fn(0.50f)));
  EXPECT_THAT(SineEaseIn(0.0f, 1.0f, 0.75f), FloatEq(fn(0.75f)));
  EXPECT_THAT(SineEaseIn(0.0f, 1.0f, 1.00f), FloatEq(fn(1.00f)));
}

TEST(InterpolationTest, SineEaseOut) {
  auto fn = [](float t) { return std::sin((t * kPi) / 2.0f); };
  EXPECT_THAT(SineEaseOut(0.0f, 1.0f, 0.00f), FloatEq(fn(0.00f)));
  EXPECT_THAT(SineEaseOut(0.0f, 1.0f, 0.25f), FloatEq(fn(0.25f)));
  EXPECT_THAT(SineEaseOut(0.0f, 1.0f, 0.50f), FloatEq(fn(0.50f)));
  EXPECT_THAT(SineEaseOut(0.0f, 1.0f, 0.75f), FloatEq(fn(0.75f)));
  EXPECT_THAT(SineEaseOut(0.0f, 1.0f, 1.00f), FloatEq(fn(1.00f)));
}

TEST(InterpolationTest, SineEaseInOut) {
  auto fn = [](float t) { return -(std::cos(kPi * t) - 1.0f) / 2.0f; };
  EXPECT_THAT(SineEaseInOut(0.0f, 1.0f, 0.00f), FloatEq(fn(0.00f)));
  EXPECT_THAT(SineEaseInOut(0.0f, 1.0f, 0.25f), FloatEq(fn(0.25f)));
  EXPECT_THAT(SineEaseInOut(0.0f, 1.0f, 0.50f), FloatEq(fn(0.50f)));
  EXPECT_THAT(SineEaseInOut(0.0f, 1.0f, 0.75f), FloatEq(fn(0.75f)));
  EXPECT_THAT(SineEaseInOut(0.0f, 1.0f, 1.00f), FloatEq(fn(1.00f)));
}

TEST(InterpolationTest, EaseInPow) {
  EXPECT_THAT(detail::EaseInPow(0.0f, 1.0f, 0.5f, 2.0f), Eq(0.25f));
}

TEST(InterpolationTest, EaseOutPow) {
  EXPECT_THAT(detail::EaseOutPow(0.0f, 1.0f, 0.5f, 2.0f), Eq(0.75f));
}
TEST(InterpolationTest, Quadratic) {
  EXPECT_THAT(QuadraticEaseIn(0.0f, 1.0f, 0.00f), Eq(0.00f));
  EXPECT_THAT(QuadraticEaseIn(0.0f, 1.0f, 0.50f), Eq(0.25f));
  EXPECT_THAT(QuadraticEaseIn(0.0f, 1.0f, 1.00f), Eq(1.00f));

  EXPECT_THAT(QuadraticEaseIn(1.0f, 0.0f, 0.00f), Eq(1.00f));
  EXPECT_THAT(QuadraticEaseIn(1.0f, 0.0f, 0.50f), Eq(0.75f));
  EXPECT_THAT(QuadraticEaseIn(1.0f, 0.0f, 1.00f), Eq(0.00f));

  EXPECT_THAT(QuadraticEaseOut(0.0f, 1.0f, 0.00f), Eq(0.00f));
  EXPECT_THAT(QuadraticEaseOut(0.0f, 1.0f, 0.50f), Eq(0.75f));
  EXPECT_THAT(QuadraticEaseOut(0.0f, 1.0f, 1.00f), Eq(1.00f));

  EXPECT_THAT(QuadraticEaseOut(1.0f, 0.0f, 0.00f), Eq(1.00f));
  EXPECT_THAT(QuadraticEaseOut(1.0f, 0.0f, 0.50f), Eq(0.25f));
  EXPECT_THAT(QuadraticEaseOut(1.0f, 0.0f, 1.00f), Eq(0.00f));

  EXPECT_THAT(QuadraticEaseInOut(0.0f, 1.0f, 0.25f), Eq(0.125f));
  EXPECT_THAT(QuadraticEaseInOut(0.0f, 1.0f, 0.50f), Eq(0.5f));
  EXPECT_THAT(QuadraticEaseInOut(0.0f, 1.0f, 0.75f), Eq(0.875f));

  EXPECT_THAT(QuadraticEaseInOut(1.0f, 0.0f, 0.25f), Eq(0.875f));
  EXPECT_THAT(QuadraticEaseInOut(1.0f, 0.0f, 0.50f), Eq(0.5f));
  EXPECT_THAT(QuadraticEaseInOut(1.0f, 0.0f, 0.75f), Eq(0.125f));
}

TEST(InterpolationTest, Cubic) {
  EXPECT_THAT(CubicEaseIn(0.0f, 1.0f, 0.0f), Eq(0.000f));
  EXPECT_THAT(CubicEaseIn(0.0f, 1.0f, 0.5f), Eq(0.125f));
  EXPECT_THAT(CubicEaseIn(0.0f, 1.0f, 1.0f), Eq(1.000f));

  EXPECT_THAT(CubicEaseIn(1.0f, 0.0f, 0.0f), Eq(1.000f));
  EXPECT_THAT(CubicEaseIn(1.0f, 0.0f, 0.5f), Eq(0.875f));
  EXPECT_THAT(CubicEaseIn(1.0f, 0.0f, 1.0f), Eq(0.000f));

  EXPECT_THAT(CubicEaseOut(0.0f, 1.0f, 0.0f), Eq(0.000f));
  EXPECT_THAT(CubicEaseOut(0.0f, 1.0f, 0.5f), Eq(0.875f));
  EXPECT_THAT(CubicEaseOut(0.0f, 1.0f, 1.0f), Eq(1.000f));

  EXPECT_THAT(CubicEaseOut(1.0f, 0.0f, 0.0f), Eq(1.000f));
  EXPECT_THAT(CubicEaseOut(1.0f, 0.0f, 0.5f), Eq(0.125f));
  EXPECT_THAT(CubicEaseOut(1.0f, 0.0f, 1.0f), Eq(0.000f));

  EXPECT_THAT(CubicEaseInOut(0.0f, 1.0f, 0.25f), Eq(0.0625f));
  EXPECT_THAT(CubicEaseInOut(0.0f, 1.0f, 0.50f), Eq(0.5000f));
  EXPECT_THAT(CubicEaseInOut(0.0f, 1.0f, 0.75f), Eq(0.9375f));

  EXPECT_THAT(CubicEaseInOut(1.0f, 0.0f, 0.25f), Eq(0.9375f));
  EXPECT_THAT(CubicEaseInOut(1.0f, 0.0f, 0.50f), Eq(0.5000f));
  EXPECT_THAT(CubicEaseInOut(1.0f, 0.0f, 0.75f), Eq(0.0625f));
}

TEST(InterpolationTest, Quartic) {
  EXPECT_THAT(QuarticEaseIn(0.0f, 1.0f, 0.0f), Eq(0.0f));
  EXPECT_THAT(QuarticEaseIn(0.0f, 1.0f, 0.5f), Eq(0.0625f));
  EXPECT_THAT(QuarticEaseIn(0.0f, 1.0f, 1.0f), Eq(1.0f));

  EXPECT_THAT(QuarticEaseIn(1.0f, 0.0f, 0.0f), Eq(1.0f));
  EXPECT_THAT(QuarticEaseIn(1.0f, 0.0f, 0.5f), Eq(0.9375f));
  EXPECT_THAT(QuarticEaseIn(1.0f, 0.0f, 1.0f), Eq(0.0f));

  EXPECT_THAT(QuarticEaseOut(0.0f, 1.0f, 0.0f), Eq(0.0f));
  EXPECT_THAT(QuarticEaseOut(0.0f, 1.0f, 0.5f), Eq(0.9375f));
  EXPECT_THAT(QuarticEaseOut(0.0f, 1.0f, 1.0f), Eq(1.0f));

  EXPECT_THAT(QuarticEaseOut(1.0f, 0.0f, 0.0f), Eq(1.0f));
  EXPECT_THAT(QuarticEaseOut(1.0f, 0.0f, 0.5f), Eq(0.0625f));
  EXPECT_THAT(QuarticEaseOut(1.0f, 0.0f, 1.0f), Eq(0.0f));

  EXPECT_THAT(QuarticEaseInOut(0.0f, 1.0f, 0.25f), Eq(0.03125f));
  EXPECT_THAT(QuarticEaseInOut(0.0f, 1.0f, 0.50f), Eq(0.5f));
  EXPECT_THAT(QuarticEaseInOut(0.0f, 1.0f, 0.75f), Eq(0.96875f));

  EXPECT_THAT(QuarticEaseInOut(1.0f, 0.0f, 0.25f), Eq(0.96875f));
  EXPECT_THAT(QuarticEaseInOut(1.0f, 0.0f, 0.50f), Eq(0.5f));
  EXPECT_THAT(QuarticEaseInOut(1.0f, 0.0f, 0.75f), Eq(0.03125f));
}

TEST(InterpolationTest, Quintic) {
  EXPECT_THAT(QuinticEaseIn(0.0f, 1.0f, 0.0f), Eq(0.0f));
  EXPECT_THAT(QuinticEaseIn(0.0f, 1.0f, 0.5f), Eq(0.03125f));
  EXPECT_THAT(QuinticEaseIn(0.0f, 1.0f, 1.0f), Eq(1.0f));

  EXPECT_THAT(QuinticEaseIn(1.0f, 0.0f, 0.0f), Eq(1.0f));
  EXPECT_THAT(QuinticEaseIn(1.0f, 0.0f, 0.5f), Eq(0.96875f));
  EXPECT_THAT(QuinticEaseIn(1.0f, 0.0f, 1.0f), Eq(0.0f));

  EXPECT_THAT(QuinticEaseOut(0.0f, 1.0f, 0.0f), Eq(0.0f));
  EXPECT_THAT(QuinticEaseOut(0.0f, 1.0f, 0.5f), Eq(0.96875f));
  EXPECT_THAT(QuinticEaseOut(0.0f, 1.0f, 1.0f), Eq(1.0f));

  EXPECT_THAT(QuinticEaseOut(1.0f, 0.0f, 0.0f), Eq(1.0f));
  EXPECT_THAT(QuinticEaseOut(1.0f, 0.0f, 0.5f), Eq(0.03125f));
  EXPECT_THAT(QuinticEaseOut(1.0f, 0.0f, 1.0f), Eq(0.0f));

  EXPECT_THAT(QuinticEaseInOut(0.0f, 1.0f, 0.25f), Eq(0.015625f));
  EXPECT_THAT(QuinticEaseInOut(0.0f, 1.0f, 0.50f), Eq(0.5f));
  EXPECT_THAT(QuinticEaseInOut(0.0f, 1.0f, 0.75f), Eq(0.984375f));

  EXPECT_THAT(QuinticEaseInOut(1.0f, 0.0f, 0.25f), Eq(0.984375f));
  EXPECT_THAT(QuinticEaseInOut(1.0f, 0.0f, 0.50f), Eq(0.5f));
  EXPECT_THAT(QuinticEaseInOut(1.0f, 0.0f, 0.75f), Eq(0.015625f));
}
}  // namespace
}  // namespace redux
