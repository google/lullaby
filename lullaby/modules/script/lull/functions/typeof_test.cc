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

TEST(ScriptFunctionsTypeOfTest, IsNil) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(nil? 1)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(nil? 2.0f)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(nil? 'hello')");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  Variant nil;
  env.SetValue(Symbol("nil"), ScriptValue::Create(nil));
  res = env.Exec("(nil? nil)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));
}

TEST(ScriptFunctionsTypeOfTest, TypeOf) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(typeof 1)");
  EXPECT_THAT(res.Is<TypeId>(), Eq(true));
  EXPECT_THAT(*res.Get<TypeId>(), Eq(GetTypeId<int>()));

  res = env.Exec("(typeof 2.0f)");
  EXPECT_THAT(res.Is<TypeId>(), Eq(true));
  EXPECT_THAT(*res.Get<TypeId>(), Eq(GetTypeId<float>()));

  res = env.Exec("(typeof 'hello')");
  EXPECT_THAT(res.Is<TypeId>(), Eq(true));
  EXPECT_THAT(*res.Get<TypeId>(), Eq(GetTypeId<std::string>()));
}

TEST(ScriptFunctionsTypeOfTest, Is) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(is 1.0f int)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));

  res = env.Exec("(is 1.0f float)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(is 'hello' std::string)");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));
}

}  // namespace
}  // namespace lull
