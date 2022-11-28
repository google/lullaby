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

#ifndef REDUX_TOOLS_SHADER_PIPELINE_SHADER_PIPELINE_H_
#define REDUX_TOOLS_SHADER_PIPELINE_SHADER_PIPELINE_H_

#include <string>
#include <string_view>
#include <vector>

#include "absl/types/span.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/graphics/enums.h"

namespace redux::tool {

struct ShaderAssetParameter {
  std::string name;
  MaterialPropertyType type;
  int array_size = 0;
  std::vector<MaterialTextureType> texture_usage;
  std::vector<int> default_ints;
  std::vector<float> default_floats;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(name, ConstHash("name"));
    archive(type, ConstHash("type"));
    archive(array_size, ConstHash("array_size"));
    archive(texture_usage, ConstHash("texture_usage"));
    archive(default_ints, ConstHash("default_ints"));
    archive(default_floats, ConstHash("default_floats"));
  }
};

struct ShaderAsset {
  enum Shading {
    Unlit,
    Lit,
    Cloth,
    Subsurface,
    SpecularGlossiness,
  };

  enum BlendMode {
    Opaque,
    Transparent,
    Fade,
    Add,
    Masked,
    Multiply,
    Screen,
  };

  enum TransparencyMode { Default, TwoPassesOneSide, TwoPassesTwoSides };

  enum CullingMode {
    None,
    FrontFace,
    BackFace,
    FrontAndBack,
  };

  std::string name;
  Shading shading;

  std::string vertex_shader;
  std::string fragment_shader;
  std::vector<std::string> defines;
  std::vector<std::string> features;

  std::vector<VertexUsage> vertex_attributes;
  std::vector<ShaderAssetParameter> parameters;

  bool color_write = true;
  bool depth_write = true;
  bool depth_cull = true;
  bool double_sided = false;
  BlendMode blending = Opaque;
  BlendMode post_lighting_blending = Transparent;
  CullingMode culling = None;
  TransparencyMode transparency = Default;
  float mask_threshold = 0.4;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(name, ConstHash("name"));
    archive(shading, ConstHash("shading"));
    archive(vertex_shader, ConstHash("vertex_shader"));
    archive(fragment_shader, ConstHash("fragment_shader"));
    archive(defines, ConstHash("defines"));
    archive(features, ConstHash("features"));
    archive(vertex_attributes, ConstHash("vertex_attributes"));
    archive(parameters, ConstHash("parameters"));
    archive(color_write, ConstHash("color_write"));
    archive(depth_write, ConstHash("depth_write"));
    archive(depth_cull, ConstHash("depth_cull"));
    archive(double_sided, ConstHash("double_sided"));
    archive(blending, ConstHash("blending"));
    archive(post_lighting_blending, ConstHash("post_lighting_blending"));
    archive(transparency, ConstHash("transparency"));
    archive(mask_threshold, ConstHash("mask_threshold"));
  }
};

DataContainer BuildShader(std::string_view name,
                          absl::Span<const ShaderAsset> assets);

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SHADER_PIPELINE_SHADER_PIPELINE_H_
