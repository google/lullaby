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
#include "lullaby/modules/script/lull/functions/testing_macros.h"
#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/util/time.h"

namespace lull {
namespace {

using ::testing::Eq;

TEST(ScriptFunctionsOperatorsTest, ComparisonOperatorsInt) {
  ScriptEnv env;
  ScriptValue res;

  LULLABY_TEST_SCRIPT_VALUE(env, "(== 1 1)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(== 1 2)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(!= 1 1)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(!= 1 2)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "( < 1 2)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "( < 1 1)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( < 2 1)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(<= 1 2)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(<= 1 1)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(<= 2 1)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( > 1 2)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( > 1 1)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( > 2 1)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(>= 1 2)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(>= 1 1)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(>= 2 1)", true);
}

TEST(ScriptFunctionsOperatorsTest, ComparisonOperatorsFloat) {
  ScriptEnv env;
  ScriptValue res;

  LULLABY_TEST_SCRIPT_VALUE(env, "(== 1.f 1.f)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(== 1.f 2.f)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(!= 1.f 1.f)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(!= 1.f 2.f)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "( < 1.f 2.f)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "( < 1.f 1.f)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( < 2.f 1.f)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(<= 1.f 2.f)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(<= 1.f 1.f)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(<= 2.f 1.f)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( > 1.f 2.f)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( > 1.f 1.f)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( > 2.f 1.f)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(>= 1.f 2.f)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(>= 1.f 1.f)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(>= 2.f 1.f)", true);
}

TEST(ScriptFunctionsOperatorsTest, ComparisonOperatorsUnsigned) {
  ScriptEnv env;
  ScriptValue res;

  LULLABY_TEST_SCRIPT_VALUE(env, "(== 1u 1u)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(== 1u 2u)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(!= 1u 1u)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(!= 1u 2u)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "( < 1u 2u)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "( < 1u 1u)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( < 2u 1u)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(<= 1u 2u)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(<= 1u 1u)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(<= 2u 1u)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( > 1u 2u)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( > 1u 1u)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "( > 2u 1u)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(>= 1u 2u)", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(>= 1u 1u)", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(>= 2u 1u)", true);
}

TEST(ScriptFunctionsOperatorsTest, ComparisonOperatorsMathfu) {
  ScriptEnv env;
  ScriptValue res;

  LULLABY_TEST_SCRIPT_VALUE(env, "(== (vec2i 1 2) (vec2i 1 2))", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(== (vec2i 1 2) (vec2i 2 1))", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(!= (vec2i 1 2) (vec2i 1 2))", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(!= (vec2i 1 2) (vec2i 2 1))", true);
}

TEST(ScriptFunctionsOperatorsTest, ComparisonOperatorsTime) {
  ScriptEnv env;
  ScriptValue res;

  LULLABY_TEST_SCRIPT_VALUE(env, "(== (seconds 1.f) (seconds 1.f))", true);
  LULLABY_TEST_SCRIPT_VALUE(env, "(== (seconds 1.f) (seconds 2.f))", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(!= (seconds 1.f) (seconds 1.f))", false);
  LULLABY_TEST_SCRIPT_VALUE(env, "(!= (seconds 1.f) (seconds 2.f))", true);
}

TEST(ScriptFunctionsOperatorsTest, MathOperatorsBasic) {
  ScriptEnv env;
  ScriptValue res;

  LULLABY_TEST_SCRIPT_VALUE(env, "(+ 1 1)", 2);
  LULLABY_TEST_SCRIPT_VALUE(env, "(- 4 1)", 3);
  LULLABY_TEST_SCRIPT_VALUE(env, "(* 2 2)", 4);
  LULLABY_TEST_SCRIPT_VALUE(env, "(/ 10 2)", 5);
  LULLABY_TEST_SCRIPT_VALUE(env, "(% 13 5)", 3);

  LULLABY_TEST_SCRIPT_VALUE(env, "(+ 1u 1u)", 2u);
  LULLABY_TEST_SCRIPT_VALUE(env, "(- 4u 1u)", 3u);
  LULLABY_TEST_SCRIPT_VALUE(env, "(* 2u 2u)", 4u);
  LULLABY_TEST_SCRIPT_VALUE(env, "(/ 10u 2u)", 5u);
  LULLABY_TEST_SCRIPT_VALUE(env, "(% 13u 5u)", 3u);

  LULLABY_TEST_SCRIPT_VALUE(env, "(+ 1.0f 1.0f)", 2.f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(- 4.0f 1.0f)", 3.f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(* 2.0f 2.0f)", 4.f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(/ 10.0f 2.0f)", 5.f);

  LULLABY_TEST_SCRIPT_VALUE(env, "(+ 1 2.0f)", 3.f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(+ 4 5u)", 9u);
  LULLABY_TEST_SCRIPT_VALUE(env, "(+ 6.0 7.0f)", 13.0);
}

TEST(ScriptFunctionsOperatorsTest, MathOperatorsTime) {
  ScriptEnv env;
  ScriptValue res;

  LULLABY_TEST_SCRIPT_VALUE(env, "(+ (seconds 1.0f) (seconds 2.0f))",
                            DurationFromSeconds(3.f));
}

TEST(ScriptFunctionsOperatorsTest, MathOperatorsMathfu) {
  ScriptEnv env;
  ScriptValue res;

  LULLABY_TEST_SCRIPT_VALUE(env, "(+ (vec2i 1 2) (vec2i 3 4))",
                            mathfu::vec2i(4, 6));
  LULLABY_TEST_SCRIPT_VALUE(env, "(- (vec2i 3 4) (vec2i 1 2))",
                            mathfu::vec2i(2, 2));
  LULLABY_TEST_SCRIPT_VALUE(env, "(* (vec2 1.f 2.f) 2.f)",
                            mathfu::vec2(2.f, 4.f));
  LULLABY_TEST_SCRIPT_VALUE(env, "(* (vec3 1.f 2.f 3.f) 2.f)",
                            mathfu::vec3(2.f, 4.f, 6.f));
  LULLABY_TEST_SCRIPT_VALUE(env, "(* 2.f (vec2 1.f 2.f))",
                            mathfu::vec2(2.f, 4.f));
  LULLABY_TEST_SCRIPT_VALUE(env, "(* 2.f (vec3 1.f 2.f 3.f))",
                            mathfu::vec3(2.f, 4.f, 6.f));
  LULLABY_TEST_SCRIPT_VALUE(env,
                            "(* (quat 0.f 1.f 0.f 0.f) (vec3 1.f 2.f 3.f))",
                            mathfu::vec3(1.f, -2.f, -3.f));
}


}  // namespace
}  // namespace lull
