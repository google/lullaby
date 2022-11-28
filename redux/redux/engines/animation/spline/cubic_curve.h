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

#ifndef REDUX_ENGINES_ANIMATION_SPLINE_CUBIC_CURVE_H_
#define REDUX_ENGINES_ANIMATION_SPLINE_CUBIC_CURVE_H_

#include <cstdint>

#include "redux/modules/math/bounds.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Initialization parameters to create a cubic curve with start and end y-values
// and derivatives. Start is x = 0. End is x = width_x.
struct CubicInit {
  CubicInit(const float start_y, const float start_derivative,
            const float end_y, const float end_derivative, const float width_x)
      : start_y(start_y),
        start_derivative(start_derivative),
        end_y(end_y),
        end_derivative(end_derivative),
        width_x(width_x) {}

  // Short-form in comments:
  float start_y;           // y0
  float start_derivative;  // s0
  float end_y;             // y1
  float end_derivative;    // s1
  float width_x;           // w
};

// Represents a cubic polynomial of the form:
//   c_[3] * x^3  +  c_[2] * x^2  +  c_[1] * x  +  c_[0]
class CubicCurve {
 public:
  // 2^22 = the max precision of significand.
  static constexpr float kEpsilonPrecision = static_cast<float>(1 << 22);
  static constexpr float kEpsilonScale = 1.0f / kEpsilonPrecision;
  static const int kNumCoeff = 4;

  CubicCurve() { memset(c_, 0, sizeof(c_)); }
  CubicCurve(const float c3, const float c2, const float c1, const float c0) {
    c_[3] = c3;
    c_[2] = c2;
    c_[1] = c1;
    c_[0] = c0;
  }
  explicit CubicCurve(const CubicInit& init) { Init(init); }
  void Init(const CubicInit& init);

  // Shift the curve along the x-axis: x_shift to the left.
  // That is x_shift becomes the curve's x=0.
  void ShiftLeft(const float x_shift);

  // Shift the curve along the x-axis: x_shift to the right.
  void ShiftRight(const float x_shift) { ShiftLeft(-x_shift); }

  // Shift the curve along the y-axis by y_offset: y_offset up the y-axis.
  void ShiftUp(float y_offset) { c_[0] += y_offset; }

  // Scale the curve along the y-axis by a factor of y_scale.
  void ScaleUp(float y_scale) {
    for (int i = 0; i < kNumCoeff; ++i) {
      c_[i] *= y_scale;
    }
  }

  // Return the cubic function's value at `x`.
  // f(x) = c3*x^3 + c2*x^2 + c1*x + c0
  float Evaluate(const float x) const {
    // Take advantage of multiply-and-add instructions that are common on FPUs.
    return ((c_[3] * x + c_[2]) * x + c_[1]) * x + c_[0];
  }

  // Return the cubic function's slope at `x`.
  // f'(x) = 3*c3*x^2 + 2*c2*x + c1
  float Derivative(const float x) const {
    return (3.0f * c_[3] * x + 2.0f * c_[2]) * x + c_[1];
  }

  // Return the cubic function's second derivative at `x`.
  // f''(x) = 6*c3*x + 2*c2
  float SecondDerivative(const float x) const {
    return 6.0f * c_[3] * x + 2.0f * c_[2];
  }

  // Return the cubic function's constant third derivative.
  // Even though `x` is unused, we pass it in for consistency with other
  // curve classes.
  // f'''(x) = 6*c3
  float ThirdDerivative(const float x) const {
    (void)x;
    return 6.0f * c_[3];
  }

  // Returns true if always curving upward or always curving downward on the
  // specified x_limits.
  // That is, returns true if the second derivative has the same sign over
  // all of x_limits.
  bool UniformCurvature(const Interval& x_limits) const;

  // Return a value below which floating point precision is unreliable.
  // If we're testing for zero, for instance, we should test against this
  // Epsilon().
  float Epsilon() const {
    using std::fabs;
    using std::max;
    const float max_c =
        max(max(max(fabs(c_[3]), fabs(c_[2])), fabs(c_[1])), fabs(c_[0]));
    return max_c * kEpsilonScale;
  }

  // Returns the coefficient for x to the ith power.
  float Coeff(int i) const { return c_[i]; }

  // Overrides the coefficent for x to the ith power.
  void SetCoeff(int i, float coeff) { c_[i] = coeff; }

  // Returns the number of coefficients in this curve.
  int NumCoeff() const { return kNumCoeff; }

  // Equality. Checks for exact match. Useful for testing.
  bool operator==(const CubicCurve& rhs) const;
  bool operator!=(const CubicCurve& rhs) const { return !operator==(rhs); }

 private:
  float c_[kNumCoeff];  // c_[3] * x^3  +  c_[2] * x^2  +  c_[1] * x  +  c_[0]
};
}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_SPLINE_CUBIC_CURVE_H_
