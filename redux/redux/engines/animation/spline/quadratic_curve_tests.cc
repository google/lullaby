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
#include "redux/engines/animation/spline/quadratic_curve.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatNear;
using ::testing::Lt;

static const float kMaxFloat = std::numeric_limits<float>::max();
static const float kMinFloat = std::numeric_limits<float>::min();

static void CheckQuadraticRoots(const QuadraticCurve& s,
                                size_t num_expected_roots) {
  // Ensure we have the correct number of roots.
  float roots[2];
  const size_t num_roots = s.Roots(roots);
  EXPECT_THAT(num_roots, Eq(num_expected_roots));

  // Ensure roots are in ascending order.
  EXPECT_TRUE(num_roots < 2 || roots[0] < roots[1]);

  // Ensure roots all evaluate to zero.
  for (size_t i = 0; i < num_roots; ++i) {
    float should_be_zero = s.Evaluate(roots[i]);
    float epsilon = s.EpsilonInInterval(roots[i]);

    // If the quadratic has crazy coefficients and evalues to infinity,
    // scale it down in y. The roots should be the same.
    // Note that we don't want to do this in general because it's a less
    // accurate test.
    if (std::fabs(should_be_zero) > std::numeric_limits<float>::max()) {
      const QuadraticCurve s_shrunk(s, 1.0f / s.MaxCoeff());
      should_be_zero = s_shrunk.Evaluate(roots[i]);
      epsilon = s_shrunk.EpsilonInInterval(roots[i]);
    }

    EXPECT_THAT(should_be_zero, FloatNear(0.0f, epsilon));
  }
}

static void CheckCriticalPoint(const QuadraticCurve& s) {
  // Derivative should be zero at critical point.
  const float critical_point_x = s.CriticalPoint();
  const float critical_point_derivative = s.Derivative(critical_point_x);
  const float epsilon = s.EpsilonInInterval(critical_point_x);
  EXPECT_THAT(std::fabs(critical_point_derivative), Lt(epsilon));
}

// Test for some coefficients as max float, one solution.
TEST(QuadraticCurveTests, QuadraticRoot_OneMaxOneSolution) {
  CheckQuadraticRoots(QuadraticCurve(kMaxFloat, 0.0f, 0.0f), 1);
}

// Test for all coefficients as max float, two solutions.
TEST(QuadraticCurveTests, QuadraticRoot_AllMaxTwoSolutions) {
  CheckQuadraticRoots(QuadraticCurve(kMaxFloat, kMaxFloat, -kMaxFloat), 2);
}

// Test for some coefficients as max float, two solutions.
TEST(QuadraticCurveTests, QuadraticRoot_TwoMaxTwoSolutions) {
  CheckQuadraticRoots(QuadraticCurve(kMaxFloat, kMaxFloat, -1.0f), 2);
}

// Test for all coefficients as max float, no solutions.
TEST(QuadraticCurveTests, QuadraticRoot_AllMaxNoSolutions) {
  CheckQuadraticRoots(QuadraticCurve(kMaxFloat, kMaxFloat, kMaxFloat), 0);
}

// Test for all coefficients as min float, no solutions.
TEST(QuadraticCurveTests, QuadraticRoot_AllMinNoSolutions) {
  CheckQuadraticRoots(QuadraticCurve(kMinFloat, kMinFloat, kMinFloat), 0);
}

// Test for all coefficients as min float, two solutions.
TEST(QuadraticCurveTests, QuadraticRoot_AllMinTwoSolutions) {
  CheckQuadraticRoots(QuadraticCurve(-kMinFloat, kMinFloat, kMinFloat), 2);
}

// Test for one coefficient as min float, one solution.
TEST(QuadraticCurveTests, QuadraticRoot_OneMinOneSolution) {
  CheckQuadraticRoots(QuadraticCurve(-kMinFloat, 0.0f, 0.0f), 1);
}

// Test for one coefficient as min float, one solution.
TEST(QuadraticCurveTests, QuadraticRoot_MinMaxMixOneSolution) {
  CheckQuadraticRoots(QuadraticCurve(-kMinFloat, kMaxFloat, 1.0f), 1);
}

// Test for one coefficient as min float, two solutions.
TEST(QuadraticCurveTests, QuadraticRoot_MaxMinMixOneSolution) {
  CheckQuadraticRoots(QuadraticCurve(kMaxFloat, -kMinFloat, 0.0f), 1);
}

// Test for zeros everywhere but the constant component.
TEST(QuadraticCurveTests, QuadraticRoot_Constant) {
  CheckQuadraticRoots(QuadraticCurve(0.0f, 0.0f, 1.0f), 0);
}

// Test for zeros everywhere but the linear component.
TEST(QuadraticCurveTests, QuadraticRoot_Linear) {
  CheckQuadraticRoots(QuadraticCurve(0.0f, 1.0f, 0.0f), 1);
}

// Test for zeros everywhere but the quadratic component.
TEST(QuadraticCurveTests, QuadraticRoot_Quadratic) {
  CheckQuadraticRoots(QuadraticCurve(1.0f, 0.0f, 0.0f), 1);
}

TEST(QuadraticCurveTests, QuadraticRoot_UpwardsAbove) {
  // Curves upwards, critical point above zero ==> no real roots.
  CheckQuadraticRoots(QuadraticCurve(60.0f, -32.0f, 6.0f), 0);
}

TEST(QuadraticCurveTests, QuadraticRoot_UpwardsAt) {
  // Curves upwards, critical point at zero ==> one real root.
  CheckQuadraticRoots(QuadraticCurve(60.0f, -32.0f, 4.26666689f), 1);
}

TEST(QuadraticCurveTests, QuadraticRoot_UpwardsBelow) {
  // Curves upwards, critical point below zero ==> two real roots.
  CheckQuadraticRoots(QuadraticCurve(60.0f, -32.0f, 4.0f), 2);
}

TEST(QuadraticCurveTests, QuadraticRoot_DownwardsAbove) {
  // Curves downwards, critical point above zero ==> two real roots.
  CheckQuadraticRoots(QuadraticCurve(-0.00006f, -0.000028f, 0.0001f), 2);
}

TEST(QuadraticCurveTests, QuadraticRoot_DownwardsAt) {
  // Curves downwards, critical point at zero ==> one real root at critical
  // point.
  CheckQuadraticRoots(QuadraticCurve(-0.00006f, -0.000028f, -0.00000326666691f),
                      1);
}

TEST(QuadraticCurveTests, QuadraticRoot_DownwardsBelow) {
  // Curves downwards, critical point below zero ==> no real roots.
  CheckQuadraticRoots(QuadraticCurve(-0.00006f, -0.000028f, -0.000006f), 0);
}

TEST(QuadraticCurveTests, QuadraticRoot_AllTinyCoefficients) {
  // Curves upwards, critical point below zero ==> two real roots.
  CheckQuadraticRoots(
      QuadraticCurve(0.000000006f, -0.0000000032f, 0.0000000004f), 2);
}

TEST(QuadraticCurveTests, QuadraticRoot_SmallSquareCoefficient) {
  CheckQuadraticRoots(QuadraticCurve(-0.00000003f, 0.0f, 0.0008f), 2);
}

TEST(QuadraticCurveTests, QuadraticRoot_TinySquareCoefficient) {
  CheckQuadraticRoots(QuadraticCurve(0.000000001f, 1.0f, -0.00000001f), 2);
}

TEST(QuadraticCurveTests, QuadraticCriticalPoint) {
  // Curves upwards, critical point above zero ==> no real roots.
  CheckCriticalPoint(QuadraticCurve(60.0f, -32.0f, 6.0f));
}

TEST(QuadraticCurveTests, QuadraticRangesMatchingSign_SmallValues) {
  const Interval limits(0.0f, 1.0f);
  const QuadraticCurve small(1.006107e-11f, -3.01832101e-11f, 1.006107e-11f);

  Interval matching[2];
  const int num_matches = small.IntervalsMatchingSign(limits, 1.0f, matching);
  EXPECT_THAT(num_matches, Eq(1));
}
}  // namespace
}  // namespace redux
