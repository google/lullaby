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

#include "lullaby/systems/render/next/uniform.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::FloatEq;

TEST(Uniform, SetGetVoidData1) {
  UniformData uniform(ShaderDataType_Float1, 1);

  static constexpr float kFloatValue = 24.0f;
  uniform.SetData(reinterpret_cast<const void*>(&kFloatValue), sizeof(float));
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(24.0f));
}

TEST(Uniform, SetGetVoidData2) {
  UniformData uniform(ShaderDataType_Float2, 1);

  static constexpr float kFloatValues[] = {32.0f, 45.0f};
  uniform.SetData(reinterpret_cast<const void*>(kFloatValues),
                  sizeof(float) * 2);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
}

TEST(Uniform, SetGetFloatData1) {
  UniformData uniform(ShaderDataType_Float1, 1);

  static constexpr float kFloatValue = 24.0f;
  uniform.SetData(&kFloatValue, 1);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(24.0f));
}

TEST(Uniform, SetGetFloatData2) {
  UniformData uniform(ShaderDataType_Float2, 1);

  static constexpr float kFloatValues[] = {32.0f, 45.0f};
  uniform.SetData(kFloatValues, 2);
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
}

TEST(Uniform, Copy) {
  UniformData uniform(ShaderDataType_Float2, 2);
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f, 99.0f};
  static constexpr size_t kNumFloats = sizeof(kFloatValues) / sizeof(float);
  uniform.SetData(kFloatValues, kNumFloats);

  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(uniform.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(uniform.GetData<float>()[3], FloatEq(99.0f));

  UniformData copy = uniform;
  EXPECT_THAT(copy.Type(), Eq(ShaderDataType_Float2));
  EXPECT_THAT(copy.Size(), Eq(sizeof(float) * kNumFloats));
  EXPECT_THAT(copy.Count(), Eq(size_t(2)));
  EXPECT_THAT(copy.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(copy.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(copy.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(copy.GetData<float>()[3], FloatEq(99.0f));
}

TEST(Uniform, Assign) {
  UniformData uniform(ShaderDataType_Float2, 2);
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f, 99.0f};
  static constexpr size_t kNumFloats = sizeof(kFloatValues) / sizeof(float);
  uniform.SetData(kFloatValues, kNumFloats);

  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(uniform.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(uniform.GetData<float>()[3], FloatEq(99.0f));

  UniformData assign(ShaderDataType_Float1, 1);
  assign = uniform;
  EXPECT_THAT(assign.Type(), Eq(ShaderDataType_Float2));
  EXPECT_THAT(assign.Size(), Eq(sizeof(float) * kNumFloats));
  EXPECT_THAT(assign.Count(), Eq(size_t(2)));
  EXPECT_THAT(assign.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(assign.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(assign.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(assign.GetData<float>()[3], FloatEq(99.0f));
}

TEST(Uniform, AssignNoRealloc) {
  UniformData uniform(ShaderDataType_Float2, 3);
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f,
                                           99.0f, 0.f,   0.f};
  static constexpr size_t kNumFloats = sizeof(kFloatValues) / sizeof(float);
  uniform.SetData(kFloatValues, kNumFloats);

  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(uniform.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(uniform.GetData<float>()[3], FloatEq(99.0f));

  // Create the target uniform to be larger then the source.
  UniformData assign(ShaderDataType_Float1, 100);

  const void* ptr = assign.GetData<void>();

  assign = uniform;
  EXPECT_THAT(assign.Type(), Eq(ShaderDataType_Float2));
  EXPECT_THAT(assign.Size(), Eq(sizeof(float) * kNumFloats));
  EXPECT_THAT(assign.Count(), Eq(size_t(3)));
  EXPECT_THAT(assign.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(assign.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(assign.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(assign.GetData<float>()[3], FloatEq(99.0f));
  EXPECT_THAT(assign.GetData<void>(), Eq(ptr));
}

TEST(Uniform, Move) {
  UniformData uniform(ShaderDataType_Float2, 3);
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f,
                                           99.0f, 0.f,   0.f};
  static constexpr size_t kNumFloats = sizeof(kFloatValues) / sizeof(float);
  uniform.SetData(kFloatValues, kNumFloats);

  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(uniform.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(uniform.GetData<float>()[3], FloatEq(99.0f));

  const void* ptr = uniform.GetData<void>();

  UniformData move = std::move(uniform);
  EXPECT_THAT(move.Type(), Eq(ShaderDataType_Float2));
  EXPECT_THAT(move.Size(), Eq(sizeof(float) * kNumFloats));
  EXPECT_THAT(move.Count(), Eq(size_t(3)));
  EXPECT_THAT(move.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(move.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(move.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(move.GetData<float>()[3], FloatEq(99.0f));
  EXPECT_THAT(move.GetData<void>(), Eq(ptr));
}

TEST(Uniform, MoveAssign) {
  UniformData uniform(ShaderDataType_Float2, 3);
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f,
                                           99.0f, 0.f,   0.f};
  static constexpr size_t kNumFloats = sizeof(kFloatValues) / sizeof(float);
  uniform.SetData(kFloatValues, kNumFloats);

  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(uniform.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(uniform.GetData<float>()[3], FloatEq(99.0f));

  const void* ptr = uniform.GetData<void>();

  UniformData assign(ShaderDataType_Float1, 0);
  assign = std::move(uniform);
  EXPECT_THAT(assign.Type(), Eq(ShaderDataType_Float2));
  EXPECT_THAT(assign.Size(), Eq(sizeof(float) * kNumFloats));
  EXPECT_THAT(assign.Count(), Eq(size_t(3)));
  EXPECT_THAT(assign.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(assign.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(assign.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(assign.GetData<float>()[3], FloatEq(99.0f));
  EXPECT_THAT(assign.GetData<void>(), Eq(ptr));
}

TEST(UniformDeathTest, SetFloatTooBig) {
  UniformData uniform(ShaderDataType_Float2, 1);

  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f};
  PORT_EXPECT_DEBUG_DEATH(uniform.SetData(kFloatValues, 3), "");
}

}  // namespace
}  // namespace lull
