/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_UTIL_INTERPOLATION_H_
#define LULLABY_UTIL_INTERPOLATION_H_

#include <cmath>

#include "lullaby/util/math.h"

namespace lull {

template <typename InFn, typename OutFn, class ValueType, class PercentType>
ValueType InOut(const ValueType& start, const ValueType& target,
                const PercentType& percent, InFn in_fn, OutFn out_fn) {
  const ValueType middle_value = (start + target) / PercentType(2.0);
  if (percent <= PercentType(0.5)) {
    return in_fn(start, middle_value, PercentType(2.0) * percent);
  } else {
    return out_fn(middle_value, target,
                  (PercentType(2.0) * percent) - PercentType(1.0));
  }
}

template <class ValueType, class PercentType>
ValueType EaseInPow(const ValueType& start, const ValueType& end,
                    const PercentType& percent, const PercentType& power) {
  return mathfu::Lerp(start, end, std::pow(percent, power));
}

template <class ValueType, class PercentType>
ValueType EaseOutPow(const ValueType& start, const ValueType& end,
                     const PercentType& percent, const PercentType& power) {
  const PercentType power_term = std::pow(percent - PercentType(1.0), power);

  if (static_cast<int>(power) & 1) {
    return mathfu::Lerp(start, end, power_term + PercentType(1.0));
  } else {
    return mathfu::Lerp(start, end, PercentType(1.0) - power_term);
  }
}

template <class ValueType, class PercentType>
ValueType QuadraticEaseIn(const ValueType& start, const ValueType& end,
                          const PercentType& percent) {
  return EaseInPow(start, end, percent, PercentType(2.0));
}

template <class ValueType, class PercentType>
ValueType QuadraticEaseOut(const ValueType& start, const ValueType& end,
                           const PercentType& percent) {
  return EaseOutPow(start, end, percent, PercentType(2.0));
}

template <class ValueType, class PercentType>
ValueType QuadraticEaseInOut(const ValueType& start, const ValueType& target,
                             const PercentType& percent) {
  return InOut(start, target, percent, QuadraticEaseIn<ValueType, PercentType>,
               QuadraticEaseOut<ValueType, PercentType>);
}

template <class ValueType, class PercentType>
ValueType CubicEaseIn(const ValueType& start, const ValueType& end,
                      const PercentType& percent) {
  return EaseInPow(start, end, percent, PercentType(3.0));
}

template <class ValueType, class PercentType>
ValueType CubicEaseOut(const ValueType& start, const ValueType& end,
                       const PercentType& percent) {
  return EaseOutPow(start, end, percent, PercentType(3.0));
}

template <class ValueType, class PercentType>
ValueType CubicEaseInOut(const ValueType& start, const ValueType& target,
                         const PercentType& percent) {
  return InOut(start, target, percent, CubicEaseIn<ValueType, PercentType>,
               CubicEaseOut<ValueType, PercentType>);
}

template <class ValueType, class PercentType>
ValueType QuarticEaseIn(const ValueType& start, const ValueType& end,
                        const PercentType& percent) {
  return EaseInPow(start, end, percent, PercentType(4.0));
}

template <class ValueType, class PercentType>
ValueType QuarticEaseOut(const ValueType& start, const ValueType& end,
                         const PercentType& percent) {
  return EaseOutPow(start, end, percent, PercentType(4.0));
}

template <class ValueType, class PercentType>
ValueType QuarticEaseInOut(const ValueType& start, const ValueType& target,
                           const PercentType& percent) {
  return InOut(start, target, percent, QuarticEaseIn<ValueType, PercentType>,
               QuarticEaseOut<ValueType, PercentType>);
}

template <class ValueType, class PercentType>
ValueType QuinticEaseIn(const ValueType& start, const ValueType& end,
                        const PercentType& percent) {
  return EaseInPow(start, end, percent, PercentType(5.0));
}

template <class ValueType, class PercentType>
ValueType QuinticEaseOut(const ValueType& start, const ValueType& end,
                         const PercentType& percent) {
  return EaseOutPow(start, end, percent, PercentType(5.0));
}

template <class ValueType, class PercentType>
ValueType QuinticEaseInOut(const ValueType& start, const ValueType& target,
                           const PercentType& percent) {
  return InOut(start, target, percent, QuinticEaseIn<ValueType, PercentType>,
               QuinticEaseOut<ValueType, PercentType>);
}

// This implements the Material Design spec for the "FastOutSlowInInterpolator".
// The interpolation is an approximation of a bezier curve with 4 control points
// placed at (0,0) P1 (0.4, 0) P2 (0.2, 1.0) P3 (1.0, 1.0).
float FastOutSlowIn(float value);

}  // namespace lull

#endif  // LULLABY_UTIL_INTERPOLATION_H_
