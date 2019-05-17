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

#include "lullaby/util/color.h"

namespace lull {

Color4ub::Color4ub() : r(255), g(255), b(255), a(255) {}
Color4ub::Color4ub(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    : r(r), g(g), b(b), a(a) {}
Color4ub::Color4ub(uint32_t rgba)
    : r(static_cast<uint8_t>((rgba >> 24) & 0xff)),
      g(static_cast<uint8_t>((rgba >> 16) & 0xff)),
      b(static_cast<uint8_t>((rgba >> 8) & 0xff)),
      a(static_cast<uint8_t>(rgba & 0xff)) {}

bool Color4ub::operator==(Color4ub rhs) const { return (packed == rhs.packed); }
bool Color4ub::operator!=(Color4ub rhs) const { return (packed != rhs.packed); }

Color4ub Color4ub::FromARGB(uint32_t argb) {
  Color4ub result;
  result.a = static_cast<uint8_t>((argb >> 24) & 0xff);
  result.r = static_cast<uint8_t>((argb >> 16) & 0xff);
  result.g = static_cast<uint8_t>((argb >> 8) & 0xff);
  result.b = static_cast<uint8_t>(argb & 0xff);
  return result;
}

Color4ub Color4ub::FromVec4(const mathfu::vec4& vec) {
  const mathfu::vec4 clamped_vec4 = mathfu::vec4::Min(
      mathfu::vec4(1.0f), mathfu::vec4::Max(vec, mathfu::vec4(0.0f)));
  Color4ub result;
  result.r = static_cast<uint8_t>(clamped_vec4.x * 255);
  result.g = static_cast<uint8_t>(clamped_vec4.y * 255);
  result.b = static_cast<uint8_t>(clamped_vec4.z * 255);
  result.a = static_cast<uint8_t>(clamped_vec4.w * 255);
  return result;
}

Color4ub Color4ub::FromColor4f(const Color4f& color) {
  const Color4f clamped_color =
      Color4f::Min(Color4f(1.0f), Color4f::Max(color, Color4f(0.0f)));
  Color4ub result;
  result.r = static_cast<uint8_t>(clamped_color.r * 255);
  result.g = static_cast<uint8_t>(clamped_color.g * 255);
  result.b = static_cast<uint8_t>(clamped_color.b * 255);
  result.a = static_cast<uint8_t>(clamped_color.a * 255);
  return result;
}

mathfu::vec4 Color4ub::ToVec4(const Color4ub color) {
  return mathfu::vec4(1.0f / 255.0f * static_cast<float>(color.r),
                      1.0f / 255.0f * static_cast<float>(color.g),
                      1.0f / 255.0f * static_cast<float>(color.b),
                      1.0f / 255.0f * static_cast<float>(color.a));
}

Color4f::Color4f(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
Color4f::Color4f(float s) : r(s), g(s), b(s), a(s) {}

Color4f Color4f::FromARGB(uint32_t argb) {
  Color4f result;
  result.a = static_cast<uint8_t>((argb >> 24) & 0xff) / 255.0f;
  result.r = static_cast<uint8_t>((argb >> 16) & 0xff) / 255.0f;
  result.g = static_cast<uint8_t>((argb >> 8) & 0xff) / 255.0f;
  result.b = static_cast<uint8_t>(argb & 0xff) / 255.0f;
  return result;
}

Color4f Color4f::FromVec4(const mathfu::vec4& vec) {
  return Color4f(vec.x, vec.y, vec.z, vec.w);
}

Color4f Color4f::FromColor4ub(const Color4ub& color) {
  return Color4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f,
                 color.a / 255.0f);
}

mathfu::vec4 Color4f::ToVec4(const Color4f& color) {
  return mathfu::vec4(color.r, color.g, color.b, color.a);
}

float& Color4f::operator[](const int i) { return data[i]; }
float Color4f::operator[](const int i) const { return data[i]; }

Color4f Color4f::Lerp(const Color4f& lhs, const Color4f& rhs,
                      const float percent) {
  const float one_minus_percent = 1.0f - percent;
  return lhs * one_minus_percent + rhs * percent;
}

Color4f Color4f::Max(const Color4f& lhs, const Color4f& rhs) {
  return Color4f(std::max(lhs.r, rhs.r), std::max(lhs.g, rhs.g),
                 std::max(lhs.b, rhs.b), std::max(lhs.a, rhs.a));
}

Color4f Color4f::Min(const Color4f& lhs, const Color4f& rhs) {
  return Color4f(std::min(lhs.r, rhs.r), std::min(lhs.g, rhs.g),
                 std::min(lhs.b, rhs.b), std::min(lhs.a, rhs.a));
}

Color4f& Color4f::operator*=(const Color4f& c) {
  r *= c.r;
  g *= c.g;
  b *= c.b;
  a *= c.a;
  return *this;
}

Color4f& Color4f::operator/=(const Color4f& c) {
  r /= c.r;
  g /= c.g;
  b /= c.b;
  a /= c.a;
  return *this;
}

Color4f& Color4f::operator+=(const Color4f& c) {
  r += c.r;
  g += c.g;
  b += c.b;
  a += c.a;
  return *this;
}

Color4f& Color4f::operator-=(const Color4f& c) {
  r -= c.r;
  g -= c.g;
  b -= c.b;
  a -= c.a;
  return *this;
}

Color4f& Color4f::operator*=(float s) {
  r *= s;
  g *= s;
  b *= s;
  a *= s;
  return *this;
}

Color4f& Color4f::operator/=(float s) {
  r /= s;
  g /= s;
  b /= s;
  a /= s;
  return *this;
}

Color4f& Color4f::operator+=(float s) {
  r += s;
  g += s;
  b += s;
  a += s;
  return *this;
}

Color4f& Color4f::operator-=(float s) {
  r -= s;
  g -= s;
  b -= s;
  a -= s;
  return *this;
}

bool Color4f::operator==(const Color4f& c) const {
  return (r == c.r && g == c.g && b == c.b && a == c.a);
}

bool Color4f::operator!=(const Color4f& c) const {
  return (r != c.r || g != c.g || b != c.b || a != c.a);
}

Color4f operator-(const Color4f& c) { return Color4f(-c.r, -c.g, -c.b, -c.a); }

Color4f operator*(const Color4f& lhs, const Color4f& rhs) {
  return Color4f(lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b, lhs.a * rhs.a);
}

Color4f operator/(const Color4f& lhs, const Color4f& rhs) {
  return Color4f(lhs.r / rhs.r, lhs.g / rhs.g, lhs.b / rhs.b, lhs.a / rhs.a);
}

Color4f operator+(const Color4f& lhs, const Color4f& rhs) {
  return Color4f(lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a);
}

Color4f operator-(const Color4f& lhs, const Color4f& rhs) {
  return Color4f(lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a);
}

Color4f operator*(const Color4f& lhs, float rhs) {
  return Color4f(lhs.r * rhs, lhs.g * rhs, lhs.b * rhs, lhs.a * rhs);
}

Color4f operator/(const Color4f& lhs, float rhs) {
  return Color4f(lhs.r / rhs, lhs.g / rhs, lhs.b / rhs, lhs.a / rhs);
}

Color4f operator+(const Color4f& lhs, float rhs) {
  return Color4f(lhs.r + rhs, lhs.g + rhs, lhs.b + rhs, lhs.a + rhs);
}

Color4f operator-(const Color4f& lhs, float rhs) {
  return Color4f(lhs.r - rhs, lhs.g - rhs, lhs.b - rhs, lhs.a - rhs);
}

Color4f operator*(float lhs, const Color4f& rhs) {
  return Color4f(lhs * rhs.r, lhs * rhs.g, lhs * rhs.b, lhs * rhs.a);
}

Color4f operator/(float lhs, const Color4f& rhs) {
  return Color4f(lhs / rhs.r, lhs / rhs.g, lhs / rhs.b, lhs / rhs.a);
}

Color4f operator+(float lhs, const Color4f& rhs) {
  return Color4f(lhs + rhs.r, lhs + rhs.g, lhs + rhs.b, lhs + rhs.a);
}

Color4f operator-(float lhs, const Color4f& rhs) {
  return Color4f(lhs - rhs.r, lhs - rhs.g, lhs - rhs.b, lhs - rhs.a);
}

}  // namespace lull
