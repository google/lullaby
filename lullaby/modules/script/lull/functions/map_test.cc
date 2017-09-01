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

TEST(ScriptFunctionsMapTest, Map) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(map (1 'a') (2 123) (4 'd'))");
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

TEST(ScriptFunctionsMapTest, MapSize) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(map-size (map))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(0));

  res = env.Exec("(map-size (map (1 'a')))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(map-size (map (1 'a') (2 123) (4 'd')))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));
}

TEST(ScriptFunctionsMapTest, MapEmpty) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(map-empty (map))");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(map-empty (map (1 'a') (2 123) (4 'd')))");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));
}

TEST(ScriptFunctionsMapTest, MapGet) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(map-get (map (1 'a') (2 123) (4 'd')) 2)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(123));
}

TEST(ScriptFunctionsMapTest, MapInsert) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= m (map (1 'a') (2 123) (4 'd')))");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));

  res = env.Exec("(map-insert m 3 456)");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(4));

  res = env.Exec("(map-get m 3)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(456));
}

TEST(ScriptFunctionsMapTest, MapErase) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= m (map (1 'a') (2 123) (4 'd')))");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));

  res = env.Exec("(map-erase m 2)");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));

  res = env.Exec("(map-get m 2)");
  EXPECT_THAT(res.IsNil(), Eq(true));
}

}  // namespace
}  // namespace lull
