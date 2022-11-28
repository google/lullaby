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

#ifndef REDUX_MODULES_MATH_DETAIL_VECTOR_LAYOUT_H_
#define REDUX_MODULES_MATH_DETAIL_VECTOR_LAYOUT_H_

#include "vectorial/simd4f.h"

namespace redux {
namespace detail {

template <class T, int N, bool TrySimd>
struct VectorLayout {
  using Scalar = T;
  static constexpr int kDims = N;

  // Just because a type requests simd support doesn't mean we actually use it.
  // Only 2, 3, and 4 dimensional floating point vectors can support simd.
  static constexpr bool kSimd = false;

  constexpr VectorLayout() {}

  T data[N] = {};
};

template <class T, bool TrySimd>
struct VectorLayout<T, 2, TrySimd> {
  using Scalar = T;
  static constexpr int kDims = 2;

  static constexpr bool kSimd = false;

  constexpr VectorLayout() {}

  union {
    T data[2] = {};
    struct {
      T x;
      T y;
    };
  };
};

template <class T, bool TrySimd>
struct VectorLayout<T, 3, TrySimd> {
  using Scalar = T;
  static constexpr int kDims = 3;

  static constexpr bool kSimd = false;

  constexpr VectorLayout() {}

  union {
    T data[3] = {};
    struct {
      T x;
      T y;
      T z;
    };
  };
};

template <class T, bool TrySimd>
struct VectorLayout<T, 4, TrySimd> {
  using Scalar = T;
  static constexpr int kDims = 4;

  static constexpr bool kSimd = false;

  constexpr VectorLayout() {}

  union {
    T data[4] = {};
    struct {
      T x;
      T y;
      T z;
      T w;
    };
  };
};

template <>
struct VectorLayout<float, 2, true> {
  using Scalar = float;
  static constexpr int kDims = 2;

  static constexpr bool kSimd = true;

  constexpr VectorLayout() {}

  union {
    float data[4] = {};  // include simd padding!
    simd4f simd;
    struct {
      float x;
      float y;
    };
  };
};

template <>
struct VectorLayout<float, 3, true> {
  using Scalar = float;
  static constexpr int kDims = 3;

  static constexpr bool kSimd = true;

  constexpr VectorLayout() {}

  union {
    float data[4] = {};  // include simd padding!
    simd4f simd;
    struct {
      float x;
      float y;
      float z;
    };
  };
};

template <>
struct VectorLayout<float, 4, true> {
  using Scalar = float;
  static constexpr int kDims = 4;

  static constexpr bool kSimd = true;

  constexpr VectorLayout() {}

  union {
    float data[4] = {};
    simd4f simd;
    struct {
      float x;
      float y;
      float z;
      float w;
    };
  };
};

}  // namespace detail
}  // namespace redux

#endif  // REDUX_MODULES_MATH_DETAIL_VECTOR_LAYOUT_H_
