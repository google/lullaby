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
#include <limits>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/math/float.h"

namespace redux {
namespace {

using ::testing::Eq;

static constexpr float kMinFloat = std::numeric_limits<float>::min();
static constexpr float kMaxFloat = std::numeric_limits<float>::max();
static constexpr float kInfinity = std::numeric_limits<float>::infinity();

TEST(FloatTest, ExponentAsIntSpecial) {
  EXPECT_THAT(ExponentAsInt(kInfinity), Eq(kInfinityExponent));
  EXPECT_THAT(ExponentAsInt(-kInfinity), Eq(kInfinityExponent));
  EXPECT_THAT(ExponentAsInt(0.0f), Eq(kZeroExponent));
  EXPECT_THAT(ExponentAsInt(-0.0f), Eq(kZeroExponent));
  EXPECT_THAT(ExponentAsInt(kMaxFloat), Eq(kMaxFloatExponent));
  EXPECT_THAT(ExponentAsInt(-kMaxFloat), Eq(kMaxFloatExponent));
  EXPECT_THAT(ExponentAsInt(kMinFloat), Eq(kMinFloatExponent));
  EXPECT_THAT(ExponentAsInt(-kMinFloat), Eq(kMinFloatExponent));
}

TEST(FloatTest, ExponentAsIntExact) {
  int exponent = kMinFloatExponent;
  for (float f = kMinFloat; f <= kMaxFloat; f *= 2.0f) {
    EXPECT_THAT(ExponentAsInt(f), Eq(exponent));
    exponent++;
  }
}

TEST(FloatTest, ExponentAsIntOffset) {
  int exponent = kMinFloatExponent;
  for (float f = kMinFloat * 1.1f; f <= kMaxFloat; f *= 2.0f) {
    EXPECT_THAT(ExponentAsInt(f), Eq(exponent));
    exponent++;
  }
}

TEST(FloatTest, ExponentAsIntNegative) {
  int i = kMinFloatExponent;
  for (float f = -kMinFloat; f >= -kMaxFloat; f *= 2.0f) {
    EXPECT_THAT(ExponentAsInt(f), Eq(i));
    i++;
  }
}

TEST(FloatTest, ExponentFromIntSpecial) {
  EXPECT_THAT(ExponentFromInt(kInfinityExponent), Eq(kInfinity));
  EXPECT_THAT(ExponentFromInt(kZeroExponent), Eq(0.0f));
  EXPECT_THAT(ExponentFromInt(kMaxFloatExponent), Eq(std::pow(2.0f, 127.0f)));
  EXPECT_THAT(ExponentFromInt(kMinFloatExponent), Eq(std::pow(2.0f, -126.0f)));
}

TEST(FloatTest, ExponentFromInt) {
  float f = kMinFloat;
  for (int i = kMinInvertableExponent; i <= kMaxInvertableExponent; ++i) {
    EXPECT_THAT(ExponentFromInt(i), Eq(f));
    f *= 2.0f;
  }
}

TEST(FloatTest, ExponentBackAndForthToInt) {
  for (int i = kZeroExponent; i <= kInfinityExponent; ++i) {
    EXPECT_THAT(ExponentAsInt(ExponentFromInt(i)), Eq(i));
  }
}

TEST(FloatTest, ReciprocalExponentExtremes) {
  EXPECT_THAT(ReciprocalExponent(kMaxInvertablePowerOf2),
              Eq(kMinInvertablePowerOf2));
  EXPECT_THAT(ReciprocalExponent(kMinInvertablePowerOf2),
              Eq(kMaxInvertablePowerOf2));
}

TEST(FloatTest, ReciprocalExponentExact) {
  for (float f = kMinInvertablePowerOf2; f <= kMaxInvertablePowerOf2;
       f *= 2.0f) {
    EXPECT_THAT(ReciprocalExponent(f), Eq(1.0f / f));
  }
}

TEST(FloatTest, ReciprocalExponentOffset) {
  for (float f = kMinInvertablePowerOf2 * 1.3f; f <= kMaxInvertablePowerOf2;
       f *= 2.0f) {
    EXPECT_THAT(ReciprocalExponent(f),
                Eq(1.0f / ExponentFromInt(ExponentAsInt(f))));
  }
}

TEST(FloatTest, SqrtReciprocalExponentExact) {
  for (float f = kMinInvertablePowerOf2; f <= kMaxInvertablePowerOf2;
       f *= 2.0f) {
    EXPECT_THAT(SqrtReciprocalExponent(f),
                Eq(1.0f / ExponentFromInt(ExponentAsInt(f) / 2)));
  }
}

TEST(FloatTest, SqrtReciprocalExponentOffset) {
  for (float f = kMinInvertablePowerOf2 * 1.7f; f <= kMaxInvertablePowerOf2;
       f *= 2.0f) {
    EXPECT_THAT(SqrtReciprocalExponent(f),
                Eq(1.0f / ExponentFromInt(ExponentAsInt(f) / 2)));
  }
}

TEST(FloatTest, MaxPowerOf2ScaleExact) {
  EXPECT_THAT(MaxPowerOf2Scale(1.0f, 2), Eq(4.0f));
  EXPECT_THAT(MaxPowerOf2Scale(2.0f, 2), Eq(2.0f));
  EXPECT_THAT(MaxPowerOf2Scale(4.0f, 2), Eq(1.0f));
}

TEST(FloatTest, MaxPowerOf2ScaleOffset) {
  EXPECT_THAT(MaxPowerOf2Scale(1.1f, 2), Eq(4.0f));
  EXPECT_THAT(MaxPowerOf2Scale(2.4f, 2), Eq(2.0f));
  EXPECT_THAT(MaxPowerOf2Scale(4.9f, 2), Eq(1.0f));
}

TEST(FloatTest, MaxPowerOf2ScaleLessThan1) {
  for (float f = kMinInvertablePowerOf2; f <= 1.0f; f *= 2.0f) {
    EXPECT_THAT(MaxPowerOf2Scale(f, kMaxFloatExponent),
                Eq(ExponentFromInt(kMaxFloatExponent)));
  }
}

TEST(FloatTest, MaxPowerOf2ScaleMoreThan1) {
  float max = ExponentFromInt(kMaxFloatExponent);
  for (float f = 1.0f; f <= kMaxInvertablePowerOf2; f *= 2.0f) {
    EXPECT_THAT(MaxPowerOf2Scale(f, kMaxFloatExponent), Eq(max));
    max /= 2.0f;
  }
}

TEST(FloatTest, ClampNearZero) {
  EXPECT_THAT(ClampNearZero(0.0f, 0.0f), Eq(0.0f));
  EXPECT_THAT(ClampNearZero(kInfinity, 0.0f), Eq(kInfinity));
  EXPECT_THAT(ClampNearZero(1.0f, 1.0f), Eq(0.0f));
  EXPECT_THAT(ClampNearZero(2.0f, 1.0f), Eq(2.0f));
  EXPECT_THAT(ClampNearZero(0.00001f, 0.0001f), Eq(0.0f));
  EXPECT_THAT(ClampNearZero(0.00001f, 0.000001f), Eq(0.00001f));
}

}  // namespace
}  // namespace redux
