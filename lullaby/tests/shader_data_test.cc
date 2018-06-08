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

#include <string>

#include "gtest/gtest.h"
#include "lullaby/systems/render/next/shader_data.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

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

TEST(ShaderDataTest, Uniform) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ("uniform mat4 model_view_projection;\n",
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, UniformArray) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 1));

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ("uniform mat4 model_view_projection[1];\n",
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, UniformMultiple) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));
  vertex_snippet.uniforms.push_back(
      CreateUniformDef("world", ShaderDataType_Float4x4, 0));

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ("uniform mat4 model_view_projection;\nuniform mat4 world;\n",
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

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ("uniform mat4 model_view_projection;\n",
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, UniformMultipleDifferentSnippets) {
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.uniforms.push_back(
      CreateUniformDef("model_view_projection", ShaderDataType_Float4x4, 0));
  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.uniforms.push_back(
      CreateUniformDef("world", ShaderDataType_Float4x4, 0));

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ("uniform mat4 model_view_projection;\nuniform mat4 world;\n",
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

TEST(ShaderDataTest, Attribute) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ("attribute vec4 position;\n",
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, AttributeMultiple) {
  ShaderSnippetDefT vertex_snippet;
  vertex_snippet.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  vertex_snippet.inputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ("attribute vec4 position;\nattribute vec4 color;\n",
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

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ("attribute vec4 position;\n",
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

TEST(ShaderDataTest, AttributeMultipleDifferentSnippets) {
  ShaderSnippetDefT vertex_snippet_1;
  vertex_snippet_1.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
  ShaderSnippetDefT vertex_snippet_2;
  vertex_snippet_2.inputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.type = ShaderStageType_Vertex;

  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));
  ShaderData shader_data(shader_def);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ("attribute vec4 position;\nattribute vec4 color;\n",
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
  EXPECT_EQ("attribute vec4 position;\n",
            shader_data.GetStageCode(ShaderStageType_Vertex));
  EXPECT_EQ("", shader_data.GetStageCode(ShaderStageType_Fragment));
}

TEST(ShaderDataTest, InputOutput_Mismatch_2) {
  ShaderSnippetDefT fragment_snippet;
  fragment_snippet.inputs.push_back(CreateAttributeDef(
      "normal", VertexAttributeType_Vec3f, VertexAttributeUsage_Normal));
  ShaderSnippetDefT vertex_position_snippet;
  vertex_position_snippet.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
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
  EXPECT_EQ("attribute vec4 position;\n",
            shader_data.GetStageCode(ShaderStageType_Vertex));
  EXPECT_EQ("", shader_data.GetStageCode(ShaderStageType_Fragment));
}

TEST(ShaderDataTest, InputOutput_Match) {
  ShaderSnippetDefT fragment_snippet;
  fragment_snippet.inputs.push_back(CreateAttributeDef(
      "color", VertexAttributeType_Vec4f, VertexAttributeUsage_Color));
  ShaderSnippetDefT vertex_position_snippet;
  vertex_position_snippet.inputs.push_back(CreateAttributeDef(
      "position", VertexAttributeType_Vec4f, VertexAttributeUsage_Position));
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
  EXPECT_EQ("attribute vec4 position;\nvarying vec4 color;\n",
            shader_data.GetStageCode(ShaderStageType_Vertex));
  EXPECT_EQ("varying vec4 color;\n",
            shader_data.GetStageCode(ShaderStageType_Fragment));
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
  EXPECT_EQ(
      R"(void GeneratedFunctionFragment0() {
gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}

void main() {
GeneratedFunctionFragment0();
}
)",
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
  EXPECT_EQ(
      R"(void GeneratedFunctionFragment0() {
gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
void GeneratedFunctionFragment1() {
// Do nothing.
}

void main() {
GeneratedFunctionFragment0();
GeneratedFunctionFragment1();
}
)",
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
  EXPECT_EQ(
      R"(void GeneratedFunctionVertex0() {
gl_Position = vec4(1.0, 1.0, 1.0, 1.0);
}

void main() {
GeneratedFunctionVertex0();
}
)",
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
  EXPECT_EQ(
      R"(void GeneratedFunctionVertex0() {
gl_Position = vec4(1.0, 1.0, 1.0, 1.0);
}
void GeneratedFunctionVertex1() {
// Do nothing.
}

void main() {
GeneratedFunctionVertex0();
GeneratedFunctionVertex1();
}
)",
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
  EXPECT_EQ(
      R"(uniform mat4 model_view_projection;
attribute vec4 position;
#include "some_header.h"
void GeneratedFunctionVertex0() {
gl_Position = model_view_projection * position;
}

void main() {
GeneratedFunctionVertex0();
}
)",
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
  EXPECT_EQ(
      R"(uniform vec4 color;
varying vec4 vert_color;
#include "some_header.h"
void GeneratedFunctionFragment0() {
gl_FragColor = vert_color * color;
}

void main() {
GeneratedFunctionFragment0();
}
)",
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

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_3));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_4));
  vertex_stage.type = ShaderStageType_Vertex;
  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));

  // Build the shader.
  ShaderCreateParams params;
  params.environment.insert(Hash("ATTR_POSITION"));
  params.environment.insert(Hash("ATTR_NORMAL"));
  ShaderData shader_data(shader_def, params);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(R"(attribute vec4 position;
attribute vec3 normal;
)",
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

  ShaderStageDefT vertex_stage;
  vertex_stage.snippets.push_back(std::move(vertex_snippet_1));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_2));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_3));
  vertex_stage.snippets.push_back(std::move(vertex_snippet_4));
  vertex_stage.type = ShaderStageType_Vertex;
  ShaderDefT shader_def;
  shader_def.stages.push_back(std::move(vertex_stage));

  // Build the shader.
  ShaderCreateParams params;
  params.features.insert(Hash("Transform"));
  params.features.insert(Hash("Light"));
  ShaderData shader_data(shader_def, params);
  EXPECT_TRUE(shader_data.IsValid());
  EXPECT_EQ(R"(attribute vec4 position;
attribute vec3 normal;
)",
            shader_data.GetStageCode(ShaderStageType_Vertex));
}

}  // namespace
}  // namespace lull
