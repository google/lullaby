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
#include "redux/modules/math/constants.h"
#include "redux/modules/math/detail/matrix_layout.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/vector.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatEq;
using ::testing::FloatNear;

// Test types.
using SisdMat22f = Matrix<float, 2, 2, false>;
using SisdMat33f = Matrix<float, 3, 3, false>;
using SisdMat44f = Matrix<float, 4, 4, false>;
using SisdMat55f = Matrix<float, 5, 5, false>;
using SimdMat22f = Matrix<float, 2, 2, true>;
using SimdMat33f = Matrix<float, 3, 3, true>;
using SimdMat44f = Matrix<float, 4, 4, true>;
using SimdMat55f = Matrix<float, 5, 5, true>;

using MatrixTestTypes =
    ::testing::Types<SisdMat22f, SisdMat33f, SisdMat44f, SisdMat55f, SimdMat22f,
                     SimdMat33f, SimdMat44f, SimdMat55f>;

template <typename T>
class MatrixTest : public ::testing::Test {};

TYPED_TEST_SUITE(MatrixTest, MatrixTestTypes);

template <typename MatType>
auto RandScalar() {
  using Scalar = typename MatType::Scalar;
  auto rand_value = (rand() / static_cast<Scalar>(RAND_MAX) * 100.0);
  return Scalar(1 + rand_value);
}

// Creates an std::array with the same type and dimensionality of the MatType
// and populated with a random set of numbers.
template <typename MatType>
auto RandArray() {
  std::array<typename MatType::Scalar, MatType::kRows * MatType::kCols> arr;
  for (size_t i = 0; i < arr.size(); ++i) {
    arr[i] = RandScalar<MatType>();
  }
  return arr;
}

template <typename MatType, typename T>
auto ArrayAt(const T& arr, int row, int col) {
  const int index = (col * MatType::kRows) + row;
  return arr[index];
}

// Returns one of the numbers:
//   offset, offset + width/max, offset + 2*width/max, ... , offset +
//   (max-1)*width/max
// For each i=0..max-1, the number returned is different.
//
// The order of the numbers is determined by 'prime' which should be a prime
// number > max.
template <class T>
static T WellSpacedNumber(int i, int prime, int max, T width, T offset) {
  // Reorder the i's from 0..(d-1) by using a prime number.
  // The numbers get reordered (i.e. no duplicates) because 'd' and 'prime' are
  // relatively prime.
  const int remapped = ((i + 1) * prime) % max;

  // Map to [0,1) range. That is, inclusive of 0, exclusive of 1.
  const T zero_to_one = static_cast<T>(remapped) / static_cast<T>(max);

  // Map to [offset, offset + width) range.
  const T offset_and_scaled = zero_to_one * width + offset;
  return offset_and_scaled;
}

// Generates a (hopefully) invertible matrix.
//
// Adjust each element of an identity matrix by a small amount. The amount must
// be small so that the matrix stays reasonably close to the identity. Matrices
// become progressively less numerically stable the further the get from
// identity.
template <class TypeParam>
auto InvertibleMatrix() {
  using Scalar = typename TypeParam::Scalar;
  const int dim = TypeParam::kRows;

  TypeParam matrix = TypeParam::Identity();
  for (int i = 0; i < dim; ++i) {
    // The width and offset constants are arbitrary. We do want to keep the
    // pseudo-random values centered near 0 though.
    const auto rand_i = WellSpacedNumber<Scalar>(i, 7, dim, 0.8f, -0.33f);
    for (int j = 0; j < dim; ++j) {
      const auto rand_j = WellSpacedNumber<Scalar>(j, 13, dim, 0.6f, -0.4f);
      matrix(i, j) += rand_i * rand_j;
    }
  }
  return matrix;
}

TEST(MatrixTest, LayoutSize) {
  static_assert(sizeof(SisdMat22f) == sizeof(float) * 2 * 2);
  static_assert(sizeof(SisdMat33f) == sizeof(float) * 3 * 3);
  static_assert(sizeof(SisdMat44f) == sizeof(float) * 4 * 4);
  static_assert(sizeof(SisdMat55f) == sizeof(float) * 5 * 5);
  static_assert(sizeof(SimdMat22f) == sizeof(float) * 4 * 2);
  static_assert(sizeof(SimdMat33f) == sizeof(float) * 4 * 3);
  static_assert(sizeof(SimdMat44f) == sizeof(float) * 4 * 4);
  static_assert(sizeof(SimdMat55f) == sizeof(float) * 5 * 5);
}

TYPED_TEST(MatrixTest, InitZero) {
  const TypeParam mat;
  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      EXPECT_THAT(mat.cols[j][i], Eq(0));
    }
  }
}

TYPED_TEST(MatrixTest, InitFromScalar) {
  const auto value = RandScalar<TypeParam>();

  const TypeParam mat(value);
  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      EXPECT_THAT(mat.cols[j][i], Eq(value));
    }
  }
}

TYPED_TEST(MatrixTest, InitFromArray) {
  auto arr = RandArray<TypeParam>();

  const TypeParam mat(arr.data());
  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      EXPECT_THAT(mat.cols[j][i], Eq(ArrayAt<TypeParam>(arr, i, j)));
    }
  }
}

TEST(VectorTest, InitFromDifferentType) {
  {
    const SisdMat44f sisd4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                           16);
    const SimdMat22f simd2(sisd4);
    EXPECT_THAT(simd2.cols[0][0], Eq(sisd4.cols[0][0]));
    EXPECT_THAT(simd2.cols[0][1], Eq(sisd4.cols[0][1]));
    EXPECT_THAT(simd2.cols[1][0], Eq(sisd4.cols[1][0]));
    EXPECT_THAT(simd2.cols[1][1], Eq(sisd4.cols[1][1]));
  }
  {
    const SisdMat22f sisd2(1, 2, 3, 4);
    const SimdMat44f simd4(sisd2);
    EXPECT_THAT(simd4.cols[0][0], Eq(sisd2.cols[0][0]));
    EXPECT_THAT(simd4.cols[0][1], Eq(sisd2.cols[0][1]));
    EXPECT_THAT(simd4.cols[0][2], Eq(0));
    EXPECT_THAT(simd4.cols[0][3], Eq(0));
    EXPECT_THAT(simd4.cols[1][0], Eq(sisd2.cols[1][0]));
    EXPECT_THAT(simd4.cols[1][1], Eq(sisd2.cols[1][1]));
    EXPECT_THAT(simd4.cols[1][2], Eq(0));
    EXPECT_THAT(simd4.cols[1][3], Eq(0));
    EXPECT_THAT(simd4.cols[2][0], Eq(0));
    EXPECT_THAT(simd4.cols[2][1], Eq(0));
    EXPECT_THAT(simd4.cols[2][2], Eq(1));
    EXPECT_THAT(simd4.cols[2][3], Eq(0));
    EXPECT_THAT(simd4.cols[3][0], Eq(0));
    EXPECT_THAT(simd4.cols[3][1], Eq(0));
    EXPECT_THAT(simd4.cols[3][2], Eq(0));
    EXPECT_THAT(simd4.cols[3][3], Eq(1));
  }
}

TYPED_TEST(MatrixTest, InitMatrix22) {
  if constexpr (TypeParam::kRows == 2 && TypeParam::kCols == 2) {
    const auto s00 = RandScalar<TypeParam>();
    const auto s01 = RandScalar<TypeParam>();
    const auto s10 = RandScalar<TypeParam>();
    const auto s11 = RandScalar<TypeParam>();

    const TypeParam mat(s00, s01, s10, s11);
    EXPECT_THAT(mat(0, 0), Eq(s00));
    EXPECT_THAT(mat(0, 1), Eq(s01));
    EXPECT_THAT(mat(1, 0), Eq(s10));
    EXPECT_THAT(mat(1, 1), Eq(s11));
  }
}

TYPED_TEST(MatrixTest, InitMatrix33) {
  if constexpr (TypeParam::kRows == 3 && TypeParam::kCols == 3) {
    const auto s00 = RandScalar<TypeParam>();
    const auto s01 = RandScalar<TypeParam>();
    const auto s02 = RandScalar<TypeParam>();
    const auto s10 = RandScalar<TypeParam>();
    const auto s11 = RandScalar<TypeParam>();
    const auto s12 = RandScalar<TypeParam>();
    const auto s20 = RandScalar<TypeParam>();
    const auto s21 = RandScalar<TypeParam>();
    const auto s22 = RandScalar<TypeParam>();

    const TypeParam mat(s00, s01, s02, s10, s11, s12, s20, s21, s22);
    EXPECT_THAT(mat(0, 0), Eq(s00));
    EXPECT_THAT(mat(0, 1), Eq(s01));
    EXPECT_THAT(mat(0, 2), Eq(s02));
    EXPECT_THAT(mat(1, 0), Eq(s10));
    EXPECT_THAT(mat(1, 1), Eq(s11));
    EXPECT_THAT(mat(1, 2), Eq(s12));
    EXPECT_THAT(mat(2, 0), Eq(s20));
    EXPECT_THAT(mat(2, 1), Eq(s21));
    EXPECT_THAT(mat(2, 2), Eq(s22));
  }
}

TYPED_TEST(MatrixTest, InitMatrix44) {
  if constexpr (TypeParam::kRows == 4 && TypeParam::kCols == 4) {
    const auto s00 = RandScalar<TypeParam>();
    const auto s01 = RandScalar<TypeParam>();
    const auto s02 = RandScalar<TypeParam>();
    const auto s03 = RandScalar<TypeParam>();
    const auto s10 = RandScalar<TypeParam>();
    const auto s11 = RandScalar<TypeParam>();
    const auto s12 = RandScalar<TypeParam>();
    const auto s13 = RandScalar<TypeParam>();
    const auto s20 = RandScalar<TypeParam>();
    const auto s21 = RandScalar<TypeParam>();
    const auto s22 = RandScalar<TypeParam>();
    const auto s23 = RandScalar<TypeParam>();
    const auto s30 = RandScalar<TypeParam>();
    const auto s31 = RandScalar<TypeParam>();
    const auto s32 = RandScalar<TypeParam>();
    const auto s33 = RandScalar<TypeParam>();

    const TypeParam mat(s00, s01, s02, s03, s10, s11, s12, s13, s20, s21, s22,
                        s23, s30, s31, s32, s33);
    EXPECT_THAT(mat(0, 0), Eq(s00));
    EXPECT_THAT(mat(0, 1), Eq(s01));
    EXPECT_THAT(mat(0, 2), Eq(s02));
    EXPECT_THAT(mat(0, 3), Eq(s03));
    EXPECT_THAT(mat(1, 0), Eq(s10));
    EXPECT_THAT(mat(1, 1), Eq(s11));
    EXPECT_THAT(mat(1, 2), Eq(s12));
    EXPECT_THAT(mat(1, 3), Eq(s13));
    EXPECT_THAT(mat(2, 0), Eq(s20));
    EXPECT_THAT(mat(2, 1), Eq(s21));
    EXPECT_THAT(mat(2, 2), Eq(s22));
    EXPECT_THAT(mat(2, 3), Eq(s23));
    EXPECT_THAT(mat(3, 0), Eq(s30));
    EXPECT_THAT(mat(3, 1), Eq(s31));
    EXPECT_THAT(mat(3, 2), Eq(s32));
    EXPECT_THAT(mat(3, 3), Eq(s33));
  }
}

TYPED_TEST(MatrixTest, Copy) {
  auto arr = RandArray<TypeParam>();

  const TypeParam m1(arr.data());
  const TypeParam m2(m1);

  for (int cc = 0; cc < TypeParam::kCols; ++cc) {
    for (int rr = 0; rr < TypeParam::kRows; ++rr) {
      EXPECT_THAT(m1.cols[cc][rr], Eq(m2.cols[cc][rr]));
    }
  }
}

TYPED_TEST(MatrixTest, Assign) {
  auto arr = RandArray<TypeParam>();

  const TypeParam m1(arr.data());
  TypeParam m2;
  m2 = m1;

  for (int cc = 0; cc < TypeParam::kCols; ++cc) {
    for (int rr = 0; rr < TypeParam::kRows; ++rr) {
      EXPECT_THAT(m1.cols[cc][rr], Eq(m2.cols[cc][rr]));
    }
  }
}

TYPED_TEST(MatrixTest, Accessors) {
  auto arr = RandArray<TypeParam>();
  const TypeParam mat(arr.data());

  if constexpr (TypeParam::kRows == 2 && TypeParam::kCols == 2) {
    EXPECT_THAT(mat.m00, Eq(ArrayAt<TypeParam>(arr, 0, 0)));
    EXPECT_THAT(mat.m01, Eq(ArrayAt<TypeParam>(arr, 0, 1)));
    EXPECT_THAT(mat.m10, Eq(ArrayAt<TypeParam>(arr, 1, 0)));
    EXPECT_THAT(mat.m11, Eq(ArrayAt<TypeParam>(arr, 1, 1)));
  } else if constexpr (TypeParam::kRows == 3 && TypeParam::kCols == 3) {
    EXPECT_THAT(mat.m00, Eq(ArrayAt<TypeParam>(arr, 0, 0)));
    EXPECT_THAT(mat.m01, Eq(ArrayAt<TypeParam>(arr, 0, 1)));
    EXPECT_THAT(mat.m02, Eq(ArrayAt<TypeParam>(arr, 0, 2)));
    EXPECT_THAT(mat.m10, Eq(ArrayAt<TypeParam>(arr, 1, 0)));
    EXPECT_THAT(mat.m11, Eq(ArrayAt<TypeParam>(arr, 1, 1)));
    EXPECT_THAT(mat.m12, Eq(ArrayAt<TypeParam>(arr, 1, 2)));
    EXPECT_THAT(mat.m20, Eq(ArrayAt<TypeParam>(arr, 2, 0)));
    EXPECT_THAT(mat.m21, Eq(ArrayAt<TypeParam>(arr, 2, 1)));
    EXPECT_THAT(mat.m22, Eq(ArrayAt<TypeParam>(arr, 2, 2)));
  } else if constexpr (TypeParam::kRows == 4 && TypeParam::kCols == 3) {
    EXPECT_THAT(mat.m00, Eq(ArrayAt<TypeParam>(arr, 0, 0)));
    EXPECT_THAT(mat.m01, Eq(ArrayAt<TypeParam>(arr, 0, 1)));
    EXPECT_THAT(mat.m02, Eq(ArrayAt<TypeParam>(arr, 0, 2)));
    EXPECT_THAT(mat.m10, Eq(ArrayAt<TypeParam>(arr, 1, 0)));
    EXPECT_THAT(mat.m11, Eq(ArrayAt<TypeParam>(arr, 1, 1)));
    EXPECT_THAT(mat.m12, Eq(ArrayAt<TypeParam>(arr, 1, 2)));
    EXPECT_THAT(mat.m20, Eq(ArrayAt<TypeParam>(arr, 2, 0)));
    EXPECT_THAT(mat.m21, Eq(ArrayAt<TypeParam>(arr, 2, 1)));
    EXPECT_THAT(mat.m22, Eq(ArrayAt<TypeParam>(arr, 2, 2)));
    EXPECT_THAT(mat.m30, Eq(ArrayAt<TypeParam>(arr, 3, 0)));
    EXPECT_THAT(mat.m31, Eq(ArrayAt<TypeParam>(arr, 3, 1)));
    EXPECT_THAT(mat.m32, Eq(ArrayAt<TypeParam>(arr, 3, 2)));
  } else if constexpr (TypeParam::kRows == 4 && TypeParam::kCols == 4) {
    EXPECT_THAT(mat.m00, Eq(ArrayAt<TypeParam>(arr, 0, 0)));
    EXPECT_THAT(mat.m01, Eq(ArrayAt<TypeParam>(arr, 0, 1)));
    EXPECT_THAT(mat.m02, Eq(ArrayAt<TypeParam>(arr, 0, 2)));
    EXPECT_THAT(mat.m03, Eq(ArrayAt<TypeParam>(arr, 0, 3)));
    EXPECT_THAT(mat.m10, Eq(ArrayAt<TypeParam>(arr, 1, 0)));
    EXPECT_THAT(mat.m11, Eq(ArrayAt<TypeParam>(arr, 1, 1)));
    EXPECT_THAT(mat.m12, Eq(ArrayAt<TypeParam>(arr, 1, 2)));
    EXPECT_THAT(mat.m13, Eq(ArrayAt<TypeParam>(arr, 1, 3)));
    EXPECT_THAT(mat.m20, Eq(ArrayAt<TypeParam>(arr, 2, 0)));
    EXPECT_THAT(mat.m21, Eq(ArrayAt<TypeParam>(arr, 2, 1)));
    EXPECT_THAT(mat.m22, Eq(ArrayAt<TypeParam>(arr, 2, 2)));
    EXPECT_THAT(mat.m23, Eq(ArrayAt<TypeParam>(arr, 2, 3)));
    EXPECT_THAT(mat.m30, Eq(ArrayAt<TypeParam>(arr, 3, 0)));
    EXPECT_THAT(mat.m31, Eq(ArrayAt<TypeParam>(arr, 3, 1)));
    EXPECT_THAT(mat.m32, Eq(ArrayAt<TypeParam>(arr, 3, 2)));
    EXPECT_THAT(mat.m33, Eq(ArrayAt<TypeParam>(arr, 3, 3)));
  }
  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      EXPECT_THAT(mat(i, j), Eq(ArrayAt<TypeParam>(arr, i, j)));
    }
  }
  for (int i = 0; i < TypeParam::kRows; ++i) {
    const auto row = mat.Row(i);
    for (int j = 0; j < TypeParam::kCols; ++j) {
      EXPECT_THAT(row[j], Eq(ArrayAt<TypeParam>(arr, i, j)));
    }
  }
  for (int j = 0; j < TypeParam::kCols; ++j) {
    const auto col = mat.Column(j);
    for (int i = 0; i < TypeParam::kRows; ++i) {
      EXPECT_THAT(col[i], Eq(ArrayAt<TypeParam>(arr, i, j)));
    }
  }
}

TYPED_TEST(MatrixTest, Equal) {
  auto arr = RandArray<TypeParam>();

  TypeParam m1(arr.data());
  TypeParam m2(arr.data());
  EXPECT_TRUE(m1 == m2);
}

TYPED_TEST(MatrixTest, NotEqual) {
  auto arr1 = RandArray<TypeParam>();
  auto arr2 = RandArray<TypeParam>();
  arr1[0] = 1;
  arr2[0] = 2;

  TypeParam m1(arr1.data());
  TypeParam m2(arr2.data());
  EXPECT_TRUE(m1 != m2);
}

TYPED_TEST(MatrixTest, Negate) {
  auto arr = RandArray<TypeParam>();

  TypeParam mat(arr.data());
  TypeParam neg(-mat);
  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      const auto expect = -ArrayAt<TypeParam>(arr, i, j);
      EXPECT_THAT(neg.cols[j][i], Eq(expect));
    }
  }
}

TYPED_TEST(MatrixTest, Add) {
  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();
  const auto scalar = RandScalar<TypeParam>();

  const TypeParam m1(arr1.data());
  const TypeParam m2(arr2.data());

  TypeParam mat_ms(m1 + scalar);
  TypeParam mat_sm(scalar + m2);
  TypeParam mat_mm(m1 + m2);
  TypeParam mat_as(m1);
  mat_as += scalar;
  TypeParam mat_am(m1);
  mat_am += m2;

  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      const auto v1 = ArrayAt<TypeParam>(arr1, i, j);
      const auto v2 = ArrayAt<TypeParam>(arr2, i, j);

      EXPECT_THAT(mat_ms.cols[j][i], Eq(v1 + scalar));
      EXPECT_THAT(mat_sm.cols[j][i], Eq(scalar + v2));
      EXPECT_THAT(mat_mm.cols[j][i], Eq(v1 + v2));
      EXPECT_THAT(mat_as.cols[j][i], Eq(v1 + scalar));
      EXPECT_THAT(mat_am.cols[j][i], Eq(v1 + v2));
    }
  }
}

TYPED_TEST(MatrixTest, Sub) {
  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();
  const auto scalar = RandScalar<TypeParam>();

  const TypeParam m1(arr1.data());
  const TypeParam m2(arr2.data());

  TypeParam mat_ms(m1 - scalar);
  TypeParam mat_sm(scalar - m2);
  TypeParam mat_mm(m1 - m2);
  TypeParam mat_as(m1);
  mat_as -= scalar;
  TypeParam mat_am(m1);
  mat_am -= m2;

  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      const auto v1 = ArrayAt<TypeParam>(arr1, i, j);
      const auto v2 = ArrayAt<TypeParam>(arr2, i, j);

      EXPECT_THAT(mat_ms.cols[j][i], Eq(v1 - scalar));
      EXPECT_THAT(mat_sm.cols[j][i], Eq(scalar - v2));
      EXPECT_THAT(mat_mm.cols[j][i], Eq(v1 - v2));
      EXPECT_THAT(mat_as.cols[j][i], Eq(v1 - scalar));
      EXPECT_THAT(mat_am.cols[j][i], Eq(v1 - v2));
    }
  }
}

TYPED_TEST(MatrixTest, MulScalar) {
  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();
  const auto scalar = RandScalar<TypeParam>();

  const TypeParam m1(arr1.data());
  const TypeParam m2(arr2.data());

  TypeParam mat_ms(m1 * scalar);
  TypeParam mat_sm(scalar * m2);
  TypeParam mat_as(m1);
  mat_as *= scalar;

  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      const auto v1 = ArrayAt<TypeParam>(arr1, i, j);
      const auto v2 = ArrayAt<TypeParam>(arr2, i, j);

      EXPECT_THAT(mat_ms.cols[j][i], Eq(v1 * scalar));
      EXPECT_THAT(mat_sm.cols[j][i], Eq(scalar * v2));
      EXPECT_THAT(mat_as.cols[j][i], Eq(v1 * scalar));
    }
  }
}

TYPED_TEST(MatrixTest, DivScalar) {
  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();
  const auto scalar = RandScalar<TypeParam>();

  const TypeParam m1(arr1.data());
  const TypeParam m2(arr2.data());

  TypeParam mat_ms(m1 / scalar);
  TypeParam mat_sm(scalar / m2);
  TypeParam mat_as(m1);
  mat_as /= scalar;

  const auto inv = typename TypeParam::Scalar(1) / scalar;
  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      const auto v1 = ArrayAt<TypeParam>(arr1, i, j);
      const auto v2 = ArrayAt<TypeParam>(arr2, i, j);

      EXPECT_THAT(mat_ms.cols[j][i], Eq(v1 * inv));
      EXPECT_THAT(mat_sm.cols[j][i], Eq(scalar / v2));
      EXPECT_THAT(mat_as.cols[j][i], Eq(v1 * inv));
    }
  }
}

TYPED_TEST(MatrixTest, MulMatrix) {
  if constexpr (TypeParam::kRows == TypeParam::kCols) {
    const auto arr1 = RandArray<TypeParam>();
    const auto arr2 = RandArray<TypeParam>();

    const TypeParam m1(arr1.data());
    const TypeParam m2(arr2.data());
    const TypeParam m3 = m1 * m2;
    TypeParam m4 = m1;
    m4 *= m2;

    for (int i = 0; i < TypeParam::kRows; ++i) {
      for (int j = 0; j < TypeParam::kCols; ++j) {
        typename TypeParam::Scalar row[TypeParam::kCols] = {};
        for (int k = 0; k < TypeParam::kCols; ++k) {
          row[k] = m1(i, k);
        }

        typename TypeParam::Scalar col[TypeParam::kRows] = {};
        for (int k = 0; k < TypeParam::kRows; ++k) {
          col[k] = m2(k, j);
        }

        typename TypeParam::Scalar dot(0);
        for (int k = 0; k < TypeParam::kCols; ++k) {
          dot += (row[k] * col[k]);
        }

        EXPECT_THAT(m3(i, j), FloatEq(dot));
        EXPECT_THAT(m4(i, j), FloatEq(dot));
      }
    }
  }
}

// Tests that a matrix multiplied by the zero matrix returns a zero matrix.
TYPED_TEST(MatrixTest, MulMatrixZero) {
  const typename TypeParam::Scalar zero = 0;

  const TypeParam m1 = InvertibleMatrix<TypeParam>();
  const TypeParam m2 = m1 * TypeParam(zero);
  TypeParam m3 = m1;
  m3 *= TypeParam(zero);

  for (int cc = 0; cc < TypeParam::kCols; ++cc) {
    for (int rr = 0; rr < TypeParam::kRows; ++rr) {
      EXPECT_THAT(m2.cols[cc][rr], Eq(0));
      EXPECT_THAT(m3.cols[cc][rr], Eq(0));
    }
  }
}

// Tests that a matrix multiplied by the identity returns itself.
TYPED_TEST(MatrixTest, MulMatrixIdentity) {
  const TypeParam m1 = InvertibleMatrix<TypeParam>();
  const TypeParam m2 = m1 * TypeParam::Identity();
  TypeParam m3 = m1;
  m3 *= TypeParam::Identity();

  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      EXPECT_THAT(m2.cols[j][i], FloatEq(m1.cols[j][i]));
      EXPECT_THAT(m3.cols[j][i], FloatEq(m1.cols[j][i]));
    }
  }
}

// Tests that a matrix multiplied by its inverse returns the identity.
TYPED_TEST(MatrixTest, MulMatrixInverse) {
  if constexpr (TypeParam::kRows <= 4 && TypeParam::kCols <= 4) {
    const TypeParam identity = TypeParam::Identity();
    const TypeParam m1 = InvertibleMatrix<TypeParam>();
    const TypeParam m2 = m1.Inversed();
    const TypeParam m3 = m1 * m2;
    TypeParam m4 = m1;
    m4 *= m2;

    // Due to the number of operations involved and the random numbers used to
    // generate the test matrices, the precision the inverse matrix is
    // calculated to is relatively low.
    const typename TypeParam::Scalar epsilon = 1e-6;

    for (int i = 0; i < TypeParam::kRows; ++i) {
      for (int j = 0; j < TypeParam::kCols; ++j) {
        EXPECT_THAT(m3.cols[j][i], FloatNear(identity.cols[j][i], epsilon));
        EXPECT_THAT(m4.cols[j][i], FloatNear(identity.cols[j][i], epsilon));
      }
    }
  }
}

TYPED_TEST(MatrixTest, PreMulVector) {
  using Vector = typename TypeParam::ColumnVector;

  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();

  const TypeParam mat(arr1.data());
  const Vector vec(arr2.data());
  const Vector res = vec * mat;

  for (int j = 0; j < TypeParam::kCols; ++j) {
    Vector col;
    for (int i = 0; i < TypeParam::kRows; ++i) {
      col[i] = mat.cols[j][i];
    }
    const auto dot = col.Dot(vec);
    EXPECT_THAT(res.data[j], Eq(dot));
  }
}

TYPED_TEST(MatrixTest, PostMulVector) {
  using Vector = typename TypeParam::RowVector;

  const auto arr1 = RandArray<TypeParam>();
  const auto arr2 = RandArray<TypeParam>();

  const TypeParam mat(arr1.data());
  const Vector vec(arr2.data());
  const Vector res = mat * vec;

  for (int i = 0; i < TypeParam::kRows; ++i) {
    Vector row;
    for (int j = 0; j < TypeParam::kCols; ++j) {
      row[j] = mat.cols[j][i];
    }
    const auto dot = row.Dot(vec);
    EXPECT_THAT(res.data[i], Eq(dot));
  }
}

TYPED_TEST(MatrixTest, Mat44MulVec3) {
  if constexpr (TypeParam::kRows == 4 && TypeParam::kCols == 4) {
    using Scalar = typename TypeParam::Scalar;
    using Vec3 = Vector<Scalar, 3, TypeParam::kSimd>;
    using Vec4 = Vector<Scalar, 4, TypeParam::kSimd>;

    const auto arr1 = RandArray<TypeParam>();
    const auto arr2 = RandArray<TypeParam>();

    const TypeParam mat(arr1.data());
    const Vec3 vec(arr2.data());
    const Vec3 res = mat * vec;

    const Vec4 tmp(vec, Scalar(1));
    const Vec4 expect = mat * tmp;

    for (int i = 0; i < 3; ++i) {
      EXPECT_THAT(res[i], FloatEq(expect[i] / expect.w));
    }
  }
}

TYPED_TEST(MatrixTest, Transpose) {
  const auto arr1 = RandArray<TypeParam>();

  const TypeParam m1(arr1.data());
  const auto m2 = m1.Transposed();
  const auto m3 = Transposed(m1);

  for (int i = 0; i < TypeParam::kRows; ++i) {
    for (int j = 0; j < TypeParam::kCols; ++j) {
      EXPECT_THAT(m2(i, j), Eq(m1(j, i)));
      EXPECT_THAT(m3(i, j), Eq(m1(j, i)));
    }
  }
}

TYPED_TEST(MatrixTest, Inverse) {
  if constexpr (TypeParam::kRows <= 4 && TypeParam::kCols <= 4) {
    const TypeParam invertible = InvertibleMatrix<TypeParam>();
    const TypeParam inverted = invertible.Inversed();
    const TypeParam identity = invertible * inverted;
    for (int i = 0; i < TypeParam::kRows; ++i) {
      for (int j = 0; j < TypeParam::kCols; ++j) {
        if (i == j) {
          EXPECT_THAT(identity(i, j), FloatNear(1.0f, kDefaultEpsilon));
        } else {
          EXPECT_THAT(identity(i, j), FloatNear(0.0f, kDefaultEpsilon));
        }
      }
    }
  }
}

constexpr float kDeterminantThresholdSmall = kDeterminantThreshold / 100.0f;
constexpr float kDeterminantThresholdLarge = kDeterminantThreshold * 100.0f;

TYPED_TEST(MatrixTest, TryInverseSmall) {
  if constexpr (TypeParam::kRows <= 4 && TypeParam::kCols <= 4) {
    const float kDeterminantPower =
        1.0f / static_cast<float>(TypeParam::kRows == 2 ? 2 : 3);
    const float kScaleMin = pow(kDeterminantThreshold, kDeterminantPower);

    TypeParam matrix = TypeParam::Identity() * kScaleMin / 2;

    auto res = matrix.TryInversed(kDeterminantThreshold);
    EXPECT_FALSE(res.has_value());

    res = matrix.TryInversed(kDeterminantThresholdSmall);
    EXPECT_TRUE(res.has_value());

    res = matrix.TryInversed(kDeterminantThresholdLarge);
    EXPECT_FALSE(res.has_value());
  }
}

TYPED_TEST(MatrixTest, TryInverseLarge) {
  if constexpr (TypeParam::kRows <= 4 && TypeParam::kCols <= 4) {
    const float kDeterminantPower =
        1.0f / static_cast<float>(TypeParam::kRows == 2 ? 2 : 3);
    const float kScaleMin = pow(kDeterminantThreshold, kDeterminantPower);

    TypeParam matrix = TypeParam::Identity() * kScaleMin * 2.0f;

    auto res = matrix.TryInversed(kDeterminantThreshold);
    EXPECT_TRUE(res.has_value());

    res = matrix.TryInversed(kDeterminantThresholdSmall);
    EXPECT_TRUE(res.has_value());

    res = matrix.TryInversed(kDeterminantThresholdLarge);
    EXPECT_FALSE(res.has_value());
  }
}

TYPED_TEST(MatrixTest, Perspective) {
  using Scalar = typename TypeParam::Scalar;

  if constexpr (TypeParam::kRows == 4 && TypeParam::kCols == 4) {
    {
      const TypeParam result = PerspectiveMatrix<Scalar, TypeParam::kSimd>(
          std::atan(Scalar(1)) * Scalar(2), 1, 1, 2);
      const TypeParam expect(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -3, -4, 0, 0, -1, 0);
      EXPECT_THAT(result, Eq(expect));
    }
  }
}

TYPED_TEST(MatrixTest, Orthographic) {
  using Scalar = typename TypeParam::Scalar;

  if constexpr (TypeParam::kRows == 4 && TypeParam::kCols == 4) {
    auto Ortho = OrthographicMatrix<Scalar, TypeParam::kSimd>;

    {
      const TypeParam result = Ortho(-1, 1, -1, 1, 0, 2);
      const TypeParam expect(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 1);
      EXPECT_TRUE(AreNearlyEqual(result, expect));
    }
    {
      const TypeParam result = Ortho(0, 2, 0, 2, 0, 2);
      const TypeParam expect(1, 0, 0, -1, 0, 1, 0, -1, 0, 0, -1, -1, 0, 0, 0,
                             1);
      EXPECT_TRUE(AreNearlyEqual(result, expect));
    }
    {
      const TypeParam result = Ortho(1, 3, 0, 2, 0, 2);
      const TypeParam expect(1, 0, 0, -2, 0, 1, 0, -1, 0, 0, -1, -1, 0, 0, 0,
                             1);
      EXPECT_TRUE(AreNearlyEqual(result, expect));
    }
    {
      const TypeParam result = Ortho(0, 2, 1, 3, 0, 2);
      const TypeParam expect(1, 0, 0, -1, 0, 1, 0, -2, 0, 0, -1, -1, 0, 0, 0,
                             1);
      EXPECT_TRUE(AreNearlyEqual(result, expect));
    }
    {
      const TypeParam result = Ortho(0, 2, 0, 2, 1, 3);
      const TypeParam expect(1, 0, 0, -1, 0, 1, 0, -1, 0, 0, -1, -2, 0, 0, 0,
                             1);
      EXPECT_TRUE(AreNearlyEqual(result, expect));
    }
  }
}

TYPED_TEST(MatrixTest, LookAt) {
  using Scalar = typename TypeParam::Scalar;

  if constexpr (TypeParam::kRows == 4 && TypeParam::kCols == 4) {
    auto LookAt = LookAtViewMatrix<Scalar, TypeParam::kSimd>;

    {
      const TypeParam result = LookAt({0, 0, 1}, {0, 0, 0}, {0, 1, 0});
      const TypeParam expect(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
      EXPECT_TRUE(AreNearlyEqual(result, expect));
    }
    {
      const TypeParam result = LookAt({0, 0, 0}, {1, 1, 1}, {0, 1, 0});
      const TypeParam expect(-0.707106781, -0.408248290, -0.577350269, 0, 0,
                             0.816496580, -0.577350269, 0, 0.707106781,
                             -0.408248290, -0.577350269, 1.732050808, 0, 0, 0,
                             1);
      EXPECT_TRUE(AreNearlyEqual(result, expect));
    }
    {
      const TypeParam result = LookAt({0, 0, 2}, {0, 0, 0}, {0, 1, 0});
      const TypeParam expect(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
      EXPECT_TRUE(AreNearlyEqual(result, expect));
    }
    {
      const TypeParam result = LookAt({1, 0, 0}, {0, 0, 0}, {0, 1, 0});
      const TypeParam expect(0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 1);
      EXPECT_TRUE(AreNearlyEqual(result, expect));
    }
    {
      const TypeParam result = LookAt({0, 1, 0}, {0, 0, 0}, {1, 0, 0});
      const TypeParam expect(0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1);
      EXPECT_TRUE(AreNearlyEqual(result, expect));
    }
  }
}
}  // namespace
}  // namespace redux
