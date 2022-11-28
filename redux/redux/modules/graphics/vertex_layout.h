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

#ifndef REDUX_MODULES_GRAPHICS_VERTEX_LAYOUT_H_
#define REDUX_MODULES_GRAPHICS_VERTEX_LAYOUT_H_

#include <cstddef>

#include "redux/modules/graphics/enums.h"

namespace redux::detail {

// Helper struct that will be of a specific layout depending on the type T and
// number of elements N. (We don't use math types here because we want to
// guarantee unaligned, non-simd types).
template <typename T, std::size_t N>
struct VertexLayout;

template <typename T>
struct VertexLayout<T, 0> {
  template <typename... U>
  void Set(U... args) {}
  template <typename V>
  void SetVector(const V& v) {}
};

template <typename T>
struct VertexLayout<T, 1> {
  template <typename U0>
  void Set(U0 x) {
    this->x = static_cast<T>(x);
  }
  template <typename V>
  void SetVector(const V& v) {
    this->x = static_cast<T>(v.x);
  }
  T x = 0;
};

template <typename T>
struct VertexLayout<T, 2> {
  template <typename U0, typename U1>
  void Set(U0 x, U1 y) {
    this->x = static_cast<T>(x);
    this->y = static_cast<T>(y);
  }
  template <typename V>
  void SetVector(const V& v) {
    this->x = static_cast<T>(v.x);
    this->y = static_cast<T>(v.y);
  }
  T x = 0;
  T y = 0;
};

template <typename T>
struct VertexLayout<T, 3> {
  template <typename U0, typename U1, typename U2>
  void Set(U0 x, U1 y, U2 z) {
    this->x = static_cast<T>(x);
    this->y = static_cast<T>(y);
    this->z = static_cast<T>(z);
  }
  template <typename V>
  void SetVector(const V& v) {
    this->x = static_cast<T>(v.x);
    this->y = static_cast<T>(v.y);
    this->z = static_cast<T>(v.z);
  }
  T x = 0;
  T y = 0;
  T z = 0;
};

template <typename T>
struct VertexLayout<T, 4> {
  template <typename U0, typename U1, typename U2, typename U3>
  void Set(U0 x, U1 y, U2 z, U3 w) {
    this->x = static_cast<T>(x);
    this->y = static_cast<T>(y);
    this->z = static_cast<T>(z);
    this->w = static_cast<T>(w);
  }
  template <typename V>
  void SetVector(const V& v) {
    this->x = static_cast<T>(v.x);
    this->y = static_cast<T>(v.y);
    this->z = static_cast<T>(v.z);
    this->w = static_cast<T>(v.w);
  }
  T x = 0;
  T y = 0;
  T z = 0;
  T w = 0;
};

// A helper struct that maps a VertexType to a VertexLayout.
template <VertexType Type>
struct VertexPayload;

template <>
struct VertexPayload<VertexType::Invalid> : VertexLayout<std::byte, 0> {};

template <>
struct VertexPayload<VertexType::Scalar1f> : VertexLayout<float, 1> {};

template <>
struct VertexPayload<VertexType::Vec2f> : VertexLayout<float, 2> {};

template <>
struct VertexPayload<VertexType::Vec3f> : VertexLayout<float, 3> {};

template <>
struct VertexPayload<VertexType::Vec4f> : VertexLayout<float, 4> {};

template <>
struct VertexPayload<VertexType::Vec2us> : VertexLayout<uint16_t, 2> {};

template <>
struct VertexPayload<VertexType::Vec4us> : VertexLayout<uint16_t, 4> {};

template <>
struct VertexPayload<VertexType::Vec4ub> : VertexLayout<uint8_t, 4> {};

}  // namespace redux::detail

#endif  // REDUX_MODULES_GRAPHICS_VERTEX_LAYOUT_H_
