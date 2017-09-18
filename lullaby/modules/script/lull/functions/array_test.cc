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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/script/lull/script_env.h"

namespace lull {
namespace {

using ::testing::Eq;

TEST(ScriptFunctionsArrayTest, Array) {
  ScriptEnv env;
  ScriptValue res;
  res = env.Exec("(make-array 1 2 3 4)");
  EXPECT_THAT(res.Is<VariantArray>(), Eq(true));
  EXPECT_THAT(res.Get<VariantArray>()->size(), Eq(size_t(4)));

  res = env.Exec("[1 2 3 4]");
  EXPECT_THAT(res.Is<VariantArray>(), Eq(true));
  EXPECT_THAT(res.Get<VariantArray>()->size(), Eq(size_t(4)));
}

TEST(ScriptFunctionsArrayTest, ArraySize) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(array-size (make-array))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(0));

  res = env.Exec("(array-size (make-array 1))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(array-size (make-array 1 2 3))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));
}

TEST(ScriptFunctionsArrayTest, ArrayEmpty) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(array-empty (make-array))");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(array-empty (make-array 1 2 3))");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));
}

TEST(ScriptFunctionsArrayTest, ArrayPush) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= a (make-array))");
  res = env.Exec("(array-size a)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(0));

  res = env.Exec("(array-push a 1)");
  res = env.Exec("(array-size a)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(array-push a 2)");
  res = env.Exec("(array-size a)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));
}

TEST(ScriptFunctionsArrayTest, ArrayPop) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= a (make-array 1 2))");
  res = env.Exec("(array-size a)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));

  res = env.Exec("(array-pop a)");
  res = env.Exec("(array-size a)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(array-pop a)");
  res = env.Exec("(array-size a)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(0));
}

TEST(ScriptFunctionsArrayTest, ArrayInsert) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= a (make-array 1 2))");
  res = env.Exec("(array-insert a 1 3)");
  res = env.Exec("(array-size a)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));

  res = env.Exec("(array-at a 0)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(array-at a 1)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));

  res = env.Exec("(array-at a 2)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));
}

TEST(ScriptFunctionsArrayTest, ArrayErase) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= a (make-array 1 2 3))");
  res = env.Exec("(array-erase a 1)");
  res = env.Exec("(array-size a)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));

  res = env.Exec("(array-at a 0)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(array-at a 1)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));
}

TEST(ScriptFunctionsArrayTest, ArrayAt) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(array-at (make-array 1 2 3 4) 2)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));
}

}  // namespace
}  // namespace lull
