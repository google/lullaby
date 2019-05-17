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

#include <string>

#include "gtest/gtest.h"
#include "lullaby/generated/flatbuffers/texture_def_generated.h"
#include "lullaby/modules/render/shader_snippets_selector.h"
#include "lullaby/systems/render/filament/shader_material_builder.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

void AddAttribute(std::vector<ShaderAttributeDefT>* vec, const char* name,
                  VertexAttributeType type, VertexAttributeUsage usage) {
  ShaderAttributeDefT def;
  def.name = name;
  def.type = type;
  def.usage = usage;
  vec->emplace_back(std::move(def));
}

void AddSampler(std::vector<ShaderSamplerDefT>* vec, const char* name,
                TextureTargetType type,
                std::vector<MaterialTextureUsage> usages) {
  static const int kMaxTextureChannelCount = 4;
  EXPECT_TRUE(usages.size() < kMaxTextureChannelCount);
  ShaderSamplerDefT def;
  def.name = name;
  def.type = type;
  def.usage_per_channel = std::move(usages);
  vec->emplace_back(std::move(def));
}

TEST(ShaderMaterialBuilder, BuildFragmentCodeFromShaderStage) {
  // clang-format off
  const char kHeader[] = R"(
  vec3 HeaderFunction(vec3 color) {
    return color;
  }
  )";

  const char kMain[] = R"(
    out_color = HeaderFunction(color);
  )";
  // clang-format on

  ShaderStage stage;
  stage.code.push_back(kHeader);
  stage.main.push_back(kMain);
  AddAttribute(&stage.inputs, "color", VertexAttributeType_Vec4f,
               VertexAttributeUsage_Color);
  AddAttribute(&stage.inputs, "uv", VertexAttributeType_Vec2f,
               VertexAttributeUsage_TexCoord);
  AddAttribute(&stage.outputs, "outColor", VertexAttributeType_Vec4f,
               VertexAttributeUsage_Color);
  AddSampler(&stage.samplers, "sampler_0", TextureTargetType_Standard2d, {});
  AddSampler(&stage.samplers, "sampler_1", TextureTargetType_Standard2d, {});

  const auto npos = std::string::npos;
  const std::string result = ShaderMaterialBuilder::BuildFragmentCode(stage);

  // The header code and main code should be included as-is.
  EXPECT_NE(result.find(kHeader), npos);
  EXPECT_NE(result.find(kMain), npos);

  // The filament "material" function should be defined.
  EXPECT_NE(result.find("void material(inout MaterialInputs material)"), npos);

  // Inputs should be defined globally.
  EXPECT_NE(result.find("vec2 uv;"), npos);
  EXPECT_NE(result.find("vec4 outColor;"), npos);

  // Inputs should be assigned from filament property functions.
  EXPECT_NE(result.find("color = getColor();"), npos);
  EXPECT_NE(result.find("uv = vec2(getUV0().x, 1. - getUV0().y);"), npos);

  // Output should be assigned to material.baseColor.
  EXPECT_NE(result.find("material.baseColor = outColor;"), npos);
}

}  // namespace
}  // namespace lull
