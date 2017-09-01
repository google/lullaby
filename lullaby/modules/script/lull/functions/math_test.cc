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

TEST(ScriptFunctionsMathTest, IntMath) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(+ 1 1)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));

  res = env.Exec("(- 4 1)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));

  res = env.Exec("(* 2 2)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(4));

  res = env.Exec("(/ 10 2)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(5));

  res = env.Exec("(% 13 7)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(6));
}

TEST(ScriptFunctionsMathTest, FloatMath) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(+ 1.0f 1.0f)");
  EXPECT_THAT(res.Is<float>(), Eq(true));
  EXPECT_THAT(*res.Get<float>(), Eq(2.f));

  res = env.Exec("(- 4.0f 1.0f)");
  EXPECT_THAT(res.Is<float>(), Eq(true));
  EXPECT_THAT(*res.Get<float>(), Eq(3.f));

  res = env.Exec("(* 2.0f 2.0f)");
  EXPECT_THAT(res.Is<float>(), Eq(true));
  EXPECT_THAT(*res.Get<float>(), Eq(4.f));

  res = env.Exec("(/ 10.0f 2.0f)");
  EXPECT_THAT(res.Is<float>(), Eq(true));
  EXPECT_THAT(*res.Get<float>(), Eq(5.f));
}

}  // namespace
}  // namespace lull
