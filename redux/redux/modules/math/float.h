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

#ifndef REDUX_MODULES_MATH_FLOAT_H_
#define REDUX_MODULES_MATH_FLOAT_H_

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace redux {

// Useful information and functions about dealing with IEEE 32-bit
// floating-point representations.

// Internal constants to extract the exponent from a float.
// See: https://en.wikipedia.org/wiki/Single-precision_floating-point_format
inline constexpr std::uint32_t kExponentMask = 0x000000FF;
inline constexpr int kExponentShift = 23;
inline constexpr int kExponentOffset = 127;

// Floats are represented as (sign) * 2^i * (1 + mantissa/2^23).
// These exponent constants represent special values for the i.
// Note that infinity and zero have special representations, but
// ones that make sense (infinity is the biggest positive exponent,
// and zero is the biggest negative exponent).
inline constexpr int kInfinityExponent = 128;
inline constexpr int kMaxFloatExponent = 127;
inline constexpr int kMaxInvertableExponent = 126;
inline constexpr int kMinInvertableExponent = -126;
inline constexpr int kMinFloatExponent = -126;
inline constexpr int kZeroExponent = -127;

// Floating point representations of the exponent consts above.
extern const float kMinInvertablePowerOf2;  // 2 ^ kMinInvertableExponent
extern const float kMaxInvertablePowerOf2;  // 2 ^ kMaxInvertableExponent

// Used internally to bit-wise process floating point values.
namespace detail {
union IntFloatUnion {
  std::uint32_t i;
  float f;
};
}  // namespace detail

// Returns floor(log2(fabs(f))), as an int.
// When f is 0, return kZeroExponent.
// When f is denormalized (i.e. has abs < min float), return kZeroExponent.
// When f is between min and max float, return i where f = 2^i * mantissa.
// When f is infinity, return kInfinityExponent.
// When f is NaN (not a number), return kInfinityExponent.
inline int ExponentAsInt(const float f) {
  detail::IntFloatUnion u;
  u.f = f;
  return ((u.i >> kExponentShift) & kExponentMask) - kExponentOffset;
}

// Returns 2^i, as a float.
// When i is kZeroExponent, return 0.
// When i is kInfinityExponent, return +infinity.
// When i is between kMinInvertableExponent and kMaxInvertableExponent,
// return 2^i.
inline float ExponentFromInt(const int i) {
  detail::IntFloatUnion u;
  u.i = ((i + kExponentOffset) & kExponentMask) << kExponentShift;
  return u.f;
}

// Returns the reciprocal of the exponent component of f.
// Note that this will always be a power of 2.
// e.g. f = 2.0, 2.1, or 3.99999 --> returns 0.5
// e.g. f = 1/4 = 0.25 --> returns 4
// Useful to bring a number near 1 (i.e. between 0.5f and 2.0f), without losing
// any precision. Note that the mantissa does not change when multiplying by a
// power of 2.
// f must have exponent be between kMinInvertablePowerOf2 and
// kMaxInvertablePowerOf2.
inline float ReciprocalExponent(const float f) {
  return ExponentFromInt(-ExponentAsInt(f));
}

// Returns the square root of the reciprocal of the exponent component of f,
// rounded down to the nearest power of 2.
// e.g. f = 4.0 --> returns 0.5
// e.g. f = 8.0 --> also returns 0.5, since sqrt(1/8) rounded down to power of
//                  2 is 1/2.
// e.g. f = 0.126 ~ 0.25 --> returns 2
inline float SqrtReciprocalExponent(const float f) {
  return ExponentFromInt(-ExponentAsInt(f) / 2);
}

// Returns the maximum power of 2 by which f can be multiplied and still have
// exponent less than 2^max_exponent.
inline float MaxPowerOf2Scale(const float f, const int max_exponent) {
  return ExponentFromInt(
      std::min(kMaxFloatExponent, max_exponent - ExponentAsInt(f)));
}

// If the absolute value of `x` is less than epsilon, returns zero. Otherwise,
// returns `x`.
// This function is useful in situations where the mathematical result depends
// on knowing if a number is zero or not.
inline float ClampNearZero(const float x, const float epsilon) {
  const bool is_near_zero = std::fabs(x) <= epsilon;
  return is_near_zero ? 0.0f : x;
}

}  // namespace redux

#endif  // REDUX_MODULES_MATH_FLOAT_H_
