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

#ifndef REDUX_MODULES_MATH_DETAIL_QUATERNION_LAYOUT_H_
#define REDUX_MODULES_MATH_DETAIL_QUATERNION_LAYOUT_H_

#include <type_traits>

#include "vectorial/simd4f.h"

namespace redux {
namespace detail {

template <typename T, bool TrySimd>
struct QuaternionLayout {
  using Scalar = T;
  static constexpr int kDims = 4;
  static constexpr bool kSimd = false;

  constexpr QuaternionLayout() {}

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
struct QuaternionLayout<float, true> {
  using Scalar = float;
  static constexpr int kDims = 4;
  static constexpr bool kSimd = true;

  constexpr QuaternionLayout() {}

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

#endif  // REDUX_MODULES_MATH_DETAIL_QUATERNION_LAYOUT_H_
