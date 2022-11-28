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

#ifndef REDUX_MODULES_MATH_MATRIX_H_
#define REDUX_MODULES_MATH_MATRIX_H_

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <type_traits>

#include "redux/modules/base/logging.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/math/constants.h"
#include "redux/modules/math/detail/matrix_layout.h"
#include "redux/modules/math/vector.h"

namespace redux {

template <typename Layout>
class MatrixImpl;

template <typename Scalar, int Rows, int Cols, bool AllowSimd>
using Matrix = MatrixImpl<detail::MatrixLayout<Scalar, Rows, Cols, AllowSimd>>;

// Common matrix type aliases.
using mat2 = Matrix<float, 2, 2, kEnableSimdByDefault>;
using mat3 = Matrix<float, 3, 3, kEnableSimdByDefault>;
using mat4 = Matrix<float, 4, 4, kEnableSimdByDefault>;
using mat34 = Matrix<float, 3, 4, kEnableSimdByDefault>;  // affine matrix

// An MxN-dimensional matrix which uses the underlying 'Layout' to determine the
// data type and dimensionality of the matrix.
template <typename Layout>
class MatrixImpl : public Layout {
 public:
  using Scalar = typename Layout::Scalar;
  static constexpr const int kRows = Layout::kRows;
  static constexpr const int kCols = Layout::kCols;
  static constexpr const bool kSimd = Layout::kSimd;

  // Creates an zero-initialized MatrixImpl.
  constexpr MatrixImpl() {}

  // Copies a matrix from another matrix, copying each element.
  constexpr MatrixImpl(const MatrixImpl& rhs);

  // Assigns a matrix from another matrix, copying each element.
  constexpr MatrixImpl& operator=(const MatrixImpl& rhs);

  // Creates a matrix from another matrix of a different type by copying each
  // element. If the other matrix is of smaller dimensionality, then the created
  // matrix will be padded with in identity-esque elements.
  template <class U>
  explicit constexpr MatrixImpl(const MatrixImpl<U>& rhs) {
    using Other = MatrixImpl<U>;
    static_assert(!std::is_same_v<Other, MatrixImpl>);

    for (int cc = 0; cc < kCols; ++cc) {
      for (int rr = 0; rr < kRows; ++rr) {
        if (Other::kCols <= cc || Other::kRows <= rr) {
          this->cols[cc][rr] = static_cast<Scalar>(cc == rr ? 1 : 0);
        } else {
          this->cols[cc][rr] = static_cast<Scalar>(rhs.cols[cc][rr]);
        }
      }
    }
  }

  // Creates a matrix with all elements set to the given scalar value.
  explicit constexpr MatrixImpl(Scalar s);

  // Creates a matrix from an array of scalar values.
  // IMPORTANT: This function does not perform any bounds checking and assumes
  // the array contains the correct number of elements.
  explicit constexpr MatrixImpl(const Scalar* a);

  // Creates a 2x2 matrix from scalar values.
  constexpr MatrixImpl(Scalar s00, Scalar s01, Scalar s10, Scalar s11);

  // Creates a 3x3 matrix from scalar values.
  constexpr MatrixImpl(Scalar s00, Scalar s01, Scalar s02, Scalar s10,
                       Scalar s11, Scalar s12, Scalar s20, Scalar s21,
                       Scalar s22);

  // Creates a 3x4 matrix from scalar values.
  constexpr MatrixImpl(Scalar s00, Scalar s01, Scalar s02, Scalar s03,
                       Scalar s10, Scalar s11, Scalar s12, Scalar s13,
                       Scalar s20, Scalar s21, Scalar s22, Scalar s23);

  // Creates a 4x4 matrix from scalar values.
  constexpr MatrixImpl(Scalar s00, Scalar s01, Scalar s02, Scalar s03,
                       Scalar s10, Scalar s11, Scalar s12, Scalar s13,
                       Scalar s20, Scalar s21, Scalar s22, Scalar s23,
                       Scalar s30, Scalar s31, Scalar s32, Scalar s33);

  // Accesses the m,n-th element of the matrix.
  // IMPORTANT: These functions do not perform any bounds checking.
  constexpr Scalar& operator()(int row, int col) {
    return this->cols[col][row];
  }
  constexpr const Scalar& operator()(int row, int col) const {
    return this->cols[col][row];
  }

  // Returns a copy of the n-th row of the matrix.
  // IMPORTANT: This function does not perform any bounds checking.
  using RowVector = Vector<Scalar, kCols, kSimd>;
  constexpr auto Row(int idx) const -> RowVector;

  // Returns a copy of the n-th column of the matrix.
  // IMPORTANT: This function does not perform any bounds checking.
  using ColumnVector = Vector<Scalar, kRows, kSimd>;
  constexpr auto Column(int idx) const -> ColumnVector;

  // Returns the transpose of the matrix.
  using TransposeMatrix = Matrix<Scalar, kCols, kRows, kSimd>;
  constexpr auto Transposed() const -> TransposeMatrix;

  // Calculates the inverse matrix such that `m * m.Inversed()` is the identity.
  constexpr auto Inversed() const -> MatrixImpl;

  // Calculates the inverse matrix such that `m * m.Inversed()` is the identity.
  // Returns nullopt if the matrix is not invertible.
  constexpr auto TryInversed(Scalar threshold = 0) const
      -> std::optional<MatrixImpl>;

  // Returns the top-left submatrix of the given dimensions.
  template <int R, int C>
  constexpr auto Submatrix() const -> Matrix<Scalar, R, C, kSimd> {
    static_assert(R <= kRows);
    static_assert(C <= kCols);

    Matrix<Scalar, R, C, kSimd> sub;
    for (int cc = 0; cc < C; ++cc) {
      for (int rr = 0; rr < R; ++rr) {
        sub.cols[cc][rr] = this->cols[cc][rr];
      }
    }
    return sub;
  }

  static constexpr MatrixImpl Zero();
  static constexpr MatrixImpl Identity();
};

// Copies a matrix from another matrix, copying each element.
template <typename T>
constexpr MatrixImpl<T>::MatrixImpl(const MatrixImpl& rhs) {
  for (int cc = 0; cc < T::kCols; ++cc) {
    for (int rr = 0; rr < T::kRows; ++rr) {
      this->cols[cc][rr] = rhs.cols[cc][rr];
    }
  }
}

// Assigns a matrix from another matrix, copying each element.
template <typename T>
constexpr MatrixImpl<T>& MatrixImpl<T>::operator=(const MatrixImpl& rhs) {
  for (int cc = 0; cc < T::kCols; ++cc) {
    for (int rr = 0; rr < T::kRows; ++rr) {
      this->cols[cc][rr] = rhs.cols[cc][rr];
    }
  }
  return *this;
}

// Creates a matrix with all elements set to the given scalar value.
template <typename T>
constexpr MatrixImpl<T>::MatrixImpl(Scalar s) {
  if constexpr (kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      this->simd[cc] = ss;
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        this->cols[cc][rr] = s;
      }
    }
  }
}

// Creates a matrix from an array of scalar values.
// IMPORTANT: This function does not perform any bounds checking and assumes
// the array contains the correct number of elements.
template <typename T>
constexpr MatrixImpl<T>::MatrixImpl(const Scalar* a) {
  for (int cc = 0; cc < T::kCols; ++cc) {
    for (int rr = 0; rr < T::kRows; ++rr) {
      this->cols[cc][rr] = *a;
      ++a;
    }
  }
}

// Creates a 2x2 matrix from scalar values.
template <typename T>
constexpr MatrixImpl<T>::MatrixImpl(Scalar s00, Scalar s01, Scalar s10,
                                    Scalar s11) {
  static_assert(kRows == 2 && kCols == 2);
  this->cols[0][0] = s00;
  this->cols[0][1] = s10;
  this->cols[1][0] = s01;
  this->cols[1][1] = s11;
}

// Creates a 3x3 matrix from scalar values.
template <typename T>
constexpr MatrixImpl<T>::MatrixImpl(Scalar s00, Scalar s01, Scalar s02,
                                    Scalar s10, Scalar s11, Scalar s12,
                                    Scalar s20, Scalar s21, Scalar s22) {
  static_assert(kRows == 3 && kCols == 3);
  this->cols[0][0] = s00;
  this->cols[0][1] = s10;
  this->cols[0][2] = s20;
  this->cols[1][0] = s01;
  this->cols[1][1] = s11;
  this->cols[1][2] = s21;
  this->cols[2][0] = s02;
  this->cols[2][1] = s12;
  this->cols[2][2] = s22;
}

// Creates a 4x3 matrix from scalar values.
template <typename T>
constexpr MatrixImpl<T>::MatrixImpl(Scalar s00, Scalar s01, Scalar s02,
                                    Scalar s03, Scalar s10, Scalar s11,
                                    Scalar s12, Scalar s13, Scalar s20,
                                    Scalar s21, Scalar s22, Scalar s23) {
  static_assert(kRows == 3 && kCols == 4);
  this->cols[0][0] = s00;
  this->cols[0][1] = s10;
  this->cols[0][2] = s20;
  this->cols[1][0] = s01;
  this->cols[1][1] = s11;
  this->cols[1][2] = s21;
  this->cols[2][0] = s02;
  this->cols[2][1] = s12;
  this->cols[2][2] = s22;
  this->cols[3][0] = s03;
  this->cols[3][1] = s13;
  this->cols[3][2] = s23;
}

// Creates a 4x4 matrix from scalar values.
template <typename T>
constexpr MatrixImpl<T>::MatrixImpl(Scalar s00, Scalar s01, Scalar s02,
                                    Scalar s03, Scalar s10, Scalar s11,
                                    Scalar s12, Scalar s13, Scalar s20,
                                    Scalar s21, Scalar s22, Scalar s23,
                                    Scalar s30, Scalar s31, Scalar s32,
                                    Scalar s33) {
  static_assert(kRows == 4 && kCols == 4);
  this->cols[0][0] = s00;
  this->cols[0][1] = s10;
  this->cols[0][2] = s20;
  this->cols[0][3] = s30;
  this->cols[1][0] = s01;
  this->cols[1][1] = s11;
  this->cols[1][2] = s21;
  this->cols[1][3] = s31;
  this->cols[2][0] = s02;
  this->cols[2][1] = s12;
  this->cols[2][2] = s22;
  this->cols[2][3] = s32;
  this->cols[3][0] = s03;
  this->cols[3][1] = s13;
  this->cols[3][2] = s23;
  this->cols[3][3] = s33;
}

template <typename T>
constexpr typename MatrixImpl<T>::RowVector MatrixImpl<T>::Row(int idx) const {
  if constexpr (kCols == 2) {
    return RowVector(this->cols[0][idx], this->cols[1][idx]);
  } else if constexpr (kCols == 3) {
    return RowVector(this->cols[0][idx], this->cols[1][idx],
                     this->cols[2][idx]);
  } else if constexpr (kCols == 4) {
    return RowVector(this->cols[0][idx], this->cols[1][idx], this->cols[2][idx],
                     this->cols[3][idx]);
  } else {
    RowVector row(Scalar(0));
    for (int cc = 0; cc < T::kCols; ++cc) {
      row[cc] = this->cols[cc][idx];
    }
    return row;
  }
}

template <typename T>
constexpr typename MatrixImpl<T>::ColumnVector MatrixImpl<T>::Column(
    int idx) const {
  return ColumnVector(this->cols[idx]);
}

// Returns the transpose of the matrix.
template <typename T>
constexpr auto MatrixImpl<T>::Transposed() const -> TransposeMatrix {
  TransposeMatrix transpose;
  for (int cc = 0; cc < T::kCols; ++cc) {
    for (int rr = 0; rr < T::kRows; ++rr) {
      transpose.cols[rr][cc] = this->cols[cc][rr];
    }
  }
  return transpose;
}

namespace detail {

template <typename T>
inline int FindLargestPivotForInverse(const MatrixImpl<T>& m) {
  static_assert(T::kRows == 4 && T::kCols == 4);
  const typename T::Scalar values[4] = {
      std::abs(m.cols[0][0]), std::abs(m.cols[0][1]), std::abs(m.cols[0][2]),
      std::abs(m.cols[0][3])};

  if (values[0] > values[1]) {
    if (values[0] > values[2]) {
      if (values[0] > values[3]) {
        return 0;
      } else {
        return 3;
      }
    } else if (values[2] > values[3]) {
      return 2;
    } else {
      return 3;
    }
  } else if (values[1] > values[2]) {
    if (values[1] > values[3]) {
      return 1;
    } else {
      return 3;
    }
  } else if (values[2] > values[3]) {
    return 2;
  } else {
    return 3;
  }
}
}  // namespace detail

template <typename T>
constexpr auto MatrixImpl<T>::Inversed() const -> MatrixImpl {
  return *TryInversed(Scalar(0));
}

// Calculates the inverse matrix such that `m * m.Inversed()` is the identity.
// Returns nullopt if the matrix is not invertible.
template <typename T>
constexpr auto MatrixImpl<T>::TryInversed(Scalar threshold) const
    -> std::optional<MatrixImpl> {
  if constexpr (kRows == 2 && kCols == 2) {
    const auto determinant = (this->m00 * this->m11) - (this->m10 * this->m01);
    if (std::abs(determinant) < threshold) {
      return std::nullopt;
    }

    const auto inv = ScalarImpl<T>(1) / determinant;
    const auto m00 = inv * this->m11;
    const auto m01 = -inv * this->m01;
    const auto m10 = -inv * this->m10;
    const auto m11 = inv * this->m00;
    return MatrixImpl<T>(m00, m01, m10, m11);
  } else if constexpr (kRows == 3 && kCols == 3) {
    const auto sub11 = (this->m11 * this->m22) - (this->m12 * this->m21);
    const auto sub12 = (this->m12 * this->m20) - (this->m10 * this->m22);
    const auto sub13 = (this->m10 * this->m21) - (this->m11 * this->m20);
    const auto determinant =
        (this->m00 * sub11) + (this->m01 * sub12) + (this->m02 * sub13);
    if (std::abs(determinant) < threshold) {
      return std::nullopt;
    }

    const auto inv = ScalarImpl<T>(1) / determinant;
    const auto m00 = sub11 * inv;
    const auto m10 = sub12 * inv;
    const auto m20 = sub13 * inv;
    const auto m01 = ((this->m02 * this->m21) - (this->m01 * this->m22)) * inv;
    const auto m11 = ((this->m00 * this->m22) - (this->m02 * this->m20)) * inv;
    const auto m21 = ((this->m01 * this->m20) - (this->m00 * this->m21)) * inv;
    const auto m02 = ((this->m01 * this->m12) - (this->m02 * this->m11)) * inv;
    const auto m12 = ((this->m02 * this->m10) - (this->m00 * this->m12)) * inv;
    const auto m22 = ((this->m00 * this->m11) - (this->m01 * this->m10)) * inv;
    return MatrixImpl<T>(m00, m01, m02, m10, m11, m12, m20, m21, m22);
  } else if constexpr (T::kRows == 4 && T::kCols == 4) {
    // This will perform the pivot and find the row, column, and 3x3 submatrix
    // for this pivot.
    using Scalar = typename T::Scalar;
    using Matrix3 = Matrix<Scalar, 3, 3, T::kSimd>;
    using Vector3 = Vector<Scalar, 3, T::kSimd>;
    using Vector4 = Vector<Scalar, 4, T::kSimd>;

    // Finds the largest element in the first column.
    const int pivot = detail::FindLargestPivotForInverse(*this);
    const auto pivot_value = this->cols[0][pivot];
    if (std::abs(pivot_value) < threshold) {
      return std::nullopt;
    }

    Vector3 row;
    Vector3 col;
    Matrix3 sub;

    if (pivot == 0) {
      // clang-format off
      row = Vector3(this->m01, this->m02, this->m03);
      col = Vector3(this->m10, this->m20, this->m30);
      sub = Matrix3(this->m11, this->m12, this->m13,
                    this->m21, this->m22, this->m23,
                    this->m31, this->m32, this->m33);
      // clang-format on
    } else if (pivot == 1) {
      // clang-format off
      row = Vector3(this->m11, this->m12, this->m13);
      col = Vector3(this->m00, this->m20, this->m30);
      sub = Matrix3(this->m01, this->m02, this->m03,
                    this->m21, this->m22, this->m23,
                    this->m31, this->m32, this->m33);
      // clang-format on
    } else if (pivot == 2) {
      row = Vector3(this->m21, this->m22, this->m23);
      col = Vector3(this->m00, this->m10, this->m30);
      sub = Matrix3(this->m01, this->m02, this->m03, this->m11, this->m12,
                    this->m13, this->m31, this->m32, this->m33);
      // clang-format on
    } else {
      // clang-format off
      row = Vector3(this->m31, this->m32, this->m33);
      col = Vector3(this->m00, this->m10, this->m20);
      sub = Matrix3(this->m01, this->m02, this->m03,
                    this->m11, this->m12, this->m13,
                    this->m21, this->m22, this->m23);
      // clang-format on
    }

    // This will compute the inverse using the row, column, and 3x3 submatrix.
    const auto inv = Scalar(-1) / pivot_value;
    row *= inv;
    Matrix3 outer_product(col[0] * row[0], col[0] * row[1], col[0] * row[2],
                          col[1] * row[0], col[1] * row[1], col[1] * row[2],
                          col[2] * row[0], col[2] * row[1], col[2] * row[2]);
    sub += outer_product;
    auto subinverse = sub.TryInversed(threshold);
    if (!subinverse) {
      return std::nullopt;
    }

    const Matrix3& mat_inv = *subinverse;
    const Vector3 col_inv = mat_inv * (col * inv);
    const Vector3 row_inv = row * mat_inv;
    const auto pivot_inv = row.Dot(col_inv) - inv;
    const Vector4 r0(pivot_inv, row_inv[0], row_inv[1], row_inv[2]);
    const Vector4 r1(col_inv[0], mat_inv.m00, mat_inv.m01, mat_inv.m02);
    const Vector4 r2(col_inv[1], mat_inv.m10, mat_inv.m11, mat_inv.m12);
    const Vector4 r3(col_inv[2], mat_inv.m20, mat_inv.m21, mat_inv.m22);

    if (pivot == 0) {
      // clang-format off
      return MatrixImpl<T>(r0.x, r0.y, r0.z, r0.w,
                           r1.x, r1.y, r1.z, r1.w,
                           r2.x, r2.y, r2.z, r2.w,
                           r3.x, r3.y, r3.z, r3.w);
      // clang-format on
    } else if (pivot == 1) {
      // clang-format off
      return MatrixImpl<T>(r1.x, r1.y, r1.z, r1.w,
                           r0.x, r0.y, r0.z, r0.w,
                           r2.x, r2.y, r2.z, r2.w,
                           r3.x, r3.y, r3.z, r3.w);
      // clang-format on
    } else if (pivot == 2) {
      // clang-format off
      return MatrixImpl<T>(r1.x, r1.y, r1.z, r1.w,
                           r2.x, r2.y, r2.z, r2.w,
                           r0.x, r0.y, r0.z, r0.w,
                           r3.x, r3.y, r3.z, r3.w);
      // clang-format on
    } else {
      // clang-format off
      return MatrixImpl<T>(r1.x, r1.y, r1.z, r1.w,
                           r2.x, r2.y, r2.z, r2.w,
                           r3.x, r3.y, r3.z, r3.w,
                           r0.x, r0.y, r0.z, r0.w);
      // clang-format on
    }
  }
}

// Returns the inverse of a matrix.
template <typename T>
constexpr auto Inversed(const MatrixImpl<T>& m) {
  return m.Inversed();
}

// Returns the transpose of a matrix.
template <typename T>
constexpr auto Transposed(const MatrixImpl<T>& m) {
  return m.Transposed();
}

// Compares two matrices for equality. Consider using AreNearlyEqual instead if
// floating-point precision is a concern.
template <typename T>
constexpr bool operator==(const MatrixImpl<T>& m1, const MatrixImpl<T>& m2) {
  for (int cc = 0; cc < T::kCols; ++cc) {
    for (int rr = 0; rr < T::kRows; ++rr) {
      if (m1.cols[cc][rr] != m2.cols[cc][rr]) {
        return false;
      }
    }
  }
  return true;
}

// Compares two matrices for inequality. Consider using !AreNearlyEqual instead
// if floating-point precision is a concern.
template <typename T>
constexpr bool operator!=(const MatrixImpl<T>& m1, const MatrixImpl<T>& m2) {
  return !(m1 == m2);
}

// Compares two matrices for equality within a given threshold.
template <typename T>
constexpr bool AreNearlyEqual(const MatrixImpl<T>& m1, const MatrixImpl<T>& m2,
                              ScalarImpl<T> epsilon = kDefaultEpsilon) {
  for (int cc = 0; cc < T::kCols; ++cc) {
    for (int rr = 0; rr < T::kRows; ++rr) {
      if (std::abs(m1.cols[cc][rr] - m2.cols[cc][rr]) > epsilon) {
        return false;
      }
    }
  }
  return true;
}

// Negates all elements of the matrix.
template <typename T>
constexpr auto operator-(const MatrixImpl<T>& m) {
  MatrixImpl<T> result;
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto zero = simd4f_zero();
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_sub(zero, m.simd[cc]);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = -m.cols[cc][rr];
      }
    }
  }
  return result;
}

// Adds a scalar to each element of a matrix.
template <typename T>
constexpr auto operator+(ScalarImpl<T> s, const MatrixImpl<T>& m) {
  MatrixImpl<T> result;
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_add(ss, m.simd[cc]);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = s + m.cols[cc][rr];
      }
    }
  }
  return result;
}

// Adds a scalar to each element of a matrix.
template <typename T>
constexpr auto operator+(const MatrixImpl<T>& m, ScalarImpl<T> s) {
  MatrixImpl<T> result;
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_add(m.simd[cc], ss);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = m.cols[cc][rr] + s;
      }
    }
  }
  return result;
}

// Adds a matrix to another matrix.
template <typename T>
constexpr auto operator+(const MatrixImpl<T>& m1, const MatrixImpl<T>& m2) {
  MatrixImpl<T> result;
  if constexpr (MatrixImpl<T>::kSimd) {
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_add(m1.simd[cc], m2.simd[cc]);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = m1.cols[cc][rr] + m2.cols[cc][rr];
      }
    }
  }
  return result;
}

// Adds (in-place) a scalar to a matrix.
template <typename T>
constexpr auto& operator+=(MatrixImpl<T>& m, ScalarImpl<T> s) {
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      m.simd[cc] = simd4f_add(m.simd[cc], ss);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        m.cols[cc][rr] += s;
      }
    }
  }
  return m;
}

// Adds (in-place) a matrix to another matrix.
template <typename T>
constexpr auto& operator+=(MatrixImpl<T>& m1, const MatrixImpl<T>& m2) {
  if constexpr (MatrixImpl<T>::kSimd) {
    for (int cc = 0; cc < T::kCols; ++cc) {
      m1.simd[cc] = simd4f_add(m1.simd[cc], m2.simd[cc]);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        m1.cols[cc][rr] += m2.cols[cc][rr];
      }
    }
  }
  return m1;
}

// Subtracts a scalar from each element of a matrix.
template <typename T>
constexpr auto operator-(ScalarImpl<T> s, const MatrixImpl<T>& m) {
  MatrixImpl<T> result;
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_sub(ss, m.simd[cc]);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = s - m.cols[cc][rr];
      }
    }
  }
  return result;
}

// Subtracts a scalar from each element of a matrix.
template <typename T>
constexpr auto operator-(const MatrixImpl<T>& m, ScalarImpl<T> s) {
  MatrixImpl<T> result;
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_sub(m.simd[cc], ss);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = m.cols[cc][rr] - s;
      }
    }
  }
  return result;
}

// Subtracts a matrix from another matrix.
template <typename T>
constexpr auto operator-(const MatrixImpl<T>& m1, const MatrixImpl<T>& m2) {
  MatrixImpl<T> result;
  if constexpr (MatrixImpl<T>::kSimd) {
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_sub(m1.simd[cc], m2.simd[cc]);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = m1.cols[cc][rr] - m2.cols[cc][rr];
      }
    }
  }
  return result;
}

// Subtracts (in-place) a scalar from a matrix.
template <typename T>
constexpr auto& operator-=(MatrixImpl<T>& m, ScalarImpl<T> s) {
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      m.simd[cc] = simd4f_sub(m.simd[cc], ss);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        m.cols[cc][rr] -= s;
      }
    }
  }
  return m;
}

// Subtracts (in-place) a matrix from another matrix.
template <typename T>
constexpr auto& operator-=(MatrixImpl<T>& m1, const MatrixImpl<T>& m2) {
  if constexpr (MatrixImpl<T>::kSimd) {
    for (int cc = 0; cc < T::kCols; ++cc) {
      m1.simd[cc] = simd4f_sub(m1.simd[cc], m2.simd[cc]);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        m1.cols[cc][rr] -= m2.cols[cc][rr];
      }
    }
  }
  return m1;
}

// Multiplies a scalar with each element of a matrix.
template <typename T>
constexpr auto operator*(ScalarImpl<T> s, const MatrixImpl<T>& m) {
  MatrixImpl<T> result;
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_mul(ss, m.simd[cc]);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = s * m.cols[cc][rr];
      }
    }
  }
  return result;
}

// Multiplies a scalar with each element of a matrix.
template <typename T>
constexpr auto operator*(const MatrixImpl<T>& m, ScalarImpl<T> s) {
  MatrixImpl<T> result;
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_mul(m.simd[cc], ss);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = m.cols[cc][rr] * s;
      }
    }
  }
  return result;
}

// Multiplies a matrix with another matrix.
template <typename T>
constexpr auto operator*(const MatrixImpl<T>& m1, const MatrixImpl<T>& m2) {
  MatrixImpl<T> result;
  for (int rr = 0; rr < T::kRows; ++rr) {
    const auto m1_row = m1.Row(rr);
    for (int cc = 0; cc < T::kCols; ++cc) {
      const auto m2_col = m2.Column(cc);
      result.cols[cc][rr] = m1_row.Dot(m2_col);
    }
  }
  return result;
}

// Multiplies (in-place) a scalar with a matrix.
template <typename T>
constexpr auto& operator*=(MatrixImpl<T>& m, ScalarImpl<T> s) {
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      m.simd[cc] = simd4f_mul(m.simd[cc], ss);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        m.cols[cc][rr] *= s;
      }
    }
  }
  return m;
}

// Multiplies (in-place) a matrix with another matrix.
template <typename T>
constexpr auto& operator*=(MatrixImpl<T>& m1, const MatrixImpl<T>& m2) {
  m1 = (m1 * m2);
  return m1;
}

// Divides a scalar with each element of a matrix.
template <typename T>
constexpr MatrixImpl<T> operator/(ScalarImpl<T> s, const MatrixImpl<T>& m) {
  MatrixImpl<T> result;
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(s);
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_div(ss, m.simd[cc]);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = s / m.cols[cc][rr];
      }
    }
  }
  return result;
}

// Divides a scalar with each element of a matrix.
template <typename T>
constexpr MatrixImpl<T> operator/(const MatrixImpl<T>& m, ScalarImpl<T> s) {
  MatrixImpl<T> result;
  const auto inv = ScalarImpl<T>(1) / s;
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(inv);
    for (int cc = 0; cc < T::kCols; ++cc) {
      result.simd[cc] = simd4f_mul(m.simd[cc], ss);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        result.cols[cc][rr] = m.cols[cc][rr] * inv;
      }
    }
  }
  return result;
}

// Divides a matrix with another matrix.
template <typename T>
constexpr MatrixImpl<T> operator/(const MatrixImpl<T>& m1,
                                  const MatrixImpl<T>& m2) {
  const auto inv = m2.TryInversed();
  CHECK(inv) << "Divide by zero";
  return m1 * inv.get();
}

// Divides (in-place) a scalar with a matrix.
template <typename T>
constexpr auto& operator/=(MatrixImpl<T>& m, ScalarImpl<T> s) {
  const auto inv = ScalarImpl<T>(1) / s;
  if constexpr (MatrixImpl<T>::kSimd) {
    const auto ss = simd4f_splat(inv);
    for (int cc = 0; cc < T::kCols; ++cc) {
      m.simd[cc] = simd4f_mul(m.simd[cc], ss);
    }
  } else {
    for (int cc = 0; cc < T::kCols; ++cc) {
      for (int rr = 0; rr < T::kRows; ++rr) {
        m.cols[cc][rr] *= inv;
      }
    }
  }
  return m;
}

// Divides (in-place) a matrix with another matrix.
template <typename T>
constexpr auto& operator/=(MatrixImpl<T>& m1, const MatrixImpl<T>& m2) {
  m1 = (m1 / m2);
  return m1;
}

// Pre-multiplies a vector with the matrix.
template <typename T>
constexpr typename MatrixImpl<T>::RowVector operator*(
    const typename MatrixImpl<T>::ColumnVector& v, const MatrixImpl<T>& m) {
  typename MatrixImpl<T>::RowVector result;
  for (int cc = 0; cc < T::kCols; ++cc) {
    result[cc] = m.Column(cc).Dot(v);
  }
  return result;
}

// Post-multiplies a matrix with a vector.
template <typename T>
constexpr typename MatrixImpl<T>::ColumnVector operator*(
    const MatrixImpl<T>& m, const typename MatrixImpl<T>::RowVector& v) {
  if constexpr (T::kRows == 2 && T::kCols == 2) {
    const ScalarImpl<T> x = m.Row(0).Dot(v);
    const ScalarImpl<T> y = m.Row(1).Dot(v);
    return typename MatrixImpl<T>::ColumnVector(x, y);
  } else if constexpr (T::kRows == 3 && T::kCols == 3) {
    const ScalarImpl<T> x = m.Row(0).Dot(v);
    const ScalarImpl<T> y = m.Row(1).Dot(v);
    const ScalarImpl<T> z = m.Row(2).Dot(v);
    return typename MatrixImpl<T>::ColumnVector(x, y, z);
  } else if constexpr (T::kRows == 4 && T::kCols == 4) {
    const ScalarImpl<T> x = m.Row(0).Dot(v);
    const ScalarImpl<T> y = m.Row(1).Dot(v);
    const ScalarImpl<T> z = m.Row(2).Dot(v);
    const ScalarImpl<T> w = m.Row(3).Dot(v);
    return typename MatrixImpl<T>::ColumnVector(x, y, z, w);
  } else {
    typename MatrixImpl<T>::ColumnVector result(ScalarImpl<T>(0));
    for (int rr = 0; rr < T::kRows; ++rr) {
      for (int cc = 0; cc < T::kCols; ++cc) {
        result[rr] += m.cols[cc][rr] * v.data[cc];
      }
    }
    return result;
  }
}

// Multiplies a 4x4 matrix with a 3-dimensional vector.
//
// Assumes the vector has a w-component equal to 1.
template <typename Scalar, bool Simd>
constexpr auto operator*(const Matrix<Scalar, 4, 4, Simd>& m,
                         const Vector<Scalar, 3, Simd>& v) {
  using Vec3 = Vector<Scalar, 3, Simd>;
  using Vec4 = Vector<Scalar, 4, Simd>;

  const Vec4 tmp = m * Vec4(v, Scalar(1));
  const Scalar inv = Scalar(1) / tmp.data[3];
  return Vec3(tmp.data[0] * inv, tmp.data[1] * inv, tmp.data[2] * inv);
}

// Multiplies a 3x4 matrix by a 3-dimensional vector.
//
// Assumes the vector has a w-component equal to 1.
template <typename Scalar, bool Simd>
constexpr auto operator*(const Matrix<Scalar, 3, 4, Simd>& m,
                         const Vector<Scalar, 3, Simd>& v) {
  return m * Vec4(v, Scalar(1));
}

// Creates a matrix from a set of column vectors.
template <typename T>
constexpr auto MatrixFromColumns(const VectorImpl<T>& c0,
                                 const VectorImpl<T>& c1) {
  constexpr int Rows = T::kDims;
  constexpr int Cols = 2;
  using Mat = Matrix<typename T::Scalar, Rows, Cols, T::kSimd>;

  Mat m;
  for (int i = 0; i < Rows; ++i) {
    m.cols[0][i] = c0[i];
    m.cols[1][i] = c1[i];
  }
  return m;
}

// Creates a matrix from a set of column vectors.
template <typename T>
constexpr auto MatrixFromColumns(const VectorImpl<T>& c0,
                                 const VectorImpl<T>& c1,
                                 const VectorImpl<T>& c2) {
  constexpr int Rows = T::kDims;
  constexpr int Cols = 3;
  using Mat = Matrix<typename T::Scalar, Rows, Cols, T::kSimd>;

  Mat m;
  for (int i = 0; i < Rows; ++i) {
    m.cols[0][i] = c0[i];
    m.cols[1][i] = c1[i];
    m.cols[2][i] = c2[i];
  }
  return m;
}

// Creates a matrix from a set of column vectors.
template <typename T>
constexpr auto MatrixFromColumns(const VectorImpl<T>& c0,
                                 const VectorImpl<T>& c1,
                                 const VectorImpl<T>& c2,
                                 const VectorImpl<T>& c3) {
  constexpr int Rows = T::kDims;
  constexpr int Cols = 4;
  using Mat = Matrix<typename T::Scalar, Rows, Cols, T::kSimd>;

  Mat m;
  for (int i = 0; i < Rows; ++i) {
    m.cols[0][i] = c0[i];
    m.cols[1][i] = c1[i];
    m.cols[2][i] = c2[i];
    m.cols[3][i] = c3[i];
  }
  return m;
}

// Creates a projection matrix similar to glFrustum. The left/right/bottom/top
// values the tangents of the angles.
template <typename T, bool kSimd = kEnableSimdByDefault>
constexpr auto FrustumMatrix(T x_left, T x_right, T y_bottom, T y_top, T z_near,
                             T z_far) {
  CHECK_LT(x_left, x_right);
  CHECK_LT(y_bottom, y_top);
  CHECK_LT(z_near, z_far);
  CHECK_GT(x_right - x_left, kDefaultEpsilon);
  CHECK_GT(y_top - y_bottom, kDefaultEpsilon);
  CHECK_GT(z_far - z_near, kDefaultEpsilon);
  CHECK_GE(z_near, T(0));

  const T x = (T(2) * z_near) / (x_right - x_left);
  const T y = (T(2) * z_near) / (y_top - y_bottom);
  const T a = (x_right + x_left) / (x_right - x_left);
  const T b = (y_top + y_bottom) / (y_top - y_bottom);
  const T c = (z_near + z_far) / (z_near - z_far);
  const T d = (T(2) * z_near * z_far) / (z_near - z_far);

  using Matrix44 = Matrix<T, 4, 4, kSimd>;
  // clang-format off
  return Matrix44(x, 0, a, 0,
                  0, y, b, 0,
                  0, 0, c, d,
                  0, 0, -1, 0);
  // clang-format on
}

// Used by PerspectiveMatrix calculation to apply an aspect ratio to the field
// of view angle.
enum class FovDirection {
  Horizontal,
  Vertical,
};

// Creates a perspective projection matrix.
template <typename T, bool kSimd = kEnableSimdByDefault>
constexpr auto PerspectiveMatrix(
    T y_fov, T aspect_ratio, T z_near, T z_far,
    FovDirection direction = FovDirection::Vertical) {
  const T tan_fov = std::tan(y_fov * T(0.5));

  T x = tan_fov * z_near;
  T y = tan_fov * z_near;
  if (direction == FovDirection::Vertical) {
    x *= aspect_ratio;
  } else {
    y *= aspect_ratio;
  }
  return FrustumMatrix<T, kSimd>(-x, x, -y, y, z_near, z_far);
}

// Creates a orthographic projection matrix.
template <typename T, bool kSimd = kEnableSimdByDefault>
constexpr auto OrthographicMatrix(T x_left, T x_right, T y_bottom, T y_top,
                                  T z_near, T z_far) {
  CHECK_GT(std::abs(x_right - x_left), kDefaultEpsilon);
  CHECK_GT(std::abs(y_top - y_bottom), kDefaultEpsilon);
  CHECK_GT(std::abs(z_far - z_near), kDefaultEpsilon);
  const T x = T(2) / (x_right - x_left);
  const T y = T(2) / (y_top - y_bottom);
  const T z = T(2) / (z_near - z_far);
  const T a = (x_left + x_right) / (x_left - x_right);
  const T b = (y_bottom + y_top) / (y_bottom - y_top);
  const T c = (z_near + z_far) / (z_near - z_far);

  using Matrix44 = Matrix<T, 4, 4, kSimd>;
  // clang-format off
  return Matrix44(x, 0, 0, a,
                  0, y, 0, b,
                  0, 0, z, c,
                  0, 0, 0, 1);
  // clang-format on
}

// Creates a camera view matrix.
template <typename T, bool kSimd = kEnableSimdByDefault>
constexpr auto LookInDirectionViewMatrix(const Vector<T, 3, kSimd>& dir,
                                         const Vector<T, 3, kSimd>& eye,
                                         const Vector<T, 3, kSimd>& up) {
  using Vector3 = Vector<T, 3, kSimd>;
  using Matrix44 = Matrix<T, 4, 4, kSimd>;

  const auto front = dir.Normalized();
  const auto right = up.Cross(front).Normalized();
  const auto new_up = front.Cross(right);
  const Vector3 pos(right.Dot(eye), new_up.Dot(eye), front.Dot(eye));

  // clang-format off
  return Matrix44(right.x, new_up.x, front.x, -pos.x,
                  right.y, new_up.y, front.y, -pos.y,
                  right.z, new_up.z, front.z, -pos.z,
                  0, 0, 0, 1);
  // clang-format on
}

// Creates a camera view matrix.
template <typename T, bool kSimd = kEnableSimdByDefault>
constexpr auto LookAtViewMatrix(const Vector<T, 3, kSimd>& at,
                                const Vector<T, 3, kSimd>& eye,
                                const Vector<T, 3, kSimd>& up) {
  return LookInDirectionViewMatrix<T, kSimd>((at - eye), eye, up);
}

template <typename T>
constexpr MatrixImpl<T> MatrixImpl<T>::Zero() {
  return MatrixImpl(Scalar(0));
}

template <typename T>
constexpr MatrixImpl<T> MatrixImpl<T>::Identity() {
  if constexpr (kRows == 2 && kCols == 2) {
    return MatrixImpl(1, 0, 0, 1);
  } else if constexpr (kRows == 3 && kCols == 3) {
    return MatrixImpl(1, 0, 0, 0, 1, 0, 0, 0, 1);
  } else if constexpr (kRows == 4 && kCols == 4) {
    return MatrixImpl(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
  } else {
    constexpr int N = kRows < kCols ? kRows : kCols;
    MatrixImpl identity(Scalar(0));
    for (int i = 0; i < N; ++i) {
      identity.cols[i][i] = Scalar(1);
    }
    return identity;
  }
}
}  // namespace redux

REDUX_SETUP_TYPEID(redux::mat2);
REDUX_SETUP_TYPEID(redux::mat3);
REDUX_SETUP_TYPEID(redux::mat4);
REDUX_SETUP_TYPEID(redux::mat34);

#endif  // REDUX_MODULES_MATH_MATRIX_H_
