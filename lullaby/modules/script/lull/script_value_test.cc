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

#include "lullaby/modules/script/lull/script_value.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;

TEST(ScriptValueTest, Nil) {
  ScriptValue value;
  EXPECT_TRUE(value.IsNil());
}

TEST(ScriptValueTest, CreateGetSet) {
  ScriptValue value = ScriptValue::Create(123);
  EXPECT_FALSE(value.IsNil());

  EXPECT_THAT(value.GetTypeId(), Eq(GetTypeId<int>()));
  EXPECT_TRUE(value.Is<int>());
  EXPECT_FALSE(value.Is<float>());

  const int* int_ptr = value.Get<int>();
  EXPECT_THAT(int_ptr, NotNull());
  EXPECT_THAT(*int_ptr, Eq(123));

  const float* float_ptr = value.Get<float>();
  EXPECT_THAT(float_ptr, IsNull());

  value.Set(456.f);
  EXPECT_THAT(value.GetTypeId(), Eq(GetTypeId<float>()));
  EXPECT_FALSE(value.Is<int>());
  EXPECT_TRUE(value.Is<float>());

  int_ptr = value.Get<int>();
  EXPECT_THAT(int_ptr, IsNull());

  float_ptr = value.Get<float>();
  EXPECT_THAT(float_ptr, NotNull());
  EXPECT_THAT(*float_ptr, Eq(456.f));

  value.Reset();
  EXPECT_TRUE(value.IsNil());
  EXPECT_FALSE(value.Is<int>());
  EXPECT_FALSE(value.Is<float>());
  EXPECT_THAT(value.Get<int>(), IsNull());
  EXPECT_THAT(value.Get<float>(), IsNull());
}

TEST(ScriptValueTest, CopyAssignMove) {
  ScriptValue value1 = ScriptValue::Create(123);
  ScriptValue value2 = ScriptValue::Create(456.f);
  ScriptValue value3;
  EXPECT_TRUE(value1.Is<int>());
  EXPECT_TRUE(value2.Is<float>());
  EXPECT_TRUE(value3.IsNil());

  value1 = value2;
  EXPECT_TRUE(value1.Is<float>());
  EXPECT_TRUE(value2.Is<float>());
  EXPECT_TRUE(value3.IsNil());
  EXPECT_THAT(value1.Get<float>(), Eq(value2.Get<float>()));

  value2.Set(789);
  EXPECT_TRUE(value1.Is<int>());
  EXPECT_TRUE(value2.Is<int>());
  EXPECT_TRUE(value3.IsNil());
  EXPECT_THAT(value1.Get<int>(), Eq(value2.Get<int>()));

  value1 = value3;
  EXPECT_TRUE(value1.IsNil());
  EXPECT_TRUE(value2.Is<int>());
  EXPECT_TRUE(value3.IsNil());

  value1 = std::move(value2);
  EXPECT_TRUE(value1.Is<int>());
  EXPECT_TRUE(value2.IsNil());
  EXPECT_TRUE(value3.IsNil());

  ScriptValue value4 = value1;
  EXPECT_TRUE(value1.Is<int>());
  EXPECT_TRUE(value2.IsNil());
  EXPECT_TRUE(value3.IsNil());
  EXPECT_TRUE(value4.Is<int>());
  EXPECT_THAT(value1.Get<int>(), Eq(value4.Get<int>()));

  ScriptValue value5 = std::move(value1);
  EXPECT_TRUE(value1.IsNil());
  EXPECT_TRUE(value2.IsNil());
  EXPECT_TRUE(value3.IsNil());
  EXPECT_TRUE(value4.Is<int>());
  EXPECT_TRUE(value5.Is<int>());
  EXPECT_THAT(value4.Get<int>(), Eq(value5.Get<int>()));
}

TEST(ScriptValueTest, NumericCast) {
  ScriptValue value = ScriptValue::Create(123);
  EXPECT_THAT(*value.NumericCast<int8_t>(), Eq(int8_t(123)));
  EXPECT_THAT(*value.NumericCast<uint8_t>(), Eq(uint8_t(123)));
  EXPECT_THAT(*value.NumericCast<int16_t>(), Eq(int16_t(123)));
  EXPECT_THAT(*value.NumericCast<uint16_t>(), Eq(uint16_t(123)));
  EXPECT_THAT(*value.NumericCast<int32_t>(), Eq(int32_t(123)));
  EXPECT_THAT(*value.NumericCast<uint32_t>(), Eq(uint32_t(123)));
  EXPECT_THAT(*value.NumericCast<int64_t>(), Eq(int64_t(123)));
  EXPECT_THAT(*value.NumericCast<uint64_t>(), Eq(uint64_t(123)));
  EXPECT_THAT(*value.NumericCast<float>(), Eq(123.f));
  EXPECT_THAT(*value.NumericCast<double>(), Eq(123.0));

  value = ScriptValue::Create(123.f);
  EXPECT_THAT(*value.NumericCast<int8_t>(), Eq(int8_t(123)));
  EXPECT_THAT(*value.NumericCast<uint8_t>(), Eq(uint8_t(123)));
  EXPECT_THAT(*value.NumericCast<int16_t>(), Eq(int16_t(123)));
  EXPECT_THAT(*value.NumericCast<uint16_t>(), Eq(uint16_t(123)));
  EXPECT_THAT(*value.NumericCast<int32_t>(), Eq(int32_t(123)));
  EXPECT_THAT(*value.NumericCast<uint32_t>(), Eq(uint32_t(123)));
  EXPECT_THAT(*value.NumericCast<int64_t>(), Eq(int64_t(123)));
  EXPECT_THAT(*value.NumericCast<uint64_t>(), Eq(uint64_t(123)));
  EXPECT_THAT(*value.NumericCast<float>(), Eq(123.f));
  EXPECT_THAT(*value.NumericCast<double>(), Eq(123.0));
}

TEST(ScriptValueTest, Variant) {
  ScriptValue value;
  EXPECT_TRUE(value.IsNil());

  const Variant* var = value.GetVariant();
  EXPECT_THAT(var, IsNull());

  Variant other = 456.f;
  value.SetFromVariant(other);
  EXPECT_TRUE(value.IsNil());

  var = value.GetVariant();
  EXPECT_THAT(var, IsNull());

  value = ScriptValue::Create(123);
  EXPECT_FALSE(value.IsNil());

  var = value.GetVariant();
  EXPECT_THAT(var, NotNull());
  EXPECT_THAT(var->GetTypeId(), Eq(GetTypeId<int>()));

  value.SetFromVariant(other);
  var = value.GetVariant();
  EXPECT_THAT(var, NotNull());
  EXPECT_THAT(var->GetTypeId(), Eq(GetTypeId<float>()));
}

}  // namespace
}  // namespace lull
