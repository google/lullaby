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

TEST(ScriptMapTest, Map) {
  ScriptEnv env;
  ScriptValue res;
  VarTable* map = nullptr;

  res = env.Exec("(make-map (:a 'a') (:b 123) (:d 'd'))");
  map = res.Get<VarTable>();
  EXPECT_THAT(map, Not(IsNull()));
  EXPECT_THAT(map->Count(), Eq(3));
  EXPECT_TRUE(map->Contains(ConstHash("a")));
  EXPECT_TRUE(map->Contains(ConstHash("b")));
  EXPECT_FALSE(map->Contains(ConstHash("c")));
  EXPECT_TRUE(map->Contains(ConstHash("d")));

  res = env.Exec("{(:a 'a') (:b 123) (:d 'd')}");
  map = res.Get<VarTable>();
  EXPECT_THAT(map, Not(IsNull()));
  EXPECT_THAT(map->Count(), Eq(3));
  EXPECT_TRUE(map->Contains(ConstHash("a")));
  EXPECT_TRUE(map->Contains(ConstHash("b")));
  EXPECT_FALSE(map->Contains(ConstHash("c")));
  EXPECT_TRUE(map->Contains(ConstHash("d")));
}

TEST(ScriptMapTest, MapSize) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-size {})", 0);
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-size {(:a 'a')})", 1);
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-size {(:a 'a') (:b 123) (:d 'd')})", 3);
}

TEST(ScriptMapTest, MapEmpty) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-empty {})", true);
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-empty {(:a 'a')})", false);
}

TEST(ScriptMapTest, MapGet) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-get {(:a 'a') (:b 123)} :a)",
                            std::string("a"));
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-get {(:a 'a') (:b 123)} :b)", 123);
  REDUX_CHECK_SCRIPT_RESULT_NIL(env, "(map-get {(:a 'a') (:b 123)} :c)");
}

TEST(ScriptMapTest, MapGetOr) {
  ScriptEnv env;
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-get-or {(:a 'a') (:b 123)} :a 1)",
                            std::string("a"));
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-get-or {(:a 'a') (:b 123)} :b 1)", 123);
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-get-or {(:a 'a') (:b 123)} :c 1)", 1);
}

TEST(ScriptMapTest, MapInsert) {
  ScriptEnv env;

  env.Exec("(= m {(:a 'a') (:b 123) (:d 'd')})");
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-size m)", 3);
  // EXPECT_FALSE(map->Contains(ConstHash("c")));
  REDUX_CHECK_SCRIPT_RESULT_NIL(env, "(map-get m :c)");

  env.Exec("(map-insert m :c 456.789f)");
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-size m)", 4);
  // EXPECT_TRUE(map->Contains(ConstHash("c")));
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-get m :c)", 456.789f);
}

TEST(ScriptMapTest, MapErase) {
  ScriptEnv env;
  Var res;

  env.Exec("(= m {(:a 'a') (:b 123) (:d 'd')})");
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-size m)", 3);
  // EXPECT_TRUE(map->Contains(ConstHash("b")));
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-get m :b)", 123);

  env.Exec("(map-erase m :b)");
  REDUX_CHECK_SCRIPT_RESULT(env, "(map-size m)", 2);
  // EXPECT_TRUE(map->Contains(ConstHash("b")));
  REDUX_CHECK_SCRIPT_RESULT_NIL(env, "(map-get m :b)");
}

TEST(ScriptMapTest, MapForEach) {
  ScriptEnv env;
  Var res;

  env.Exec("(= map {(:a 1) (:b 3) (:d 5)})");
  res = env.Exec(
      "(do"
      "  (= vsum 0)"
      "  (map-foreach map (key val) (= vsum (+ vsum val)))"
      ")");
  REDUX_CHECK_SCRIPT_RESULT(env, "(do vsum)", 9);
}

}  // namespace
}  // namespace redux
