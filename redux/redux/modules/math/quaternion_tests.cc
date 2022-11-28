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

#include <cmath>
#include <cstddef>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/math/constants.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatEq;
using ::testing::FloatNear;

// Test types.
using SisdQuat = Quaternion<float, false>;
using SimdQuat = Quaternion<float, true>;

using QuaternionTestTypes = ::testing::Types<SisdQuat, SimdQuat>;

template <typename T>
class QuaternionTest : public ::testing::Test {};

TYPED_TEST_SUITE(QuaternionTest, QuaternionTestTypes);

template <typename T>
inline auto RotationMatrixFromAngles(const VectorImpl<T>& angles) {
  using Matrix33 = Matrix<typename T::Scalar, 3, 3, T::kSimd>;

  // clang-format off
  const Matrix33 mx( 1,  0,                   0,
                     0,  std::cos(angles.x), -std::sin(angles.x),
                     0,  std::sin(angles.x),  std::cos(angles.x));

  const Matrix33 my( std::cos(angles.y), 0,  std::sin(angles.y),
                     0,                  1,  0,
                    -std::sin(angles.y), 0,  std::cos(angles.y));

  const Matrix33 mz( std::cos(angles.z), -std::sin(angles.z), 0,
                     std::sin(angles.z),  std::cos(angles.z), 0,
                     0,                   0,                  1);
  // clang-format on
  return mz * my * mx;
}

TEST(QuaternionTest, LayoutSize) {
  static_assert(sizeof(SisdQuat) == sizeof(float) * 4);
  static_assert(sizeof(SimdQuat) == sizeof(float) * 4);
}

TYPED_TEST(QuaternionTest, InitZero) {
  const TypeParam quat;
  EXPECT_THAT(quat[0], Eq(0));
  EXPECT_THAT(quat[1], Eq(0));
  EXPECT_THAT(quat[2], Eq(0));
  EXPECT_THAT(quat[3], Eq(0));
}

TYPED_TEST(QuaternionTest, InitFromScalars) {
  const TypeParam quat(1, 2, 3, 4);
  EXPECT_THAT(quat[0], Eq(1));
  EXPECT_THAT(quat[1], Eq(2));
  EXPECT_THAT(quat[2], Eq(3));
  EXPECT_THAT(quat[3], Eq(4));
}

TYPED_TEST(QuaternionTest, InitFromVectorAndScalar) {
  using Vector = typename TypeParam::Vector3;

  const TypeParam quat(Vector(1, 2, 3), 4);
  EXPECT_THAT(quat[0], Eq(1));
  EXPECT_THAT(quat[1], Eq(2));
  EXPECT_THAT(quat[2], Eq(3));
  EXPECT_THAT(quat[3], Eq(4));
}

TYPED_TEST(QuaternionTest, InitFromScalarArray) {
  const typename TypeParam::Scalar arr[] = {1, 2, 3, 4};

  const TypeParam quat(arr);
  EXPECT_THAT(quat[0], Eq(1));
  EXPECT_THAT(quat[1], Eq(2));
  EXPECT_THAT(quat[2], Eq(3));
  EXPECT_THAT(quat[3], Eq(4));
}

TYPED_TEST(QuaternionTest, Copy) {
  const TypeParam q1(1, 2, 3, 4);
  const TypeParam q2(q1);
  EXPECT_THAT(q1[0], Eq(q2[0]));
  EXPECT_THAT(q1[1], Eq(q2[1]));
  EXPECT_THAT(q1[2], Eq(q2[2]));
  EXPECT_THAT(q1[3], Eq(q2[3]));
}

TYPED_TEST(QuaternionTest, Assign) {
  const TypeParam q1(1, 2, 3, 4);
  TypeParam q2;
  q2 = q1;
  EXPECT_THAT(q1[0], Eq(q2[0]));
  EXPECT_THAT(q1[1], Eq(q2[1]));
  EXPECT_THAT(q1[2], Eq(q2[2]));
  EXPECT_THAT(q1[3], Eq(q2[3]));
}

TYPED_TEST(QuaternionTest, Accessors) {
  const TypeParam quat(1, 2, 3, 4);
  EXPECT_THAT(quat.x, Eq(1));
  EXPECT_THAT(quat.y, Eq(2));
  EXPECT_THAT(quat.z, Eq(3));
  EXPECT_THAT(quat.w, Eq(4));
  EXPECT_THAT(quat[0], Eq(1));
  EXPECT_THAT(quat[1], Eq(2));
  EXPECT_THAT(quat[2], Eq(3));
  EXPECT_THAT(quat[3], Eq(4));
  EXPECT_THAT(quat.xyz().x, Eq(1));
  EXPECT_THAT(quat.xyz().y, Eq(2));
  EXPECT_THAT(quat.xyz().z, Eq(3));
}

TYPED_TEST(QuaternionTest, Equal) {
  const TypeParam q1(1, 2, 3, 4);
  const TypeParam q2(1, 2, 3, 4);
  EXPECT_TRUE(q1 == q2);
}

TYPED_TEST(QuaternionTest, NotEqual) {
  const TypeParam q1(1, 2, 3, 4);
  const TypeParam q2(1, 2, 3, 5);
  EXPECT_TRUE(q1 != q2);
}

TYPED_TEST(QuaternionTest, Identity) {
  using Vector3 = typename TypeParam::Vector3;

  const TypeParam identity = TypeParam::Identity();
  EXPECT_THAT(identity.x, Eq(0));
  EXPECT_THAT(identity.y, Eq(0));
  EXPECT_THAT(identity.z, Eq(0));
  EXPECT_THAT(identity.w, Eq(1));
  EXPECT_THAT(ToEulerAngles(identity), Eq(Vector3::Zero()));
}

TYPED_TEST(QuaternionTest, Inversed) {
  const TypeParam quat(1.4, 6.3, 8.5, 5.9);
  const TypeParam inv = quat.Inversed();
  const auto angles = ToEulerAngles(quat * inv);
  EXPECT_THAT(angles[0], FloatEq(0));
  EXPECT_THAT(angles[1], FloatEq(0));
  EXPECT_THAT(angles[2], FloatEq(0));
}

TYPED_TEST(QuaternionTest, Normalized) {
  const TypeParam quat(1.4, 6.3, 8.5, 5.9);
  const TypeParam norm1 = quat.Normalized();
  const TypeParam norm2 = Normalized(quat);
  EXPECT_THAT(norm1.Length(), FloatEq(1.0f));
  EXPECT_THAT(norm2.Length(), FloatEq(1.0f));
}

TYPED_TEST(QuaternionTest, Dot) {
  using Scalar = typename TypeParam::Scalar;
  using Vector3 = typename TypeParam::Vector3;

  const Vector3 axis = Vector3(4.3, 7.6, 1.2).Normalized();

  // A quaternion dot'ed with itself should be 1.0.
  const Scalar angle1 = 1.2;
  const TypeParam q1 = QuaternionFromAxisAngle(axis, angle1);
  const float actual1 = Dot(q1, q1);
  const float expect1 = 1.0f;
  EXPECT_THAT(actual1, FloatNear(expect1, kDefaultEpsilon));

  // A quaternion dot'ed with something at right angles.
  const Scalar angle2 = angle1 + kPi / 2.0;
  const TypeParam q2 = QuaternionFromAxisAngle(axis, angle2);
  const float actual2 = Dot(q1, q2);
  const float expect2 = std::sqrt(2.0f) / 2.0f;
  EXPECT_THAT(actual2, FloatNear(expect2, kDefaultEpsilon));

  // A quaternion dot'ed with its opposite should be 0.0.
  const Scalar angle3 = angle1 + kPi;
  const TypeParam q3 = QuaternionFromAxisAngle(axis, angle3);
  const float actual3 = Dot(q1, q3);
  const float expect3 = 0.0f;
  EXPECT_THAT(actual3, FloatNear(expect3, kDefaultEpsilon));

  // The angle between two quaternions ios: 2 x acos(q1.dot(q2)).
  const Scalar angle4 = 0.7;
  const TypeParam q4 = QuaternionFromAxisAngle(axis, angle4);
  const float actual4 = Dot(q1, q4);
  const float expect4 = std::cos((angle1 - angle4) / 2.0f);
  EXPECT_THAT(actual4, FloatNear(expect4, kDefaultEpsilon));
}

TYPED_TEST(QuaternionTest, MulScalar) {
  using Scalar = typename TypeParam::Scalar;
  using Vector3 = typename TypeParam::Vector3;

  const Vector3 axis = Vector3(4.3, 7.6, 1.2).Normalized();
  const Scalar angle = 1.2;
  const Scalar multiplier = 2.1;

  const TypeParam q1 = QuaternionFromAxisAngle(axis, angle);
  const TypeParam q2 = q1 * multiplier;
  const TypeParam q3 = multiplier * q1;
  TypeParam q4 = q1;
  q4 *= multiplier;

  // Multiplying a quaternion with a scalar corresponds to scaling the rotation.
  const Scalar expect = angle * multiplier;
  const auto aa2 = ToAxisAngle(q2);
  const auto aa3 = ToAxisAngle(q3);
  const auto aa4 = ToAxisAngle(q4);
  EXPECT_THAT(aa2.w, FloatEq(expect));
  EXPECT_THAT(aa3.w, FloatEq(expect));
  EXPECT_THAT(aa4.w, FloatEq(expect));
}

TYPED_TEST(QuaternionTest, MulScalarFlip) {
  // Confirm that `quat * scalar` changes the direction of the quat to keep it
  // in the "small" hemisphere, before doing the multiplication.  This makes
  // scalar factors < 1 act intuitively, at the cost of sometimes making
  // multiplication non-associative for scale factors > 1.
  //
  // For example, you are not guaranteed that (q * 2) * .5 and q * (2 * .5)
  // are the same orientation, let alone the same quaternion.
  using Scalar = typename TypeParam::Scalar;
  using Vector3 = typename TypeParam::Vector3;

  const Vector3 axis = Vector3(4.3, 7.6, 1.2).Normalized();

  // Multiplying by 1 will flip the quaternion if it is the large hemisphere.
  const Scalar big_angle(kPi * 1.50);
  const TypeParam big = QuaternionFromAxisAngle(axis, big_angle);
  const TypeParam actual = big * Scalar(1);
  EXPECT_TRUE(AreNearlyEqual(actual, big));
  EXPECT_TRUE(!AreNearlyEqual(actual.xyzw(), big.xyzw()));

  const Scalar small_angle(kPi * 0.75);
  const TypeParam small = QuaternionFromAxisAngle(axis, small_angle);

  // Scale the angle beyond pi, eg. (0.75 * 2) * 0.6
  const TypeParam pre_multiply = (small * 2) * .6f;
  // The angle will be flipped after multiplying it by 2.0.
  //   0.75pi * 2.0 = 1.5pi => -0.5pi
  const Scalar flipped_angle(kPi * -0.5 * 0.6);
  const TypeParam flipped = QuaternionFromAxisAngle(axis, flipped_angle);
  EXPECT_TRUE(AreNearlyEqual(pre_multiply, flipped));
  EXPECT_TRUE(AreNearlyEqual(pre_multiply.xyzw(), flipped.xyzw()));

  // Scale the angle so that it remains small, eg. 0.75 * (2 * 0.6)
  const TypeParam post_multiply = small * (2 * .6f);
  const Scalar unflipped_angle(kPi * 0.75 * 1.2);
  const TypeParam unflipped = QuaternionFromAxisAngle(axis, unflipped_angle);
  EXPECT_TRUE(AreNearlyEqual(post_multiply, unflipped));
  EXPECT_TRUE(AreNearlyEqual(post_multiply.xyzw(), unflipped.xyzw()));
}

TYPED_TEST(QuaternionTest, MulQuaternion) {
  using Scalar = typename TypeParam::Scalar;
  using Vector3 = typename TypeParam::Vector3;

  const Vector3 axis = Vector3(4.3, 7.6, 1.2).Normalized();
  const Scalar angle1 = 1.2;
  const Scalar angle2 = 0.7;

  const TypeParam q1 = QuaternionFromAxisAngle(axis, angle1);
  const TypeParam q2 = QuaternionFromAxisAngle(axis, angle2);
  const TypeParam q3 = q1 * q2;
  TypeParam q4 = q1;
  q4 *= q2;

  // Multiplying two quaternions sums the rotations.
  const Scalar expect = angle1 + angle2;
  const auto aa3 = ToAxisAngle(q3);
  const auto aa4 = ToAxisAngle(q4);
  EXPECT_THAT(aa3.w, FloatEq(expect));
  EXPECT_THAT(aa4.w, FloatEq(expect));
}

TYPED_TEST(QuaternionTest, MulVector) {
  using Scalar = typename TypeParam::Scalar;
  using Vector3 = typename TypeParam::Vector3;

  const Vector3 axis = Vector3(4.3, 7.6, 1.2).Normalized();
  const Scalar angle = 1.2;

  const TypeParam quat = QuaternionFromAxisAngle(axis, angle);
  const Vector3 vec(3.5, 6.4, 7.0);

  // Multiplying a vector with a quaternion rotates the vector.
  const Vector3 actual = quat * vec;
  const Vector3 expect = ToRotationMatrix(quat) * vec;
  EXPECT_TRUE(AreNearlyEqual(actual, expect));
}

TYPED_TEST(QuaternionTest, MulIdentity) {
  using Vector3 = typename TypeParam::Vector3;
  const Vector3 angles(1.5, 2.3, 0.6);

  // A quaternion multiplied by the identity returns itself.
  const TypeParam q1 = QuaternionFromEulerAngles(angles);
  const TypeParam q2 = TypeParam::Identity();
  const TypeParam q3 = q1 * q2;
  EXPECT_TRUE(AreNearlyEqual(q3, q1));
}

TYPED_TEST(QuaternionTest, MulInverse) {
  using Vector3 = typename TypeParam::Vector3;
  const Vector3 angles(1.5, 2.3, 0.6);

  // A quaternion multiplied by its inverse returns the identity.
  const TypeParam q1 = QuaternionFromEulerAngles(angles);
  const TypeParam q2 = q1.Inversed();
  const TypeParam q3 = q1 * q2;
  EXPECT_TRUE(AreNearlyEqual(q3, TypeParam::Identity()));
}

TYPED_TEST(QuaternionTest, ToEulerAngles) {
  using Vector3 = typename TypeParam::Vector3;

  const Vector3 angles(1.5, 2.3, 0.6);
  const TypeParam expect(0.0686388, 0.7203152, -0.50606, 0.4694018);

  const auto quat = QuaternionFromEulerAngles(angles);
  EXPECT_THAT(quat.x, FloatNear(expect.x, kDefaultEpsilon));
  EXPECT_THAT(quat.y, FloatNear(expect.y, kDefaultEpsilon));
  EXPECT_THAT(quat.z, FloatNear(expect.z, kDefaultEpsilon));
  EXPECT_THAT(quat.w, FloatNear(expect.w, kDefaultEpsilon));

  const auto actual = ToEulerAngles(quat);
  EXPECT_THAT(angles.x, FloatEq(kPi + actual.x));
  EXPECT_THAT(angles.y, FloatEq(kPi - actual.y));
  EXPECT_THAT(angles.z, FloatEq(kPi + actual.z));
}

TYPED_TEST(QuaternionTest, ToRotationMatrix) {
  using Vector3 = typename TypeParam::Vector3;
  using Matrix33 = Matrix<typename TypeParam::Scalar, 3, 3, TypeParam::kSimd>;

  const Vector3 angles(1.5, 2.3, 0.6);
  const auto matrix = RotationMatrixFromAngles(angles);
  const Matrix33 expected(-0.5499013, 0.5739741, 0.6067637, -0.3762077,
                          0.4783840, -0.7934837, -0.7457052, -0.6646070,
                          -0.0471305);
  EXPECT_TRUE(AreNearlyEqual(matrix, expected));

  const auto quat = QuaternionFromRotationMatrix(matrix);
  const auto actual = ToRotationMatrix(quat);
  EXPECT_TRUE(AreNearlyEqual(actual, expected));
}

TYPED_TEST(QuaternionTest, ToAxisAngle) {
  using Vector3 = typename TypeParam::Vector3;
  using Vector4 = typename TypeParam::Vector4;

  const Vector3 axis = Vector3(4.3, 7.6, 1.2).Normalized();
  const float angle = 1.2f;
  const auto quat = QuaternionFromAxisAngle(axis, angle);
  const auto actual = ToAxisAngle(quat);
  EXPECT_TRUE(AreNearlyEqual(actual, Vector4{axis, angle}));
}

TYPED_TEST(QuaternionTest, ToAxisAngleShortestPath) {
  using Vector4 = typename TypeParam::Vector4;
  const Vector4 k350LeftAxisAngle(0, 1, 0, 350 * kDegreesToRadians);
  const Vector4 k10RightAxisAngle(0, -1, 0, 10 * kDegreesToRadians);

  const TypeParam left350 = QuaternionFromAxisAngle(k350LeftAxisAngle);

  const Vector4 actual = ToAxisAngle(left350);
  EXPECT_TRUE(AreNearlyEqual(actual, k350LeftAxisAngle));

  const Vector4 shortest = ToAxisAngleShortestPath(left350);
  EXPECT_TRUE(AreNearlyEqual(shortest, k10RightAxisAngle));
}

TYPED_TEST(QuaternionTest, NLerp) {
  using Scalar = typename TypeParam::Scalar;
  using Vector3 = typename TypeParam::Vector3;
  using Vector4 = typename TypeParam::Vector4;

  const Vector3 angles1(0.66, 1.30, 0.76);
  const Vector3 angles2(0.85, 0.33, 1.60);
  const TypeParam quat1 = QuaternionFromEulerAngles(angles1);
  const TypeParam quat2 = QuaternionFromEulerAngles(angles2);

  const Scalar percents[] = {0.00, 0.01, 0.25, 0.50, 0.75, 0.98, 1.00};
  for (size_t i = 0; i < sizeof(percents) / sizeof(percents[0]); ++i) {
    const TypeParam actual = NLerp(quat1, quat2, percents[i]);

    // NLerp should be a normalized vector4 Lerp.
    const auto x = quat1.x + (quat2.x - quat1.x) * percents[i];
    const auto y = quat1.y + (quat2.y - quat1.y) * percents[i];
    const auto z = quat1.z + (quat2.z - quat1.z) * percents[i];
    const auto w = quat1.w + (quat2.w - quat1.w) * percents[i];
    const Vector4 expect = Vector4(x, y, z, w).Normalized();
    EXPECT_TRUE(AreNearlyEqual(actual, TypeParam(expect)));
  }
}

TYPED_TEST(QuaternionTest, SLerp) {
  using Vector3 = typename TypeParam::Vector3;

  const Vector3 angles1(0.66, 1.30, 0.76);
  const Vector3 angles2(0.85, 0.33, 1.60);

  const TypeParam quat1 = QuaternionFromEulerAngles(angles1);
  const TypeParam quat2 = QuaternionFromEulerAngles(angles2);
  const TypeParam slerp = SLerp(quat1, quat2, 0.5);

  const Vector3 actual = ToEulerAngles(slerp);
  const Vector3 expect(0.933747, 0.819862, 1.32655);
  EXPECT_TRUE(AreNearlyEqual(actual, expect));
}

TYPED_TEST(QuaternionTest, SLerpPrecents) {
  using Scalar = typename TypeParam::Scalar;
  using Vector3 = typename TypeParam::Vector3;

  const Vector3 axis = Vector3(4.3, 7.6, 1.2).Normalized();
  const Scalar angle1 = 0.7;
  const Scalar angle2 = 1.2;
  const TypeParam q1 = QuaternionFromAxisAngle(axis, angle1);
  const TypeParam q2 = QuaternionFromAxisAngle(axis, angle2);

  const Scalar percents[] = {0.00, 0.01, 0.25, 0.50, 0.75, 0.98, 1.00};
  for (size_t i = 0; i < sizeof(percents) / sizeof(percents[0]); ++i) {
    const Scalar expect = angle1 + ((angle2 - angle1) * percents[i]);

    // Slerping two quaternions corresponds to interpolating the angle.
    const TypeParam slerp = SLerp(q1, q2, percents[i]);
    EXPECT_THAT(ToAxisAngle(slerp).w, FloatEq(expect));

    // Test the invariant that SLerp(a, b, t) == slerp(b, a, 1-t).
    const TypeParam backward = SLerp(q2, q1, 1 - percents[i]);
    EXPECT_THAT(ToAxisAngle(backward).w, FloatEq(expect));
  }
}

TYPED_TEST(QuaternionTest, SLerpShortestPath) {
  using Vector3 = typename TypeParam::Vector3;

  // We'll be slerp'ing from the identity to the given angle around an axis.
  // x: angle, y: expected, z: percent
  const Vector3 axis = Vector3::YAxis();
  const Vector3 test_cases[] = {
      // Easy and unambiguous cases.
      {+160, +60, 0.375},
      {-160, -60, 0.375},

      // Shortening a "long way around" (> 180 degree) rotation
      // NOTE: These results are different from the mathematical slerp
      {+320, -15, 0.375},  // Mathematically, should be +120
      {-320, +15, 0.375},  // Mathematically, should be -120

      // Lengthening a "long way around" rotation
      {320, -60, 1.5},  // Mathematically, should be 480 (ie -240)

      // Lengthening to a "long way around" (> 180 degree) rotation
      {+70, +210, 3},
      {-70, -210, 3},

      // An edge case that often causes NaNs
      {0, 0, .5f},

      // This edge case is ill-defined for "intuitive" slerp and can't be
      // tested.
      // {180, 45, .25},

      // Conversely, this edge case is well-defined for "intuitive" slerp.
      // For mathematical slerp, the axis is ill-defined and can take many
      // values.
      {360, 0, .25},
  };

  for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); ++i) {
    const auto angle1 = test_cases[i].x * kDegreesToRadians;
    const auto angle2 = test_cases[i].y * kDegreesToRadians;
    const auto percent = test_cases[i].z;

    const TypeParam origin = TypeParam::Identity();
    const TypeParam target = QuaternionFromAxisAngle(axis, angle1);
    const TypeParam expected = QuaternionFromAxisAngle(axis, angle2);
    const TypeParam actual = SLerp(origin, target, percent);
    EXPECT_TRUE(AreNearlyEqual(actual, expected));
  }
}

TYPED_TEST(QuaternionTest, SLerpSmallAngles) {
  using Scalar = typename TypeParam::Scalar;
  using Vector3 = typename TypeParam::Vector3;

  // Slerp algorithms commonly have trouble with angles near zero.
  // To give a sense of what that means for common quaternion-dot cutoffs:
  // - Quaternion dot of .99999 = .512 degrees
  // - Quaternion dot of .9999 = 1.62 degrees
  // - Quaternion dot of .9995 = 3.62 degrees
  const Scalar kSlerpTestAnglesInDegrees[]{
      0.0,
      0.5,
      1.5,
      3.5,
      80.0,
      // 180 has no numerical problems, unless there's a bug. But worth
      // checking.
      179.0,
      180.0,
      181.0,
      // Slerp is ill-defined at angles near 360.
      359.0,
      359.5,
      360.0,
      360.5,
      361.0,
  };

  // const float kLengthEpsilon = 5e-6f;
  const Vector3 axis(0, 1, 0);
  for (Scalar angle : kSlerpTestAnglesInDegrees) {
    const auto quat = QuaternionFromAxisAngle(axis, angle * kDegreesToRadians);
    const auto result = SLerp(TypeParam::Identity(), quat, 0.5);
    EXPECT_THAT(result.Length(), FloatEq(1.0));
  }
}

TYPED_TEST(QuaternionTest, RotationBetween) {
  using Vector3 = typename TypeParam::Vector3;
  const TypeParam x_to_y = RotationBetween(Vector3::XAxis(), Vector3::YAxis());
  const TypeParam y_to_z = RotationBetween(Vector3::YAxis(), Vector3::ZAxis());
  const TypeParam z_to_x = RotationBetween(Vector3::ZAxis(), Vector3::XAxis());

  // By definition, RotationBetween(v1, v2) * v2 should always equal v2.
  // If v1 and v2 are 90 degrees apart (as they are in the case of axes),
  // applying the same rotation twice should invert the vector.
  const Vector3 x_to_y_result = x_to_y * Vector3::XAxis();
  const Vector3 x_to_y_twice_result = x_to_y * x_to_y * Vector3::XAxis();
  EXPECT_TRUE(AreNearlyEqual(x_to_y_result, Vector3::YAxis()));
  EXPECT_TRUE(AreNearlyEqual(x_to_y_twice_result, -Vector3::XAxis()));

  const Vector3 y_to_z_result = y_to_z * Vector3::YAxis();
  const Vector3 y_to_z_twice_result = y_to_z * y_to_z * Vector3::YAxis();
  EXPECT_TRUE(AreNearlyEqual(y_to_z_result, Vector3::ZAxis()));
  EXPECT_TRUE(AreNearlyEqual(y_to_z_twice_result, -Vector3::YAxis()));

  const Vector3 z_to_x_result = z_to_x * Vector3::ZAxis();
  const Vector3 z_to_x_twice_result = z_to_x * z_to_x * Vector3::ZAxis();
  EXPECT_TRUE(AreNearlyEqual(z_to_x_result, Vector3::XAxis()));
  EXPECT_TRUE(AreNearlyEqual(z_to_x_twice_result, -Vector3::ZAxis()));

  // Try some arbitrary vectors.
  const Vector3 v1(2, -5, 9);
  const Vector3 v2(-1, 3, 16);
  const TypeParam v1_to_v2 = RotationBetween(v1, v2);
  const Vector3 v1_to_v2_result = (v1_to_v2 * v1).Normalized();
  EXPECT_TRUE(AreNearlyEqual(v1_to_v2_result, v2.Normalized()));

  // Using RotationBetween on the same vector should give us the identity.
  const TypeParam identity = RotationBetween(v1, v1);
  const Vector3 identity_result = identity * v2;
  EXPECT_TRUE(AreNearlyEqual(identity_result, v2));

  // Using RotationBetween on opposite vectors should be a 180 degree rotation.
  const TypeParam reverse = RotationBetween(v1, -v1);
  const Vector3 reverse_result = reverse * v1;
  EXPECT_TRUE(AreNearlyEqual(reverse_result, -v1));
}

}  // namespace
}  // namespace redux
