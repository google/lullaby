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

#ifndef REDUX_MODULES_MATH_MATH_H_
#define REDUX_MODULES_MATH_MATH_H_

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "redux/modules/base/logging.h"
#include "redux/modules/math/constants.h"

namespace redux {

// Determines whether the given value is almost zero within a threshold.
template <typename T>
inline bool IsNearlyZero(T value, T epsilon = kDefaultEpsilon) {
  CHECK_GE(epsilon, 0);
  return std::abs(value) <= epsilon;
}

// Determines whether the two values are equal within a threshold.
template <typename T>
inline bool AreNearlyEqual(T a, T b, T epsilon = kDefaultEpsilon) {
  return IsNearlyZero(a - b, epsilon);
}

// Returns the value such that it is within the given bounds. Assumes |lower| is
// less than or equal to |upper|.
template <typename T>
T Clamp(const T& value, const T& lower, const T& upper) {
  return std::max<T>(lower, std::min<T>(value, upper));
}

// Returns the minimum of two values.
template <typename T>
T Min(const T& v1, const T& v2) {
  if constexpr (std::is_arithmetic_v<T>) {
    return std::min<T>(v1, v2);
  }
}

// Returns the maximum of two values.
template <typename T>
T Max(const T& v1, const T& v2) {
  if constexpr (std::is_arithmetic_v<T>) {
    return std::max<T>(v1, v2);
  }
}

// Returns the "distance" of the value from zero. This mostly exists to support
// generic programming with vector types.
template <typename T>
T Length(const T& value) {
  static_assert(std::is_arithmetic_v<T>);
  return value;
}

// Converts the input degrees to radians.
template <typename T>
inline T DegreesToRadians(T degree) {
  return degree * kDegreesToRadians;
}

// Converts the input radians to degrees.
template <typename T>
inline T RadiansToDegrees(T radians) {
  return radians * kRadiansToDegrees;
}

// Angle modulus in degrees, adjusted such that the output is in the range
// (-180, 180].
template <typename T>
T ModDegrees(T degrees) {
  const T m = std::fmod(degrees + T(180), T(360));
  return m < T(0) ? m + T(180) : m - T(180);
}

// Angle modulus in radians, adjusted such that the output is in the range
// (-pi, pi].
template <typename T>
T ModRadians(T radians) {
  const T m = std::fmod(radians + kPi, T(2) * kPi);
  return m < T(0) ? m + kPi : m - kPi;
}

// Tests whether |n| is a positive power of 2.
template <typename T>
inline bool IsPowerOf2(T n) {
  static_assert(std::is_integral_v<T>);
  return (n & (n - 1)) == 0 && n != 0;
}

// Aligns |n| to the next multiple of |align| (or |n| iff |n| == |align|).
// Expects |align| to be a power of 2.
template <typename T>
inline T AlignToPowerOf2(T n, T align) {
  DCHECK(IsPowerOf2(align));
  return ((n + (align - 1)) & ~(align - 1));
}

namespace detail {

template <typename T>
struct ScalarType {
  using Type = typename T::Scalar;
};
template <>
struct ScalarType<float> {
  using Type = float;
};
template <>
struct ScalarType<int> {
  using Type = int;
};

}  // namespace detail
}  // namespace redux

#endif  // REDUX_MODULES_MATH_MATH_H_
