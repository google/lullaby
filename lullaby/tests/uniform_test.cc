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

#include "lullaby/systems/render/detail/uniform_data.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::FloatEq;

TEST(Uniform, SetGetFloatData1) {
  static constexpr float kFloatValue = 24.0f;

  detail::UniformData uniform;
  uniform.SetData(ShaderDataType_Float1, ToByteSpan(&kFloatValue));
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(24.0f));
}

TEST(Uniform, SetGetFloatData2) {
  static constexpr float kFloatValues[] = {32.0f, 45.0f};

  detail::UniformData uniform;
  uniform.SetData(ShaderDataType_Float2, ToByteSpan(kFloatValues, 2));
  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
}

TEST(Uniform, Copy) {
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f, 99.0f};
  static constexpr size_t kNumFloats = sizeof(kFloatValues) / sizeof(float);

  detail::UniformData uniform;
  uniform.SetData(ShaderDataType_Float2, ToByteSpan(kFloatValues, kNumFloats));

  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(uniform.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(uniform.GetData<float>()[3], FloatEq(99.0f));

  detail::UniformData copy = uniform;
  EXPECT_THAT(copy.Type(), Eq(ShaderDataType_Float2));
  EXPECT_THAT(copy.Size(), Eq(sizeof(float) * kNumFloats));
  EXPECT_THAT(copy.Count(), Eq(size_t(2)));
  EXPECT_THAT(copy.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(copy.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(copy.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(copy.GetData<float>()[3], FloatEq(99.0f));
}

TEST(Uniform, Assign) {
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f, 99.0f};
  static constexpr size_t kNumFloats = sizeof(kFloatValues) / sizeof(float);

  detail::UniformData uniform;
  uniform.SetData(ShaderDataType_Float2, ToByteSpan(kFloatValues, kNumFloats));

  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(uniform.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(uniform.GetData<float>()[3], FloatEq(99.0f));

  detail::UniformData assign;
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
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f,
                                           99.0f, 0.f,   0.f};
  static constexpr size_t kNumFloats = sizeof(kFloatValues) / sizeof(float);

  detail::UniformData uniform;
  uniform.SetData(ShaderDataType_Float2, ToByteSpan(kFloatValues, kNumFloats));

  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(uniform.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(uniform.GetData<float>()[3], FloatEq(99.0f));

  // Create a target uniform to be larger then the source.
  detail::UniformData assign;
  std::vector<float> large_data(100);
  assign.SetData(ShaderDataType_Float1, ToByteSpan(large_data));

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
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f,
                                           99.0f, 0.f,   0.f};
  static constexpr size_t kNumFloats = sizeof(kFloatValues) / sizeof(float);

  detail::UniformData uniform;
  uniform.SetData(ShaderDataType_Float2, ToByteSpan(kFloatValues, kNumFloats));

  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(uniform.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(uniform.GetData<float>()[3], FloatEq(99.0f));

  const void* ptr = uniform.GetData<void>();

  detail::UniformData move = std::move(uniform);
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
  static constexpr float kFloatValues[] = {32.0f, 45.0f, 82.0f,
                                           99.0f, 0.f,   0.f};
  static constexpr size_t kNumFloats = sizeof(kFloatValues) / sizeof(float);

  detail::UniformData uniform;
  uniform.SetData(ShaderDataType_Float2, ToByteSpan(kFloatValues, kNumFloats));

  EXPECT_THAT(uniform.GetData<float>()[0], FloatEq(32.0f));
  EXPECT_THAT(uniform.GetData<float>()[1], FloatEq(45.0f));
  EXPECT_THAT(uniform.GetData<float>()[2], FloatEq(82.0f));
  EXPECT_THAT(uniform.GetData<float>()[3], FloatEq(99.0f));

  const void* ptr = uniform.GetData<void>();

  detail::UniformData assign;
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

}  // namespace
}  // namespace lull
