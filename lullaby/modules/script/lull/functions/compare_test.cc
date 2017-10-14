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

TEST(ScriptFunctionsCompareTest, And) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(and)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(and true)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(and false)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(and true true)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(and false true)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  env.PushScope();
  res = env.Exec("(and false (do (= foo 123) true))");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));
  res = env.GetValue(Symbol("foo"));
  EXPECT_THAT(res.IsNil(), Eq(true));

  res = env.Exec("(and (do (= foo 123) true) false)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));
  res = env.GetValue(Symbol("foo"));
  EXPECT_THAT(res.IsNil(), Eq(false));
  env.PopScope();

  res = env.Exec("(and true true true)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(and true false true)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));
}

TEST(ScriptFunctionsCompareTest, Or) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(or)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(or true)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(or false)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(or false false)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(or false true)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  env.PushScope();
  res = env.Exec("(or true (do (= foo 123) false))");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));
  res = env.GetValue(Symbol("foo"));
  EXPECT_THAT(res.IsNil(), Eq(true));

  res = env.Exec("(or (do (= foo 123) false) false)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));
  res = env.GetValue(Symbol("foo"));
  EXPECT_THAT(res.IsNil(), Eq(false));
  env.PopScope();

  res = env.Exec("(or false false false)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(or false true false)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));
}

TEST(ScriptFunctionsCompareTest, Not) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(not true)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(not false)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));
}

}  // namespace
}  // namespace lull
