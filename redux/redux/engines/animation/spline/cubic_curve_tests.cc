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
#include "redux/engines/animation/spline/cubic_curve.h"

namespace redux {
namespace {

using ::testing::FloatNear;
using ::testing::Lt;

typedef void ShiftFn(float shift, CubicCurve* c);
static void ShiftLeft(float shift, CubicCurve* c) { c->ShiftLeft(shift); }
static void ShiftRight(float shift, CubicCurve* c) { c->ShiftRight(shift); }

static void TestShift(const CubicInit& init, float shift, ShiftFn* fn,
                      float direction) {
  const CubicCurve c(init);
  CubicCurve shifted(c);
  fn(shift, &shifted);

  const float epsilon = shifted.Epsilon();
  const float offset = direction * shift;
  EXPECT_THAT(shifted.Evaluate(offset), FloatNear(c.Evaluate(0.0f), epsilon));
  EXPECT_THAT(shifted.Evaluate(0.0f), FloatNear(c.Evaluate(-offset), epsilon));
  EXPECT_THAT(shifted.Derivative(offset),
              FloatNear(c.Derivative(0.0f), epsilon));
  EXPECT_THAT(shifted.Derivative(0.0f),
              FloatNear(c.Derivative(-offset), epsilon));
  EXPECT_THAT(shifted.SecondDerivative(offset),
              FloatNear(c.SecondDerivative(0.0f), epsilon));
  EXPECT_THAT(shifted.SecondDerivative(0.0f),
              FloatNear(c.SecondDerivative(-offset), epsilon));
}

static void TestShiftLeft(const CubicInit& init, float shift) {
  TestShift(init, shift, ShiftLeft, -1.0f);
}

static void TestShiftRight(const CubicInit& init, float shift) {
  TestShift(init, shift, ShiftRight, 1.0f);
}

TEST(CubicCurveTests, CubicWithWidth) {
  const CubicInit init(1.0f, -8.0f, 0.3f, -4.0f, 1.0f);
  const CubicCurve c(init);
  const float epsilon = c.Epsilon();
  EXPECT_THAT(std::fabs(c.Evaluate(init.width_x) - init.end_y), Lt(epsilon));
}

TEST(CubicCurveTests, CubicShiftLeft) {
  const CubicInit init(1.0f, -8.0f, 0.3f, -4.0f, 1.0f);
  TestShiftLeft(init, 0.0f);
  TestShiftLeft(init, 1.0f);
  TestShiftLeft(init, -0.1f);
  TestShiftLeft(init, 0.00001f);
  TestShiftLeft(init, 10.0f);
}

TEST(CubicCurveTests, CubicShiftRight) {
  const CubicInit init(1.0f, -8.0f, 0.3f, -4.0f, 1.0f);
  TestShiftRight(init, 0.0f);
  TestShiftRight(init, 1.0f);
  TestShiftRight(init, -0.1f);
  TestShiftRight(init, 0.00001f);
  TestShiftRight(init, 10.0f);
}
}  // namespace
}  // namespace redux
