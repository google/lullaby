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

#include "lullaby/systems/render/uniform.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::FloatEq;

TEST(Uniform, GetDescription) {
  Uniform::Description desc;
  desc.name = "test_uniform";
  desc.type = Uniform::Type::kMatrix;
  desc.num_bytes = 64;
  desc.count = 1;
  Uniform uniform(desc);

  EXPECT_THAT(uniform.GetDescription().name, Eq("test_uniform"));
  EXPECT_THAT(uniform.GetDescription().type, Eq(Uniform::Type::kMatrix));
  EXPECT_THAT(uniform.GetDescription().num_bytes, Eq(size_t(64)));
  EXPECT_THAT(uniform.GetDescription().count, Eq(1));
  EXPECT_THAT(uniform.GetDescription().binding, Eq(-1));
}

TEST(Uniform, SetGetVoidData1) {
  Uniform::Description desc;
  desc.name = "test_uniform";
  desc.type = Uniform::Type::kFloats;
  desc.num_bytes = 4;
  desc.count = 1;
  Uniform uniform(desc);

  static constexpr float kFloatValue = 24.0f;
  uniform.SetData(reinterpret_cast<const void*>(&kFloatValue), sizeof(float),
                  0);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(24.0f));
}

TEST(Uniform, SetGetVoidData2) {
  Uniform::Description desc;
  desc.name = "test_uniform";
  desc.type = Uniform::Type::kFloats;
  desc.num_bytes = 8;
  desc.count = 1;
  Uniform uniform(desc);

  static constexpr float kFloatValues[] = {32.0f, 45.0f};
  uniform.SetData(reinterpret_cast<const void*>(kFloatValues),
                  sizeof(float) * 2, 0);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
}

TEST(Uniform, SetGetVoidDataOffset) {
  Uniform::Description desc;
  desc.name = "test_uniform";
  desc.type = Uniform::Type::kFloats;
  desc.num_bytes = 8;
  desc.count = 1;
  Uniform uniform(desc);

  static constexpr float kFloatValues[] = {32.0f, 45.0f};
  static constexpr float kFloatValue = 24.0f;
  uniform.SetData(reinterpret_cast<const void*>(kFloatValues),
                  sizeof(float) * 2, 0);
  uniform.SetData(reinterpret_cast<const void*>(&kFloatValue), sizeof(float),
                  4);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(24.0f));

  uniform.SetData(reinterpret_cast<const void*>(&kFloatValue), sizeof(float),
                  0);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(24.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(24.0f));
}

TEST(Uniform, SetGetFloatData1) {
  Uniform::Description desc;
  desc.name = "test_uniform";
  desc.type = Uniform::Type::kFloats;
  desc.num_bytes = 4;
  desc.count = 1;
  Uniform uniform(desc);

  static constexpr float kFloatValue = 24.0f;
  uniform.SetData(&kFloatValue, 1, 0);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(24.0f));
}

TEST(Uniform, SetGetFloatData2) {
  Uniform::Description desc;
  desc.name = "test_uniform";
  desc.type = Uniform::Type::kFloats;
  desc.num_bytes = 8;
  desc.count = 1;
  Uniform uniform(desc);

  static constexpr float kFloatValues[] = {32.0f, 45.0f};
  uniform.SetData(kFloatValues, 2, 0);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
}

TEST(Uniform, SetGetFloatDataOffset) {
  Uniform::Description desc;
  desc.name = "test_uniform";
  desc.type = Uniform::Type::kFloats;
  desc.num_bytes = 8;
  desc.count = 1;
  Uniform uniform(desc);

  static constexpr float kFloatValues[] = {32.0f, 45.0f};
  static constexpr float kFloatValue = 24.0f;
  uniform.SetData(kFloatValues, 2, 0);
  uniform.SetData(&kFloatValue, 1, 4);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(24.0f));

  uniform.SetData(&kFloatValue, 1, 0);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(24.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(24.0f));
}

TEST(UniformDeathTest, SetFloatTooBig) {
  Uniform::Description desc;
  desc.name = "test_uniform";
  desc.type = Uniform::Type::kFloats;
  desc.num_bytes = 8;
  desc.count = 1;
  Uniform uniform(desc);

  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f };
  PORT_EXPECT_DEBUG_DEATH(uniform.SetData(kFloatValues, 3, 0), "");
}

TEST(UniformDeathTest, SetFloatTooMuchOffset) {
  Uniform::Description desc;
  desc.name = "test_uniform";
  desc.type = Uniform::Type::kFloats;
  desc.num_bytes = 8;
  desc.count = 1;
  Uniform uniform(desc);

  static constexpr float kFloatValue = 24.0f;
  PORT_EXPECT_DEBUG_DEATH(uniform.SetData(&kFloatValue, 1, 8), "");
}

}  // namespace
}  // namespace lull
