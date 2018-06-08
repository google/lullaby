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

#ifndef LULLABY_UTIL_COLOR_H_
#define LULLABY_UTIL_COLOR_H_

#include <stdint.h>

#include "mathfu/glsl_mappings.h"

namespace lull {

struct Color4ub {
  Color4ub() : r(255), g(255), b(255), a(255) {}
  Color4ub(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      : r(r), g(g), b(b), a(a) {}
  explicit Color4ub(uint32_t rgba)
      : r(static_cast<uint8_t>((rgba >> 24) & 0xff)),
        g(static_cast<uint8_t>((rgba >> 16) & 0xff)),
        b(static_cast<uint8_t>((rgba >>  8) & 0xff)),
        a(static_cast<uint8_t>( rgba        & 0xff)) {}
  explicit Color4ub(const mathfu::vec4& v)
      : r(static_cast<uint8_t>(v.x * 255)),
        g(static_cast<uint8_t>(v.y * 255)),
        b(static_cast<uint8_t>(v.z * 255)),
        a(static_cast<uint8_t>(v.w * 255)) {}
  bool operator==(Color4ub rhs) const {
    return (packed == rhs.packed);
  }
  bool operator!=(Color4ub rhs) const {
    return (packed != rhs.packed);
  }

  static Color4ub FromARGB(uint32_t argb) {
    Color4ub result;
    result.a = static_cast<uint8_t>((argb >> 24) & 0xff);
    result.r = static_cast<uint8_t>((argb >> 16) & 0xff);
    result.g = static_cast<uint8_t>((argb >>  8) & 0xff);
    result.b = static_cast<uint8_t>( argb        & 0xff);
    return result;
  }

  static mathfu::vec4 ToVec4(const Color4ub color) {
    return mathfu::vec4(1.0f / 255.0f * static_cast<float>(color.r),
                        1.0f / 255.0f * static_cast<float>(color.g),
                        1.0f / 255.0f * static_cast<float>(color.b),
                        1.0f / 255.0f * static_cast<float>(color.a));
  }

  union {
    struct {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      uint8_t a;
    };
    uint32_t packed;
  };
};

}  // namespace lull

#endif  // LULLABY_UTIL_COLOR_H_
