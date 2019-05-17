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
#include "lullaby/systems/render/next/shader_data.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {
static const char kCompatibilityShaderMacros[] = R"(#version 100 es
#define UNIFORM(X) X
#define SAMPLER(X) X
)";

std::string AddCompatibilityMacro(const char* shader) {
  return std::string(kCompatibilityShaderMacros) + shader;
}

ShaderUniformDefT CreateUniformDef(const char* name, ShaderDataType type,
                                   unsigned int array_size) {
  ShaderUniformDefT def;
  def.name = name;
  def.type = type;
  def.array_size = array_size;

  return def;
}

ShaderAttributeDefT CreateAttributeDef(const char* name,
                                       VertexAttributeType type,
                                       VertexAttributeUsage usage) {
  ShaderAttributeDefT def;
  def.name = name;
  def.type = type;
  def.usage = usage;

  return def;
}

static const int kMaxTextureChannelCount = 4;

ShaderSamplerDefT CreateSamplerDef(const char* name, TextureTargetType type,
                                   std::vector<MaterialTextureUsage> usages) {
  EXPECT_TRUE(usages.size() < kMaxTextureChannelCount);

  ShaderSamplerDefT def;
  def.name = name;
  def.type = type;
  def.usage_per_channel = std::move(usages);

  return def;
}

TEST(ShaderDataTest, Uniform) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro("uniform mat4 model_view_projection;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, UniformArray) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 1));
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(
      AddCompatibilityMacro("uniform mat4 model_view_projection[1];\n;\n"),
      shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, UniformMultiple) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));
  vertex_snippet.uniforms.push_back(
      CreateUniformDef("world", ShaderDataType_Float4x4, 0));
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(
      AddCompatibilityMacro(
          "uniform mat4 model_view_projection;\nuniform mat4 world;\n;\n"),
      shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, UniformCollapse) {
  // Test that two uniforms with the same name collapse into one.
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));
  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));
  vertex_snippet_2.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro("uniform mat4 model_view_projection;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, UniformMultipleDifferentSnippets) {
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));
  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.uniforms.push_back(
      CreateUniformDef("world", ShaderDataType_Float4x4, 0));
  vertex_snippet_2.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(
      AddCompatibilityMacro(
          "uniform mat4 model_view_projection;\nuniform mat4 world;\n;\n"),
      shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataDeathTest, UniformMismatch) {
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));
  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4, 0));

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  PORT_EXPECT_DEBUG_DEATH(ShaderData{shader_def}, "");
}

TEST(ShaderDataTest, UniformBufferObject) {
  ShaderUniformDefT uniform_buffer_object_def =
      CreateUniformDef("MyBlock", ShaderDataType_BufferObject, 0);
  uniform_buffer_object_def.fields.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));
  uniform_buffer_object_def.fields.push_back(
      CreateUniformDef("camera_position", ShaderDataType_Float4, 0));
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.uniforms.push_back(uniform_buffer_object_def);
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(
      AddCompatibilityMacro(
          "layout (std140) uniform MyBlock {\nmat4 model_view_projection;\n"
          "vec4 camera_position;\n};\n;\n"),
      shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, UniformBufferObjectWithArray) {
  ShaderUniformDefT uniform_buffer_object_def =
      CreateUniformDef("MyBlock", ShaderDataType_BufferObject, 0);
  uniform_buffer_object_def.fields.push_back(
      CreateUniformDef("bones", ShaderDataType_Float4, 512));
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.uniforms.push_back(uniform_buffer_object_def);
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro("layout (std140) uniform MyBlock {\n"
                                  "vec4 bones[512];\n};\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, Attribute) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro("attribute vec4 position;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, AttributeMultiple) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  vertex_snippet.inputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(
                "attribute vec4 position;\nattribute vec4 color;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, AttributeCollapse) {
  // Test that two uniforms with the same name collapse into one.
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  vertex_snippet_2.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro("attribute vec4 position;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, AttributeMultipleDifferentSnippets) {
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.inputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));
  vertex_snippet_2.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(
                "attribute vec4 position;\nattribute vec4 color;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataDeathTest, AttributeMismatch) {
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec3f, VertexAttributeUsage_Color));

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  PORT_EXPECT_DEBUG_DEATH(ShaderData{shader_def}, "");
}

TEST(ShaderDataTest, InputOutput_Mismatch_1) {
  ShaderSnippetDefT fragment_snippet;
  fragment_snippet.inputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT fragment_stage;
  fragment_stage.type = ShaderStageType_Fragment;
  fragment_stage.snippets.push_back(std::move(fragment_snippet));
  ShaderStageDefT vertex_stage;
  vertex_stage.type = ShaderStageType_Vertex;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(fragment_stage));
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro("attribute vec4 position;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
  EXPECT_EQ("", shader_data.GetStageCode(ShaderStageType_Fragment));
}

TEST(ShaderDataTest, InputOutput_Mismatch_2) {
  ShaderSnippetDefT fragment_snippet;
  fragment_snippet.inputs.push_back(CreateAttributeDef(
      "normal", VertexAttributeType_Vec3f, VertexAttributeUsage_Normal));
  fragment_snippet.code = ";";  // Used to compile a "valid" shader.
  ShaderSnippetDefT vertex_position_snippet;
  vertex_position_snippet.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  vertex_position_snippet.code = ";";  // Used to compile a "valid" shader.
  ShaderSnippetDefT vertex_color_snippet;
  vertex_color_snippet.outputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));

  ShaderStageDefT fragment_stage;
  fragment_stage.type = ShaderStageType_Fragment;
  fragment_stage.snippets.push_back(std::move(fragment_snippet));
  ShaderStageDefT vertex_stage;
  vertex_stage.type = ShaderStageType_Vertex;
  vertex_stage.snippets.push_back(std::move(vertex_position_snippet));
  vertex_stage.snippets.push_back(std::move(vertex_color_snippet));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(fragment_stage));
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro("attribute vec4 position;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
  EXPECT_EQ("", shader_data.GetStageCode(ShaderStageType_Fragment));
}

TEST(ShaderDataTest, InputOutput_Match) {
  ShaderSnippetDefT fragment_snippet;
  fragment_snippet.inputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));
  fragment_snippet.code = ";";  // Used to compile a "valid" shader.
  ShaderSnippetDefT vertex_position_snippet;
  vertex_position_snippet.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  ShaderSnippetDefT vertex_color_snippet;
  vertex_color_snippet.outputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));
  vertex_color_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT fragment_stage;
  fragment_stage.type = ShaderStageType_Fragment;
  fragment_stage.snippets.push_back(std::move(fragment_snippet));
  ShaderStageDefT vertex_stage;
  vertex_stage.type = ShaderStageType_Vertex;
  vertex_stage.snippets.push_back(std::move(vertex_position_snippet));
  vertex_stage.snippets.push_back(std::move(vertex_color_snippet));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(fragment_stage));
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(
                "attribute vec4 position;\nvarying vec4 color;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
  EXPECT_EQ(AddCompatibilityMacro("varying vec4 color;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Fragment));
}

TEST(ShaderDataTest, Sampler_Match) {
  ShaderSnippetDefT fragment_snippet;
  fragment_snippet.samplers.push_back(
      CreateSamplerDef("baseColor", TextureTargetType_Standard2d,
                       {MaterialTextureUsage_BaseColor}));
  fragment_snippet.code = ";";  // Used to compile a "valid" shader.
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.samplers.push_back(
      CreateSamplerDef("baseColor", TextureTargetType_Standard2d,
                       {MaterialTextureUsage_BaseColor}));
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT fragment_stage;
  fragment_stage.type = ShaderStageType_Fragment;
  fragment_stage.snippets.push_back(std::move(fragment_snippet));
  ShaderStageDefT vertex_stage;
  vertex_stage.type = ShaderStageType_Vertex;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(fragment_stage));
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro("uniform sampler2D baseColor;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
  EXPECT_EQ(AddCompatibilityMacro("uniform sampler2D baseColor;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Fragment));
}

TEST(ShaderDataTest, Sampler_Match_Multi_Usage) {
  ShaderSnippetDefT fragment_snippet;
  fragment_snippet.samplers.push_back(CreateSamplerDef(
      "occlusionRoughnessMetallic", TextureTargetType_Standard2d,
      {MaterialTextureUsage_Occlusion, MaterialTextureUsage_Roughness,
       MaterialTextureUsage_Metallic}));
  fragment_snippet.code = ";";  // Used to compile a "valid" shader.
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.samplers.push_back(CreateSamplerDef(
      "occlusionRoughnessMetallic", TextureTargetType_Standard2d,
      {MaterialTextureUsage_Occlusion, MaterialTextureUsage_Roughness,
       MaterialTextureUsage_Metallic}));
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT fragment_stage;
  fragment_stage.type = ShaderStageType_Fragment;
  fragment_stage.snippets.push_back(std::move(fragment_snippet));
  ShaderStageDefT vertex_stage;
  vertex_stage.type = ShaderStageType_Vertex;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(fragment_stage));
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(
                "uniform sampler2D occlusionRoughnessMetallic;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
  EXPECT_EQ(AddCompatibilityMacro(
                "uniform sampler2D occlusionRoughnessMetallic;\n;\n"),
            shader_data.GetStageCode(ShaderStageType_Fragment));
}

TEST(ShaderDataTest, Sampler_Usage_Mismatch) {
  ShaderSnippetDefT fragment_snippet;
  fragment_snippet.samplers.push_back(
      CreateSamplerDef("baseColorAndOcclusion", TextureTargetType_Standard2d,
                       {MaterialTextureUsage_BaseColor}));
  fragment_snippet.code = ";";  // Used to compile a "valid" shader.
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.samplers.push_back(
      CreateSamplerDef("baseColorAndOcclusion", TextureTargetType_Standard2d,
                       {MaterialTextureUsage_Occlusion}));
  vertex_snippet.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT fragment_stage;
  fragment_stage.type = ShaderStageType_Fragment;
  fragment_stage.snippets.push_back(std::move(fragment_snippet));
  ShaderStageDefT vertex_stage;
  vertex_stage.type = ShaderStageType_Vertex;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(fragment_stage));
  shader_def.stages.push_back(std::move(vertex_stage));
  PORT_EXPECT_DEBUG_DEATH(ShaderData{shader_def}, "");
}

TEST(ShaderDataTest, GeneratedMainFragment) {
  ShaderSnippetDefT fragment_snippet;
  fragment_snippet.main_code = "gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);";

  ShaderStageDefT fragment_stage;
  fragment_stage.type = ShaderStageType_Fragment;
  fragment_stage.snippets.push_back(std::move(fragment_snippet));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(fragment_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(R"(void GeneratedFunctionFragment0() {
gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}

void main() {
GeneratedFunctionFragment0();
}
)"),
            shader_data.GetStageCode(ShaderStageType_Fragment));
}

TEST(ShaderDataTest, GeneratedMainFragmentMultiple) {
  ShaderSnippetDefT fragment_snippet_0;
  fragment_snippet_0.main_code = "gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);";
  ShaderSnippetDefT fragment_snippet_1;
  fragment_snippet_1.main_code = "// Do nothing.";

  ShaderStageDefT fragment_stage;
  fragment_stage.type = ShaderStageType_Fragment;
  fragment_stage.snippets.push_back(std::move(fragment_snippet_0));
  fragment_stage.snippets.push_back(std::move(fragment_snippet_1));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(fragment_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(R"(void GeneratedFunctionFragment0() {
gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
void GeneratedFunctionFragment1() {
// Do nothing.
}

void main() {
GeneratedFunctionFragment0();
GeneratedFunctionFragment1();
}
)"),
            shader_data.GetStageCode(ShaderStageType_Fragment));
}

TEST(ShaderDataTest, GeneratedMainVertex) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.main_code = "gl_Position = vec4(1.0, 1.0, 1.0, 1.0);";

  ShaderStageDefT vertex_stage;
  vertex_stage.type = ShaderStageType_Vertex;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(R"(void GeneratedFunctionVertex0() {
gl_Position = vec4(1.0, 1.0, 1.0, 1.0);
}

void main() {
GeneratedFunctionVertex0();
}
)"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, GeneratedMainVertexMultiple) {
  ShaderSnippetDefT vertex_snippet_0;
  vertex_snippet_0.main_code = "gl_Position = vec4(1.0, 1.0, 1.0, 1.0);";
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.main_code = "// Do nothing.";

  ShaderStageDefT vertex_stage;
  vertex_stage.type = ShaderStageType_Vertex;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_0));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(R"(void GeneratedFunctionVertex0() {
gl_Position = vec4(1.0, 1.0, 1.0, 1.0);
}
void GeneratedFunctionVertex1() {
// Do nothing.
}

void main() {
GeneratedFunctionVertex0();
GeneratedFunctionVertex1();
}
)"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, FullVertex) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  vertex_snippet.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));
  vertex_snippet.code = R"(#include "some_header.h")";
  vertex_snippet.main_code = "gl_Position = model_view_projection * position;";

  ShaderStageDefT vertex_stage;
  vertex_stage.type = ShaderStageType_Vertex;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(R"(uniform mat4 model_view_projection;
attribute vec4 position;
#include "some_header.h"
void GeneratedFunctionVertex0() {
gl_Position = model_view_projection * position;
}

void main() {
GeneratedFunctionVertex0();
}
)"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, FullFragment) {
  ShaderSnippetDefT fragment_snippet;
  fragment_snippet.inputs.push_back(CreateAttributeDef(
      "vert_color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));
  fragment_snippet.uniforms.push_back(
      CreateUniformDef("color", ShaderDataType_Float4, 0));
  fragment_snippet.code = R"(#include "some_header.h")";
  fragment_snippet.main_code = "gl_FragColor = vert_color * color;";

  ShaderStageDefT fragment_stage;
  fragment_stage.type = ShaderStageType_Fragment;
  fragment_stage.snippets.push_back(std::move(fragment_snippet));

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(fragment_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(R"(uniform vec4 color;
varying vec4 vert_color;
#include "some_header.h"
void GeneratedFunctionFragment0() {
gl_FragColor = vert_color * color;
}

void main() {
GeneratedFunctionFragment0();
}
)"),
            shader_data.GetStageCode(ShaderStageType_Fragment));
}

TEST(ShaderDataTest, EnvironmentFlags) {
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  vertex_snippet_1.environment.push_back(Hash("ATTR_POSITION"));

  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.inputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));
  vertex_snippet_2.environment.push_back(Hash("ATTR_COLOR"));

  ShaderSnippetDefT vertex_snippet_3;
  vertex_snippet_3.inputs.push_back(CreateAttributeDef(
      "normal", VertexAttributeType_Vec3f, VertexAttributeUsage_Normal));
  vertex_snippet_3.environment.push_back(Hash("ATTR_NORMAL"));

  ShaderSnippetDefT vertex_snippet_4;
  vertex_snippet_4.inputs.push_back(
      CreateAttributeDef("orientation", VertexAttributeType_Vec3f,
                         VertexAttributeUsage_Orientation));
  vertex_snippet_4.environment.push_back(Hash("ATTR_ORIENTATION"));

  ShaderSnippetDefT vertex_snippet_5;
  vertex_snippet_5.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_3));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_4));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_5));
  vertex_stage.type = ShaderStageType_Vertex;
  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));

  // Build the shader.
  ShaderCreateParams params;
  params.selection_params.environment.insert(Hash("ATTR_POSITION"));
  params.selection_params.environment.insert(Hash("ATTR_NORMAL"));
  ShaderData shader_data(shader_def, params);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(R"(attribute vec4 position;
attribute vec3 normal;
;
)"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, FeatureFlags) {
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  vertex_snippet_1.features.push_back(Hash("Transform"));

  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.inputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));
  vertex_snippet_2.features.push_back(Hash("VertexColor"));

  ShaderSnippetDefT vertex_snippet_3;
  vertex_snippet_3.inputs.push_back(CreateAttributeDef(
      "position_2", VertexAttributeType_Vec3f, VertexAttributeUsage_Position));
  vertex_snippet_3.features.push_back(Hash("Transform"));

  ShaderSnippetDefT vertex_snippet_4;
  vertex_snippet_4.inputs.push_back(CreateAttributeDef(
      "normal", VertexAttributeType_Vec3f, VertexAttributeUsage_Normal));
  vertex_snippet_4.features.push_back(Hash("Light"));

  ShaderSnippetDefT vertex_snippet_5;
  vertex_snippet_5.code = ";";  // Used to compile a "valid" shader.

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_3));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_4));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_5));
  vertex_stage.type = ShaderStageType_Vertex;
  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));

  // Build the shader.
  ShaderCreateParams params;
  params.selection_params.features.insert(Hash("Transform"));
  params.selection_params.features.insert(Hash("Light"));
  ShaderData shader_data(shader_def, params);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(R"(attribute vec4 position;
attribute vec3 normal;
;
)"),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, ShaderVersionInStage) {
  ShaderSnippetVersionDefT version;
  version.lang = ShaderLanguage_GL_Compat;
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.code = "1";
  version.min_version = 0;  // No minimum.
  version.max_version = 0;
  vertex_snippet_1.versions.push_back(version);
  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.code = "2";
  version.min_version = 300;
  version.max_version = 0;  // No maximum.
  vertex_snippet_2.versions.push_back(version);
  ShaderSnippetDefT vertex_snippet_3;
  vertex_snippet_3.code = "3";
  version.min_version = 0;  // No minimum.
  version.max_version = 300;
  vertex_snippet_3.versions.push_back(version);
  ShaderSnippetDefT vertex_snippet_4;
  vertex_snippet_4.code = "4";
  version.min_version = 100;
  version.max_version = 300;
  vertex_snippet_4.versions.push_back(version);
  ShaderSnippetDefT vertex_snippet_5;
  vertex_snippet_5.code = "5";
  version.min_version = 200;
  version.max_version = 300;
  vertex_snippet_5.versions.push_back(version);
  ShaderSnippetDefT vertex_snippet_6;
  vertex_snippet_6.code = "6";
  version.min_version = 100;
  version.max_version = 0;  // No maximum.
  vertex_snippet_6.versions.push_back(version);

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_3));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_4));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_5));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_6));
  vertex_stage.type = ShaderStageType_Vertex;
  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));

  // Build the shader.
  ShaderCreateParams params;
  params.selection_params.max_shader_version = 100;
  ShaderData shader_data(shader_def, params);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(AddCompatibilityMacro(R"(1
3
4
6
)")
                .c_str(),
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, ShaderVersionAcrossStages) {
  ShaderSnippetVersionDefT version;
  version.lang = ShaderLanguage_GL_Compat;
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.code = "1";
  version.min_version = 0;  // No minimum.
  version.max_version = 0;
  vertex_snippet_1.versions.push_back(version);
  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.code = "2";
  version.min_version = 300;
  version.max_version = 0;  // No maximum.
  vertex_snippet_2.versions.push_back(version);
  ShaderSnippetDefT vertex_snippet_3;
  vertex_snippet_3.code = "3";
  version.min_version = 0;  // No minimum.
  version.max_version = 300;  // Up to 300 (excluding).
  vertex_snippet_3.versions.push_back(version);
  ShaderSnippetDefT vertex_snippet_4;
  vertex_snippet_4.code = "4";
  version.min_version = 100;
  version.max_version = 200;
  vertex_snippet_4.versions.push_back(version);
  ShaderSnippetDefT vertex_snippet_5;
  vertex_snippet_5.code = "5";
  version.min_version = 0;
  version.max_version = 200;
  vertex_snippet_5.versions.push_back(version);
  ShaderSnippetDefT vertex_snippet_6;
  vertex_snippet_6.code = "6";
  version.min_version = 100;
  version.max_version = 0;  // No maximum.
  vertex_snippet_6.versions.push_back(version);

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_3));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_4));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_5));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_6));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderSnippetDefT fragment_snippet_1;
  fragment_snippet_1.code = "1";
  version.min_version = 0;  // No minimum.
  version.max_version = 0;
  fragment_snippet_1.versions.push_back(version);
  ShaderSnippetDefT fragment_snippet_2;
  fragment_snippet_2.code = "2";
  version.min_version = 300;
  version.max_version = 0;  // No maximum.
  fragment_snippet_2.versions.push_back(version);
  ShaderSnippetDefT fragment_snippet_3;
  fragment_snippet_3.code = "3";
  version.min_version = 100;  // Only version 100.
  version.max_version = 100;
  fragment_snippet_3.versions.push_back(version);
  ShaderSnippetDefT fragment_snippet_4;
  fragment_snippet_4.code = "4";
  version.min_version = 100;
  version.max_version = 200;
  fragment_snippet_4.versions.push_back(version);
  ShaderSnippetDefT fragment_snippet_5;
  fragment_snippet_5.code = "5";
  version.min_version = 0;    // No minimum.
  version.max_version = 100;  // Up to version 100.
  fragment_snippet_5.versions.push_back(version);

  ShaderStageDefT fragment_stage;
  fragment_stage.snippets.push_back(std::move(fragment_snippet_1));
  fragment_stage.snippets.push_back(std::move(fragment_snippet_2));
  fragment_stage.snippets.push_back(std::move(fragment_snippet_3));
  fragment_stage.snippets.push_back(std::move(fragment_snippet_4));
  fragment_stage.snippets.push_back(std::move(fragment_snippet_5));
  fragment_stage.type = ShaderStageType_Fragment;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  shader_def.stages.push_back(std::move(fragment_stage));

  // Build the shader.
  ShaderCreateParams params;
  params.selection_params.max_shader_version = 300;
  ShaderData shader_data(shader_def, params);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(R"(#version 300 es
#define UNIFORM(X) X
#define SAMPLER(X) X
1
2
6
)",
            shader_data.GetStageCode(ShaderStageType_Vertex));
  EXPECT_EQ(R"(#version 300 es
#define UNIFORM(X) X
#define SAMPLER(X) X
1
2
)",
            shader_data.GetStageCode(ShaderStageType_Fragment));
}

}  // namespace
}  // namespace lull
