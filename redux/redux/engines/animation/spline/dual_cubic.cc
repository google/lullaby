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

#include "redux/engines/animation/spline/dual_cubic.h"

#include <cmath>

#include "redux/engines/animation/spline/quadratic_curve.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/interpolation.h"

namespace redux {

static const float kMaxSteepness = 4.0f;
static const float kMinMidPercent = 0.1f;
static const float kMaxMidPercent = 1.0f - kMinMidPercent;
static const float kMinYDiff = 0.1f;  // Prevent division by zero.

// One node of a spline that specifies both first and second derivatives.
// Only used internally.
struct SplineControlNode {
  SplineControlNode(float x, float y, float derivative,
                    float second_derivative = 0.0f)
      : x(x),
        y(y),
        derivative(derivative),
        second_derivative(second_derivative) {}

  float x;
  float y;
  float derivative;
  float second_derivative;
};

static const Interval kZeroToOne(0.0f, 1.0f);

static QuadraticCurve CalculateValidMidRangeSplineForStart(
    const SplineControlNode& start, const SplineControlNode& end) {
  const float yd = end.y - start.y;
  const float sd = end.derivative - start.derivative;
  const float wd = end.second_derivative - start.second_derivative;
  const float w0 = start.second_derivative;
  const float w1 = end.second_derivative;
  const float s0 = start.derivative;
  const float s1 = end.derivative;

  // r_g(k) = wd * k^2  +  (4*sd - w0 - 2w1)k  +  6yd - 2s0 - 4s1 + w1
  const float c2 = wd;
  const float c1 = 4 * sd - w0 - 2 * w1;
  const float c0 = 6 * yd - 2 * s0 - 4 * s1 + w1;
  return QuadraticCurve(c2, c1, c0);
}

static QuadraticCurve CalculateValidMidRangeSplineForEnd(
    const SplineControlNode& start, const SplineControlNode& end) {
  const float yd = end.y - start.y;
  const float sd = end.derivative - start.derivative;
  const float wd = end.second_derivative - start.second_derivative;
  const float w1 = end.second_derivative;
  const float s1 = end.derivative;

  // r_g(k) = -wd * k^2  +  (-4*sd + 3w1)k  -  6yd + 6s1 - 2w1
  const float c2 = -wd;
  const float c1 = -4 * sd + 3 * w1;
  const float c0 = -6 * yd + 6 * s1 - 2 * w1;
  return QuadraticCurve(c2, c1, c0);
}

template <size_t kMaxLen>
struct IntervalArray {
  Interval arr[kMaxLen];
  size_t len = 0;
};

// Return the index of the longest range in `ranges`.
static size_t IndexOfLongest(const Interval* ranges, size_t len) {
  float longest_length = -1.0f;
  size_t longest_index = 0;
  for (size_t i = 0; i < len; ++i) {
    const float length = ranges[i].Size();
    if (length > longest_length) {
      longest_length = length;
      longest_index = i;
    }
  }
  return longest_index;
}

// Return the index of the shortest range in `ranges`.
static size_t IndexOfShortest(const Interval* ranges, size_t len) {
  float shortest_length = std::numeric_limits<float>::infinity();
  size_t shortest_index = 0;
  for (size_t i = 0; i < len; ++i) {
    const float length = ranges[i].Size();
    if (length < shortest_length) {
      shortest_length = length;
      shortest_index = i;
    }
  }
  return shortest_index;
}

// Intersect every element of 'a' with every element of 'b'. Append
// intersections to 'intersections'. Note that 'intersections' is not reset
// at the start of the call.
template <size_t N>
static void IntersectIntervals(const IntervalArray<N>& a,
                               const IntervalArray<N>& b,
                               IntervalArray<N * N>* intersections,
                               IntervalArray<N * N>* gaps) {
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      const Interval intersection(Max(a.arr[i].min, b.arr[i].min),
                                  Min(a.arr[i].max, b.arr[i].max));
      if (intersection.Size() > 0.f) {
        intersections->arr[intersections->len] = intersection;
        ++intersections->len;
      } else {
        // Invert invalid intersections to get the gap between the ranges.
        gaps->arr[gaps->len] = Interval(intersection.max, intersection.min);
        ++gaps->len;
      }
    }
  }
}

static Interval CalculateValidMidRange(const SplineControlNode& start,
                                       const SplineControlNode& end,
                                       bool* is_valid = nullptr) {
  // The sign of these quadratics determine where the mid-node is valid.
  // One quadratic for the start cubic, and one for the end cubic.
  const QuadraticCurve start_spline =
      CalculateValidMidRangeSplineForStart(start, end);
  const QuadraticCurve end_spline =
      CalculateValidMidRangeSplineForEnd(start, end);

  // The mid node is valid when the quadratic sign matches the second
  // derivative's sign.
  IntervalArray<2> start_ranges;
  IntervalArray<2> end_ranges;
  start_ranges.len = start_spline.IntervalsMatchingSign(
      kZeroToOne, start.second_derivative, start_ranges.arr);
  end_ranges.len = end_spline.IntervalsMatchingSign(
      kZeroToOne, end.second_derivative, end_ranges.arr);

  // Find the valid overlapping ranges, or the gaps inbetween the ranges.
  IntervalArray<4> intersections;
  IntervalArray<4> gaps;
  IntersectIntervals(start_ranges, end_ranges, &intersections, &gaps);

  // The mid-node is valid only if there is an overlapping range.
  if (is_valid != nullptr) {
    *is_valid = intersections.len > 0;
  }

  // Take the largest overlapping range. If none, find the smallest gap
  // between the ranges.
  return intersections.len > 0
             ? intersections
                   .arr[IndexOfLongest(intersections.arr, intersections.len)]
         : gaps.len > 0 ? gaps.arr[IndexOfShortest(gaps.arr, gaps.len)]
                        : kZeroToOne;
}

static float CalculateMidPercent(const SplineControlNode& start,
                                 const SplineControlNode& end) {
  // The mid value we called 'k' in the dual cubic documentation.
  // It's between 0~1 and determines where the start and end cubics are joined
  // along the x-axis.
  const Interval valid_range = CalculateValidMidRange(start, end);

  // Return the part of the range closest to the half-way mark. This seems to
  // generate the smoothest looking curves.
  const float mid_unclamped = Clamp(0.5f, valid_range.min, valid_range.max);

  // Clamp away from 0 and 1. The math requires the mid node to be strictly
  // between 0 and 1. If we get to close to 0 or 1, some divisions are going to
  // explode and we'll lose numerical precision.
  const float mid = Clamp(mid_unclamped, kMinMidPercent, kMaxMidPercent);
  return mid;
}

static SplineControlNode CalculateMidNode(const SplineControlNode& start,
                                          const SplineControlNode& end,
                                          const float k) {
  // The mid node is at x = Lerp(start.x, end.x, k)
  // It has y value of 'y' and slope of 's', defined as:
  //
  // s = 3(y1-y0) - 2Lerp(s1,s0,k) - 1/2(k^2*w0 - (1-k)^2*w1)
  // y = Lerp(y0,y1,k) + k(1-k)(-2/3(s1-s0) + 1/6 Lerp(w1,w0,k))
  //
  // where (x0, y0, s0, w0) is the start control node's x, y, derivative, and
  // second derivative, and (x1, y1, s1, w1) similarly represents the end
  // control node.
  //
  // See the document on "Dual Cubics" for a derivation of this solution.
  const float y_diff = end.y - start.y;
  const float s_diff = end.derivative - start.derivative;
  const float derivative_k = Lerp(end.derivative, start.derivative, k);
  const float y_k = Lerp(start.y, end.y, k);
  const float second_k =
      Lerp(end.second_derivative, start.second_derivative, k);
  const float j = 1.0f - k;
  const float second_k_squared =
      k * k * start.second_derivative - j * j * end.second_derivative;

  const float s = 3.0f * y_diff - 2.0f * derivative_k - 0.5f * second_k_squared;
  const float y =
      y_k + k * j * (-2.0f / 3.0f * s_diff + 1.0f / 6.0f * second_k);
  const float x = Lerp(start.x, end.x, k);

  return SplineControlNode(x, y, s, 0.0f);
}

// See the Dual Cubics document for a derivation of this equation.
static float ExtremeSecondDerivativeForStart(const SplineControlNode& start,
                                             const SplineControlNode& end,
                                             const float mid_percent) {
  const float y_diff = end.y - start.y;
  const float s_diff = end.derivative - start.derivative;
  const float k = mid_percent;
  const float extreme_second =
      s_diff +
      (1.0f / k) * (3.0f * y_diff - 2.0f * start.derivative - end.derivative);
  return extreme_second;
}

// See the Dual Cubics document for a derivation of this equation.
static float ExtremeSecondDerivativeForEnd(const SplineControlNode& start,
                                           const SplineControlNode& end,
                                           const float mid_percent) {
  const float y_diff = end.y - start.y;
  const float s_diff = end.derivative - start.derivative;
  const float k = mid_percent;
  const float extreme_second =
      (1.0f / (k - 1.0f)) *
      (s_diff * k + 3.0f * y_diff - 3.0f * end.derivative);
  return extreme_second;
}

// Android does not support log2 in its math.h.
static inline float Log2(const float x) {
#if defined(__ANDROID__) || defined(_WIN32)
  static const float kOneOverLog2 = 3.32192809489f;
  return log(x) * kOneOverLog2;  // log2(x) = log(x) / log(2)
#else
  return std::log2(x);
#endif
}

// Steepness is a notion of how much the derivative has to change from the
// start (x=0) to the end (x=1). For derivatives under 1, we don't really care,
// since cubics can change fast enough to cover those differences. Only extreme
// differences in derivatives cause trouble.
static float CalculateSteepness(const float derivative) {
  const float abs_derivative = fabs(derivative);
  return abs_derivative <= 1.0f ? 0.0f : Log2(abs_derivative);
}

static float ApproximateMidPercent(const SplineControlNode& start,
                                   const SplineControlNode& end,
                                   float* start_percent, float* end_percent) {
  // The greater the difference in steepness, the more skewed the mid percent.
  const float abs_y_diff = fabs(end.y - start.y);
  const float y_diff_recip = 1.0f / std::max(abs_y_diff, kMinYDiff);
  const float start_steepness =
      CalculateSteepness(start.derivative * y_diff_recip);
  const float end_steepness = CalculateSteepness(end.derivative * y_diff_recip);
  const float diff_steepness = fabs(start_steepness - end_steepness);
  const float percent_extreme = std::min(diff_steepness / kMaxSteepness, 1.0f);

  // We skew the mid percent towards the steeper side.
  // If equally steep, the mid percent is right in the middle: 0.5.
  const bool start_is_steeper = start_steepness >= end_steepness;
  const float extreme_percent =
      start_is_steeper ? kMinMidPercent : kMaxMidPercent;
  const float mid_percent = Lerp(0.5f, extreme_percent, percent_extreme);

  // Later, when we calculate the second derivatives, we want to skew to the
  // extreme second derivatives in the same manner (steeper side gets skewed to
  // more).
  *start_percent = start_is_steeper ? percent_extreme : 1.0f - percent_extreme;
  *end_percent = start_is_steeper ? 1.0f - percent_extreme : percent_extreme;
  return mid_percent;
}

void CalculateDualCubicMidNode(const CubicInit& init, float* x, float* y,
                               float* derivative) {
  // The initial y and derivative values of our node are given by the
  // 'init' control nodes. We scale to x from 0~1, because all of our math
  // assumes x on this domain.
  SplineControlNode start(0.0f, init.start_y,
                          init.start_derivative * init.width_x);
  SplineControlNode end(1.0f, init.end_y, init.end_derivative * init.width_x);

  // Use a heuristic to guess a reasonably close place to split the cubic into
  // two cubics.
  float start_percent, end_percent;
  const float approx_mid_percent =
      ApproximateMidPercent(start, end, &start_percent, &end_percent);

  // Given the start and end conditions and the place to split the cubic,
  // find the extreme second derivatives for start and end curves. See the
  // Dual Cubic document for a derivation of the math here.
  const float start_extreme_second =
      ExtremeSecondDerivativeForStart(start, end, approx_mid_percent);
  const float end_extreme_second =
      ExtremeSecondDerivativeForEnd(start, end, approx_mid_percent);

  // Don't just use the extreme values since this will create a curve that's
  // flat in the middle. Skew the second derivative to favor the steeper side.
  start.second_derivative = Lerp(0.0f, start_extreme_second, start_percent);
  end.second_derivative = Lerp(0.0f, end_extreme_second, end_percent);

  // Now that we have the full characterization of the start end end nodes
  // (including second derivatives), calculate the actual ideal mid percent
  // (i.e. the place to split the curve).
  const float mid_percent = CalculateMidPercent(start, end);

  // With a full characterization of start and end nodes, and a place to
  // split the curve, we can uniquely calculate the mid node.
  const SplineControlNode mid = CalculateMidNode(start, end, mid_percent);

  // Re-scale the output values to the proper x-width.
  *x = mid.x * init.width_x;
  *y = mid.y;
  *derivative = mid.derivative / init.width_x;
}

}  // namespace redux
