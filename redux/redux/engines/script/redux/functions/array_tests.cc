/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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
#include "redux/engines/script/redux/script_env.h"
#include "redux/engines/script/redux/testing.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::Not;

TEST(ScriptArrayTest, Array) {
  ScriptEnv env;
  ScriptValue res;
  VarArray* arr = nullptr;

  res = env.Exec("(make-array 1 2 3 4)");
  arr = res.Get<VarArray>();
  EXPECT_THAT(arr, Not(IsNull()));
  EXPECT_THAT(arr->Count(), Eq(4));

  res = env.Exec("[1 2 3 4]");
  arr = res.Get<VarArray>();
  EXPECT_THAT(arr, Not(IsNull()));
  EXPECT_THAT(arr->Count(), Eq(4));
}

TEST(ScriptArrayTest, ArraySize) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size [])", 0);
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size [1])", 1);
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size [1 2 3])", 3);
}

TEST(ScriptArrayTest, ArrayEmpty) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-empty [])", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-empty [1])", false);
}

TEST(ScriptArrayTest, ArrayAt) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-at [1 2 3] 0)", 1);
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-at [1 2 3] 1)", 2);
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-at [1 2 3] 2)", 3);
}

TEST(ScriptArrayTest, ArrayPush) {
  ScriptEnv env;

  env.Exec("(= arr [])");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size arr)", 0);

  env.Exec("(array-push arr 1)");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size arr)", 1);

  env.Exec("(array-push arr 2)");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size arr)", 2);
}

TEST(ScriptArrayTest, ArrayPop) {
  ScriptEnv env;

  env.Exec("(= arr [1 2])");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size arr)", 2);

  env.Exec("(array-pop arr)");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size arr)", 1);

  env.Exec("(array-pop arr)");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size arr)", 0);
}

TEST(ScriptArrayTest, ArrayInsert) {
  ScriptEnv env;

  env.Exec("(= arr [1 2])");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size arr)", 2);

  env.Exec("(array-insert arr 1 3)");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size arr)", 3);
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-at arr 0)", 1);
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-at arr 1)", 3);
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-at arr 2)", 2);
}

TEST(ScriptArrayTest, ArrayErase) {
  ScriptEnv env;

  env.Exec("(= arr [1 2 3])");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size arr)", 3);

  env.Exec("(array-erase arr 1)");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-size arr)", 2);
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-at arr 0)", 1);
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-at arr 1)", 3);
}

TEST(ScriptArrayTest, ArraySet) {
  ScriptEnv env;

  env.Exec("(= arr [1 2 3])");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-at arr 1)", 2);

  env.Exec("(array-set arr 1 5)");
  REDUX_CHECK_SCRIPT_RESULT(env, "(array-at arr 1)", 5);
}

TEST(ScriptArrayTest, ArrayForEach) {
  ScriptEnv env;
  ScriptValue res;

  env.Exec("(= arr [1 3 5])");
  res = env.Exec(
      "(do"
      "  (= sum 0)"
      "  (array-foreach arr (val) (= sum (+ sum val)))"
      ")");
  REDUX_CHECK_SCRIPT_RESULT(env, "(do sum)", 9);

  env.Exec(
      "(do"
      "  (= isum 0)"
      "  (= vsum 0)"
      "  (array-foreach arr (idx val) (do"
      "    (= isum (+ isum idx))"
      "    (= vsum (+ vsum val))"
      "  ))"
      ")");
  REDUX_CHECK_SCRIPT_RESULT(env, "(do vsum)", 9);
  REDUX_CHECK_SCRIPT_RESULT(env, "(do isum)", 3);
}

}  // namespace
}  // namespace redux
