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

#ifndef REDUX_MODULES_GRAPHICS_COLOR_H_
#define REDUX_MODULES_GRAPHICS_COLOR_H_

#include <cstddef>

#include "redux/modules/math/vector.h"

namespace redux {

// Forward-declarations of color types.
struct Color4f;
struct Color4ub;

// Linear RGBA Color type represented with 4 unsigned bytes.
struct Color4ub {
  Color4ub();
  Color4ub(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
  explicit Color4ub(std::uint32_t rgba);

  static Color4ub FromARGB(std::uint32_t argb);
  static Color4ub FromVec4(const vec4& vec);
  static Color4ub FromColor4f(const Color4f& color);
  static vec4 ToVec4(const Color4ub color);

  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
  std::uint8_t a;

  friend bool operator==(Color4ub lhs, Color4ub rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
  }

  friend bool operator!=(Color4ub lhs, Color4ub rhs) { return !(lhs == rhs); }
};

// Linear RGBA Color type represented with 4 floating-point values.
struct Color4f {
  explicit Color4f(float s = 1.0f);
  Color4f(float r, float g, float b, float a);

  static Color4f FromARGB(std::uint32_t argb);
  static Color4f FromVec4(const vec4& vec);
  static Color4f FromColor4ub(const Color4ub& color);
  static vec4 ToVec4(const Color4f& color);

  static Color4f Lerp(const Color4f& lhs, const Color4f& rhs,
                      const float percent);
  static Color4f Max(const Color4f& lhs, const Color4f& rhs);
  static Color4f Min(const Color4f& lhs, const Color4f& rhs);

  float r;
  float g;
  float b;
  float a;

  Color4f& operator*=(const Color4f& rhs) {
    r *= rhs.r;
    g *= rhs.g;
    b *= rhs.b;
    a *= rhs.a;
    return *this;
  }

  Color4f& operator/=(const Color4f& rhs) {
    r /= rhs.r;
    g /= rhs.g;
    b /= rhs.b;
    a /= rhs.a;
    return *this;
  }

  Color4f& operator+=(const Color4f& rhs) {
    r += rhs.r;
    g += rhs.g;
    b += rhs.b;
    a += rhs.a;
    return *this;
  }

  Color4f& operator-=(const Color4f& rhs) {
    r -= rhs.r;
    g -= rhs.g;
    b -= rhs.b;
    a -= rhs.a;
    return *this;
  }

  Color4f& operator*=(float s) {
    r *= s;
    g *= s;
    b *= s;
    a *= s;
    return *this;
  }

  Color4f& operator/=(float s) {
    r /= s;
    g /= s;
    b /= s;
    a /= s;
    return *this;
  }

  Color4f& operator+=(float s) {
    r += s;
    g += s;
    b += s;
    a += s;
    return *this;
  }

  Color4f& operator-=(float s) {
    r -= s;
    g -= s;
    b -= s;
    a -= s;
    return *this;
  }

  friend bool operator==(const Color4f& lhs, const Color4f& rhs) {
    return (lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b &&
            lhs.a == rhs.a);
  }

  friend bool operator!=(const Color4f& lhs, const Color4f& rhs) {
    return (lhs.r != rhs.r || lhs.g != rhs.g || lhs.b != rhs.b ||
            lhs.a != rhs.a);
  }

  friend Color4f operator-(const Color4f& c) {
    return Color4f(-c.r, -c.g, -c.b, -c.a);
  }

  friend Color4f operator*(const Color4f& lhs, const Color4f& rhs) {
    return Color4f(lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b, lhs.a * rhs.a);
  }

  friend Color4f operator/(const Color4f& lhs, const Color4f& rhs) {
    return Color4f(lhs.r / rhs.r, lhs.g / rhs.g, lhs.b / rhs.b, lhs.a / rhs.a);
  }

  friend Color4f operator+(const Color4f& lhs, const Color4f& rhs) {
    return Color4f(lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a);
  }

  friend Color4f operator-(const Color4f& lhs, const Color4f& rhs) {
    return Color4f(lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a);
  }

  friend Color4f operator*(const Color4f& lhs, float rhs) {
    return Color4f(lhs.r * rhs, lhs.g * rhs, lhs.b * rhs, lhs.a * rhs);
  }

  friend Color4f operator/(const Color4f& lhs, float rhs) {
    return Color4f(lhs.r / rhs, lhs.g / rhs, lhs.b / rhs, lhs.a / rhs);
  }

  friend Color4f operator+(const Color4f& lhs, float rhs) {
    return Color4f(lhs.r + rhs, lhs.g + rhs, lhs.b + rhs, lhs.a + rhs);
  }

  friend Color4f operator-(const Color4f& lhs, float rhs) {
    return Color4f(lhs.r - rhs, lhs.g - rhs, lhs.b - rhs, lhs.a - rhs);
  }

  friend Color4f operator*(float lhs, const Color4f& rhs) {
    return Color4f(lhs * rhs.r, lhs * rhs.g, lhs * rhs.b, lhs * rhs.a);
  }

  friend Color4f operator/(float lhs, const Color4f& rhs) {
    return Color4f(lhs / rhs.r, lhs / rhs.g, lhs / rhs.b, lhs / rhs.a);
  }

  friend Color4f operator+(float lhs, const Color4f& rhs) {
    return Color4f(lhs + rhs.r, lhs + rhs.g, lhs + rhs.b, lhs + rhs.a);
  }

  friend Color4f operator-(float lhs, const Color4f& rhs) {
    return Color4f(lhs - rhs.r, lhs - rhs.g, lhs - rhs.b, lhs - rhs.a);
  }
};

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_COLOR_H_
