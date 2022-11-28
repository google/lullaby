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

#include <algorithm>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/engines/animation/spline/bulk_spline_evaluator.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatNear;

static const Interval kAngleInterval(-kPi, kPi);
static const float kFixedPointEpsilon = 0.02f;
static const float kDerivativePrecision = 0.01f;
static const float kSecondDerivativePrecision = 0.26f;
static const float kThirdDerivativePrecision = 6.0f;
static const float kXGranularityScale = 0.01f;

// Data structure to hold the results of sampling a spline.
struct SampledSpline {
  std::vector<vec2> points;
  std::vector<vec3> derivatives;
};

// A collection of simple splines. We'll be using these to test the evaluator.
static const CubicInit kSimpleSplines[] = {
    //    start_y         end_y        width_x
    //       start_derivative  end_derivative
    CubicInit(0.0f, 1.0f, 0.1f, 0.0f, 1.0f),
    CubicInit(1.0f, -8.0f, 0.0f, 0.0f, 1.0f),
    CubicInit(1.0f, -8.0f, -1.0f, 0.0f, 1.0f),
};
static const int kNumSimpleSplines = ABSL_ARRAYSIZE(kSimpleSplines);

static Interval CubicInitYInterval(const CubicInit& init,
                                   float buffer_percent) {
  const auto min = std::min(init.start_y, init.end_y);
  const auto max = std::max(init.start_y, init.end_y);
  return Interval(min, max).Scaled(buffer_percent);
}

SampledSpline SampleSpline(const CubicInit& init, bool is_angle = false,
                           float y_scale = 1.0f, float y_offset = 0.0f) {
  // Create a spline using the `init` arguments.
  CompactSpline spline;
  const Interval y_range = CubicInitYInterval(init, 0.1f);
  spline.Init(y_range, init.width_x * kXGranularityScale);
  spline.AddNode(0.0f, init.start_y, init.start_derivative);
  spline.AddNode(init.width_x, init.end_y, init.end_derivative);

  // Load the spline into the evaluator.
  BulkSplineEvaluator evaluator;
  evaluator.SetNumIndices(1);
  if (is_angle) {
    evaluator.SetYRanges(0, 1, kAngleInterval);
  }
  SplinePlayback playback;
  playback.y_offset = y_offset;
  playback.y_scale = y_scale;
  evaluator.SetSplines(0, 1, &spline, playback);

  // Get the extents of the loaded spline to determine our precision.
  const float y_precision =
      evaluator.SourceSpline(0)->IntervalY().Size() * kFixedPointEpsilon;
  const Interval range_x = evaluator.SourceSpline(0)->IntervalX();
  const float derivative_precision = fabs(y_scale) * kDerivativePrecision;

  // Figure out the sampling frequency.
  static const int kNumSamples = 80;
  const float delta_x = range_x.Size() / (kNumSamples - 1);

  // Sample the evaluator, storing the results.
  SampledSpline samples;
  for (int i = 0; i < kNumSamples; ++i) {
    const CubicCurve& c = evaluator.Cubic(0);
    const float x = evaluator.CubicX(0);

    // Verify that the spline and the evaluator agree on the values.
    EXPECT_THAT(c.Evaluate(x), FloatNear(evaluator.Y(0), y_precision));
    EXPECT_THAT(c.Derivative(x),
                FloatNear(evaluator.Derivative(0), kDerivativePrecision));

    const vec2 point(evaluator.X(0), evaluator.Y(0));
    const vec3 derivatives(evaluator.Derivative(0), c.SecondDerivative(x),
                           c.ThirdDerivative(x));
    samples.points.push_back(point);
    samples.derivatives.push_back(derivatives);

    evaluator.AdvanceFrame(delta_x);
  }

  // Double-check start and end y values and derivitives, taking y-scale and
  // y-offset into account.
  const CubicCurve c(init);
  EXPECT_THAT(c.Evaluate(0.0f) * y_scale + y_offset,
              FloatNear(samples.points.front().y, y_precision));
  EXPECT_THAT(c.Evaluate(init.width_x) * y_scale + y_offset,
              FloatNear(samples.points.back().y, y_precision));

  EXPECT_THAT(c.Derivative(0.0f) * y_scale,
              FloatNear(samples.derivatives.front().x, derivative_precision));
  EXPECT_THAT(c.Derivative(init.width_x) * y_scale,
              FloatNear(samples.derivatives.back().x, derivative_precision));
  return samples;
}

TEST(BulkSplineEvaluatorTests, SampleSimpleSplines) {
  for (int i = 0; i < kNumSimpleSplines; ++i) {
    // SampleSpline performs several internal consistency checks, so let's run
    // those for the simple cases.
    SampleSpline(kSimpleSplines[i]);
  }
}

TEST(BulkSplineEvaluatorTests, DoNotOvershoot) {
  for (int i = 0; i < kNumSimpleSplines; ++i) {
    const CubicInit& init = kSimpleSplines[i];
    const SampledSpline samples = SampleSpline(init);

    // Ensure the splines don't overshoot their mark.
    const Interval x_range(-kXGranularityScale,
                           init.width_x * (1.0f + kXGranularityScale));
    const Interval y_range = CubicInitYInterval(init, 0.001f);
    for (const vec2& point : samples.points) {
      EXPECT_TRUE(x_range.Contains(point.x));
      EXPECT_TRUE(y_range.Contains(point.y));
    }
  }
}

// Ensure that the curves are mirrored in y when node y's are mirrored.
TEST(BulkSplineEvaluatorTests, MirrorY) {
  for (int i = 0; i < kNumSimpleSplines; ++i) {
    const CubicInit& init = kSimpleSplines[i];
    const CubicInit mirrored_init =
        CubicInit(-init.start_y, -init.start_derivative, -init.end_y,
                  -init.end_derivative, init.width_x);

    const float y_precision =
        fabs(init.start_y - init.end_y) * kFixedPointEpsilon;

    const SampledSpline lhs = SampleSpline(init);
    const SampledSpline rhs = SampleSpline(mirrored_init);

    ASSERT_EQ(lhs.points.size(), rhs.points.size());
    for (size_t j = 0; j < lhs.points.size(); ++j) {
      EXPECT_THAT(lhs.points[j].x, Eq(rhs.points[j].x));
      EXPECT_THAT(lhs.points[j].y, FloatNear(-rhs.points[j].y, y_precision));
      EXPECT_THAT(lhs.derivatives[j].x,
                  FloatNear(-rhs.derivatives[j].x, kDerivativePrecision));
      EXPECT_THAT(lhs.derivatives[j].y,
                  FloatNear(-rhs.derivatives[j].y, kSecondDerivativePrecision));
      EXPECT_THAT(lhs.derivatives[j].z,
                  FloatNear(-rhs.derivatives[j].z, kThirdDerivativePrecision));
    }
  }
}

// Ensure that the curves are scaled in x when node's x is scaled.
TEST(BulkSplineEvaluatorTests, ScaleX) {
  static const float kScale = 100.0f;
  for (int i = 0; i < kNumSimpleSplines; ++i) {
    const CubicInit& init = kSimpleSplines[i];
    const CubicInit scaled_init =
        CubicInit(init.start_y, init.start_derivative / kScale, init.end_y,
                  init.end_derivative / kScale, init.width_x * kScale);

    const float x_precision = init.width_x * kFixedPointEpsilon;
    const float y_precision =
        fabs(init.start_y - init.end_y) * kFixedPointEpsilon;

    const SampledSpline lhs = SampleSpline(init);
    const SampledSpline rhs = SampleSpline(scaled_init);

    ASSERT_EQ(lhs.points.size(), rhs.points.size());
    for (size_t j = 0; j < lhs.points.size(); ++j) {
      EXPECT_THAT(lhs.points[j].x,
                  FloatNear(rhs.points[j].x / kScale, x_precision));
      EXPECT_THAT(lhs.points[j].y, FloatNear(rhs.points[j].y, y_precision));

      EXPECT_THAT(lhs.derivatives[j].x, FloatNear(rhs.derivatives[j].x * kScale,
                                                  kDerivativePrecision));
      EXPECT_THAT(lhs.derivatives[j].y,
                  FloatNear(rhs.derivatives[j].y * kScale * kScale,
                            kSecondDerivativePrecision));
      EXPECT_THAT(lhs.derivatives[j].z,
                  FloatNear(rhs.derivatives[j].z * kScale * kScale * kScale,
                            kThirdDerivativePrecision));
    }
  }
}

TEST(BulkSplineEvaluatorTests, YScaleAndOffset) {
  static const float kOffsets[] = {0.0f, 2.0f, 0.111f, 10.0f, -1.5f, -1.0f};
  static const float kScales[] = {1.0f, 2.0f, 0.1f, 1.1f, 0.0f, -1.0f, -1.3f};

  for (size_t k = 0; k < ABSL_ARRAYSIZE(kOffsets); ++k) {
    for (size_t j = 0; j < ABSL_ARRAYSIZE(kScales); ++j) {
      for (int i = 0; i < kNumSimpleSplines; ++i) {
        // SampleSpline does some basic, internal consistency checks. We'll
        // just rely on those for this test.
        const CubicInit& init = kSimpleSplines[i];
        SampleSpline(init, false, kScales[j], kOffsets[k]);
      }
    }
  }
}

}  // namespace
}  // namespace redux
