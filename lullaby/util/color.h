/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

struct Color4f;
struct Color4ub {
  Color4ub();
  Color4ub(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
  explicit Color4ub(uint32_t rgba);
  bool operator==(Color4ub rhs) const;
  bool operator!=(Color4ub rhs) const;

  static Color4ub FromARGB(uint32_t argb);
  static Color4ub FromVec4(const mathfu::vec4& vec);
  static Color4ub FromColor4f(const Color4f& color);
  static mathfu::vec4 ToVec4(const Color4ub color);

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

struct Color4f {
  Color4f(float s = 1.0f);
  Color4f(float r, float g, float b, float a);

  static Color4f FromARGB(uint32_t argb);
  static Color4f FromVec4(const mathfu::vec4& vec);
  static Color4f FromColor4ub(const Color4ub& color);
  static mathfu::vec4 ToVec4(const Color4f& color);

  float& operator[](const int i);
  float operator[](const int i) const;

  static Color4f Lerp(const Color4f& lhs, const Color4f& rhs,
                      const float percent);
  static Color4f Max(const Color4f& lhs, const Color4f& rhs);
  static Color4f Min(const Color4f& lhs, const Color4f& rhs);

  Color4f& operator*=(const Color4f& c);
  Color4f& operator/=(const Color4f& c);
  Color4f& operator+=(const Color4f& c);
  Color4f& operator-=(const Color4f& c);
  Color4f& operator*=(float s);
  Color4f& operator/=(float s);
  Color4f& operator+=(float s);
  Color4f& operator-=(float s);
  bool operator==(const Color4f& c) const;
  bool operator!=(const Color4f& c) const;

  union {
    struct {
        float r;
        float g;
        float b;
        float a;
    };
    float data[4];
  };
};

Color4f operator-(const Color4f& c);
Color4f operator*(const Color4f& lhs, const Color4f& rhs);
Color4f operator/(const Color4f& lhs, const Color4f& rhs);
Color4f operator+(const Color4f& lhs, const Color4f& rhs);
Color4f operator-(const Color4f& lhs, const Color4f& rhs);
Color4f operator*(const Color4f& lhs, float rhs);
Color4f operator/(const Color4f& lhs, float rhs);
Color4f operator+(const Color4f& lhs, float rhs);
Color4f operator-(const Color4f& lhs, float rhs);
Color4f operator*(float lhs, const Color4f& rhs);
Color4f operator/(float lhs, const Color4f& rhs);
Color4f operator+(float lhs, const Color4f& rhs);
Color4f operator-(float lhs, const Color4f& rhs);

}  // namespace lull

#endif  // LULLABY_UTIL_COLOR_H_
