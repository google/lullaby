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

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/systems/render/next/material.h"
#include "lullaby/systems/render/next/shader.h"
#include "lullaby/systems/render/next/texture.h"
#include "lullaby/systems/render/next/texture_factory.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {

namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Pointee;

MATCHER_P(TextureIdEquals, expected_texture_id, "") {
  return *arg.GetResourceId() == expected_texture_id;
}

TEST(Material, SetGetUniform) {
  static constexpr HashValue kName = ConstHash("uniform");
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 50.0f, 75.f};

  Material material;
  material.SetUniform<float>(kName, ShaderDataType_Float1, {kFloatValues, 4});

  const detail::UniformData* uniform = material.GetUniformData(kName);
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
  Registry registry;
  TextureFactoryImpl factory(&registry);

  const TextureUsageInfo color(MaterialTextureUsage_BaseColor);
  const TextureUsageInfo metallic(MaterialTextureUsage_Metallic);
  const TextureUsageInfo specular(MaterialTextureUsage_Specular);

  Material material;
  EXPECT_THAT(material.GetTexture(color), IsNull());

  // Set a few textures one at a time and see that they are set.
  material.SetTexture(color, factory.CreateTexture(0, 5));
  EXPECT_THAT(material.GetTexture(color), Pointee(TextureIdEquals(5)));

  material.SetTexture(metallic, factory.CreateTexture(0, 20));
  EXPECT_THAT(material.GetTexture(metallic), Pointee(TextureIdEquals(20)));

  material.SetTexture(specular, factory.CreateTexture(0, 15));
  EXPECT_THAT(material.GetTexture(specular), Pointee(TextureIdEquals(15)));

  // Re-write a texture.
  material.SetTexture(color, factory.CreateTexture(0, 42));
  EXPECT_THAT(material.GetTexture(color), Pointee(TextureIdEquals(42)));

  // Re-write another texture.
  material.SetTexture(metallic, factory.CreateTexture(0, 8100));
  EXPECT_THAT(material.GetTexture(metallic), Pointee(TextureIdEquals(8100)));

  // See that all the textures are correct.
  EXPECT_THAT(material.GetTexture(color), Pointee(TextureIdEquals(42)));
  EXPECT_THAT(material.GetTexture(metallic), Pointee(TextureIdEquals(8100)));
  EXPECT_THAT(material.GetTexture(specular), Pointee(TextureIdEquals(15)));
}

}  // namespace
}  // namespace lull
