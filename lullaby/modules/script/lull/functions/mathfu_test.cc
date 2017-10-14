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

namespace lull {
namespace {

using ::testing::Eq;

TEST(ScriptFunctionsMathfuTest, Mathfu) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(vec2i 1 2)");
  EXPECT_THAT(res.Is<mathfu::vec2i>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::vec2i>()->x, Eq(1));
  EXPECT_THAT(res.Get<mathfu::vec2i>()->y, Eq(2));

  res = env.Exec("(vec3i 3 4 5)");
  EXPECT_THAT(res.Is<mathfu::vec3i>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::vec3i>()->x, Eq(3));
  EXPECT_THAT(res.Get<mathfu::vec3i>()->y, Eq(4));
  EXPECT_THAT(res.Get<mathfu::vec3i>()->z, Eq(5));

  res = env.Exec("(vec4i 6 7 8 9)");
  EXPECT_THAT(res.Is<mathfu::vec4i>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::vec4i>()->x, Eq(6));
  EXPECT_THAT(res.Get<mathfu::vec4i>()->y, Eq(7));
  EXPECT_THAT(res.Get<mathfu::vec4i>()->z, Eq(8));
  EXPECT_THAT(res.Get<mathfu::vec4i>()->w, Eq(9));

  res = env.Exec("(vec2 1.0f 2.0f)");
  EXPECT_THAT(res.Is<mathfu::vec2>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::vec2>()->x, Eq(1.f));
  EXPECT_THAT(res.Get<mathfu::vec2>()->y, Eq(2.f));

  res = env.Exec("(vec3 3.0f 4.0f 5.0f)");
  EXPECT_THAT(res.Is<mathfu::vec3>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::vec3>()->x, Eq(3.f));
  EXPECT_THAT(res.Get<mathfu::vec3>()->y, Eq(4.f));
  EXPECT_THAT(res.Get<mathfu::vec3>()->z, Eq(5.f));

  res = env.Exec("(vec4 6.0f 7.0f 8.0f 9.0f)");
  EXPECT_THAT(res.Is<mathfu::vec4>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::vec4>()->x, Eq(6.f));
  EXPECT_THAT(res.Get<mathfu::vec4>()->y, Eq(7.f));
  EXPECT_THAT(res.Get<mathfu::vec4>()->z, Eq(8.f));
  EXPECT_THAT(res.Get<mathfu::vec4>()->w, Eq(9.f));

  res = env.Exec("(quat 0.1f 0.2f 0.3f 0.4f)");
  EXPECT_THAT(res.Is<mathfu::quat>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::quat>()->scalar(), Eq(0.1f));
  EXPECT_THAT(res.Get<mathfu::quat>()->vector().x, Eq(0.2f));
  EXPECT_THAT(res.Get<mathfu::quat>()->vector().y, Eq(0.3f));
  EXPECT_THAT(res.Get<mathfu::quat>()->vector().z, Eq(0.4f));
}

TEST(ScriptFunctionsMathfuTest, Getters) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= v2i (vec2i 1 2))");
  res = env.Exec("(= v3i (vec3i 3 4 5))");
  res = env.Exec("(= v4i (vec4i 6 7 8 9))");
  res = env.Exec("(= v2f (vec2 1.0f 2.0f))");
  res = env.Exec("(= v3f (vec3 3.0f 4.0f 5.0f))");
  res = env.Exec("(= v4f (vec4 6.0f 7.0f 8.0f 9.0f))");
  res = env.Exec("(= qtf (quat 0.1f 0.2f 0.3f 0.4f))");

  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v2i)", 1);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v3i)", 3);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v4i)", 6);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v2f)", 1.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v3f)", 3.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v4f)", 6.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x qtf)", 0.2f);

  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v2i)", 2);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v3i)", 4);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v4i)", 7);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v2f)", 2.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v3f)", 4.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v4f)", 7.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y qtf)", 0.3f);

  LULLABY_TEST_SCRIPT_VALUE(env, "(get-z v3i)", 5);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-z v4i)", 8);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-z v3f)", 5.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-z v4f)", 8.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-z qtf)", 0.4f);

  LULLABY_TEST_SCRIPT_VALUE(env, "(get-w v4i)", 9);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-w v4f)", 9.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-w qtf)", 0.1f);
}

TEST(ScriptFunctionsMathfuTest, Setters) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= v2i (vec2i 1 2))");
  res = env.Exec("(= v3i (vec3i 3 4 5))");
  res = env.Exec("(= v4i (vec4i 6 7 8 9))");
  res = env.Exec("(= v2f (vec2 1.0f 2.0f))");
  res = env.Exec("(= v3f (vec3 3.0f 4.0f 5.0f))");
  res = env.Exec("(= v4f (vec4 6.0f 7.0f 8.0f 9.0f))");
  res = env.Exec("(= qtf (quat 0.1f 0.2f 0.3f 0.4f))");

  env.Exec("(set-x v2i 0)");
  env.Exec("(set-x v3i 0)");
  env.Exec("(set-x v4i 0)");
  env.Exec("(set-x v2f 0.0f)");
  env.Exec("(set-x v3f 0.0f)");
  env.Exec("(set-x v4f 0.0f)");
  env.Exec("(set-x qtf 0.0f)");
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v2i)", 0);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v3i)", 0);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v4i)", 0);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v2f)", 0.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v3f)", 0.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x v4f)", 0.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-x qtf)", 0.0f);

  env.Exec("(set-y v2i 0)");
  env.Exec("(set-y v3i 0)");
  env.Exec("(set-y v4i 0)");
  env.Exec("(set-y v2f 0.0)");
  env.Exec("(set-y v3f 0.0f)");
  env.Exec("(set-y v4f 0.0f)");
  env.Exec("(set-y qtf 0.0f)");
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v2i)", 0);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v3i)", 0);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v4i)", 0);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v2f)", 0.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v3f)", 0.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y v4f)", 0.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-y qtf)", 0.0f);

  env.Exec("(set-z v3i 0)");
  env.Exec("(set-z v4i 0)");
  env.Exec("(set-z v3f 0.0f)");
  env.Exec("(set-z v4f 0.0f)");
  env.Exec("(set-z qtf 0.0f)");
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-z v3i)", 0);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-z v4i)", 0);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-z v3f)", 0.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-z v4f)", 0.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-z qtf)", 0.0f);

  env.Exec("(set-w v4i 0)");
  env.Exec("(set-w v4f 0.0f)");
  env.Exec("(set-w qtf 0.0f)");
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-w v4i)", 0);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-w v4f)", 0.0f);
  LULLABY_TEST_SCRIPT_VALUE(env, "(get-w qtf)", 0.0f);
}


}  // namespace
}  // namespace lull
