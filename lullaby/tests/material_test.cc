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
#include "lullaby/tests/portable_test_macros.h"

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

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Pointee;

MATCHER_P(TextureIdEquals, expected_texture_id, "") {
  return arg.id == expected_texture_id;
}

TEST(Material, SetGetUniform) {
  static constexpr HashValue kName = ConstHash("uniform");
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 50.0f, 75.f};

  Material material;
  material.SetUniform<float>(kName, ShaderDataType_Float1, {kFloatValues, 4});

  const UniformData* uniform = material.GetUniformData(kName);
  EXPECT_THAT(uniform, NotNull());

  const float* data = uniform->GetData<float>();
  EXPECT_THAT(data[0], Eq(kFloatValues[0]));
  EXPECT_THAT(data[1], Eq(kFloatValues[1]));
  EXPECT_THAT(data[2], Eq(kFloatValues[2]));
  EXPECT_THAT(data[3], Eq(kFloatValues[3]));
}

TEST(Material, SetGetShader) {
  Material material;
  EXPECT_THAT(material.GetShader(), IsNull());

  material.SetShader(nullptr);
  EXPECT_THAT(material.GetShader(), IsNull());
}

TEST(Material, SetGetTexture) {
  Material material;
  EXPECT_THAT(material.GetTexture(MaterialTextureUsage_BaseColor), IsNull());

  // Set a few textures one at a time and see that they are set.
  material.SetTexture(MaterialTextureUsage_BaseColor,
                      TexturePtr(new Texture(5)));
  EXPECT_THAT(material.GetTexture(MaterialTextureUsage_BaseColor),
              Pointee(TextureIdEquals(5)));
  material.SetTexture(MaterialTextureUsage_Metallic,
                      TexturePtr(new Texture(20)));
  EXPECT_THAT(material.GetTexture(MaterialTextureUsage_Metallic),
              Pointee(TextureIdEquals(20)));
  material.SetTexture(MaterialTextureUsage_Specular,
                      TexturePtr(new Texture(15)));
  EXPECT_THAT(material.GetTexture(MaterialTextureUsage_Specular),
              Pointee(TextureIdEquals(15)));

  // Re-write a texture.
  material.SetTexture(MaterialTextureUsage_BaseColor,
                      TexturePtr(new Texture(42)));
  EXPECT_THAT(material.GetTexture(MaterialTextureUsage_BaseColor),
              Pointee(TextureIdEquals(42)));

  // Re-write another texture.
  material.SetTexture(MaterialTextureUsage_Metallic,
                      TexturePtr(new Texture(8100)));
  EXPECT_THAT(material.GetTexture(MaterialTextureUsage_Metallic),
              Pointee(TextureIdEquals(8100)));

  // See that all the textures are correct.
  EXPECT_THAT(material.GetTexture(MaterialTextureUsage_BaseColor),
              Pointee(TextureIdEquals(42)));
  EXPECT_THAT(material.GetTexture(MaterialTextureUsage_Metallic),
              Pointee(TextureIdEquals(8100)));
  EXPECT_THAT(material.GetTexture(MaterialTextureUsage_Specular),
              Pointee(TextureIdEquals(15)));
}

}  // namespace
}  // namespace lull
