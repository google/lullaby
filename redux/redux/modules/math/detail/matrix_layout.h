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

#ifndef REDUX_MODULES_MATH_DETAIL_MATRIX_LAYOUT_H_
#define REDUX_MODULES_MATH_DETAIL_MATRIX_LAYOUT_H_

#include <type_traits>

#include "vectorial/simd4f.h"

namespace redux {
namespace detail {

template <class T, int Rows, int Cols, bool TrySimd>
struct MatrixLayout {
  using Scalar = T;
  static constexpr int kRows = Rows;
  static constexpr int kCols = Cols;

  // Just because a type requests simd support doesn't mean we actually use it.
  // Only 2x2, 3x3, and 4x3, and 4x4 dimensional floating point matrices
  // currently support simd.
  static constexpr bool kSimd = false;

  constexpr MatrixLayout() {}

  using ColArray = T[Rows];
  union {
    T data[Rows * Cols] = {};
    ColArray cols[Cols];
  };
};

template <class T, bool TrySimd>
struct MatrixLayout<T, 2, 2, TrySimd> {
  using Scalar = T;
  static constexpr int kRows = 2;
  static constexpr int kCols = 2;

  static constexpr bool kSimd = false;

  constexpr MatrixLayout() {}

  using ColArray = T[2];
  union {
    T data[4] = {};
    ColArray cols[2];

    struct {
      T m00;
      T m10;
      T m01;
      T m11;
    };
  };
};

template <class T, bool TrySimd>
struct MatrixLayout<T, 3, 3, TrySimd> {
  using Scalar = T;
  static constexpr int kRows = 3;
  static constexpr int kCols = 3;

  static constexpr bool kSimd = false;

  constexpr MatrixLayout() {}

  using ColArray = T[3];
  union {
    T data[9] = {};
    ColArray cols[3];

    struct {
      T m00;
      T m10;
      T m20;
      T m01;
      T m11;
      T m21;
      T m02;
      T m12;
      T m22;
    };
  };
};

template <class T, bool TrySimd>
struct MatrixLayout<T, 4, 3, TrySimd> {
  using Scalar = T;
  static constexpr int kRows = 4;
  static constexpr int kCols = 3;

  static constexpr bool kSimd = false;

  constexpr MatrixLayout() {}

  using ColArray = T[4];
  union {
    T data[12] = {};
    ColArray cols[3];

    struct {
      T m00;
      T m10;
      T m20;
      T m30;

      T m01;
      T m11;
      T m21;
      T m31;

      T m02;
      T m12;
      T m22;
      T m32;
    };
  };
};

template <class T, bool TrySimd>
struct MatrixLayout<T, 4, 4, TrySimd> {
  using Scalar = T;
  static constexpr int kRows = 4;
  static constexpr int kCols = 4;

  static constexpr bool kSimd = false;

  constexpr MatrixLayout() {}

  using ColArray = T[4];
  union {
    T data[16] = {};
    ColArray cols[4];

    struct {
      T m00;
      T m10;
      T m20;
      T m30;
      T m01;
      T m11;
      T m21;
      T m31;
      T m02;
      T m12;
      T m22;
      T m32;
      T m03;
      T m13;
      T m23;
      T m33;
    };
  };
};

template <>
struct MatrixLayout<float, 2, 2, true> {
  using Scalar = float;
  static constexpr int kRows = 2;
  static constexpr int kCols = 2;

  static constexpr bool kSimd = true;

  using ColArray = float[4];  // 4 to match simd4f
  union {
    float data[8] = {};  // include padding!
    ColArray cols[2];
    simd4f simd[2];

    struct {
      float m00;
      float m10;
      float padding_20;
      float padding_30;
      float m01;
      float m11;
      float padding_21;
      float padding_31;
    };
  };
};

template <>
struct MatrixLayout<float, 3, 3, true> {
  using Scalar = float;
  static constexpr int kRows = 3;
  static constexpr int kCols = 3;

  static constexpr bool kSimd = true;

  constexpr MatrixLayout() {}

  using ColArray = float[4];  // 4 to match simd4f
  union {
    float data[12] = {};  // include padding!
    ColArray cols[3];
    simd4f simd[3];

    struct {
      float m00;
      float m10;
      float m20;
      float padding_m30;
      float m01;
      float m11;
      float m21;
      float padding_m31;
      float m02;
      float m12;
      float m22;
      float padding_m32;
    };
  };
};

template <>
struct MatrixLayout<float, 4, 3, true> {
  using Scalar = float;
  static constexpr int kRows = 4;
  static constexpr int kCols = 3;

  static constexpr bool kSimd = true;

  constexpr MatrixLayout() {}

  using ColArray = float[4];
  union {
    float data[12] = {};
    ColArray cols[3];
    simd4f simd[3];

    struct {
      float m00;
      float m10;
      float m20;
      float m01;
      float m11;
      float m21;
      float m02;
      float m12;
      float m22;
      float m03;
      float m13;
      float m23;
    };
  };
};

template <>
struct MatrixLayout<float, 4, 4, true> {
  using Scalar = float;
  static constexpr int kRows = 4;
  static constexpr int kCols = 4;

  static constexpr bool kSimd = true;

  constexpr MatrixLayout() {}

  using ColArray = float[4];
  union {
    float data[16] = {};
    ColArray cols[4];
    simd4f simd[4];

    struct {
      float m00;
      float m10;
      float m20;
      float m30;
      float m01;
      float m11;
      float m21;
      float m31;
      float m02;
      float m12;
      float m22;
      float m32;
      float m03;
      float m13;
      float m23;
      float m33;
    };
  };
};

}  // namespace detail
}  // namespace redux

#endif  // REDUX_MODULES_MATH_DETAIL_MATRIX_LAYOUT_H_
