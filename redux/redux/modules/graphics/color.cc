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

#include "redux/modules/graphics/color.h"

namespace redux {

Color4ub::Color4ub() : r(255), g(255), b(255), a(255) {}

Color4ub::Color4ub(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                   std::uint8_t a)
    : r(r), g(g), b(b), a(a) {}

Color4ub::Color4ub(std::uint32_t rgba)
    : r(static_cast<std::uint8_t>((rgba >> 24) & 0xff)),
      g(static_cast<std::uint8_t>((rgba >> 16) & 0xff)),
      b(static_cast<std::uint8_t>((rgba >> 8) & 0xff)),
      a(static_cast<std::uint8_t>(rgba & 0xff)) {}

Color4ub Color4ub::FromARGB(std::uint32_t argb) {
  Color4ub result;
  result.a = static_cast<std::uint8_t>((argb >> 24) & 0xff);
  result.r = static_cast<std::uint8_t>((argb >> 16) & 0xff);
  result.g = static_cast<std::uint8_t>((argb >> 8) & 0xff);
  result.b = static_cast<std::uint8_t>(argb & 0xff);
  return result;
}

Color4ub Color4ub::FromVec4(const vec4& vec) {
  const vec4 clamped_vec4 = Min(vec4(1.0f), Max(vec, vec4(0.0f)));
  Color4ub result;
  result.r = static_cast<std::uint8_t>(clamped_vec4.x * 255);
  result.g = static_cast<std::uint8_t>(clamped_vec4.y * 255);
  result.b = static_cast<std::uint8_t>(clamped_vec4.z * 255);
  result.a = static_cast<std::uint8_t>(clamped_vec4.w * 255);
  return result;
}

Color4ub Color4ub::FromColor4f(const Color4f& color) {
  const Color4f clamped_color =
      Color4f::Min(Color4f(1.0f), Color4f::Max(color, Color4f(0.0f)));
  Color4ub result;
  result.r = static_cast<std::uint8_t>(clamped_color.r * 255);
  result.g = static_cast<std::uint8_t>(clamped_color.g * 255);
  result.b = static_cast<std::uint8_t>(clamped_color.b * 255);
  result.a = static_cast<std::uint8_t>(clamped_color.a * 255);
  return result;
}

vec4 Color4ub::ToVec4(const Color4ub color) {
  return vec4(1.0f / 255.0f * static_cast<float>(color.r),
              1.0f / 255.0f * static_cast<float>(color.g),
              1.0f / 255.0f * static_cast<float>(color.b),
              1.0f / 255.0f * static_cast<float>(color.a));
}

Color4f::Color4f(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}

Color4f::Color4f(float s) : r(s), g(s), b(s), a(s) {}

Color4f Color4f::FromARGB(std::uint32_t argb) {
  Color4f result;
  result.a = static_cast<std::uint8_t>((argb >> 24) & 0xff) / 255.0f;
  result.r = static_cast<std::uint8_t>((argb >> 16) & 0xff) / 255.0f;
  result.g = static_cast<std::uint8_t>((argb >> 8) & 0xff) / 255.0f;
  result.b = static_cast<std::uint8_t>(argb & 0xff) / 255.0f;
  return result;
}

Color4f Color4f::FromVec4(const vec4& vec) {
  return Color4f(vec.x, vec.y, vec.z, vec.w);
}

Color4f Color4f::FromColor4ub(const Color4ub& color) {
  return Color4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f,
                 color.a / 255.0f);
}

vec4 Color4f::ToVec4(const Color4f& color) {
  return vec4(color.r, color.g, color.b, color.a);
}

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

}  // namespace redux
