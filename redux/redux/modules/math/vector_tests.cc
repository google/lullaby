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

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/math/vector.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatEq;
using ::testing::FloatNear;

// Test types.
using SisdVec2i = Vector<int, 2, false>;
using SisdVec3i = Vector<int, 3, false>;
using SisdVec4i = Vector<int, 4, false>;
using SisdVec5i = Vector<int, 5, false>;
using SisdVec2f = Vector<float, 2, false>;
using SisdVec3f = Vector<float, 3, false>;
using SisdVec4f = Vector<float, 4, false>;
using SisdVec5f = Vector<float, 5, false>;
using SimdVec2i = Vector<int, 2, true>;
using SimdVec3i = Vector<int, 3, true>;
using SimdVec4i = Vector<int, 4, true>;
using SimdVec5i = Vector<int, 5, true>;
using SimdVec2f = Vector<float, 2, true>;
using SimdVec3f = Vector<float, 3, true>;
using SimdVec4f = Vector<float, 4, true>;
using SimdVec5f = Vector<float, 5, true>;

using VectorTestTypes =
    ::testing::Types<SisdVec2i, SisdVec3i, SisdVec4i, SisdVec5i, SisdVec2f,
                     SisdVec3f, SisdVec4f, SisdVec5f, SimdVec2i, SimdVec3i,
                     SimdVec4i, SimdVec5i, SimdVec2f, SimdVec3f, SimdVec4f,
                     SimdVec5f>;

template <typename T>
class VectorTest : public ::testing::Test {};

TYPED_TEST_SUITE(VectorTest, VectorTestTypes);

template <typename VecType>
auto RandScalar() {
  using Scalar = typename VecType::Scalar;
  auto rand_value = (rand() / static_cast<Scalar>(RAND_MAX) * 100.0);
  return Scalar(1 + rand_value);
}

// Creates an std::array with the same type and dimensionality of the VecType
// and populated with a random set of numbers.
template <typename VecType>
auto RandArray() {
  std::array<typename VecType::Scalar, VecType::kDims> arr;
  for (size_t i = 0; i < arr.size(); ++i) {
    arr[i] = RandScalar<VecType>();
  }
  return arr;
}

TEST(VectorTest, LayoutSize) {
  static_assert(sizeof(SisdVec2i) == sizeof(int) * 2);
  static_assert(sizeof(SisdVec3i) == sizeof(int) * 3);
  static_assert(sizeof(SisdVec4i) == sizeof(int) * 4);
  static_assert(sizeof(SisdVec5i) == sizeof(int) * 5);
  static_assert(sizeof(SisdVec2f) == sizeof(float) * 2);
  static_assert(sizeof(SisdVec3f) == sizeof(float) * 3);
  static_assert(sizeof(SisdVec4f) == sizeof(float) * 4);
  static_assert(sizeof(SisdVec5f) == sizeof(float) * 5);
  static_assert(sizeof(SimdVec2i) == sizeof(int) * 2);
  static_assert(sizeof(SimdVec3i) == sizeof(int) * 3);
  static_assert(sizeof(SimdVec4i) == sizeof(int) * 4);
  static_assert(sizeof(SimdVec5i) == sizeof(int) * 5);
  static_assert(sizeof(SimdVec2f) == sizeof(float) * 4);
  static_assert(sizeof(SimdVec3f) == sizeof(float) * 4);
  static_assert(sizeof(SimdVec4f) == sizeof(float) * 4);
  static_assert(sizeof(SimdVec5f) == sizeof(float) * 5);
}

TYPED_TEST(VectorTest, InitZero) {
  const TypeParam vec;
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(vec[i], Eq(0));
  }
}

TYPED_TEST(VectorTest, InitFromScalar) {
  const auto value = RandScalar<TypeParam>();

  const TypeParam vec(value);
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(vec[i], Eq(value));
  }
}

TYPED_TEST(VectorTest, InitFromArray) {
  auto arr = RandArray<TypeParam>();

  const TypeParam vec(arr.data());
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(vec[i], arr[i]);
  }
}

TEST(VectorTest, InitFromDifferentType) {
  {
    const SisdVec2i sisd2(1, 2);
    const SisdVec4i sisd4(1, 2, 3, 4);

    const SimdVec2f simd2(sisd2);
    EXPECT_THAT(simd2[0], Eq(sisd2[0]));
    EXPECT_THAT(simd2[1], Eq(sisd2[1]));

    const SimdVec4f simd4(sisd4);
    EXPECT_THAT(simd4[0], Eq(sisd4[0]));
    EXPECT_THAT(simd4[1], Eq(sisd4[1]));

    const SimdVec2f simd24(sisd4);
    EXPECT_THAT(simd24[0], Eq(sisd4[0]));
    EXPECT_THAT(simd24[1], Eq(sisd4[1]));

    const SimdVec4f simd42(sisd2);
    EXPECT_THAT(simd42[0], Eq(sisd2[0]));
    EXPECT_THAT(simd42[1], Eq(sisd2[1]));
    EXPECT_THAT(simd42[2], Eq(0));
    EXPECT_THAT(simd42[3], Eq(0));
  }

  {
    const SimdVec2i simd2(1, 2);
    const SimdVec4i simd4(1, 2, 3, 4);

    const SisdVec2f sisd2(simd2);
    EXPECT_THAT(sisd2[0], Eq(simd2[0]));
    EXPECT_THAT(sisd2[1], Eq(simd2[1]));

    const SisdVec4f sisd4(simd4);
    EXPECT_THAT(sisd4[0], Eq(simd4[0]));
    EXPECT_THAT(sisd4[1], Eq(simd4[1]));

    const SisdVec2f sisd24(sisd4);
    EXPECT_THAT(sisd24[0], Eq(simd4[0]));
    EXPECT_THAT(sisd24[1], Eq(simd4[1]));

    const SisdVec4f sisd42(sisd2);
    EXPECT_THAT(sisd42[0], Eq(simd2[0]));
    EXPECT_THAT(sisd42[1], Eq(simd2[1]));
    EXPECT_THAT(sisd42[2], Eq(0));
    EXPECT_THAT(sisd42[3], Eq(0));
  }
}

TYPED_TEST(VectorTest, InitVec2) {
  if constexpr (TypeParam::kDims == 2) {
    const auto arr = RandArray<TypeParam>();

    const TypeParam v1(arr[0], arr[1]);
    EXPECT_THAT(v1[0], Eq(arr[0]));
    EXPECT_THAT(v1[1], Eq(arr[1]));
  }
}

TYPED_TEST(VectorTest, InitVec3) {
  if constexpr (TypeParam::kDims == 3) {
    const auto arr = RandArray<TypeParam>();

    const TypeParam v1(arr[0], arr[1], arr[2]);
    EXPECT_THAT(v1[0], Eq(arr[0]));
    EXPECT_THAT(v1[1], Eq(arr[1]));
    EXPECT_THAT(v1[2], Eq(arr[2]));

    const TypeParam v2(v1.xy(), arr[2]);
    EXPECT_THAT(v2[0], Eq(arr[0]));
    EXPECT_THAT(v2[1], Eq(arr[1]));
    EXPECT_THAT(v2[2], Eq(arr[2]));
  }
}

TYPED_TEST(VectorTest, InitVec4) {
  if constexpr (TypeParam::kDims == 4) {
    const auto arr = RandArray<TypeParam>();

    const TypeParam v1(arr[0], arr[1], arr[2], arr[3]);
    EXPECT_THAT(v1[0], Eq(arr[0]));
    EXPECT_THAT(v1[1], Eq(arr[1]));
    EXPECT_THAT(v1[2], Eq(arr[2]));
    EXPECT_THAT(v1[3], Eq(arr[3]));

    const TypeParam v2(v1.xyz(), arr[3]);
    EXPECT_THAT(v2[0], Eq(arr[0]));
    EXPECT_THAT(v2[1], Eq(arr[1]));
    EXPECT_THAT(v2[2], Eq(arr[2]));
    EXPECT_THAT(v2[3], Eq(arr[3]));

    const TypeParam v3(v1.xy(), v1.zw());
    EXPECT_THAT(v3[0], Eq(arr[0]));
    EXPECT_THAT(v3[1], Eq(arr[1]));
    EXPECT_THAT(v3[2], Eq(arr[2]));
    EXPECT_THAT(v3[3], Eq(arr[3]));
  }
}

TYPED_TEST(VectorTest, Copy) {
  auto arr = RandArray<TypeParam>();

  const TypeParam v1(arr.data());
  const TypeParam v2(v1);

  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(v1[i], Eq(v2[i]));
  }
}

TYPED_TEST(VectorTest, Assign) {
  auto arr = RandArray<TypeParam>();

  const TypeParam v1(arr.data());
  TypeParam v2;
  v2 = v1;

  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(v1[i], Eq(v2[i]));
  }
}

TYPED_TEST(VectorTest, Accessors) {
  auto arr = RandArray<TypeParam>();
  const TypeParam vec(arr.data());

  if constexpr (TypeParam::kDims == 2) {
    EXPECT_THAT(vec.x, Eq(arr[0]));
    EXPECT_THAT(vec.y, Eq(arr[1]));
  } else if constexpr (TypeParam::kDims == 3) {
    EXPECT_THAT(vec.x, Eq(arr[0]));
    EXPECT_THAT(vec.y, Eq(arr[1]));
    EXPECT_THAT(vec.z, Eq(arr[2]));
  } else if constexpr (TypeParam::kDims == 4) {
    EXPECT_THAT(vec.x, Eq(arr[0]));
    EXPECT_THAT(vec.y, Eq(arr[1]));
    EXPECT_THAT(vec.z, Eq(arr[2]));
    EXPECT_THAT(vec.w, Eq(arr[3]));
  }
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(vec[i], arr[i]);
  }
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(vec.data[i], arr[i]);
  }
}

TYPED_TEST(VectorTest, Equal) {
  auto arr = RandArray<TypeParam>();

  TypeParam v1(arr.data());
  TypeParam v2(arr.data());
  EXPECT_TRUE(v1 == v2);
}

TYPED_TEST(VectorTest, NotEqual) {
  auto arr1 = RandArray<TypeParam>();
  auto arr2 = RandArray<TypeParam>();
  arr1[0] = 1;
  arr2[0] = 2;

  TypeParam v1(arr1.data());
  TypeParam v2(arr2.data());
  EXPECT_TRUE(v1 != v2);
}

TYPED_TEST(VectorTest, Comparison) {
  auto arr1 = RandArray<TypeParam>();
  auto arr2 = RandArray<TypeParam>();
  arr1[0] = 1;
  arr2[0] = 2;

  TypeParam v1(arr1.data());
  TypeParam v2(arr2.data());

  TypeParam min = Min(v1, v2) - TypeParam(1);
  TypeParam max = Max(v1, v2) + TypeParam(1);
  EXPECT_TRUE(min < max);
  EXPECT_TRUE(max > min);
  EXPECT_TRUE(min <= min);
  EXPECT_TRUE(max >= max);
}

TYPED_TEST(VectorTest, Negate) {
  auto arr = RandArray<TypeParam>();

  TypeParam vec(arr.data());
  TypeParam neg(-vec);
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(neg.data[i], Eq(-arr[i]));
  }
}

TYPED_TEST(VectorTest, Add) {
  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();
  const auto scalar = RandScalar<TypeParam>();

  const TypeParam v1(arr1.data());
  const TypeParam v2(arr2.data());

  TypeParam vec_vs(v1 + scalar);
  TypeParam vec_sv(scalar + v2);
  TypeParam vec_vv(v1 + v2);
  TypeParam vec_as(v1);
  vec_as += scalar;
  TypeParam vec_av(v1);
  vec_av += v2;

  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(vec_vs[i], Eq(arr1[i] + scalar));
    EXPECT_THAT(vec_sv[i], Eq(scalar + arr2[i]));
    EXPECT_THAT(vec_vv[i], Eq(arr1[i] + arr2[i]));
    EXPECT_THAT(vec_as[i], Eq(arr1[i] + scalar));
    EXPECT_THAT(vec_av[i], Eq(arr1[i] + arr2[i]));
  }
}

TYPED_TEST(VectorTest, Sub) {
  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();
  const auto scalar = RandScalar<TypeParam>();

  const TypeParam v1(arr1.data());
  const TypeParam v2(arr2.data());

  TypeParam vec_vs(v1 - scalar);
  TypeParam vec_sv(scalar - v2);
  TypeParam vec_vv(v1 - v2);
  TypeParam vec_as(v1);
  vec_as -= scalar;
  TypeParam vec_av(v1);
  vec_av -= v2;

  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(vec_vs[i], Eq(arr1[i] - scalar));
    EXPECT_THAT(vec_sv[i], Eq(scalar - arr2[i]));
    EXPECT_THAT(vec_vv[i], Eq(arr1[i] - arr2[i]));
    EXPECT_THAT(vec_as[i], Eq(arr1[i] - scalar));
    EXPECT_THAT(vec_av[i], Eq(arr1[i] - arr2[i]));
  }
}

TYPED_TEST(VectorTest, Mul) {
  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();
  const auto scalar = RandScalar<TypeParam>();

  const TypeParam v1(arr1.data());
  const TypeParam v2(arr2.data());

  TypeParam vec_vs(v1 * scalar);
  TypeParam vec_sv(scalar * v2);
  TypeParam vec_vv(v1 * v2);
  TypeParam vec_as(v1);
  vec_as *= scalar;
  TypeParam vec_av(v1);
  vec_av *= v2;

  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(vec_vs[i], Eq(arr1[i] * scalar));
    EXPECT_THAT(vec_sv[i], Eq(scalar * arr2[i]));
    EXPECT_THAT(vec_vv[i], Eq(arr1[i] * arr2[i]));
    EXPECT_THAT(vec_as[i], Eq(arr1[i] * scalar));
    EXPECT_THAT(vec_av[i], Eq(arr1[i] * arr2[i]));
  }
}

TYPED_TEST(VectorTest, Div) {
  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();
  const auto scalar = RandScalar<TypeParam>();

  const TypeParam v1(arr1.data());
  const TypeParam v2(arr2.data());

  TypeParam vec_vs(v1 / scalar);
  TypeParam vec_sv(scalar / v2);
  TypeParam vec_vv(v1 / v2);
  TypeParam vec_as(v1);
  vec_as /= scalar;
  TypeParam vec_av(v1);
  vec_av /= v2;

  const auto inv = typename TypeParam::Scalar(1) / scalar;
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(vec_vs[i], Eq(arr1[i] * inv));
    EXPECT_THAT(vec_sv[i], Eq(scalar / arr2[i]));
    EXPECT_THAT(vec_vv[i], Eq(arr1[i] / arr2[i]));
    EXPECT_THAT(vec_as[i], Eq(arr1[i] * inv));
    EXPECT_THAT(vec_av[i], Eq(arr1[i] / arr2[i]));
  }
}

TYPED_TEST(VectorTest, Dot) {
  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();

  const TypeParam v1(arr1.data());
  const TypeParam v2(arr2.data());

  using Scalar = typename TypeParam::Scalar;
  Scalar expected = 0;
  for (int i = 0; i < TypeParam::kDims; ++i) {
    expected += arr1[i] * arr2[i];
  }

  if constexpr (std::is_same_v<Scalar, float>) {
    EXPECT_THAT(Dot(v1, v2), FloatEq(expected));
    EXPECT_THAT(v1.Dot(v2), FloatEq(expected));
  } else {
    EXPECT_THAT(Dot(v1, v2), Eq(expected));
    EXPECT_THAT(v1.Dot(v2), Eq(expected));
  }
}

TYPED_TEST(VectorTest, Hadamard) {
  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();

  const TypeParam v1(arr1.data());
  const TypeParam v2(arr2.data());

  const TypeParam res1 = Hadamard(v1, v2);
  const TypeParam res2 = v1.Hadamard(v2);
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(res1[i], Eq(arr1[i] * arr2[i]));
    EXPECT_THAT(res2[i], Eq(arr1[i] * arr2[i]));
  }
}

TYPED_TEST(VectorTest, Cross) {
  // Only supported for 3D float vectors.
  using Scalar = typename TypeParam::Scalar;
  if constexpr (std::is_same_v<Scalar, float> && TypeParam::kDims == 3) {
    TypeParam v1(Scalar(1.1), Scalar(4.5), Scalar(9.8));
    TypeParam v2(Scalar(-1.4), Scalar(9.5), Scalar(3.2));
    v1.SetNormalized();
    v2.SetNormalized();
    const TypeParam cross1 = Cross(v1, v2);
    const TypeParam cross2 = v1.Cross(v2);
    // This will verify that v1 * (v1 x v2) and v2 * (v1 x v2) are 0.
    auto dot1 = Dot(cross1, v1);
    auto dot2 = Dot(cross1, v2);
    auto dot3 = Dot(cross2, v1);
    auto dot4 = Dot(cross2, v2);
    EXPECT_THAT(dot1, FloatNear(0, 1.0e-8f));
    EXPECT_THAT(dot2, FloatNear(0, 1.0e-8f));
    EXPECT_THAT(dot3, FloatNear(0, 1.0e-8f));
    EXPECT_THAT(dot4, FloatNear(0, 1.0e-8f));
  }
}

TYPED_TEST(VectorTest, Normalized) {
  using Scalar = typename TypeParam::Scalar;
  if constexpr (std::is_same_v<Scalar, float>) {
    const auto arr = RandArray<TypeParam>();
    const TypeParam vec(arr.data());
    const TypeParam norm1 = vec.Normalized();
    const TypeParam norm2 = Normalized(vec);
    EXPECT_THAT(norm1.Length(), FloatNear(1.0f, 1.0e-6f));
    EXPECT_THAT(norm2.Length(), FloatNear(1.0f, 1.0e-6f));
  }
}

TYPED_TEST(VectorTest, Length) {
  using Scalar = typename TypeParam::Scalar;
  if constexpr (std::is_same_v<Scalar, float>) {
    const auto arr = RandArray<TypeParam>();
    const TypeParam vec(arr.data());

    Scalar expect = 0;
    for (int i = 0; i < TypeParam::kDims; ++i) {
      expect += (arr[i] * arr[i]);
    }

    EXPECT_THAT(vec.Length(), FloatEq(sqrt(expect)));
    EXPECT_THAT(vec.LengthSquared(), FloatEq(expect));
    EXPECT_THAT(Length(vec), FloatEq(sqrt(expect)));
    EXPECT_THAT(LengthSquared(vec), FloatEq(expect));
  }
}

TYPED_TEST(VectorTest, Min) {
  using Scalar = typename TypeParam::Scalar;

  const Scalar arr1[] = {1, 2, 3, 4, 5};
  const Scalar arr2[] = {-5, -4, -3, -2, -1};
  const TypeParam v1(arr1);
  const TypeParam v2(arr2);

  // Ensure both min(a, b) and min(b, a) return the same value.
  const TypeParam min1 = Min(v1, v2);
  const TypeParam min2 = Min(v2, v1);
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(min1[i], Eq(arr2[i]));
    EXPECT_THAT(min2[i], Eq(arr2[i]));
  }

  // Check for interleaved min/max values.
  const Scalar arr3[] = {0, 2, 0, 4, 0};
  const Scalar arr4[] = {1, 0, 3, 0, 5};
  const TypeParam v3(arr3);
  const TypeParam v4(arr4);

  const TypeParam min3 = Min(v3, v4);
  for (int i = 0; i < TypeParam::kDims; ++i) {
    auto* expect = (i % 2 == 0) ? arr3 : arr4;
    EXPECT_THAT(min3[i], Eq(expect[i]));
  }
}

TYPED_TEST(VectorTest, Max) {
  using Scalar = typename TypeParam::Scalar;

  const Scalar arr1[] = {1, 2, 3, 4, 5};
  const Scalar arr2[] = {-5, -4, -3, -2, -1};
  const TypeParam v1(arr1);
  const TypeParam v2(arr2);

  // Ensure both min(a, b) and min(b, a) return the same value.
  const TypeParam max1 = Max(v1, v2);
  const TypeParam max2 = Max(v2, v1);
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(max1[i], Eq(arr1[i]));
    EXPECT_THAT(max2[i], Eq(arr1[i]));
  }

  // Check for interleaved min/max values.
  const Scalar arr3[] = {0, 2, 0, 4, 0};
  const Scalar arr4[] = {1, 0, 3, 0, 5};
  const TypeParam v3(arr3);
  const TypeParam v4(arr4);

  const TypeParam max3 = Max(v3, v4);
  for (int i = 0; i < TypeParam::kDims; ++i) {
    auto* expect = (i % 2 == 0) ? arr4 : arr3;
    EXPECT_THAT(max3[i], Eq(expect[i]));
  }
}

TYPED_TEST(VectorTest, Clamp) {
  using Scalar = typename TypeParam::Scalar;

  const TypeParam min(Scalar(-1));
  const TypeParam max(Scalar(8));
  const TypeParam inside(Scalar(7));
  const TypeParam above(Scalar(9));
  const TypeParam below(Scalar(-11));

  EXPECT_THAT(Clamp(inside, min, max), Eq(inside));
  EXPECT_THAT(Clamp(inside, min, max), Eq(inside));
  EXPECT_THAT(Clamp(above, min, max), Eq(max));
  EXPECT_THAT(Clamp(below, min, max), Eq(min));
  EXPECT_THAT(Clamp(max, min, max), Eq(max));
  EXPECT_THAT(Clamp(min, min, max), Eq(min));
}

TYPED_TEST(VectorTest, Lerp) {
  using Scalar = typename TypeParam::Scalar;
  if constexpr (std::is_same_v<Scalar, float>) {
    const auto arr1 = RandArray<TypeParam>();
    const auto arr2 = RandArray<TypeParam>();

    const TypeParam v1(arr1.data());
    const TypeParam v2(arr2.data());

    const TypeParam lerp_zero = Lerp(v1, v2, Scalar(0));
    const TypeParam lerp_one = Lerp(v1, v2, Scalar(1));
    const TypeParam lerp_half = Lerp(v1, v2, Scalar(0.5));
    for (int i = 0; i < TypeParam::kDims; ++i) {
      EXPECT_THAT(lerp_zero[i], Eq(arr1[i]));
      EXPECT_THAT(lerp_half[i], Eq((arr1[i] + arr2[i]) / Scalar(2)));
      EXPECT_THAT(lerp_one[i], Eq(arr2[i]));
    }
  }
}

TYPED_TEST(VectorTest, DistanceBetween) {
  using Scalar = typename TypeParam::Scalar;
  if constexpr (std::is_same_v<Scalar, float>) {
    const auto arr1 = RandArray<TypeParam>();
    const auto arr2 = RandArray<TypeParam>();
    const TypeParam v1(arr1.data());
    const TypeParam v2(arr2.data());

    Scalar expect = 0;
    for (int i = 0; i < TypeParam::kDims; ++i) {
      const auto diff = arr2[i] - arr1[i];
      expect += (diff * diff);
    }

    EXPECT_THAT(DistanceBetween(v1, v2), FloatEq(sqrt(expect)));
    EXPECT_THAT(DistanceSquaredBetween(v1, v2), FloatEq(expect));
  }
}

TYPED_TEST(VectorTest, AngleBetween) {
  using Scalar = typename TypeParam::Scalar;
  if constexpr (std::is_same_v<Scalar, float>) {
    if constexpr (TypeParam::kDims == 2) {
      const TypeParam v1(0.0f, 1.0f);
      const TypeParam v2(1.0f, 0.0f);
      EXPECT_THAT(AngleBetween(v1, v2), FloatEq(M_PI * 0.5f));

      const TypeParam v3(1.0f, 1.0f);
      const TypeParam v4(0.0f, -1.0f);
      EXPECT_THAT(AngleBetween(v3, v4), FloatEq(M_PI * 0.75f));
    } else if constexpr (TypeParam::kDims == 3) {
      const TypeParam v1(0.0f, 0.0f, 1.0f);
      const TypeParam v2(0.0f, 1.0f, 0.0f);
      EXPECT_THAT(AngleBetween(v1, v2), FloatEq(M_PI * 0.5f));

      const TypeParam v3(1.0f, 2.0f, 3.0f);
      const TypeParam v4(-10.0f, 3.0f, -1.0f);
      EXPECT_THAT(AngleBetween(v3, v4), FloatEq(1.75013259f));

      const TypeParam v5(1.0f, 2.0f, 3.0f);
      const TypeParam v6(-1.0f, -2.0f, -3.0f);
      EXPECT_THAT(AngleBetween(v5, v6), FloatNear(M_PI, 1.0e-3f));
    }
  }
}

TYPED_TEST(VectorTest, Swizzle) {
  const auto arr = RandArray<TypeParam>();
  const TypeParam vec(arr.data());

  if constexpr (TypeParam::kDims > 2) {
    const auto xy = vec.xy();
    EXPECT_THAT(xy[0], Eq(arr[0]));
    EXPECT_THAT(xy[1], Eq(arr[1]));
  }
  if constexpr (TypeParam::kDims > 3) {
    const auto xyz = vec.xyz();
    EXPECT_THAT(xyz[0], Eq(arr[0]));
    EXPECT_THAT(xyz[1], Eq(arr[1]));
    EXPECT_THAT(xyz[2], Eq(arr[2]));
  }
  if constexpr (TypeParam::kDims > 4) {
    const auto zw = vec.zw();
    EXPECT_THAT(zw[0], Eq(arr[2]));
    EXPECT_THAT(zw[1], Eq(arr[3]));
  }
}

TYPED_TEST(VectorTest, Constants) {
  const auto zero = TypeParam::Zero();
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(zero[i], Eq(0));
  }

  const auto one = TypeParam::One();
  for (int i = 0; i < TypeParam::kDims; ++i) {
    EXPECT_THAT(one[i], Eq(1));
  }

  if constexpr (TypeParam::kDims >= 2 && TypeParam::kDims <= 4) {
    const auto axis = TypeParam::XAxis();
    for (int i = 0; i < TypeParam::kDims; ++i) {
      EXPECT_THAT(axis[i], Eq(i == 0 ? 1 : 0));
    }
  }
  if constexpr (TypeParam::kDims >= 2 && TypeParam::kDims <= 4) {
    const auto axis = TypeParam::YAxis();
    for (int i = 0; i < TypeParam::kDims; ++i) {
      EXPECT_THAT(axis[i], Eq(i == 1 ? 1 : 0));
    }
  }
  if constexpr (TypeParam::kDims >= 3 && TypeParam::kDims <= 4) {
    const auto axis = TypeParam::ZAxis();
    for (int i = 0; i < TypeParam::kDims; ++i) {
      EXPECT_THAT(axis[i], Eq(i == 2 ? 1 : 0));
    }
  }
  if constexpr (TypeParam::kDims >= 4 && TypeParam::kDims <= 4) {
    const auto axis = TypeParam::WAxis();
    for (int i = 0; i < TypeParam::kDims; ++i) {
      EXPECT_THAT(axis[i], Eq(i == 3 ? 1 : 0));
    }
  }
}

TYPED_TEST(VectorTest, ConstExprs) {
  using Scalar = typename TypeParam::Scalar;

  if constexpr (!TypeParam::kSimd) {
    if constexpr (TypeParam::Zero()[0] != Scalar(0)) {
      EXPECT_TRUE(false);
    }
    if constexpr (TypeParam::One()[0] != Scalar(1)) {
      EXPECT_TRUE(false);
    }
    if constexpr (TypeParam::kDims >= 2 && TypeParam::kDims <= 4) {
      if constexpr (TypeParam::XAxis()[0] != Scalar(1)) {
        EXPECT_TRUE(false);
      }
    }
    if constexpr (TypeParam::kDims >= 2 && TypeParam::kDims <= 4) {
      if constexpr (TypeParam::YAxis()[0] != Scalar(0)) {
        EXPECT_TRUE(false);
      }
    }
    if constexpr (TypeParam::kDims >= 3 && TypeParam::kDims <= 4) {
      if constexpr (TypeParam::ZAxis()[0] != Scalar(0)) {
        EXPECT_TRUE(false);
      }
    }
    if constexpr (TypeParam::kDims >= 4 && TypeParam::kDims <= 4) {
      if constexpr (TypeParam::WAxis()[0] != Scalar(0)) {
        EXPECT_TRUE(false);
      }
    }
  }
}

}  // namespace
}  // namespace redux
