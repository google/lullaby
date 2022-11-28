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

#ifndef REDUX_MODULES_FLATBUFFERS_MATH_H_
#define REDUX_MODULES_FLATBUFFERS_MATH_H_

#include "redux/modules/flatbuffers/common.h"
#include "redux/modules/flatbuffers/math_generated.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Converts a flatbuffer Vec2f into a vec2.
inline vec2 ReadFbs(const fbs::Vec2f& in) { return {in.x(), in.y()}; }

// Converts a flatbuffer Vec2i into a vec2i.
inline vec2i ReadFbs(const fbs::Vec2i& in) { return {in.x(), in.y()}; }

// Converts a flatbuffer Vec3f into a vec3.
inline vec3 ReadFbs(const fbs::Vec3f& in) { return {in.x(), in.y(), in.z()}; }

// Converts a flatbuffer Vec3i into a vec3i.
inline vec3i ReadFbs(const fbs::Vec3i& in) { return {in.x(), in.y(), in.z()}; }

// Converts a flatbuffer Vec4f into a vec4.
inline vec4 ReadFbs(const fbs::Vec4f& in) {
  return {in.x(), in.y(), in.z(), in.w()};
}

// Converts a flatbuffer Vec4i into a vec4i.
inline vec4i ReadFbs(const fbs::Vec4i& in) {
  return {in.x(), in.y(), in.z(), in.w()};
}

// Converts a flatbuffer Quatf into a quat.
inline quat ReadFbs(const fbs::Quatf& in) {
  return quat(in.x(), in.y(), in.z(), in.w()).Normalized();
}

// Converts a flatbuffer Mat4x4 into a mat4.
inline mat4 ReadFbs(const fbs::Mat4x4f& in) {
  const vec4 c0 = ReadFbs(in.col0());
  const vec4 c1 = ReadFbs(in.col1());
  const vec4 c2 = ReadFbs(in.col2());
  const vec4 c3 = ReadFbs(in.col3());
  return MatrixFromColumns(c0, c1, c2, c3);
}

// Converts a flatbuffer Rectf into a Bounds2f.
inline Bounds2f ReadFbs(const fbs::Rectf& in) {
  const vec2 pos(in.x(), in.y());
  const vec2 size(in.w(), in.h());
  return Bounds2f(pos, pos + size);
}

// Converts a flatbuffer Recti into a Bounds2i.
inline Bounds2i ReadFbs(const fbs::Recti& in) {
  const vec2i pos(in.x(), in.y());
  const vec2i size(in.w(), in.h());
  return Bounds2i(pos, pos + size);
}

// Converts a flatbuffer Boxf into a Box.
inline Box ReadFbs(const fbs::Boxf& in) {
  const vec3 min = ReadFbs(in.min());
  const vec3 max = ReadFbs(in.max());
  return {min, max};
}

}  // namespace redux

#endif  // REDUX_MODULES_FLATBUFFERS_MATH_H_
