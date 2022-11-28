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

#ifndef REDUX_MODULES_MATH_INTERPOLATION_H_
#define REDUX_MODULES_MATH_INTERPOLATION_H_

#include <cmath>

#include "redux/modules/math/math.h"

// These functions are used to find points between two end points using common
// interpolation functions. The `ratio` parameter defines how far between
// the two points you want to calculate. (e.g. A point 10% of the way between A
// and B would specify a `ratio` of 0.1). Ratios outside of the range [0, 1]
// will be extrapolated, not interpolated.

namespace redux {

template <typename ValueType, typename RatioType>
ValueType Lerp(const ValueType& v1, const ValueType& v2,
               const RatioType& ratio) {
  const RatioType one_minus_ratio = static_cast<RatioType>(1.0) - ratio;
  return v1 * one_minus_ratio + v2 * ratio;
}

namespace detail {

// Performs an interpolation between two values at the given ratio using the
// provided functions. Uses in_fn if the interpolation is in the first half,
// otherwise uses out_fn.
template <typename InFn, typename OutFn, class ValueType, class RatioType>
ValueType InOut(const ValueType& start, const ValueType& target,
                const RatioType& ratio, InFn in_fn, OutFn out_fn) {
  const ValueType middle_value = (start + target) / RatioType(2.0);
  if (ratio <= RatioType(0.5)) {
    return in_fn(start, middle_value, RatioType(2.0) * ratio);
  } else {
    return out_fn(middle_value, target,
                  (RatioType(2.0) * ratio) - RatioType(1.0));
  }
}

// An exponential lerp that uses the given power exponent.
template <class ValueType, class RatioType>
ValueType EaseInPow(const ValueType& start, const ValueType& end,
                    const RatioType& ratio, const RatioType& power) {
  return Lerp(start, end, std::pow(ratio, power));
}

// An inverse exponential lerp that uses the given power exponent.
template <class ValueType, class RatioType>
ValueType EaseOutPow(const ValueType& start, const ValueType& end,
                     const RatioType& ratio, const RatioType& power) {
  const RatioType power_term = std::pow(ratio - RatioType(1.0), power);

  if (static_cast<int>(power) & 1) {
    return Lerp(start, end, power_term + RatioType(1.0));
  } else {
    return Lerp(start, end, RatioType(1.0) - power_term);
  }
}

}  // namespace detail

// Performs a quadratic interpolation for the lower half of the curve and a
// linear interpolation for the upper half between two points.
template <class ValueType, class RatioType>
ValueType QuadraticEaseIn(const ValueType& start, const ValueType& end,
                          const RatioType& ratio) {
  return detail::EaseInPow(start, end, ratio, RatioType(2.0));
}

// Performs a linear interpolation for the lower half of the curve and a
// quadratic interpolation for the upper half between two points.
template <class ValueType, class RatioType>
ValueType QuadraticEaseOut(const ValueType& start, const ValueType& end,
                           const RatioType& ratio) {
  return detail::EaseOutPow(start, end, ratio, RatioType(2.0));
}

// Performs a quadratic interpolation of curve between two points.
template <class ValueType, class RatioType>
ValueType QuadraticEaseInOut(const ValueType& start, const ValueType& target,
                             const RatioType& ratio) {
  return detail::InOut(start, target, ratio,
                       QuadraticEaseIn<ValueType, RatioType>,
                       QuadraticEaseOut<ValueType, RatioType>);
}

// Performs a cubic interpolation for the lower half of the curve and a
// linear interpolation for the upper half between two points.
template <class ValueType, class RatioType>
ValueType CubicEaseIn(const ValueType& start, const ValueType& end,
                      const RatioType& ratio) {
  return detail::EaseInPow(start, end, ratio, RatioType(3.0));
}

// Performs a linear interpolation for the lower half of the curve and a cubic
// interpolation for the upper half between two points.
template <class ValueType, class RatioType>
ValueType CubicEaseOut(const ValueType& start, const ValueType& end,
                       const RatioType& ratio) {
  return detail::EaseOutPow(start, end, ratio, RatioType(3.0));
}

// Performs a cubic interpolation of curve between two points.
template <class ValueType, class RatioType>
ValueType CubicEaseInOut(const ValueType& start, const ValueType& target,
                         const RatioType& ratio) {
  return detail::InOut(start, target, ratio,
                       CubicEaseIn<ValueType, RatioType>,
                       CubicEaseOut<ValueType, RatioType>);
}

// Performs a quartic interpolation for the lower half of the curve and a
// linear interpolation for the upper half between two points.
template <class ValueType, class RatioType>
ValueType QuarticEaseIn(const ValueType& start, const ValueType& end,
                        const RatioType& ratio) {
  return detail::EaseInPow(start, end, ratio, RatioType(4.0));
}

// Performs a linear interpolation for the lower half of the curve and a quartic
// interpolation for the upper half between two points.
template <class ValueType, class RatioType>
ValueType QuarticEaseOut(const ValueType& start, const ValueType& end,
                         const RatioType& ratio) {
  return detail::EaseOutPow(start, end, ratio, RatioType(4.0));
}

// Performs a quartic interpolation of curve between two points.
template <class ValueType, class RatioType>
ValueType QuarticEaseInOut(const ValueType& start, const ValueType& target,
                           const RatioType& ratio) {
  return detail::InOut(start, target, ratio,
                       QuarticEaseIn<ValueType, RatioType>,
                       QuarticEaseOut<ValueType, RatioType>);
}

// Performs a quintic interpolation for the lower half of the curve and a
// linear interpolation for the upper half between two points.
template <class ValueType, class RatioType>
ValueType QuinticEaseIn(const ValueType& start, const ValueType& end,
                        const RatioType& ratio) {
  return detail::EaseInPow(start, end, ratio, RatioType(5.0));
}

// Performs a linear interpolation for the lower half of the curve and a quintic
// interpolation for the upper half between two points.
template <class ValueType, class RatioType>
ValueType QuinticEaseOut(const ValueType& start, const ValueType& end,
                         const RatioType& ratio) {
  return detail::EaseOutPow(start, end, ratio, RatioType(5.0));
}

// Performs a quintic interpolation of curve between two points.
template <class ValueType, class RatioType>
ValueType QuinticEaseInOut(const ValueType& start, const ValueType& target,
                           const RatioType& ratio) {
  return detail::InOut(start, target, ratio,
                       QuinticEaseIn<ValueType, RatioType>,
                       QuinticEaseOut<ValueType, RatioType>);
}

// Sinusodial easing functions.
template <class ValueType, class RatioType>
ValueType SineEaseIn(const ValueType& start, const ValueType& end,
                     const RatioType& ratio) {
  constexpr RatioType kOne(1.0);
  const RatioType t = std::sin(kHalfPi * ratio - kHalfPi) + kOne;
  return (end - start) * t + start;
}

template <class ValueType, class RatioType>
ValueType SineEaseOut(const ValueType& start, const ValueType& end,
                      const RatioType& ratio) {
  const RatioType t = std::sin(kHalfPi * ratio);
  return (end - start) * t + start;
}

template <class ValueType, class RatioType>
ValueType SineEaseInOut(const ValueType& start, const ValueType& end,
                        const RatioType& ratio) {
  constexpr RatioType kHalf(0.5);
  constexpr RatioType kOne(1.0);
  const RatioType t = kHalf * (kOne - std::cos(kPi * ratio));
  return (end - start) * t + start;
}

// This implements the Material Design spec for the "FastOutSlowInInterpolator".
// The interpolation is an approximation of a bezier curve with 4 control points
// placed at (0,0) P1 (0.4, 0) P2 (0.2, 1.0) P3 (1.0, 1.0).
float FastOutSlowIn(float value);

}  // namespace redux

#endif  // REDUX_MODULES_MATH_INTERPOLATION_H_
