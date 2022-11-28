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
#include "redux/modules/var/var_table.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Not;

constexpr HashValue key1 = ConstHash("key1");
constexpr HashValue key2 = ConstHash("key2");
constexpr HashValue key3 = ConstHash("key3");

TEST(VarTable, Clear) {
  VarTable tbl;
  tbl.Insert(key1, 1);
  EXPECT_THAT(tbl.Count(), Eq(1));

  tbl.Clear();
  EXPECT_THAT(tbl.Count(), Eq(0));
}

TEST(VarTable, Swap) {
  VarTable tbl1;
  VarTable tbl2;
  tbl1.Insert(key1, 1);
  tbl1.Insert(key2, 2);
  EXPECT_THAT(tbl1.Count(), Eq(2));
  EXPECT_THAT(tbl2.Count(), Eq(0));

  tbl1.Swap(tbl2);
  EXPECT_THAT(tbl1.Count(), Eq(0));
  EXPECT_THAT(tbl2.Count(), Eq(2));
}

TEST(VarTable, Insert) {
  VarTable tbl;
  tbl.Insert(key1, 1);
  tbl.Insert(key2, 2.0f);
  tbl.Insert(key3, true);
  EXPECT_THAT(tbl.Count(), Eq(3));
  EXPECT_TRUE(tbl.Contains(key1));
  EXPECT_TRUE(tbl.Contains(key2));
  EXPECT_TRUE(tbl.Contains(key3));
}

TEST(VarTable, Erase) {
  VarTable tbl;
  tbl.Insert(key1, 1);
  tbl.Insert(key2, 2.0f);
  tbl.Insert(key3, true);
  EXPECT_THAT(tbl.Count(), Eq(3));
  EXPECT_TRUE(tbl.Contains(key1));
  EXPECT_TRUE(tbl.Contains(key2));
  EXPECT_TRUE(tbl.Contains(key3));

  tbl.Erase(key2);
  EXPECT_THAT(tbl.Count(), Eq(2));
  EXPECT_TRUE(tbl.Contains(key1));
  EXPECT_FALSE(tbl.Contains(key2));
  EXPECT_TRUE(tbl.Contains(key3));
}

TEST(VarTable, TryFind) {
  VarTable tbl;
  tbl.Insert(key1, 1);
  tbl.Insert(key3, true);

  Var* v1 = tbl.TryFind(key1);
  Var* v2 = tbl.TryFind(key2);
  EXPECT_THAT(v1, Not(Eq(nullptr)));
  EXPECT_THAT(v2, Eq(nullptr));
  EXPECT_TRUE(v1->Is<int>());
}

TEST(VarTable, At) {
  VarTable tbl;
  tbl[key1] = 1;
  tbl[key2] = 2.0f;
  tbl[key3] = true;
  EXPECT_TRUE(tbl[key1].Is<int>());
  EXPECT_TRUE(tbl[key2].Is<float>());
  EXPECT_TRUE(tbl[key3].Is<bool>());
}

TEST(VarTable, ValueOr) {
  VarTable tbl;
  tbl.Insert(key1, 1);
  tbl.Insert(key3, true);

  EXPECT_THAT(tbl.ValueOr(key1, 0), Eq(1));
  EXPECT_THAT(tbl.ValueOr(key2, 0.0f), Eq(0.0f));
}

}  // namespace
}  // namespace redux
