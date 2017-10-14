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

  res = env.Exec("(make-map (1u 'a') (2u 123) (4u 'd'))");
  EXPECT_THAT(res.Is<VariantMap>(), Eq(true));
  EXPECT_THAT(res.Get<VariantMap>()->size(), Eq(size_t(3)));
  EXPECT_THAT(res.Get<VariantMap>()->count(1), Eq(size_t(1)));
  EXPECT_THAT(res.Get<VariantMap>()->count(2), Eq(size_t(1)));
  EXPECT_THAT(res.Get<VariantMap>()->count(3), Eq(size_t(0)));
  EXPECT_THAT(res.Get<VariantMap>()->count(4), Eq(size_t(1)));

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

  res = env.Exec("(map-size (make-map))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(0));

  res = env.Exec("(map-size {})");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(0));

  res = env.Exec("(map-size (make-map (1u 'a')))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(1));

  res = env.Exec("(map-size (make-map (1u 'a') (2u 123) (4u 'd')))");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));
}

TEST(ScriptFunctionsMapTest, MapEmpty) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(map-empty (make-map))");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(true));

  res = env.Exec("(map-empty (make-map (1u 'a') (2u 123) (4u 'd')))");
  EXPECT_THAT(res.Is<bool>(), Eq(true));
  EXPECT_THAT(*res.Get<bool>(), Eq(false));
}

TEST(ScriptFunctionsMapTest, MapGet) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(map-get (make-map (1u 'a') (2u 123) (4u 'd')) 2u)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(123));

  res = env.Exec("(map-get-or (make-map (1u 'a')) 2u 124)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(124));
}

TEST(ScriptFunctionsMapTest, MapInsert) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= m (make-map (1u 'a') (2u 123) (4u 'd')))");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));

  res = env.Exec("(map-insert m 3u 456)");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(4));

  res = env.Exec("(map-get m 3u)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(456));
}

TEST(ScriptFunctionsMapTest, MapSet) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= m (make-map (1u 'a') (2u 123) (4u 'd')))");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));

  // map-insert doesn't modify existing elements.
  res = env.Exec("(map-insert m 2u 456)");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));

  res = env.Exec("(map-get m 2u)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(123));

  // map-set does modify existing elements.
  res = env.Exec("(map-set m 2u 456)");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));

  res = env.Exec("(map-get m 2u)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(456));
}

TEST(ScriptFunctionsMapTest, MapErase) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec("(= m (make-map (1u 'a') (2u 123) (4u 'd')))");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(3));

  res = env.Exec("(map-erase m 2u)");
  res = env.Exec("(map-size m)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(2));

  res = env.Exec("(map-get m 2u)");
  EXPECT_THAT(res.IsNil(), Eq(true));
}


TEST(ScriptFunctionsMapTest, MapForeach) {
  ScriptEnv env;
  ScriptValue res;

  res = env.Exec(
      "(do (= sum 0)"
      "  (map-foreach (make-map (1u 2) (3u 4)) (k v) (= sum (+ sum v))) sum)");
  EXPECT_THAT(res.Is<int>(), Eq(true));
  EXPECT_THAT(*res.Get<int>(), Eq(6));

  res = env.Exec(
      "(do (= sum 0u)"
      "  (map-foreach (make-map (1u 2) (3u 4)) (k v) (= sum (+ sum k))) sum)");
  EXPECT_THAT(res.Is<unsigned>(), Eq(true));
  EXPECT_THAT(*res.Get<unsigned>(), Eq(4));
}

}  // namespace
}  // namespace lull
