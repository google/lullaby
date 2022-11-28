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

#include "redux/engines/animation/spline/quadratic_curve.h"

#include "redux/modules/math/float.h"

#ifdef _DEBUG
#define REDUX_CURVE_SANITY_CHECKS
#endif  // _DEBUG

namespace redux {

static bool InsideInvertablePowerOf2Range(float x) {
  return kMinInvertablePowerOf2 <= x && x <= kMaxInvertablePowerOf2;
}

void QuadraticCurve::ShiftLeft(const float x_shift) {
  // Early out optimization.
  if (x_shift == 0.0f) return;

  // s = x_shift
  // f(x) = cx^2 + bx + a
  // f(x + s) = c(x+s)^2 + b(x+s) + a
  //          = c(x^2 + 2sx + s^2) + b(x + s) + a
  //          = cx^2 + (2c + b)x + (cs^2 + bs + a)
  //          = cx^2 + f'(s) x + f(s)
  //
  // Or, for an more general formulation, see:
  //     http://math.stackexchange.com/questions/694565/polynomial-shift
  const float new_b = Derivative(x_shift);
  const float new_a = Evaluate(x_shift);
  c_[0] = new_a;
  c_[1] = new_b;
}

float QuadraticCurve::ReliableDiscriminant(const float epsilon) const {
  // When discriminant is (relative to coefficients) close to zero, we treat
  // it as zero. It's possible that the discriminant is barely below zero due
  // to floating point error.
  const float discriminant = Discriminant();
  return ClampNearZero(discriminant, epsilon);
}

size_t QuadraticCurve::Roots(float roots[2]) const {
  // Leave a little headroom for arithmetic.
  static const int kMaxExponentForRootCoeff = kMaxInvertableExponent - 1;

  // Scale in the x-axis so that c2 is in the range of the larger of c1 or c0.
  // This eliminates numerical precision problems in cases where, for example,
  // we have a tiny second derivative and a large constant.
  //
  // The x-axis scale is applied non-uniformly across the polynomial.
  //    f(x_scale * x) = x_scale^2 * c2 * x^2  +  x_scale * c1 * x  +  c0
  // We use this to bring x_scale^2 * c2 in approximately equal to
  // either x_scale * c1 or c0.
  const QuadraticCurve abs = AbsCoeff();
  const bool scale_with_linear = abs.c_[1] >= abs.c_[0];
  const float comparison_coeff = std::max(abs.c_[1], abs.c_[0]);
  const float x_scale_quotient = abs.c_[2] / comparison_coeff;
  const float x_scale_reciprocal_unclamped =
      !InsideInvertablePowerOf2Range(x_scale_quotient) ? 1.0f
      : scale_with_linear ? ReciprocalExponent(x_scale_quotient)
                          : SqrtReciprocalExponent(x_scale_quotient);

  // Since we normalize through powers of 2, the scale can be large without
  // losing precision. But we still have to worry about scaling to infinity.
  // Note that in x-scale, only the linear (c1) and quadratic (c2) coefficients
  // are
  // scaled, and the quadratic coefficient is scaled to match an existing
  // coefficient,
  // so we only need to check the linear coefficient.
  const float x_scale_reciprocal_max =
      MaxPowerOf2Scale(abs.c_[1], kMaxInvertableExponent);
  const float x_scale_reciprocal =
      std::min(x_scale_reciprocal_unclamped, x_scale_reciprocal_max);

  // Create the quatratic scaled in x.
  const QuadraticCurve x_scaled = ScaleInXByReciprocal(x_scale_reciprocal);
  const QuadraticCurve x_scaled_abs = x_scaled.AbsCoeff();

#ifdef REDUX_CURVE_SANITY_CHECKS
  // Sanity checks to ensure our math is correct.
  if (InsideInvertablePowerOf2Range(x_scale_quotient)) {
    const float x_scaled_quotient =
        x_scaled_abs.c_[2] /
        (scale_with_linear ? x_scaled_abs.c_[1] : x_scaled_abs.c_[0]);
    assert(0.5f <= x_scaled_quotient && x_scaled_quotient <= 2.0f);
    (void)x_scaled_quotient;
  }
#endif  // REDUX_CURVE_SANITY_CHECKS

  // Calculate the y-axis scale so that c2 is near 1.
  // We need this because the quadratic equation divides by c2.
  //
  // The y-scale is applied evenly to all coefficients, and doesn't affect the
  // roots.
  //   y_scale * f(x) = y_scale * c2 * x^2  +  y_scale * c1 * x  +  y_scale * c0
  //
  // Check need to clamp our y-scale so that the linear (c1) and constant (c0)
  // coefficients
  // don't go to infinity or denormalize. Note that the y-scale is calculated to
  // bring the
  // quadratic (c2) coefficient near 1, so we don't have to check the quadratic
  // coefficient.
  const float y_scale_unclamped = ReciprocalExponent(Clamp(
      x_scaled_abs.c_[2], kMinInvertablePowerOf2, kMaxInvertablePowerOf2));
  const float y_scale_max =
      std::min(MaxPowerOf2Scale(x_scaled_abs.c_[0], kMaxExponentForRootCoeff),
               MaxPowerOf2Scale(x_scaled_abs.c_[1], kMaxExponentForRootCoeff));
  const float y_scale = std::min(y_scale_max, y_scale_unclamped);

  // Create a scaled version of our quadratic.
  const QuadraticCurve x_and_y_scaled = x_scaled.ScaleInY(y_scale);

#ifdef REDUX_CURVE_SANITY_CHECKS
  // Sanity check to ensure our math is correct.
  const QuadraticCurve x_and_y_scaled_abs = x_and_y_scaled.AbsCoeff();
  assert((Range(0.5f, 2.0f).Contains(x_and_y_scaled_abs.c_[2]) ||
          !kInvertablePowerOf2Range.Contains(x_scaled_abs.c_[2]) ||
          y_scale != y_scale_unclamped) &&
         x_and_y_scaled_abs.c_[1] <= std::numeric_limits<float>::max() &&
         x_and_y_scaled_abs.c_[0] <= std::numeric_limits<float>::max());
  (void)x_and_y_scaled_abs;
#endif  // REDUX_CURVE_SANITY_CHECKS

  // Calculate the roots and then undo the x_scaling.
  const size_t num_roots = x_and_y_scaled.RootsWithoutNormalizing(roots);
  for (size_t i = 0; i < num_roots; ++i) {
    roots[i] *= x_scale_reciprocal;
  }
  return num_roots;
}

// See the Quadratic Formula for details:
// http://en.wikipedia.org/wiki/Quadratic_formula
// Roots returned in sorted order, smallest to largest.
size_t QuadraticCurve::RootsWithoutNormalizing(float roots[2]) const {
  // x^2 coefficient of zero means that curve is linear or constant.
  const float epsilon = EpsilonOfCoefficients();
  if (std::fabs(c_[2]) < epsilon) {
    // If constant, even if zero, return no roots. This is arbitrary.
    if (std::fabs(c_[1]) < epsilon) return 0;

    // Linear 0 = c1x + c0 ==> x = -c0 / c1.
    roots[0] = -c_[0] / c_[1];
    return 1;
  }

  // A negative discriminant means no real roots.
  const float discriminant = ReliableDiscriminant(epsilon);
  if (discriminant < 0.0f) return 0;

  // A zero discriminant means there is only one root.
  const float divisor = (1.0f / c_[2]) * 0.5f;
  if (discriminant == 0.0f) {
    roots[0] = -c_[1] * divisor;
    return 1;
  }

  // Positive discriminant means two roots. We use the quadratic formula.
  const float sqrt_discriminant = std::sqrt(discriminant);
  const float root_minus = (-c_[1] - sqrt_discriminant) * divisor;
  const float root_plus = (-c_[1] + sqrt_discriminant) * divisor;
  assert(root_minus != root_plus);
  roots[0] = std::min(root_minus, root_plus);
  roots[1] = std::max(root_minus, root_plus);
  return 2;
}

// Only keep entries in 'values' if they are in
// (range.start - epsition, range.end + epsilon).
// Any values that are kept are clamped to 'range'.
//
// This function is useful when floating point precision error might put a
// value slightly outside 'range' even though mathematically it should be
// inside 'range'. This often happens with values right on the border of the
// valid range.
static size_t ValuesInInterval(const Interval& range, float epsilon,
                               size_t num_values, float* values) {
  size_t num_returned = 0;
  for (size_t i = 0; i < num_values; ++i) {
    const float value = values[i];
    const float clamped = Clamp(value, range.min, range.max);
    const float dist = fabs(value - clamped);

    // If the distance from the range is small, keep the clamped value.
    if (dist <= epsilon) {
      values[num_returned++] = clamped;
    }
  }
  return num_returned;
}

size_t QuadraticCurve::RootsInInterval(const Interval& valid_x,
                                       float roots[2]) const {
  const size_t num_roots = Roots(roots);

  // We allow the roots to be slightly outside the bounds, since this may
  // happen due to floating point error.
  const float epsilon_x = valid_x.Size() * kEpsilonScale;

  // Loop through each root and only return it if it is within the range
  // [start_x - epsilon_x, end_x + epsilon_x]. Clamp to [start_x, end_x].
  return ValuesInInterval(valid_x, epsilon_x, num_roots, roots);
}

size_t QuadraticCurve::IntervalsMatchingSign(const Interval& x_limits,
                                             float sign,
                                             Interval matching[2]) const {
  // Gather the roots of the validity spline. These are transitions between
  // valid and invalid regions.
  float roots[2];
  const size_t num_roots = RootsInInterval(x_limits, roots);

  // We want ranges where the spline's sign equals valid_sign's.
  const bool valid_at_start = sign * Evaluate(x_limits.min) >= 0.0f;
  const bool valid_at_end = sign * Evaluate(x_limits.max) >= 0.0f;

  // If no roots, the curve never crosses zero, so the start and end validity
  // must be the same.
  // If two roots, the curve crosses zero twice, so the start and end validity
  // must be the same.
  assert(num_roots == 1 || valid_at_start == valid_at_end);

  // Starts invalid, and never crosses zero so never becomes valid.
  if (num_roots == 0 && !valid_at_start) return 0;

  // Starts valid, crosses zero to invalid, crosses zero again back to valid,
  // then ends valid.
  if (num_roots == 2 && valid_at_start) {
    matching[0] = Interval(x_limits.min, roots[0]);
    matching[1] = Interval(roots[1], x_limits.max);
    return 2;
  }

  // If num_roots == 0: must be valid at both start and end so entire range.
  // If num_roots == 1: crosses zero once, or just touches zero.
  // If num_roots == 2: must start and end invalid, so valid range is between
  // roots.
  const float start = valid_at_start ? x_limits.min : roots[0];
  const float end = valid_at_end     ? x_limits.max
                    : num_roots == 2 ? roots[1]
                                     : roots[0];
  matching[0] = Interval(start, end);
  return 1;
}

bool QuadraticCurve::operator==(const QuadraticCurve& rhs) const {
  for (int i = 0; i < kNumCoeff; ++i) {
    if (c_[i] != rhs.c_[i]) return false;
  }
  return true;
}
}  // namespace redux
