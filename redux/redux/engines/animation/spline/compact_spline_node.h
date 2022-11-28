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

#ifndef REDUX_ENGINES_ANIMATION_SPLINE_COMPACT_SPLINE_NODE_H_
#define REDUX_ENGINES_ANIMATION_SPLINE_COMPACT_SPLINE_NODE_H_

#include <cstdint>

#include "redux/modules/math/bounds.h"
#include "redux/modules/math/interpolation.h"
#include "redux/modules/math/math.h"

namespace redux {

// X-axis is quantized into units of `x_granularity`. X values are represented
// by multiples of `x_granularity`. One unit of CompactSplineXGrain represents
// one multiple of `x_granularity`.
typedef uint16_t CompactSplineXGrain;

// Y values within `y_range` can be represented. We quantize the `y_range`
// into equally-sized rungs, and round to the closest rung.
typedef uint16_t CompactSplineYRung;

// Angles strictly between -90 and +90 can be represented. We record the angle
// instead of the slope for more uniform distribution.
typedef int16_t CompactSplineAngle;

namespace detail {

// A spline is composed of a series of spline nodes (x, y, derivative) that are
// interpolated to form a smooth curve.
//
// This class represents a single spline node in 6-bytes. It quantizes the
// valid ranges of x, y, and slope into three 16-bit integers = 6 bytes.
//
// The x and y values are quantized to the valid range. The valid range is
// stored externally and passed in to each call. Please see comments on
// CompactSplineXGrain and CompactSplineYRung for more detail.
//
// The derivative is stored as the angle from the x-axis. This is so that we
// can equally represent derivatives <= 1 (that is, <= 45 degrees) and
// derivatives >= 1 (that is, >= 45 degrees) with a quantized number.
//
class CompactSplineNode {
 public:
  // Don't initialize the data to save cycles.
  CompactSplineNode() {}

  // Construct with values that have already been converted to quantized values.
  // This constructor is useful when deserializing pre-converted data.
  CompactSplineNode(const CompactSplineXGrain x, const CompactSplineYRung y,
                    const CompactSplineAngle angle)
      : x_(x), y_(y), angle_(angle) {}

  // Construct with real-world values. Must pass in the valid x and y ranges.
  CompactSplineNode(const float x, const float y, const float derivative,
                    const float x_granularity, const Interval& y_range) {
    SetX(x, x_granularity);
    SetY(y, y_range);
    SetDerivative(derivative);
  }

  // Set with real-world values. The valid range of x and y must be passed in.
  // These values are passed in so that we don't have to store multiple copies
  // of them. Memory compactness is the purpose of this class.
  void SetX(const float x, const float x_granularity) {
    x_ = CompactX(x, x_granularity);
  }
  void SetY(const float y, const Interval& y_range) {
    y_ = CompactY(y, y_range);
  }
  void SetDerivative(const float derivative) {
    angle_ = CompactDerivative(derivative);
  }

  // Get real world values. The valid range of x and y must be passed in.
  // The valid range must be the same as when x and y values were set.
  float X(const float x_granularity) const {
    return static_cast<float>(x_) * x_granularity;
  }
  float Y(const Interval& y_range) const {
    return Lerp(y_range.min, y_range.max, YPercent());
  }
  float Derivative() const { return tan(Angle()); }

  // Get the quantized values. Useful for serializing a series of nodes.
  CompactSplineXGrain x() const { return x_; }
  CompactSplineYRung y() const { return y_; }
  CompactSplineAngle angle() const { return angle_; }

  // Equivalence can be tested reasonably because internal types are integral,
  // not floating point.
  bool operator==(const CompactSplineNode& rhs) const {
    return x_ == rhs.x_ && y_ == rhs.y_ && angle_ == rhs.angle_;
  }
  bool operator!=(const CompactSplineNode& rhs) const {
    return !operator==(rhs);
  }

  // Convert from real-world to quantized values.
  // Please see type definitions for documentation on the quantized format.
  static int QuantizeX(const float x, const float x_granularity) {
    return static_cast<int>(x / x_granularity + 0.5f);
  }

  static CompactSplineXGrain CompactX(const float x,
                                      const float x_granularity) {
    const int x_quantized = QuantizeX(x, x_granularity);
    assert(0 <= x_quantized && x_quantized <= kMaxX);
    return static_cast<CompactSplineXGrain>(x_quantized);
  }

  static CompactSplineYRung CompactY(const float y, const Interval& y_range) {
    assert(y_range.Contains(y));

    // Prevent a divide-by-zero if the range has zero length.
    const float length = y_range.Size();
    if (length == 0.0f) {
      return 0;
    }

    const float percent = (y - y_range.min) / length;
    const float clamped_percent = Clamp(percent, 0.f, 1.f);
    const CompactSplineYRung compact_y =
        static_cast<CompactSplineYRung>(kMaxY * clamped_percent);
    return compact_y;
  }

  static CompactSplineAngle CompactDerivative(const float derivative) {
    const float angle_radians = atan(derivative);
    const CompactSplineAngle angle =
        static_cast<CompactSplineAngle>(angle_radians / kAngleScale);
    return angle;
  }

  static CompactSplineXGrain MaxX() { return kMaxX; }

 private:
  static const CompactSplineXGrain kMaxX;
  static const CompactSplineYRung kMaxY;
  static const CompactSplineAngle kMinAngle;
  static const float kYScale;
  static const float kAngleScale;

  float YPercent() const { return static_cast<float>(y_) * kYScale; }
  float Angle() const { return static_cast<float>(angle_) * kAngleScale; }

  // Position along x-axis. Multiplied by x-granularity to get actual domain.
  // 0 ==> start. kMaxX ==> end, we should never reach the end. If we do,
  // the x_granularity should be increased.
  CompactSplineXGrain x_;

  // Position within y_range. 0 ==> y_range.start. kMaxY ==> y_range.end.
  CompactSplineYRung y_;

  // Angle from x-axis. tan(angle) = rise / run = derivative.
  CompactSplineAngle angle_;
};

}  // namespace detail
}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_SPLINE_COMPACT_SPLINE_NODE_H_
