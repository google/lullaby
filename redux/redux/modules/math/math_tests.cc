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

#include <cstdint>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/math/constants.h"
#include "redux/modules/math/math.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatNear;

TEST(MathTest, IsNearlyZero) {
  EXPECT_TRUE(IsNearlyZero(0));
  EXPECT_TRUE(IsNearlyZero(0.0f));
  EXPECT_TRUE(IsNearlyZero(1.0e-8f));
  EXPECT_FALSE(IsNearlyZero(1));
  EXPECT_FALSE(IsNearlyZero(0.1f));
  EXPECT_TRUE(IsNearlyZero(1, 2));
  EXPECT_TRUE(IsNearlyZero(0.1f, 0.2f));
}

TEST(MathTest, AreNearlyEqual) {
  EXPECT_TRUE(AreNearlyEqual(0, 0));
  EXPECT_TRUE(AreNearlyEqual(1, 1));
  EXPECT_TRUE(AreNearlyEqual(-1, -1));
  EXPECT_FALSE(AreNearlyEqual(-1, 1));
  EXPECT_TRUE(AreNearlyEqual(-1, 1, 3));

  EXPECT_TRUE(AreNearlyEqual(0.0f, 0.0f));
  EXPECT_TRUE(AreNearlyEqual(0.1f, 0.1f));
  EXPECT_TRUE(AreNearlyEqual(1.0e-8f, 0.0f));
  EXPECT_TRUE(AreNearlyEqual(1.0f, 1.0f - 1.0e-8f));
  EXPECT_TRUE(AreNearlyEqual(0.0f, 0.1f, 0.1f));
  EXPECT_FALSE(AreNearlyEqual(0.0f, 0.1f));
}

TEST(MathTest, Clamp) {
  EXPECT_THAT(Clamp(1, 0, 2), Eq(1));
  EXPECT_THAT(Clamp(-1, 0, 2), Eq(0));
  EXPECT_THAT(Clamp(3, 0, 2), Eq(2));
  EXPECT_THAT(Clamp(0, 0, 2), Eq(0));
  EXPECT_THAT(Clamp(2, 0, 2), Eq(2));
}

TEST(MathTest, MinMax) {
  EXPECT_THAT(Min(0, 1), Eq(0));
  EXPECT_THAT(Max(0, 1), Eq(1));
  EXPECT_THAT(Min(1, 1), Eq(1));
  EXPECT_THAT(Max(1, 1), Eq(1));
  EXPECT_THAT(Min(2, 1), Eq(1));
  EXPECT_THAT(Max(2, 1), Eq(2));
}

TEST(MathTest, Length) {
  EXPECT_THAT(Length(0), Eq(0));
  EXPECT_THAT(Length(123), Eq(123));
}

TEST(MathTest, DegreesToRadians) {
  EXPECT_THAT(DegreesToRadians(0.0f), Eq(0.0f));
  EXPECT_THAT(DegreesToRadians(180.0f), Eq(kPi));
  EXPECT_THAT(DegreesToRadians(-180.0f), Eq(-kPi));
  EXPECT_THAT(DegreesToRadians(360.0f), Eq(2.0f * kPi));
  EXPECT_THAT(DegreesToRadians(-360.0f), Eq(-2.0f * kPi));
  EXPECT_THAT(DegreesToRadians(45.0f), Eq(0.25f * kPi));
}

TEST(MathTest, RadiansToDegrees) {
  EXPECT_THAT(RadiansToDegrees(0.0f), Eq(0.0f));
  EXPECT_THAT(RadiansToDegrees(kPi), Eq(180.0f));
  EXPECT_THAT(RadiansToDegrees(-kPi), Eq(-180.0f));
  EXPECT_THAT(RadiansToDegrees(2.0f * kPi), Eq(360.0f));
  EXPECT_THAT(RadiansToDegrees(-2.0f * kPi), Eq(-360.0f));
  EXPECT_THAT(RadiansToDegrees(0.25f * kPi), Eq(45.0f));
}

TEST(MathTest, ModDegrees) {
  EXPECT_THAT(ModDegrees(0.0f), Eq(0.0f));
  EXPECT_THAT(ModDegrees(90.0f), Eq(90.0f));
  EXPECT_THAT(ModDegrees(180.0f), Eq(-180.0f));
  EXPECT_THAT(ModDegrees(270.0f), Eq(-90.0f));
  EXPECT_THAT(ModDegrees(360.0f), Eq(0.0f));
  EXPECT_THAT(ModDegrees(540.0f), Eq(-180.0f));
  EXPECT_THAT(ModDegrees(-90.0f), Eq(-90.0f));
  EXPECT_THAT(ModDegrees(-180.0f), Eq(-180.0f));
  EXPECT_THAT(ModDegrees(-270.0f), Eq(90.0f));
  EXPECT_THAT(ModDegrees(-360.0f), Eq(0.0f));

  // Check values around the -180 and 180.
  EXPECT_THAT(ModDegrees(180.0f - kDefaultEpsilon),
              FloatNear(-180.0f, kDefaultEpsilon));
  EXPECT_THAT(ModDegrees(180.0f + kDefaultEpsilon),
              FloatNear(-180.0f, kDefaultEpsilon));
  EXPECT_THAT(ModDegrees(360.0f - kDefaultEpsilon),
              FloatNear(0.0f, kDefaultEpsilon));
  EXPECT_THAT(ModDegrees(360.0f + kDefaultEpsilon),
              FloatNear(0.0f, kDefaultEpsilon));
  EXPECT_THAT(ModDegrees(540.0f - kDefaultEpsilon),
              FloatNear(-180.0f, kDefaultEpsilon));
  EXPECT_THAT(ModDegrees(540.0f + kDefaultEpsilon),
              FloatNear(-180.0f, kDefaultEpsilon));
}

TEST(MathTest, ModRadians) {
  EXPECT_THAT(ModRadians(kPi * 0.0f), FloatNear(kPi * 0.0f, kDefaultEpsilon));
  EXPECT_THAT(ModRadians(kPi * 0.5f), FloatNear(kPi * 0.5f, kDefaultEpsilon));
  EXPECT_THAT(ModRadians(kPi * 1.0f), FloatNear(kPi * -1.0f, kDefaultEpsilon));
  EXPECT_THAT(ModRadians(kPi * 1.5f), FloatNear(kPi * -0.5f, kDefaultEpsilon));
  EXPECT_THAT(ModRadians(kPi * 2.0f), FloatNear(kPi * 0.0f, kDefaultEpsilon));
  EXPECT_THAT(ModRadians(kPi * 2.5f), FloatNear(kPi * 0.5f, kDefaultEpsilon));
  EXPECT_THAT(ModRadians(kPi * -0.5f), FloatNear(kPi * -0.5f, kDefaultEpsilon));
  EXPECT_THAT(ModRadians(kPi * -1.0f), FloatNear(-kPi * 1.0f, kDefaultEpsilon));
  EXPECT_THAT(ModRadians(kPi * -1.5f), FloatNear(kPi * 0.5f, kDefaultEpsilon));
  EXPECT_THAT(ModRadians(kPi * -2.0f), FloatNear(kPi * 0.0f, kDefaultEpsilon));

  // fmod causes some loss of precision.
  const float epsilon = kDefaultEpsilon * 10.0f;
  EXPECT_THAT(ModRadians(kPi * 1.0f - kDefaultEpsilon),
              FloatNear(kPi * 1.0f, epsilon));
  EXPECT_THAT(ModRadians(kPi * 1.0f + kDefaultEpsilon),
              FloatNear(kPi * -1.0f, epsilon));
  EXPECT_THAT(ModRadians(kPi * 2.0f - kDefaultEpsilon),
              FloatNear(kPi * 0.0f, epsilon));
  EXPECT_THAT(ModRadians(kPi * 2.0f + kDefaultEpsilon),
              FloatNear(kPi * 0.0f, epsilon));
}

TEST(MathTest, IsPowerOf2) {
  EXPECT_TRUE(IsPowerOf2(1));
  EXPECT_TRUE(IsPowerOf2(2));
  EXPECT_TRUE(IsPowerOf2(4));
  EXPECT_TRUE(IsPowerOf2(8));
  EXPECT_TRUE(IsPowerOf2(16));
  EXPECT_TRUE(IsPowerOf2(32));

  EXPECT_FALSE(IsPowerOf2(0));
  EXPECT_FALSE(IsPowerOf2(-1));
  EXPECT_FALSE(IsPowerOf2(-2));
  EXPECT_FALSE(IsPowerOf2(-3));
  EXPECT_FALSE(IsPowerOf2(127));
  EXPECT_FALSE(IsPowerOf2(129));

  for (uint32_t i = 2; i < 32; ++i) {
    const uint32_t n = 1 << i;
    EXPECT_TRUE(IsPowerOf2(n));
    EXPECT_FALSE(IsPowerOf2(n + 1));
    EXPECT_FALSE(IsPowerOf2(n - 1));
  }
}

TEST(MathTest, AlignToPowerOf2) {
  // Step up through some powers of 2 and perform tests around them.
  const uint32_t kMaxExponent = 8;
  for (uint32_t a = 0; a < kMaxExponent; ++a) {
    const uint32_t lower_pow2 = 1 << a;
    EXPECT_THAT(AlignToPowerOf2(lower_pow2, lower_pow2), Eq(lower_pow2));
    if (lower_pow2 != 1) {
      EXPECT_THAT(AlignToPowerOf2(lower_pow2 - 1, lower_pow2), Eq(lower_pow2));
    }
    EXPECT_THAT(AlignToPowerOf2(lower_pow2 + 1, lower_pow2),
                Eq(2 * lower_pow2));

    for (uint32_t b = a + 1; b < kMaxExponent; ++b) {
      const uint32_t higher_pow2 = 1 << b;

      EXPECT_THAT(AlignToPowerOf2(lower_pow2, higher_pow2), Eq(higher_pow2));
      EXPECT_THAT(AlignToPowerOf2(higher_pow2, lower_pow2), Eq(higher_pow2));
    }
  }
}

}  // namespace
}  // namespace redux
