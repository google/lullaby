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

#ifndef REDUX_MODULES_MATH_QUATERNION_H_
#define REDUX_MODULES_MATH_QUATERNION_H_

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>

#include "redux/modules/base/typeid.h"
#include "redux/modules/math/constants.h"
#include "redux/modules/math/detail/quaternion_layout.h"
#include "redux/modules/math/math.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/vector.h"
#include "vectorial/simd4f.h"

namespace redux {

template <typename Layout>
class QuaternionImpl;

template <typename Scalar, bool AllowSimd>
using Quaternion = QuaternionImpl<detail::QuaternionLayout<Scalar, AllowSimd>>;

// Common quaternion type aliases.
using quat = Quaternion<float, kEnableSimdByDefault>;

// An quaternion which uses the underlying 'Layout' to determine the data type
// of the quaternion.
template <typename Layout>
class QuaternionImpl : public Layout {
 public:
  using Scalar = typename Layout::Scalar;
  static constexpr const int kDims = Layout::kDims;
  static constexpr const bool kSimd = Layout::kSimd;

  // Useful type aliases to other math types used in the API.
  using Vector3 = Vector<Scalar, 3, kSimd>;
  using Vector4 = Vector<Scalar, 4, kSimd>;

  // Creates a zero-initialized quaternion.
  constexpr QuaternionImpl() {}

  // Copies a quaternion from another quaternion, copying each element.
  constexpr QuaternionImpl(const QuaternionImpl& rhs);

  // Assigns a quaternion from another quaternion, copying each element.
  constexpr auto& operator=(const QuaternionImpl& rhs);

  // Creates a quaternion from another quaternion of a different type by copying
  // each element. If the other quaternion is of smaller dimensionality, then
  // the created quaternion will be padded with zero-valued elements.
  template <class U>
  explicit constexpr QuaternionImpl(const QuaternionImpl<U>& rhs) {
    using Other = QuaternionImpl<U>;
    static_assert(!std::is_same_v<QuaternionImpl, Other>);
    for (int i = 0; i < kDims; ++i) {
      this->data[i] = static_cast<Scalar>(rhs.data[i]);
    }
  }

  // Creates a quaternion from four scalar values.
  constexpr QuaternionImpl(Scalar x, Scalar y, Scalar z, Scalar w);

  // Creates a quaternion from a vector and scalar value.
  constexpr QuaternionImpl(const Vector3& xyz, Scalar w);

  // Creates a quaternion from a 4D vector.
  explicit constexpr QuaternionImpl(const Vector4& v);

  // Creates a quaternion from an array of scalar values.
  // IMPORTANT: This function does not perform any bounds checking and assumes
  // the array contains at least 4 elements.
  explicit constexpr QuaternionImpl(const Scalar* a);

  // Accesses the n-th element of the quaternion such that:
  //   n == 0 -> x,  n == 1 -> y,  n == 2 -> z,  n == 3 -> w
  constexpr Scalar& operator[](int i) { return this->data[i]; }
  constexpr const Scalar& operator[](int i) const { return this->data[i]; }

  // Returns the vector (xyz) component of the quaternion.
  constexpr Vector3 xyz() const { return Vector3(this->data); }

  // Returns a Vector4 version of the quaternion.
  constexpr Vector4 xyzw() const { return Vector4(this->data); }

  // Returns the length of this quaternion.
  constexpr auto Length() const -> Scalar;

  // Returns the squared length of this quaternion.
  constexpr auto LengthSquared() const -> Scalar;

  // Returns a normalized copy of this quaternion.
  constexpr auto Normalized() const -> QuaternionImpl;

  // Normalizes this quaternion, returning its prenormalized length.
  constexpr auto SetNormalized() -> Scalar;

  // Returns an inversed copy of this quaternion.
  constexpr auto Inversed() const -> QuaternionImpl;

  // Inverts this quaternion.
  constexpr void SetInversed();

  // Returns the dot product of this quaternion and another quaternion.
  constexpr auto Dot(const QuaternionImpl& other) const -> Scalar;

  // Returns the identity quaternion constant.
  static constexpr QuaternionImpl Identity();
};

// Copies a quaternion from another quaternion, copying each element.
template <typename T>
constexpr QuaternionImpl<T>::QuaternionImpl(const QuaternionImpl& rhs) {
  if constexpr (kSimd) {
    this->simd = rhs.simd;
  } else {
    for (int i = 0; i < kDims; ++i) {
      this->data[i] = rhs.data[i];
    }
  }
}

// Assigns a quaternion from another quaternion, copying each element.
template <typename T>
constexpr auto& QuaternionImpl<T>::operator=(const QuaternionImpl& rhs) {
  if constexpr (kSimd) {
    this->simd = rhs.simd;
  } else {
    for (int i = 0; i < kDims; ++i) {
      this->data[i] = rhs.data[i];
    }
  }
  return *this;
}

// Creates a quaternion from four scalar values.
template <typename T>
constexpr QuaternionImpl<T>::QuaternionImpl(Scalar x, Scalar y, Scalar z,
                                            Scalar w) {
  this->data[0] = x;
  this->data[1] = y;
  this->data[2] = z;
  this->data[3] = w;
}

// Creates a quaternion from a vector and scalar value.
template <typename T>
constexpr QuaternionImpl<T>::QuaternionImpl(const Vector3& xyz, Scalar w) {
  this->data[0] = xyz.data[0];
  this->data[1] = xyz.data[1];
  this->data[2] = xyz.data[2];
  this->data[3] = w;
}

// Creates a quaternion from a 4D vector.
template <typename T>
constexpr QuaternionImpl<T>::QuaternionImpl(const Vector4& v) {
  this->data[0] = v.data[0];
  this->data[1] = v.data[1];
  this->data[2] = v.data[2];
  this->data[3] = v.data[3];
}

// Creates a quaternion from an array of scalar values.
// IMPORTANT: This function does not perform any bounds checking and assumes
// the array contains at least 4 elements.
template <typename T>
constexpr QuaternionImpl<T>::QuaternionImpl(const Scalar* a) {
  for (int i = 0; i < kDims; ++i) {
    this->data[i] = a[i];
  }
}

// Returns the length of this quaternion.
template <typename T>
constexpr auto QuaternionImpl<T>::Length() const -> Scalar {
  return sqrt(LengthSquared());
}

// Returns the squared length of this quaternion.
template <typename T>
constexpr auto QuaternionImpl<T>::LengthSquared() const -> Scalar {
  return Dot(*this);
}

// Returns a normalized copy of this quaternion.
template <typename T>
constexpr auto QuaternionImpl<T>::Normalized() const -> QuaternionImpl {
  QuaternionImpl result;
  if constexpr (kSimd) {
    result.simd = simd4f_normalize4(this->simd);
  } else {
    const auto length = Length();
    const auto one_over_length = Scalar(1) / length;
    for (int i = 0; i < kDims; ++i) {
      result.data[i] = this->data[i] * one_over_length;
    }
  }
  return result;
}

template <typename T>
constexpr auto QuaternionImpl<T>::SetNormalized() -> Scalar {
  const auto length = Length();
  const auto one_over_length = Scalar(1) / length;
  if constexpr (kSimd) {
    this->simd = simd4f_mul(this->simd, simd4f_splat(one_over_length));
  } else {
    for (int i = 0; i < kDims; ++i) {
      this->data[i] *= one_over_length;
    }
  }
  return length;
}

// Returns an inversed copy of this quaternion.
template <typename T>
constexpr auto QuaternionImpl<T>::Inversed() const -> QuaternionImpl {
  return QuaternionImpl(-this->x, -this->y, -this->z, this->w);
}

template <typename T>
constexpr void QuaternionImpl<T>::SetInversed() {
  this->x = -this->x;
  this->y = -this->y;
  this->z = -this->z;
}

// Returns the dot product of this quaternion and another quaternion.
template <typename T>
constexpr auto QuaternionImpl<T>::Dot(const QuaternionImpl& other) const
    -> Scalar {
  Scalar result = 0;
  if constexpr (kSimd) {
    result = simd4f_get_x(simd4f_dot4(this->simd, other.simd));
  } else {
    for (int i = 0; i < kDims; ++i) {
      result += this->data[i] * other.data[i];
    }
  }
  return result;
}

template <typename T>
constexpr QuaternionImpl<T> QuaternionImpl<T>::Identity() {
  return QuaternionImpl(Scalar(0), Scalar(0), Scalar(0), Scalar(1));
}

// Returns the dot product scalar of two quaternions.
template <typename T>
constexpr auto Dot(const QuaternionImpl<T>& v1, const QuaternionImpl<T>& v2) {
  return v1.Dot(v2);
}

// Returns the scalar squared length of the quaternion.
template <typename T>
constexpr auto LengthSquared(const QuaternionImpl<T>& v) {
  return v.LengthSquared();
}

// Returns the scalar length of the quaternion.
template <typename T>
constexpr auto Length(const QuaternionImpl<T>& v) {
  return v.Length();
}

// Returns a quaternion with the same direction as the given quaternion, but
// with a length of 1.
template <typename T>
constexpr auto Normalized(const QuaternionImpl<T>& v) {
  return v.Normalized();
}

// Compares two Quaternions for strict floating-point equality. Consider using
// AreNearlyEqual instead if you want to know if two quaternions represent the
// same rotation or if floating-point precision is a concern.
template <typename T>
constexpr bool operator==(const QuaternionImpl<T>& q1,
                          const QuaternionImpl<T>& q2) {
  for (int i = 0; i < 4; ++i) {
    if (q1.data[i] != q2.data[i]) {
      return false;
    }
  }
  return true;
}

// Compares two Quaternions for strict floating-point inequality. Consider using
// !AreNearlyEqual instead if if you want to know if two quaternions do not
// represent the same rotation or if floating-point precision is a concern.
template <typename T>
constexpr bool operator!=(const QuaternionImpl<T>& q1,
                          const QuaternionImpl<T>& q2) {
  return !(q1 == q2);
}

// Compares two Quaternions for similarity of rotation within a given threshold.
template <typename T>
constexpr bool AreNearlyEqual(const QuaternionImpl<T>& q1,
                              const QuaternionImpl<T>& q2,
                              ScalarImpl<T> epsilon = kDefaultEpsilon) {
  return std::abs(Dot(q1, q2)) > ScalarImpl<T>(1) - epsilon;
}

// Creates a quaternion from an axis and angle.
template <typename T>
constexpr auto QuaternionFromAxisAngle(const VectorImpl<T>& axis,
                                       ScalarImpl<T> angle) {
  static_assert(T::kDims == 3);

  const auto half_angle = ScalarImpl<T>(0.5) * angle;
  const auto sin_half_angle = std::sin(half_angle);
  const auto cos_half_angle = std::cos(half_angle);

  using QuaternionType = Quaternion<typename T::Scalar, T::kSimd>;
  return QuaternionType(axis * sin_half_angle, cos_half_angle);
}

// Creates a quaternion from an axis and angle stored in a Vector4 where the
// w-component is the angle.
template <typename T>
constexpr auto QuaternionFromAxisAngle(const VectorImpl<T>& axis_angle) {
  static_assert(T::kDims == 4);
  return QuaternionFromAxisAngle(axis_angle.xyz(), axis_angle.w);
}

// Converts a quaternion to an axis and angle, encoded into a Vector4.
//
// The resulting axis-angle uses the full range of angles supported by
// quaternions, and will convert back to the original quaternion.
//
// The axis will be stored in the xyz component of the result and the angle will
// be stored in the w component.
template <typename T>
constexpr auto ToAxisAngle(const QuaternionImpl<T>& quat) {
  using Scalar = typename T::Scalar;
  using AxisAngle = Vector<Scalar, 4, T::kSimd>;

  auto axis = quat.xyz();
  const auto length = axis.SetNormalized();

  if (length == Scalar(0)) {
    // Normalize has left NaNs in axis.  This happens at angle = 0 and 360.
    // All axes are correct, so any will do.
    return AxisAngle(1, 0, 0, 0);
  }

  const auto angle = Scalar(2) * std::atan2(length, quat.w);
  return AxisAngle(axis, angle);
}

// Converts a quaternion to an axis and angle with the shortest path, encoded
// into a Vector4.
//
// The resulting axis-angle is guaranteed to have have angle <= 180 and
// represents the same orientation as the input quaternion, but it may not
// convert back the quaternion.
//
// For example, "Rotate 350 degrees left" will return the axis-angle "Rotate 10
// degrees right".
template <typename T>
constexpr auto ToAxisAngleShortestPath(const QuaternionImpl<T>& quat) {
  return ToAxisAngle(quat.w > 0 ? quat : -quat);
}

// Creates a quaternion from 3 euler angles in zyx order.
template <typename T>
constexpr auto QuaternionFromEulerAngles(const VectorImpl<T>& angles) {
  const auto half_angles = angles * ScalarImpl<T>(0.5);
  const auto sx = std::sin(half_angles.x);
  const auto cx = std::cos(half_angles.x);
  const auto sy = std::sin(half_angles.y);
  const auto cy = std::cos(half_angles.y);
  const auto sz = std::sin(half_angles.z);
  const auto cz = std::cos(half_angles.z);
  const auto x = sx * cy * cz - cx * sy * sz;
  const auto y = cx * sy * cz + sx * cy * sz;
  const auto z = cx * cy * sz - sx * sy * cz;
  const auto w = cx * cy * cz + sx * sy * sz;

  using QuaternionType = Quaternion<typename T::Scalar, T::kSimd>;
  return QuaternionType(x, y, z, w);
}

template <typename T>
constexpr auto ToEulerAngles(const QuaternionImpl<T>& quat) {
  using Vector3 = Vector<typename T::Scalar, 3, T::kSimd>;

  const auto m = ToRotationMatrix(quat);
  const auto cos2 = m(0, 0) * m(0, 0) + m(1, 0) * m(1, 0);
  if (cos2 < kDefaultEpsilon) {
    const ScalarImpl<T> x = 0;
    const ScalarImpl<T> y = m(2, 0) < 0 ? kHalfPi : -kHalfPi;
    const ScalarImpl<T> z = -std::atan2(m(0, 1), m(1, 1));
    return Vector3(x, y, z);
  } else {
    const ScalarImpl<T> x = std::atan2(m(2, 1), m(2, 2));
    const ScalarImpl<T> y = std::atan2(-m(2, 0), std::sqrt(cos2));
    const ScalarImpl<T> z = std::atan2(m(1, 0), m(0, 0));
    return Vector3(x, y, z);
  }
}

// Creates a quaternion from the 3x3 rotation part of a matrix.
template <typename T>
constexpr auto QuaternionFromRotationMatrix(const MatrixImpl<T>& m) {
  using QuaternionType = Quaternion<typename T::Scalar, T::kSimd>;
  static_assert(T::kRows >= 3 && T::kRows <= 4);
  static_assert(T::kCols >= 3 && T::kCols <= 4);

  constexpr ScalarImpl<T> one(1);
  constexpr ScalarImpl<T> two(2);
  constexpr ScalarImpl<T> quarter(0.25);

  const auto trace = m.cols[0][0] + m.cols[1][1] + m.cols[2][2];
  if (trace > 0) {
    const auto s = std::sqrt(trace + one) * two;
    const auto one_over_s = one / s;
    const auto qx = (m.cols[1][2] - m.cols[2][1]) * one_over_s;
    const auto qy = (m.cols[2][0] - m.cols[0][2]) * one_over_s;
    const auto qz = (m.cols[0][1] - m.cols[1][0]) * one_over_s;
    const auto qw = quarter * s;
    return QuaternionType(qx, qy, qz, qw);
  } else if ((m.cols[0][0] > m.cols[1][1]) & (m.cols[0][0] > m.cols[2][2])) {
    const auto s =
        std::sqrt(m.cols[0][0] - m.cols[1][1] - m.cols[2][2] + one) * two;
    const auto one_over_s = one / s;
    const auto qx = quarter * s;
    const auto qy = (m.cols[1][0] + m.cols[0][1]) * one_over_s;
    const auto qz = (m.cols[2][0] + m.cols[0][2]) * one_over_s;
    const auto qw = (m.cols[1][2] - m.cols[2][1]) * one_over_s;
    return QuaternionType(qx, qy, qz, qw);
  } else if (m.cols[1][1] > m.cols[2][2]) {
    const auto s =
        std::sqrt(m.cols[1][1] - m.cols[0][0] - m.cols[2][2] + one) * two;
    const auto one_over_s = one / s;
    const auto qx = (m.cols[1][0] + m.cols[0][1]) * one_over_s;
    const auto qy = quarter * s;
    const auto qz = (m.cols[2][1] + m.cols[1][2]) * one_over_s;
    const auto qw = (m.cols[2][0] - m.cols[0][2]) * one_over_s;
    return QuaternionType(qx, qy, qz, qw);
  } else {
    const auto s =
        std::sqrt(m.cols[2][2] - m.cols[0][0] - m.cols[1][1] + one) * two;
    const auto one_over_s = one / s;
    const auto qx = (m.cols[2][0] + m.cols[0][2]) * one_over_s;
    const auto qy = (m.cols[2][1] + m.cols[1][2]) * one_over_s;
    const auto qz = quarter * s;
    const auto qw = (m.cols[0][1] - m.cols[1][0]) * one_over_s;
    return QuaternionType(qx, qy, qz, qw);
  }
}

template <typename T>
inline constexpr auto RotationMatrixFromQuaternion(
    const QuaternionImpl<T>& quat) {
  using Scalar = typename T::Scalar;
  using Matrix = Matrix<Scalar, 3, 3, T::kSimd>;

  constexpr Scalar kOne = Scalar(1);
  constexpr Scalar kTwo = Scalar(2);
  const auto xx = quat.x * quat.x;
  const auto yy = quat.y * quat.y;
  const auto zz = quat.z * quat.z;
  const auto wx = quat.w * quat.x;
  const auto wy = quat.w * quat.y;
  const auto wz = quat.w * quat.z;
  const auto xz = quat.x * quat.z;
  const auto yz = quat.y * quat.z;
  const auto xy = quat.x * quat.y;

  // clang-format off
  return Matrix(kOne - kTwo * (yy + zz),        kTwo * (xy - wz),        kTwo * (wy + xz),
                       kTwo * (xy + wz), kOne - kTwo * (xx + zz),        kTwo * (yz - wx),
                       kTwo * (xz - wy),        kTwo * (wx + yz), kOne - kTwo * (xx + yy));
  // clang-format on
}

template <typename T>
inline constexpr auto ToRotationMatrix(const QuaternionImpl<T>& quat) {
  return RotationMatrixFromQuaternion(quat);
}

// Negates all elements of the quaternion.
template <typename T>
constexpr auto operator-(const QuaternionImpl<T>& v) {
  QuaternionImpl<T> result;
  if constexpr (T::kSimd) {
    result.simd = simd4f_sub(simd4f_zero(), v.simd);
  } else {
    for (int i = 0; i < T::kDims; ++i) {
      result.data[i] = -v.data[i];
    }
  }
  return result;
}

// Multiplies this QuaternionImpl by a scalar.
//
// This conditions the QuaternionImpl to be a rotation <= 180 degrees, then
// multiplies the angle of the rotation by a scalar factor.
//
// If the scalar factor is < 1, the resulting rotation will be on the shorter
// of the two paths to the identity orientation, which is often intuitive but
// can trip you up if you really did want to take the longer path.
//
// If the scalar factor is > 1, the resulting rotation will be on the longer
// of the two paths to the identity orientation, which can be unintuitive.
// For example, you are not guaranteed that (q * 2) * .5 and q * (2 * .5)
// are the same orientation, let alone the same quaternion.
template <typename T>
constexpr auto operator*(ScalarImpl<T> s, const QuaternionImpl<T>& q) {
  auto axis_angle = ToAxisAngleShortestPath(q);
  axis_angle.w *= s;
  return QuaternionFromAxisAngle(axis_angle);
}

template <typename T>
constexpr auto operator*(const QuaternionImpl<T>& q, ScalarImpl<T> s) {
  return s * q;
}

template <typename T>
constexpr auto& operator*=(QuaternionImpl<T>& q, ScalarImpl<T> s) {
  q = q * s;
  return q;
}

template <typename T>
constexpr auto operator*(const QuaternionImpl<T>& q1,
                         const QuaternionImpl<T>& q2) {
  const auto scalar = q1.w * q2.w - q1.xyz().Dot(q2.xyz());
  const auto vector =
      q1.w * q2.xyz() + q2.w * q1.xyz() + q1.xyz().Cross(q2.xyz());
  return QuaternionImpl<T>(vector, scalar);
}

template <typename T>
constexpr auto& operator*=(QuaternionImpl<T>& q1, const QuaternionImpl<T>& q2) {
  q1 = q1 * q2;
  return q1;
}

// Returns the vector resulting from rotating the input vector by this
// quaternion.
template <typename T, typename U>
constexpr auto operator*(const QuaternionImpl<T>& quat,
                         const VectorImpl<U>& vec) {
  static_assert(U::kDims == 3);
  static_assert(std::is_same_v<typename T::Scalar, typename U::Scalar>);

  const auto xyz = quat.xyz();
  const ScalarImpl<T> ww = quat.w + quat.w;
  return (ww * xyz.Cross(vec)) + ((ww * quat.w - 1) * vec) +
         (2 * xyz.Dot(vec) * xyz);
}

// Returns a quaternion that is a normalized linear interpolation between two
// quaternions by the given percentage.
template <typename T>
constexpr auto NLerp(const QuaternionImpl<T>& v1, const QuaternionImpl<T>& v2,
                     ScalarImpl<T> percent) {
  QuaternionImpl<T> result;
  if constexpr (QuaternionImpl<T>::kSimd) {
    const auto simd_percent = simd4f_splat(percent);
    const auto simd_one = simd4f_splat(1.0f);
    const auto simd_one_minus_percent = simd4f_sub(simd_one, simd_percent);
    const auto a = simd4f_mul(simd_one_minus_percent, v1.simd);
    const auto b = simd4f_mul(simd_percent, v2.simd);
    result.simd = simd4f_add(a, b);
  } else {
    const auto one_minus_percent = ScalarImpl<T>(1) - percent;
    for (int i = 0; i < 4; ++i) {
      result.data[i] =
          (one_minus_percent * v1.data[i]) + (percent * v2.data[i]);
    }
  }
  return result.Normalized();
}

// Returns a quaternion that is a spherical interpolation between two
// quaternions by the given percentage.
//
// This method always gives you the "short way around" interpolation. If you
// need mathematical slerp, use ToAxisAngle() and FromAxisAngle().
template <typename T>
constexpr auto SLerp(const QuaternionImpl<T>& q1, const QuaternionImpl<T>& q2,
                     ScalarImpl<T> percent) {
  using Scalar = ScalarImpl<T>;

  // The two quaternions are almost identical, so lerp their vector forms
  // instead to prevent division be zero.
  const auto dot = q1.Dot(q2);
  if (dot > Scalar(0.9999)) {
    const auto vector = Lerp(q1.xyzw(), q2.xyzw(), percent);
    return QuaternionImpl<T>(vector.Normalized());
  }

  const Scalar npq = std::sqrt(Dot(q1, q1) * Dot(q2, q2));
  const Scalar a = std::acos(Clamp(std::abs(dot) / npq, Scalar(-1), Scalar(1)));
  const Scalar a0 = a * (Scalar(1) - percent);
  const Scalar a1 = a * percent;
  const Scalar sina = sin(a);
  if (sina < kDefaultEpsilon) {
    return NLerp(q1, q2, percent);
  }

  const Scalar isina = Scalar(1) / sina;
  const Scalar s0 = std::sin(a0) * isina;
  // Take the shorter side based on the dot product.
  const Scalar s1 = std::sin(a1) * ((dot < 0) ? -isina : isina);
  return QuaternionImpl<T>(Normalized(s0 * q1.xyzw() + s1 * q2.xyzw()));
}

// Returns the a quaternion that rotates from v1 to v2. If v1 and v2 are
// parallel, there are an infinite number of valid axis between the two vectors.
// If a `preferred_axis` is specified, will return that axis in this
// special case, otherwise will pick an arbitrary axis.
template <typename T>
constexpr auto RotationBetween(
    const VectorImpl<T>& v1, const VectorImpl<T>& v2,
    const std::optional<VectorImpl<T>>& preferred_axis = std::nullopt) {
  static_assert(T::kDims == 3);
  using Quaternion = Quaternion<typename T::Scalar, T::kSimd>;

  // The final equation used here is fairly elegant, but its derivation is
  // not obvious. First, keep in mind that a quaternion can be defined as:
  //   q.xyz = axis *  sin(angle / 2)
  //   q.w   = cos(angle / 2))
  //
  // To begin, the following equations can be used to determine the rotation
  // from start to end:
  //   angle: ArcCos(Dot(start, end) / (|start| * |end|))
  //   axis: Cross(start, end).Normalized()
  //
  // Using the trig identity:
  //  sin(theta * 2) = 2 * sin(theta) * cos(theta)
  //
  // We can derive:
  //  sin(angle) = 2 * sin(angle/2) * cos(angle/2)
  //
  // And further:
  //  sin(angle/2) = 0.5 * sin(angle) / cos(angle/2)
  //
  // Let's plug all this back into our definition of a quaternion:
  //   q.xyz = Cross(start, end).Normalized() * 0.5 * sin(angle) / cos(angle/2)
  //   q.w   = cos(angle/2)
  //
  // Next, we scale the whole thing up by 2 * cos(angle/2) then we get:
  //   q.xyz = Cross(start, end).Normalized() * sin(angle)
  //   q.w   = 2 * cos(angle/2) * cos(angle/2)
  //
  // Note that the quaternion is no longer normalized after this scaling.
  //
  // Now, because we know that:
  //   |Cross(start, end)| = |start| * |end| * sin(angle),
  //
  // we can derive the following:
  //   Cross(start, end).Normalized() =
  //       Cross(start, end) / (|start| * |end| * sin(angle))
  //
  // Substituting that back into our quaternion gets us:
  //   q.xyz = Cross(start, end) / (|start| * |end|)
  //   q.w   = 2 * cos(angle/2) * cos(angle/2)
  //
  // Next, we use another trig identity to simplify the scalar component:
  //   cos(angle/2) = sqrt((1 + cos(angle)) / 2)
  //
  // which, when plugged into our quaternion, gives us:
  //   q.xyz = Cross(start, end) / (|start| * |end|)
  //   q.w   = 1 + cos(angle)
  //
  // Finally, to get rid of the cos calculation, we use the trig formula:
  //   Dot(start, end) = |start| * |end| * cos(angle)
  //
  // And we get:
  //   q.xyz = Cross(start, end) / (|start| * |end|)
  //   q.w   = 1 + Dot(start, end) / (|start| * |end|)
  //
  // Finally, if start and end are normalized, we get a final quaternion of:
  //   q.xyz = Cross(start, end)
  //   q.w   = 1 + Dot(start, end)
  const auto start = v1.Normalized();
  const auto end = v2.Normalized();
  const auto dot_product = start.Dot(end);

  // Any rotation < 0.1 degrees is treated as no rotation in order to avoid
  // division by zero errors.
  // cos(0.1 degrees) = 0.99999847691
  constexpr const auto epsilon = ScalarImpl<T>(0.99999847691);
  if (dot_product >= epsilon) {
    return Quaternion::Identity();
  }

  // If the vectors point in opposite directions, return a 180 degree rotation
  // along the preferred axis.
  if (dot_product <= -epsilon) {
    if (preferred_axis) {
      return Quaternion(*preferred_axis, 0);
    } else {
      return Quaternion(PerpendicularVector(start), 0);
    }
  }

  // Degenerate cases have been handled, so if we're here, we have to
  // actually compute the angle we want:
  const auto cross_product = start.Cross(end);
  return Quaternion(cross_product, ScalarImpl<T>(1.0) + dot_product)
      .Normalized();
}
}  // namespace redux

REDUX_SETUP_TYPEID(redux::quat);

#endif  // REDUX_MODULES_MATH_QUATERNION_H_
