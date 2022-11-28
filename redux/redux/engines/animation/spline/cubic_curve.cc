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

#include "redux/engines/animation/spline/cubic_curve.h"

#include "redux/modules/math/float.h"

namespace redux {

void CubicCurve::Init(const CubicInit& init) {
  //  f(x) = dx^3 + cx^2 + bx + a
  //
  // Solve for a and b by substituting with x = 0.
  //  y0 = f(0) = a
  //  s0 = f'(0) = b
  //
  // Solve for c and d by substituting with x = init.width_x = w. Gives two
  // linear equations with unknowns 'c' and 'd'.
  //  y1 = f(x1) = dw^3 + cw^2 + bw + a
  //  s1 = f'(x1) = 3dw^2 + 2cw + b
  //    ==> 3*y1 - w*s1 = (3dw^3 + 3cw^2 + 3bw + 3a) - (3dw^3 + 2cw^2 + bw)
  //        3*y1 - w*s1 = cw^2 - 2bw + 3a
  //               cw^2 = 3*y1 - w*s1 + 2bw - 3a
  //               cw^2 = 3*y1 - w*s1 + 2*s0*w - 3*y0
  //               cw^2 = 3(y1 - y0) - w*(s1 + 2*s0)
  //                  c = (3/w^2)*(y1 - y0) - (1/w)*(s1 + 2*s0)
  //    ==> 2*y1 - w*s1 = (2dw^3 + 2cw^2 + 2bw + 2a) - (3dw^3 + 2cw^2 + bw)
  //        2*y1 - w*s1 = -dw^3 + bw + 2a
  //               dw^3 = -2*y1 + w*s1 + bw + 2a
  //               dw^3 = -2*y1 + w*s1 + s0*w + 2*y0
  //               dw^3 = 2(y0 - y1) + w*(s1 + s0)
  //                  d = (2/w^3)*(y0 - y1) + (1/w^2)*(s1 + s0)
  const float one_over_w = init.width_x > 0.f ? (1.0f / init.width_x) : 1.f;
  const float one_over_w_sq = one_over_w * one_over_w;
  const float one_over_w_cubed = one_over_w_sq * one_over_w;
  c_[0] = init.start_y;
  c_[1] = init.width_x > 0.f ? init.start_derivative : 0.f;
  c_[2] = 3.0f * one_over_w_sq * (init.end_y - init.start_y) -
          one_over_w * (init.end_derivative + 2.0f * init.start_derivative);
  c_[3] = 2.0f * one_over_w_cubed * (init.start_y - init.end_y) +
          one_over_w_sq * (init.end_derivative + init.start_derivative);
}

void CubicCurve::ShiftLeft(const float x_shift) {
  // Early out optimization.
  if (x_shift == 0.0f) return;

  // s = x_shift
  // f(x) = dx^3 + cx^2 + bx + a
  // f(x + s) = d(x+s)^3 + c(x+s)^2 + b(x+s) + a
  //          = d(x^3 + 3sx^2 + 3s^2x + s^3) + c(x^2 + 2sx + s^2) + b(x + s) + a
  //          = dx^3 + (3sd + c)x^2 + (3ds^2 + 2c + b)x + (ds^3 + cs^2 + bs + a)
  //          = dx^3 + (f''(s)/2) x^2 + f'(s) x + f(s)
  //
  // Or, for an more general formulation, see:
  //     http://math.stackexchange.com/questions/694565/polynomial-shift
  const float new_c = SecondDerivative(x_shift) * 0.5f;
  const float new_b = Derivative(x_shift);
  const float new_a = Evaluate(x_shift);
  c_[0] = new_a;
  c_[1] = new_b;
  c_[2] = new_c;
}

bool CubicCurve::UniformCurvature(const Interval& x_limits) const {
  // Curvature is given by the second derivative. The second derivative is
  // linear. So, the curvature is uniformly positive or negative iff
  //     Sign(f''(x_limits.start)) == Sign(f''(x_limits.end))
  const float epsilon = Epsilon();
  const float start_second_derivative =
      ClampNearZero(SecondDerivative(x_limits.min), epsilon);
  const float end_second_derivative =
      ClampNearZero(SecondDerivative(x_limits.max), epsilon);
  return start_second_derivative * end_second_derivative >= 0.0f;
}

bool CubicCurve::operator==(const CubicCurve& rhs) const {
  for (int i = 0; i < kNumCoeff; ++i) {
    if (c_[i] != rhs.c_[i]) return false;
  }
  return true;
}
}  // namespace redux
