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

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/systems/render/next/material.h"
#include "tests/portable_test_macros.h"

namespace lull {

struct Shader {
  explicit Shader(int id) : id(id) {}
  int id;
};

struct Texture {
  explicit Texture(int id) : id(id) {}
  int id;
};

namespace {

using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::FloatEq;
using ::testing::IsNull;
using ::testing::Pair;
using ::testing::Pointee;
using ::testing::UnorderedElementsAre;

MATCHER_P(DescriptionEquals, expected_description, "") {
  const Uniform::Description& actual_description = arg.GetDescription();
  return actual_description.name == expected_description.name &&
         actual_description.type == expected_description.type &&
         actual_description.num_bytes == expected_description.num_bytes &&
         actual_description.count == expected_description.count &&
         actual_description.binding == expected_description.binding;
}

MATCHER_P(ShaderIdEquals, expected_shader_id, "") {
  return arg.id == expected_shader_id;
}

MATCHER_P(TextureIdEquals, expected_texture_id, "") {
  return arg.id == expected_texture_id;
}

TEST(Material, Constructor) {
  const Uniform::Description desc1("u0", Uniform::Type::kFloats, 4, 1, -1);
  const Uniform::Description desc2("u1", Uniform::Type::kFloats, 16, 2, 5);
  const Uniform::Description desc3("u2", Uniform::Type::kMatrix, 64, 1, 2);
  const std::vector<Uniform::Description> kUniformDescriptions = {desc1, desc2,
                                                                  desc3};

  Material material(nullptr, kUniformDescriptions);

  EXPECT_THAT(material.GetShader(), IsNull());

  const std::vector<Uniform>& uniforms = material.GetUniforms();
  EXPECT_THAT(uniforms.size(), Eq(size_t(3)));
  EXPECT_THAT(uniforms,
              ElementsAre(DescriptionEquals(desc1), DescriptionEquals(desc2),
                          DescriptionEquals(desc3)));
}

TEST(Material, AddUniformFromDescription) {
  const Uniform::Description desc1("u0", Uniform::Type::kFloats, 4, 1);
  const Uniform::Description desc2("u1", Uniform::Type::kFloats, 16, 2);
  Material material;
  material.AddUniform(desc1);
  material.AddUniform(desc2);

  const std::vector<Uniform>& uniforms = material.GetUniforms();
  EXPECT_THAT(uniforms.size(), Eq(size_t(2)));
  EXPECT_THAT(uniforms,
              ElementsAre(DescriptionEquals(desc1), DescriptionEquals(desc2)));
}

TEST(Material, AddUniformFromUniform) {
  Material material;
  material.AddUniform(
      Uniform(Uniform::Description("u0", Uniform::Type::kFloats, 4, 1, 5)));
  material.AddUniform(
      Uniform(Uniform::Description("u1", Uniform::Type::kFloats, 16, 2, 20)));

  const std::vector<Uniform>& uniforms = material.GetUniforms();
  EXPECT_THAT(uniforms.size(), Eq(size_t(2)));
  EXPECT_THAT(uniforms,
              ElementsAre(DescriptionEquals(Uniform::Description(
                              "u0", Uniform::Type::kFloats, 4, 1, -1)),
                          DescriptionEquals(Uniform::Description(
                              "u1", Uniform::Type::kFloats, 16, 2, -1))));
}

TEST(Material, GetUniformByIndex) {
  const Uniform::Description desc1("u0", Uniform::Type::kFloats, 4, 1);
  const Uniform::Description desc2("u1", Uniform::Type::kFloats, 16, 2);
  const Uniform::Description desc3("u2", Uniform::Type::kMatrix, 64, 1);
  const std::vector<Uniform::Description> kUniformDescriptions = {desc1, desc2,
                                                                  desc3};
  Material material(nullptr, kUniformDescriptions);

  EXPECT_THAT(material.GetUniformByIndex(0), Pointee(DescriptionEquals(desc1)));
  EXPECT_THAT(material.GetUniformByIndex(1), Pointee(DescriptionEquals(desc2)));
  EXPECT_THAT(material.GetUniformByIndex(2), Pointee(DescriptionEquals(desc3)));
}

TEST(Material, GetUniformByName) {
  const Uniform::Description desc1("u0", Uniform::Type::kFloats, 4, 1);
  const Uniform::Description desc2("u1", Uniform::Type::kFloats, 16, 2);
  const Uniform::Description desc3("u2", Uniform::Type::kMatrix, 64, 1);
  const std::vector<Uniform::Description> kUniformDescriptions = {desc1, desc2,
                                                                  desc3};
  Material material(nullptr, kUniformDescriptions);

  EXPECT_THAT(material.GetUniformByName("u235rt4ey"), IsNull());
  EXPECT_THAT(material.GetUniformByName("u1"),
              Pointee(DescriptionEquals(desc2)));
  EXPECT_THAT(material.GetUniformByName("u0"),
              Pointee(DescriptionEquals(desc1)));
  EXPECT_THAT(material.GetUniformByName("u2"),
              Pointee(DescriptionEquals(desc3)));
}

TEST(Material, GetUniformByHash) {
  const Uniform::Description desc1("u0", Uniform::Type::kFloats, 4, 1);
  const Uniform::Description desc2("u1", Uniform::Type::kFloats, 16, 2);
  const Uniform::Description desc3("u2", Uniform::Type::kMatrix, 64, 1);
  const std::vector<Uniform::Description> kUniformDescriptions = {desc1, desc2,
                                                                  desc3};
  Material material(nullptr, kUniformDescriptions);

  EXPECT_THAT(material.GetUniformByHash(Hash("u235rt4ey")), IsNull());
  EXPECT_THAT(material.GetUniformByHash(Hash("u1")),
              Pointee(DescriptionEquals(desc2)));
  EXPECT_THAT(material.GetUniformByHash(Hash("u2")),
              Pointee(DescriptionEquals(desc3)));
  EXPECT_THAT(material.GetUniformByHash(Hash("u0")),
              Pointee(DescriptionEquals(desc1)));
}

TEST(Material, SetUniformByIndex) {
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 50.0f};
  const std::vector<Uniform::Description> kUniformDescriptions = {
      Uniform::Description("u0", Uniform::Type::kFloats, 4, 1),
      Uniform::Description("u1", Uniform::Type::kFloats, 16, 2),
      Uniform::Description("u2", Uniform::Type::kMatrix, 64, 1)};
  Material material(nullptr, kUniformDescriptions);

  material.SetUniformByIndex(0, &kFloatValues[0], 1, 0);
  EXPECT_THAT(material.GetUniformByIndex(0)->GetData<float>()[0],
              FloatEq(32.0f));
  material.SetUniformByIndex(1, &kFloatValues[1], 1, 0);
  EXPECT_THAT(material.GetUniformByIndex(1)->GetData<float>()[0],
              FloatEq(45.0f));
  material.SetUniformByIndex(2, &kFloatValues[2], 1, 0);
  EXPECT_THAT(material.GetUniformByIndex(2)->GetData<float>()[0],
              FloatEq(50.0f));
}

TEST(Material, SetUniformByHash) {
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 50.0f};
  const std::vector<Uniform::Description> kUniformDescriptions = {
      Uniform::Description("u0", Uniform::Type::kFloats, 4, 1),
      Uniform::Description("u1", Uniform::Type::kFloats, 16, 2),
      Uniform::Description("u2", Uniform::Type::kMatrix, 64, 1)};
  Material material(nullptr, kUniformDescriptions);

  material.SetUniformByHash(Hash("u0"), &kFloatValues[0], 1, 0);
  EXPECT_THAT(material.GetUniformByIndex(0)->GetData<float>()[0],
              FloatEq(32.0f));
  material.SetUniformByHash(Hash("u1"), &kFloatValues[1], 1, 0);
  EXPECT_THAT(material.GetUniformByIndex(1)->GetData<float>()[0],
              FloatEq(45.0f));
  material.SetUniformByHash(Hash("u2"), &kFloatValues[2], 1, 0);
  EXPECT_THAT(material.GetUniformByIndex(2)->GetData<float>()[0],
              FloatEq(50.0f));
}

TEST(Material, SetUniformByName) {
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 50.0f};
  const std::vector<Uniform::Description> kUniformDescriptions = {
      Uniform::Description("u0", Uniform::Type::kFloats, 4, 1),
      Uniform::Description("u1", Uniform::Type::kFloats, 16, 2),
      Uniform::Description("u2", Uniform::Type::kMatrix, 64, 1)};
  Material material(nullptr, kUniformDescriptions);

  material.SetUniformByName("u0", &kFloatValues[0], 1, 0);
  EXPECT_THAT(material.GetUniformByIndex(0)->GetData<float>()[0],
              FloatEq(32.0f));
  material.SetUniformByName("u1", &kFloatValues[1], 1, 0);
  EXPECT_THAT(material.GetUniformByIndex(1)->GetData<float>()[0],
              FloatEq(45.0f));
  material.SetUniformByName("u2", &kFloatValues[2], 1, 0);
  EXPECT_THAT(material.GetUniformByIndex(2)->GetData<float>()[0],
              FloatEq(50.0f));
}

TEST(Material, SetNewUniformByName) {
  static constexpr float kFloatValue = 42.0f;
  Material material;

  const std::vector<Uniform>& uniforms = material.GetUniforms();
  EXPECT_THAT(uniforms.size(), Eq(size_t(0)));

  material.SetUniformByName("u0", &kFloatValue, 1, 0);
  EXPECT_THAT(material.GetUniformByIndex(0)->GetData<float>()[0],
              FloatEq(42.0f));

  EXPECT_THAT(uniforms.size(), Eq(size_t(1)));
  EXPECT_THAT(uniforms, ElementsAre(DescriptionEquals(Uniform::Description(
                            "u0", Uniform::Type::kFloats, 4, 1))));
}

TEST(Material, SetGetShader) {
  Material material;
  EXPECT_THAT(material.GetShader(), IsNull());

  material.SetShader(ShaderPtr(new Shader(5)));
  EXPECT_THAT(material.GetShader(), Pointee(ShaderIdEquals(5)));

  material.SetShader(ShaderPtr(new Shader(4)));
  EXPECT_THAT(material.GetShader(), Pointee(ShaderIdEquals(4)));
}

TEST(Material, SetGetTexture) {
  Material material;
  EXPECT_THAT(material.GetTexture(0), IsNull());

  // Set a few textures one at a time and see that they are set.
  material.SetTexture(0, TexturePtr(new Texture(5)));
  EXPECT_THAT(material.GetTexture(0), Pointee(TextureIdEquals(5)));
  material.SetTexture(1, TexturePtr(new Texture(20)));
  EXPECT_THAT(material.GetTexture(1), Pointee(TextureIdEquals(20)));
  material.SetTexture(4, TexturePtr(new Texture(15)));
  EXPECT_THAT(material.GetTexture(4), Pointee(TextureIdEquals(15)));

  // Re-write a texture.
  material.SetTexture(0, TexturePtr(new Texture(42)));
  EXPECT_THAT(material.GetTexture(0), Pointee(TextureIdEquals(42)));

  // Re-write another texture.
  material.SetTexture(1, TexturePtr(new Texture(8100)));
  EXPECT_THAT(material.GetTexture(1), Pointee(TextureIdEquals(8100)));

  // See that all the textures are correct.
  EXPECT_THAT(material.GetTexture(0), Pointee(TextureIdEquals(42)));
  EXPECT_THAT(material.GetTexture(1), Pointee(TextureIdEquals(8100)));
  EXPECT_THAT(material.GetTexture(4), Pointee(TextureIdEquals(15)));
}

TEST(Material, GetTextures) {
  Material material;

  // Set a a bunch of textures and see that the returned map is correct.
  material.SetTexture(0, TexturePtr(new Texture(5)));
  material.SetTexture(1, TexturePtr(new Texture(20)));
  material.SetTexture(4, TexturePtr(new Texture(15)));

  const std::unordered_map<int, TexturePtr>& textures = material.GetTextures();
  EXPECT_THAT(textures.size(), Eq(size_t(3)));
  EXPECT_THAT(textures,
              UnorderedElementsAre(Pair(0, Pointee(TextureIdEquals(5))),
                                   Pair(1, Pointee(TextureIdEquals(20))),
                                   Pair(4, Pointee(TextureIdEquals(15)))));
}

TEST(MaterialDeathTest, SetUniformByIndexOutOfBounds) {
  static constexpr float kFloatValue = 42.0f;
  Material material;

  PORT_EXPECT_DEBUG_DEATH(material.SetUniformByIndex(2, &kFloatValue, 1, 0),
                          "");
}

}  // namespace
}  // namespace lull
