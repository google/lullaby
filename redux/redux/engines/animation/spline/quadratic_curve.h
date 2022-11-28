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

#ifndef REDUX_ENGINES_ANIMATION_SPLINE_QUADRATIC_CURVE_H_
#define REDUX_ENGINES_ANIMATION_SPLINE_QUADRATIC_CURVE_H_

#include <cstdint>

#include "redux/modules/base/logging.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Represents a quadratic polynomial in the form:
//        c_[2] * x^2  +  c_[1] * x  +  c_[0]
class QuadraticCurve {
 public:
  // 2^22 = the max precision of significand.
  static constexpr float kEpsilonPrecision = static_cast<float>(1 << 22);
  static constexpr float kEpsilonScale = 1.0f / kEpsilonPrecision;
  static const int kNumCoeff = 3;

  QuadraticCurve() { memset(c_, 0, sizeof(c_)); }

  QuadraticCurve(const float c2, const float c1, const float c0) {
    c_[2] = c2;
    c_[1] = c1;
    c_[0] = c0;
  }

  QuadraticCurve(const QuadraticCurve& q, const float y_scale) {
    for (int i = 0; i < kNumCoeff; ++i) {
      c_[i] = y_scale * q.c_[i];
    }
  }

  // Shift the curve along the x-axis: x_shift to the left.
  // That is x_shift becomes the curve's x=0.
  void ShiftLeft(const float x_shift);

  // Shift the curve along the x-axis: x_shift to the right.
  void ShiftRight(const float x_shift) { ShiftLeft(-x_shift); }

  // Shift the curve along the y-axis by y_offset: y_offset up the y-axis.
  void ShiftUp(const float y_offset) { c_[0] += y_offset; }

  // Scale the curve along the y-axis by a factor of y_scale.
  void ScaleUp(const float y_scale) {
    for (int i = 0; i < kNumCoeff; ++i) {
      c_[i] *= y_scale;
    }
  }

  // Return the quadratic function's value at `x`.
  // f(x) = c2*x^2 + c1*x + c0
  float Evaluate(const float x) const {
    return (c_[2] * x + c_[1]) * x + c_[0];
  }

  // Return the quadratic function's slope at `x`.
  // f'(x) = 2*c2*x + c1
  float Derivative(const float x) const { return 2.0f * c_[2] * x + c_[1]; }

  // Return the quadratic function's constant second derivative.
  // f''(x) = 2*c2
  float SecondDerivative() const { return 2.0f * c_[2]; }

  // Return the quadratic function's constant second derivative.
  // Even though `x` is unused, we pass it in for consistency with other
  // curve classes.
  float SecondDerivative(const float /*x*/) const { return SecondDerivative(); }

  // Return the quadratic function's constant third derivative: 0.
  // Even though `x` is unused, we pass it in for consistency with other
  // curve classes.
  // f'''(x) = 0
  float ThirdDerivative(const float x) const {
    (void)x;
    return 0.0f;
  }

  // Returns a value below which floating point precision is unreliable.
  // If we're testing for zero, for instance, we should test against this
  // value. Pass in the x-range as well, in case that coefficient dominates
  // the others.
  float EpsilonInInterval(const float max_x) const {
    return Epsilon(std::max(std::fabs(max_x), MaxCoeff()));
  }

  // Returns a value below which floating point precision is unreliable.
  // If we're testing for zero, for instance, we should test against this
  // value. We don't consider the x-range here, so this value is useful only
  // when dealing with the equation itself.
  float EpsilonOfCoefficients() const { return Epsilon(MaxCoeff()); }

  // Given values in the range of `x`, returns a value below which should be
  // considered zero.
  float Epsilon(const float x) const {
    return x * kEpsilonScale;
  }

  // Returns the largest absolute value of the coefficients. Gives a sense
  // of the scale of the quaternion. If all the coefficients are tiny or
  // huge, we'll need to renormalize around 1, since we take reciprocals of
  // the coefficients, and floating point precision is poor for floats.
  float MaxCoeff() const {
    using std::max;
    const QuadraticCurve abs = AbsCoeff();
    return max(max(abs.c_[2], abs.c_[1]), abs.c_[0]);
  }

  // Used for finding roots, and more.
  // See http://en.wikipedia.org/wiki/Discriminant
  float Discriminant() const { return c_[1] * c_[1] - 4.0f * c_[2] * c_[0]; }

  // When Discriminant() is close to zero, set to zero.
  // Often floating point precision problems can make the discriminant
  // very slightly non-zero, even though mathematically it should be zero.
  float ReliableDiscriminant(const float epsilon) const;

  // Return the x at which the derivative is zero.
  float CriticalPoint() const {
    DCHECK(std::fabs(c_[2]) >= EpsilonOfCoefficients());

    // 0 = f'(x) = 2*c2*x + c1  ==>  x = -c1 / 2c2
    return -(c_[1] / c_[2]) * 0.5f;
  }

  // Returns the coefficient for x-to-the-ith -power.
  float Coeff(int i) const { return c_[i]; }

  // Returns the number of coefficients in this curve.
  int NumCoeff() const { return kNumCoeff; }

  // Returns the curve f(x / x_scale), where f is the current quadratic.
  // This stretches the curve along the x-axis by x_scale.
  QuadraticCurve ScaleInX(const float x_scale) const {
    return ScaleInXByReciprocal(1.0f / x_scale);
  }

  // Returns the curve f(x * x_scale_reciprocal), where f is the current
  // quadratic.
  // This stretches the curve along the x-axis by 1/x_scale_reciprocal
  QuadraticCurve ScaleInXByReciprocal(const float x_scale_reciprocal) const {
    return QuadraticCurve(c_[2] * x_scale_reciprocal * x_scale_reciprocal,
                          c_[1] * x_scale_reciprocal, c_[0]);
  }

  // Returns the curve y_scale * f(x).
  QuadraticCurve ScaleInY(const float y_scale) const {
    return QuadraticCurve(*this, y_scale);
  }

  // Returns the same curve but with all coefficients in absolute value.
  // Useful for numerical precision work.
  QuadraticCurve AbsCoeff() const {
    using std::fabs;
    return QuadraticCurve(fabs(c_[2]), fabs(c_[1]), fabs(c_[0]));
  }

  // Equality. Checks for exact match. Useful for testing.
  bool operator==(const QuadraticCurve& rhs) const;
  bool operator!=(const QuadraticCurve& rhs) const { return !operator==(rhs); }

  size_t IntervalsMatchingSign(const Interval& x_limits, const float sign,
                               Interval matching[2]) const;

  size_t Roots(float roots[2]) const;

 private:
  // Feel free to expose these versions in the external API if they're useful.
  size_t RootsWithoutNormalizing(float roots[2]) const;
  size_t RootsInInterval(const Interval& x_limits, float roots[2]) const;

  float c_[kNumCoeff];  // c_[2] * x^2  +  c_[1] * x  +  c_[0]
};
}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_SPLINE_QUADRATIC_CURVE_H_
