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

TEST(ScriptFunctionsTest, IntMath) {
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

TEST(ScriptFunctionsTest, FloatMath) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(+ 1.0 1.0)");
  EXPECT_THAT(res.Is<float>(), Eq(true));
  EXPECT_THAT(*res.Get<float>(), Eq(2.f));

  res = env.Exec("(- 4.0 1.0)");
  EXPECT_THAT(res.Is<float>(), Eq(true));
  EXPECT_THAT(*res.Get<float>(), Eq(3.f));

  res = env.Exec("(* 2.0 2.0)");
  EXPECT_THAT(res.Is<float>(), Eq(true));
  EXPECT_THAT(*res.Get<float>(), Eq(4.f));

  res = env.Exec("(/ 10.0 2.0)");
  EXPECT_THAT(res.Is<float>(), Eq(true));
  EXPECT_THAT(*res.Get<float>(), Eq(5.f));
}

TEST(ScriptFunctionsTest, IntComparison) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(== 1 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(== 1 2)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(!= 1 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(!= 1 2)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(< 1 2)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(< 1 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(< 2 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(<= 1 2)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(<= 1 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(<= 2 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(> 1 2)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(> 1 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(> 2 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(>= 1 2)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(>= 1 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(>= 2 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));
}

TEST(ScriptFunctionsTest, FloatComparison) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(== 1.0 1.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(== 1.0 2.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(!= 1.0 1.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(!= 1.0 2.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(< 1.0 2.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(< 1.0 1.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(< 2.0 1.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(<= 1.0 2.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(<= 1.0 1.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(<= 2.0 1.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(> 1.0 2.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(> 1.0 1.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(> 2.0 1.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(>= 1.0 2.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(>= 1.0 1.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(>= 2.0 1.0)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));
}

TEST(ScriptFunctionsTest, Cond) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(if true 1 2)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(if false 1 2)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));
}

/*
TEST(ScriptFunctionsTest, Print) {
  ScriptEnv env;
  ScriptValue res;

  std::string str;
  env.SetPrintFunction([&str](const std::string& txt) {
    str = txt;
  });

  res = env.Exec("(? 'hello world')");
  EXPECT_THAT(res.Is<std::string>(), Eq(true));
  EXPECT_THAT(*res.Get<std::string>(), Eq("hello world"));
  EXPECT_THAT(str, Eq("hello world"));
}
*/

TEST(ScriptFunctionsTest, Do) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(do 1 2 3)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));
}

TEST(ScriptFunctionsTest, Mathfu) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(vec2 1.0 2.0)");
  EXPECT_THAT(res.Is<mathfu::vec2>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::vec2>()->x, Eq(1.f));
  EXPECT_THAT(res.Get<mathfu::vec2>()->y, Eq(2.f));

  res = env.Exec("(vec3 3.0 4.0 5.0)");
  EXPECT_THAT(res.Is<mathfu::vec3>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::vec3>()->x, Eq(3.f));
  EXPECT_THAT(res.Get<mathfu::vec3>()->y, Eq(4.f));
  EXPECT_THAT(res.Get<mathfu::vec3>()->z, Eq(5.f));

  res = env.Exec("(vec4 6.0 7.0 8.0 9.0)");
  EXPECT_THAT(res.Is<mathfu::vec4>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::vec4>()->x, Eq(6.f));
  EXPECT_THAT(res.Get<mathfu::vec4>()->y, Eq(7.f));
  EXPECT_THAT(res.Get<mathfu::vec4>()->z, Eq(8.f));
  EXPECT_THAT(res.Get<mathfu::vec4>()->w, Eq(9.f));

  res = env.Exec("(quat 0.1 0.2 0.3 0.4)");
  EXPECT_THAT(res.Is<mathfu::quat>(), Eq(true));
  EXPECT_THAT(res.Get<mathfu::quat>()->scalar(), Eq(0.1f));
  EXPECT_THAT(res.Get<mathfu::quat>()->vector().x, Eq(0.2f));
  EXPECT_THAT(res.Get<mathfu::quat>()->vector().y, Eq(0.3f));
  EXPECT_THAT(res.Get<mathfu::quat>()->vector().z, Eq(0.4f));
}

/*
TEST(ScriptFunctionsTest, Map) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(make-map (1 'a') (2 123) (4 'd'))");
  EXPECT_THAT(res.Is<VariantMap>(), Eq(true));
  EXPECT_THAT(res.Get<VariantMap>()->size(), Eq(3));
  EXPECT_THAT(res.Get<VariantMap>()->count(1), Eq(1));
  EXPECT_THAT(res.Get<VariantMap>()->count(2), Eq(1));
  EXPECT_THAT(res.Get<VariantMap>()->count(3), Eq(0));
  EXPECT_THAT(res.Get<VariantMap>()->count(4), Eq(1));

  auto iter = res.Get<VariantMap>()->find(1);
  EXPECT_THAT(iter->second.GetTypeId(), Eq(GetTypeId<std::string>()));
  EXPECT_THAT(*iter->second.Get<std::string>(), Eq("a"));

  iter = res.Get<VariantMap>()->find(2);
  EXPECT_THAT(iter->second.GetTypeId(), Eq(GetTypeId<int>()));
  EXPECT_THAT(*iter->second.Get<int>(), Eq(123));

  iter = res.Get<VariantMap>()->find(4);
  EXPECT_THAT(iter->second.GetTypeId(), Eq(GetTypeId<std::string>()));
  EXPECT_THAT(*iter->second.Get<std::string>(), Eq("d"));
}
*/

}  // namespace
}  // namespace lull
