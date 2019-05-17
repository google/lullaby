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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_FILAMENT_UTILS_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_FILAMENT_UTILS_H_

#include "filament/Color.h"
#include "filament/TextureSampler.h"
#include "math/mat4.h"
#include "math/vec3.h"
#include "lullaby/generated/flatbuffers/texture_def_generated.h"
#include "lullaby/generated/texture_def_generated.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

template <typename T>
using FilamentResourcePtr = std::unique_ptr<T, std::function<void(T*)>>;

inline filament::LinearColor ToLinearColor(const mathfu::vec4& src) {
  return filament::LinearColor{src.x, src.y, src.z};
}

inline filament::LinearColorA ToLinearColorA(const mathfu::vec4& src) {
  return filament::LinearColorA{src.x, src.y, src.z, src.w};
}

inline filament::math::float3 FilamentFloat3FromMathfuVec3(
    const mathfu::vec3& src) {
  return {src.x, src.y, src.z};
}

inline filament::math::float4 FilamentFloat4FromMathfuVec4(
    const mathfu::vec4& src) {
  return {src.x, src.y, src.z, src.w};
}

inline filament::TextureSampler::MinFilter ToFilamentMinFilter(
    TextureFiltering filter) {
  switch (filter) {
    case TextureFiltering_Nearest:
      return filament::TextureSampler::MinFilter::NEAREST;
    case TextureFiltering_Linear:
      return filament::TextureSampler::MinFilter::LINEAR;
    case TextureFiltering_NearestMipmapNearest:
      return filament::TextureSampler::MinFilter::NEAREST_MIPMAP_NEAREST;
    case TextureFiltering_LinearMipmapNearest:
      return filament::TextureSampler::MinFilter::LINEAR_MIPMAP_NEAREST;
    case TextureFiltering_NearestMipmapLinear:
      return filament::TextureSampler::MinFilter::NEAREST_MIPMAP_LINEAR;
    case TextureFiltering_LinearMipmapLinear:
      return filament::TextureSampler::MinFilter::LINEAR_MIPMAP_LINEAR;
  }
}

inline filament::TextureSampler::MagFilter ToFilamentMagFilter(
    TextureFiltering filter) {
  switch (filter) {
    case TextureFiltering_Nearest:
      return filament::TextureSampler::MagFilter::NEAREST;
    case TextureFiltering_Linear:
      return filament::TextureSampler::MagFilter::LINEAR;
    case TextureFiltering_NearestMipmapNearest:
      return filament::TextureSampler::MagFilter::NEAREST;
    case TextureFiltering_LinearMipmapNearest:
      return filament::TextureSampler::MagFilter::NEAREST;
    case TextureFiltering_NearestMipmapLinear:
      return filament::TextureSampler::MagFilter::LINEAR;
    case TextureFiltering_LinearMipmapLinear:
      return filament::TextureSampler::MagFilter::LINEAR;
  }
}

inline filament::TextureSampler::WrapMode ToFilamentWrapMode(TextureWrap wrap) {
  switch (wrap) {
    case TextureWrap_Repeat:
      return filament::TextureSampler::WrapMode::REPEAT;
    case TextureWrap_MirroredRepeat:
      return filament::TextureSampler::WrapMode::MIRRORED_REPEAT;
    case TextureWrap_ClampToBorder:
      return filament::TextureSampler::WrapMode::CLAMP_TO_EDGE;
    case TextureWrap_ClampToEdge:
      return filament::TextureSampler::WrapMode::CLAMP_TO_EDGE;
    case TextureWrap_MirrorClampToEdge:
      return filament::TextureSampler::WrapMode::CLAMP_TO_EDGE;
  }
}

inline filament::math::mat4f MathFuMat4ToFilamentMat4f(
    const mathfu::mat4& src) {
  return filament::math::mat4f{
      filament::math::float4{src[0], src[1], src[2], src[3]},
      filament::math::float4{src[4], src[5], src[6], src[7]},
      filament::math::float4{src[8], src[9], src[10], src[11]},
      filament::math::float4{src[12], src[13], src[14], src[15]}};
}

inline filament::math::mat4 MathFuMat4ToFilamentMat4(const mathfu::mat4& src) {
  return filament::math::mat4{
      filament::math::float4{src[0], src[1], src[2], src[3]},
      filament::math::float4{src[4], src[5], src[6], src[7]},
      filament::math::float4{src[8], src[9], src[10], src[11]},
      filament::math::float4{src[12], src[13], src[14], src[15]}};
}

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_FILAMENT_UTILS_H_
