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

TEST(ScriptFunctionsCondTest, Cond) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(cond (true 1) (false 2))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(cond (false 1) (true 2))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));
}

TEST(ScriptFunctionsCondTest, If) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(if true 1 2)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(if false 1 2)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));
}

}  // namespace
}  // namespace lull
