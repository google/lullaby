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

#include "lullaby/modules/script/lull/script_scoped_symbol_table.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::NotNull;

TEST(ScriptScopedSymbolTableTest, SetGet) {
  ScriptScopedSymbolTable table;
  const HashValue key = 123;

  ScriptValue value = table.GetValue(key);
  EXPECT_TRUE(value.IsNil());

  table.SetValue(key, ScriptValue::Create(456.f));
  value = table.GetValue(key);
  EXPECT_FALSE(value.IsNil());
  EXPECT_TRUE(value.Is<float>());

  const float* ptr = value.Get<float>();
  EXPECT_THAT(ptr, NotNull());
  EXPECT_THAT(*ptr, Eq(456.f));
}

TEST(ScriptScopedSymbolTableTest, PushPop) {
  ScriptScopedSymbolTable table;
  const HashValue key = 123;

  ScriptValue value = table.GetValue(key);
  EXPECT_TRUE(value.IsNil());

  table.SetValue(key, ScriptValue::Create(123));

  value = table.GetValue(key);
  EXPECT_TRUE(value.Is<int>());

  table.PushScope();

  table.SetValue(key, ScriptValue::Create(456.f));

  value = table.GetValue(key);
  EXPECT_TRUE(value.Is<float>());
  EXPECT_THAT(*value.Get<float>(), Eq(456.f));

  table.PopScope();

  value = table.GetValue(key);
  EXPECT_TRUE(value.Is<int>());
  EXPECT_THAT(*value.Get<int>(), Eq(123));
}

}  // namespace
}  // namespace lull
