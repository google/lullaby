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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_MATERIAL_H_
#define REDUX_TOOLS_SCENE_PIPELINE_MATERIAL_H_

#include <string>

#include "redux/tools/scene_pipeline/index.h"
#include "redux/tools/scene_pipeline/types.h"
#include "redux/tools/scene_pipeline/variant.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"

namespace redux::tool {

// The shading model and material properties used to render an object.
struct Material {
  // The shading model used by this material.
  std::string shading_model;

  // Key-value properties for the material.
  absl::flat_hash_map<std::string, Variant> properties;

  // Returns a span of the given type of the data stored in the material. If
  // no such property exists, then an empty span is returned.
  template <typename T>
  absl::Span<const T> span(absl::string_view name) const {
    auto iter = properties.find(name);
    return iter == properties.end() ? absl::Span<const T>()
                                    : iter->second.span<T>();
  }

  // Common names for known shading models and material properties. We do our
  // best to map data from scene formats to these names. Any name not prefixed
  // with "$" is considered format-specific.
  //
  // Any property suffixed with ".texture" should be treated as a TextureInfo.
  // Values retrieved from the texture should be multiplied by the base property
  // value unless otherwise specified.

  // Shading models.
  SCENE_NAME(kUnlit, "$unlit");
  SCENE_NAME(kMetallicRoughness, "$metallic_roughness");
  SCENE_NAME(kSpecularGlossiness, "$specular_glossiness");
  SCENE_NAME(kClearCoat, "$clearcoat");

  // Flip UV (bool): Whether or not both the UVs should be flipped in the y
  // direction.
  SCENE_NAME(kFlipUv, "$flip_uv");

  // Double-sided (bool): Whether or not both sides of an object should be
  // rendered. False if unspecified.
  SCENE_NAME(kDoubleSided, "$double_sided");

  // Alpha cutoff (float): The alpha value below which the fragment is
  // considered transparent. An unspecified value is assumed to be 0.
  SCENE_NAME(kAlphaCutoff, "$alpha_cutoff");

  // Alpha-mode (int): The type of alpha blending to use. Opaque if unspecified.
  SCENE_NAME(kAlphaMode, "$alpha_mode");
  static constexpr int kAlphaModeOpaque = 0;
  static constexpr int kAlphaModeBlend = 1;
  static constexpr int kAlphaModeMask = 2;

  // Base color (float3 or float4). The base RGBA color of the material. Also
  // sometimes referred to as diffuse or albedo in various contexts. Assume
  // alpha=1.0 if only 3 values are specified.
  SCENE_NAME(kBaseColor, "$base_color");
  SCENE_NAME(kBaseColorTexture, "$base_color.texture");

  // Emissive color (float3): A color emitted by the material itself.
  SCENE_NAME(kEmissive, "$emissive");
  SCENE_NAME(kEmissiveTexture, "$emissive.texture");

  // Normal (float3): The surface normal map, where x,y are along the surface
  // and z is the normal. A "default" normal has the value (0.5, 0.5, 1.0). An
  // optional scale parameter is a multiplier that is to be applied to the x, y
  // values of the normal.
  SCENE_NAME(kNormalTexture, "$normal.texture");
  SCENE_NAME(kNormalScale, "$normal.scale");

  // Metallic (float): The metallicity of the material, used by the
  // "metallic_roughness" shading model.
  SCENE_NAME(kMetallic, "$metallic");
  SCENE_NAME(kMetallicTexture, "$metallic.texture");
  SCENE_NAME(kMetallicChannelMask, "$metallic.channel_mask");

  // Roughness (float): The roughness of the material, used by the
  // "metallic_roughness" shading model.
  SCENE_NAME(kRoughness, "$roughness");
  SCENE_NAME(kRoughnessTexture, "$roughness.texture");

  // Occlusion (float): Higher values indicate areas that receive full indirect
  // lighting while lower values indicate no indirect lighting. A strength value
  // of 0.0 means no occlusion is to be applied whereas a value of 1.0 means
  // full occlusion. The final occlusion value can be calculated as:
  //   1.0 + strength * (<sampled value> - 1.0)
  SCENE_NAME(kOcclusionTexture, "$occlusion.texture");
  SCENE_NAME(kOcclusionStrength, "$occlusion.strength");

  // Specular (float3): The specular color of the material measuring the
  // reflectance value at normal incidence. Used by the "specular_glossiness"
  // shading model.
  SCENE_NAME(kSpecular, "$specular");
  SCENE_NAME(kSpecularTexture, "$specular.texture");

  // Glossiness (float): The glossiness property is a factor between 0.0 (rough
  // surface) and 1.0 (smooth surface). Used by the "specular_glossiness"
  // shading model.
  SCENE_NAME(kGlossiness, "$glossiness");
  SCENE_NAME(kGlossinessTexture, "$glossiness.texture");

  // Clearcoat (float): The strength of the clear coat layer. Only used by the
  // "clearcoat" shading model. A value of 0 disables the clear coat layer.
  SCENE_NAME(kClearCoatFactor, "$clearcoat");
  SCENE_NAME(kClearCoatTexture, "$clearcoat.texture");

  // Roughness (float): The roughness of the clear coat layer on the material,
  // used by the "clearcoat" shading model.
  SCENE_NAME(kClearCoatRoughness, "$clearcoat_roughness");
  SCENE_NAME(kClearCoatRoughnessTexture, "$clearcoat_roughness.texture");

  // Clearcoat normal (float3): An additional normal map for "clearcoat" shading
  // models.
  SCENE_NAME(kClearCoatNormalTexture, "$clearcoat_normal.texture");
  SCENE_NAME(kClearCoatNormalScale, "$clearcoat_normal.scale");
};

using MaterialIndex = Index<Material>;

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_MATERIAL_H_
