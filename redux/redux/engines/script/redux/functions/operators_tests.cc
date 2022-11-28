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

TEST(ScriptOperatorTest, BasicMath) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(+ 1 1)", 2);
  REDUX_CHECK_SCRIPT_RESULT(env, "(- 4 1)", 3);
  REDUX_CHECK_SCRIPT_RESULT(env, "(* 2 3)", 6);
  REDUX_CHECK_SCRIPT_RESULT(env, "(/ 8 2)", 4);
  REDUX_CHECK_SCRIPT_RESULT(env, "(+ 1u 1u)", 2u);
  REDUX_CHECK_SCRIPT_RESULT(env, "(- 4u 1u)", 3u);
  REDUX_CHECK_SCRIPT_RESULT(env, "(* 2u 3u)", 6u);
  REDUX_CHECK_SCRIPT_RESULT(env, "(/ 8u 2u)", 4u);
  REDUX_CHECK_SCRIPT_RESULT(env, "(+ 1.f 1.f)", 2.f);
  REDUX_CHECK_SCRIPT_RESULT(env, "(- 4.f 1.f)", 3.f);
  REDUX_CHECK_SCRIPT_RESULT(env, "(* 2.f 3.f)", 6.f);
  REDUX_CHECK_SCRIPT_RESULT(env, "(/ 8.f 2.f)", 4.f);
  REDUX_CHECK_SCRIPT_RESULT(env, "(+ 1.0 1.0)", 2.0);
  REDUX_CHECK_SCRIPT_RESULT(env, "(- 4.0 1.0)", 3.0);
  REDUX_CHECK_SCRIPT_RESULT(env, "(* 2.0 3.0)", 6.0);
  REDUX_CHECK_SCRIPT_RESULT(env, "(/ 8.0 2.0)", 4.0);
}

TEST(ScriptOperatorTest, BasicComparisons) {
  ScriptEnv env;

  REDUX_CHECK_SCRIPT_RESULT(env, "(== 1 1)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(== 1 2)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= 1 1)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= 1 2)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 1 2)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 1 1)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 2 1)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 1 2)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 1 1)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 2 1)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 1 2)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 1 1)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 2 1)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 1 2)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 1 1)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 2 1)", true);

  REDUX_CHECK_SCRIPT_RESULT(env, "(== 1u 1u)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(== 1u 2u)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= 1u 1u)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= 1u 2u)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 1u 2u)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 1u 1u)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 2u 1u)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 1u 2u)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 1u 1u)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 2u 1u)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 1u 2u)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 1u 1u)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 2u 1u)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 1u 2u)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 1u 1u)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 2u 1u)", true);

  REDUX_CHECK_SCRIPT_RESULT(env, "(== 1.f 1.f)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(== 1.f 2.f)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= 1.f 1.f)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= 1.f 2.f)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 1.f 2.f)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 1.f 1.f)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 2.f 1.f)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 1.f 2.f)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 1.f 1.f)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 2.f 1.f)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 1.f 2.f)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 1.f 1.f)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 2.f 1.f)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 1.f 2.f)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 1.f 1.f)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 2.f 1.f)", true);

  REDUX_CHECK_SCRIPT_RESULT(env, "(== 1.0 1.0)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(== 1.0 2.0)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= 1.0 1.0)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= 1.0 2.0)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 1.0 2.0)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 1.0 1.0)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < 2.0 1.0)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 1.0 2.0)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 1.0 1.0)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= 2.0 1.0)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 1.0 2.0)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 1.0 1.0)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > 2.0 1.0)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 1.0 2.0)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 1.0 1.0)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= 2.0 1.0)", true);
}

TEST(ScriptOperatorTest, And) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(and)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(and true)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(and false)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(and true true)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(and true false)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(and false true)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(and true true true)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(and true false true)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(and false false false)", false);
}

TEST(ScriptOperatorTest, Or) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(or)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(or true)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(or false)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(or true true)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(or true false)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(or false true)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(or true true true)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(or true false true)", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(or false false false)", false);
}

TEST(ScriptOperatorTest, Not) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(not true)", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(not false)", true);
}

TEST(ScriptOperatorTest, Time) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(== (seconds 1.f) (seconds 1.f))", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(== (seconds 1.f) (seconds 2.f))", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= (seconds 1.f) (seconds 1.f))", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= (seconds 1.f) (seconds 2.f))", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(+ (seconds 1.0f) (seconds 2.0f))",
                            absl::Seconds(3.f));
}

TEST(ScriptOperatorTest, Entity) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(== (entity 1) (entity 1))", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(== (entity 1) (entity 2))", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= (entity 1) (entity 1))", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(!= (entity 1) (entity 2))", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < (entity 1) (entity 2))", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < (entity 1) (entity 1))", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( < (entity 2) (entity 1))", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > (entity 1) (entity 2))", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > (entity 1) (entity 1))", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "( > (entity 2) (entity 1))", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= (entity 1) (entity 2))", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= (entity 1) (entity 1))", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(<= (entity 2) (entity 1))", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= (entity 1) (entity 2))", false);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= (entity 1) (entity 1))", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(>= (entity 2) (entity 1))", true);
}
}  // namespace
}  // namespace redux
