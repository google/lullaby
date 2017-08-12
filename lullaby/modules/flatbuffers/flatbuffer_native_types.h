/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_MODULES_FLATBUFFERS_FLATBUFFER_NATIVE_TYPES_H_
#define LULLABY_MODULES_FLATBUFFERS_FLATBUFFER_NATIVE_TYPES_H_

#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "lullaby/util/color.h"
#include "lullaby/util/math.h"

namespace lull {

// Several flatbuffer structs have been marked with a "native_type" attribute.
// These structs are designed to serialize directly into non-flatbuffer
// generated types such as mathfu types.  In order for serialization to work,
// a set of conversion functions needs to be implemented for them.  This is done
// by specializing the FlatbufferNativeType class with the type in question.
template <typename T>
struct FlatbufferNativeType {};

// Specialization for mathfu::vec2 which has a Flatbuffer type of lull::Vec2.
template <>
struct FlatbufferNativeType<mathfu::vec2> {
  static constexpr size_t kFlatbufferStructSize = 2 * sizeof(float);
  static constexpr size_t kFlatbufferStructAlignment = 4;

  static mathfu::vec2 Read(const void* src, size_t len) {
    if (src && len >= kFlatbufferStructSize) {
      const float* values = reinterpret_cast<const float*>(src);
      return mathfu::vec2(values);
    } else {
      return mathfu::kZeros2f;
    }
  }

  static void Write(const mathfu::vec2& src, void* dst, size_t len) {
    if (dst && len >= kFlatbufferStructSize) {
      float* values = reinterpret_cast<float*>(dst);
      values[0] = src.x;
      values[1] = src.y;
    }
  }
};

// Specialization for mathfu::vec2i which has a Flatbuffer type of lull::Vec2i.
template <>
struct FlatbufferNativeType<mathfu::vec2i> {
  static constexpr size_t kFlatbufferStructSize = 2 * sizeof(int);
  static constexpr size_t kFlatbufferStructAlignment = 4;

  static mathfu::vec2i Read(const void* src, size_t len) {
    if (src && len >= kFlatbufferStructSize) {
      const int* values = reinterpret_cast<const int*>(src);
      return mathfu::vec2i(values);
    } else {
      return mathfu::kZeros2i;
    }
  }

  static void Write(const mathfu::vec2i& src, void* dst, size_t len) {
    if (dst && len >= kFlatbufferStructSize) {
      int* values = reinterpret_cast<int*>(dst);
      values[0] = src.x;
      values[1] = src.y;
    }
  }
};

// Specialization for mathfu::vec3 which has a Flatbuffer type of lull::Vec3.
template <>
struct FlatbufferNativeType<mathfu::vec3> {
  static constexpr size_t kFlatbufferStructSize = 3 * sizeof(float);
  static constexpr size_t kFlatbufferStructAlignment = 4;

  static mathfu::vec3 Read(const void* src, size_t len) {
    if (src && len >= kFlatbufferStructSize) {
      const float* values = reinterpret_cast<const float*>(src);
      return mathfu::vec3(values);
    } else {
      return mathfu::kZeros3f;
    }
  }

  static void Write(const mathfu::vec3& src, void* dst, size_t len) {
    if (dst && len >= kFlatbufferStructSize) {
      float* values = reinterpret_cast<float*>(dst);
      values[0] = src.x;
      values[1] = src.y;
      values[2] = src.z;
    }
  }
};

// Specialization for mathfu::vec4 which has a Flatbuffer type of lull::Vec4.
template <>
struct FlatbufferNativeType<mathfu::vec4> {
  static constexpr size_t kFlatbufferStructSize = 4 * sizeof(float);
  static constexpr size_t kFlatbufferStructAlignment = 4;

  static mathfu::vec4 Read(const void* src, size_t len) {
    if (src && len >= kFlatbufferStructSize) {
      const float* values = reinterpret_cast<const float*>(src);
      return mathfu::vec4(values);
    } else {
      return mathfu::kZeros4f;
    }
  }

  static void Write(const mathfu::vec4& src, void* dst, size_t len) {
    if (dst && len >= kFlatbufferStructSize) {
      float* values = reinterpret_cast<float*>(dst);
      values[0] = src.x;
      values[1] = src.y;
      values[2] = src.z;
      values[3] = src.w;
    }
  }
};

// Specialization for mathfu::quat which has a Flatbuffer type of lull::Quat.
template <>
struct FlatbufferNativeType<mathfu::quat> {
  static constexpr size_t kFlatbufferStructSize = 4 * sizeof(float);
  static constexpr size_t kFlatbufferStructAlignment = 4;

  static mathfu::quat Read(const void* src, size_t len) {
    mathfu::quat result;
    if (src && len >= kFlatbufferStructSize) {
      const float* values = reinterpret_cast<const float*>(src);
      result.vector() = mathfu::vec3(values);
      result.scalar() = values[3];
    } else {
      result = mathfu::quat::identity;
    }
    return result;
  }

  static void Write(const mathfu::quat& src, void* dst, size_t len) {
    if (dst && len >= kFlatbufferStructSize) {
      float* values = reinterpret_cast<float*>(dst);
      values[0] = src.vector().x;
      values[1] = src.vector().y;
      values[2] = src.vector().z;
      values[3] = src.scalar();
    }
  }
};

// Specialization for mathfu::rectf which has a Flatbuffer type of lull::Rect.
template <>
struct FlatbufferNativeType<mathfu::rectf> {
  static constexpr size_t kFlatbufferStructSize = 4 * sizeof(float);
  static constexpr size_t kFlatbufferStructAlignment = 4;

  static mathfu::rectf Read(const void* src, size_t len) {
    mathfu::rectf result;
    if (src && len >= kFlatbufferStructSize) {
      const float* values = reinterpret_cast<const float*>(src);
      result.pos.x = values[0];
      result.pos.y = values[1];
      result.size.x = values[2];
      result.size.y = values[3];
    }
    return result;
  }

  static void Write(const mathfu::rectf& src, void* dst, size_t len) {
    if (dst && len >= kFlatbufferStructSize) {
      float* values = reinterpret_cast<float*>(dst);
      values[0] = src.pos.x;
      values[1] = src.pos.y;
      values[2] = src.size.x;
      values[3] = src.size.y;
    }
  }
};

// Specialization for lull::Aabb which has a Flatbuffer type of lull::AabbDef.
template <>
struct FlatbufferNativeType<lull::Aabb> {
  static constexpr size_t kFlatbufferStructSize = 6 * sizeof(float);
  static constexpr size_t kFlatbufferStructAlignment = 4;

  static lull::Aabb Read(const void* src, size_t len) {
    lull::Aabb result;
    if (src && len >= kFlatbufferStructSize) {
      const float* values = reinterpret_cast<const float*>(src);
      result.min = mathfu::vec3(&values[0]);
      result.max = mathfu::vec3(&values[3]);
    } else {
      result = lull::Aabb();
    }
    return result;
  }

  static void Write(const lull::Aabb& src, void* dst, size_t len) {
    if (dst && len >= kFlatbufferStructSize) {
      float* values = reinterpret_cast<float*>(dst);
      values[0] = src.min.x;
      values[1] = src.min.y;
      values[2] = src.min.z;
      values[3] = src.max.x;
      values[4] = src.max.y;
      values[5] = src.max.z;
    }
  }
};

// Specialization for lull::Color4ub which has a Flatbuffer type of lull::Color.
template <>
struct FlatbufferNativeType<lull::Color4ub> {
  static constexpr size_t kFlatbufferStructSize = 4 * sizeof(float);
  static constexpr size_t kFlatbufferStructAlignment = 4;

  static lull::Color4ub Read(const void* src, size_t len) {
    lull::Color4ub result;
    if (src && len >= kFlatbufferStructSize) {
      const float* values = reinterpret_cast<const float*>(src);
      result.r = static_cast<uint8_t>(values[0] * 255);
      result.g = static_cast<uint8_t>(values[1] * 255);
      result.b = static_cast<uint8_t>(values[2] * 255);
      result.a = static_cast<uint8_t>(values[3] * 255);
    } else {
      result = lull::Color4ub();
    }
    return result;
  }

  static void Write(const lull::Color4ub& src, void* dst, size_t len) {
    if (dst && len >= kFlatbufferStructSize) {
      float* values = reinterpret_cast<float*>(dst);
      values[0] = static_cast<float>(src.r) / 255.f;
      values[1] = static_cast<float>(src.g) / 255.f;
      values[2] = static_cast<float>(src.b) / 255.f;
      values[3] = static_cast<float>(src.a) / 255.f;
    }
  }
};

}  // namespace lull

#endif  // LULLABY_MODULES_FLATBUFFERS_FLATBUFFER_NATIVE_TYPES_H_
