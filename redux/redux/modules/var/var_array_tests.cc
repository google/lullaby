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
#include "redux/modules/var/var_array.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Ge;

TEST(VarArray, Resize) {
  VarArray v;
  v.Resize(3);
  EXPECT_THAT(v.Count(), Eq(3));
}

TEST(VarArray, Reserve) {
  VarArray v;
  v.Reserve(3);
  EXPECT_THAT(v.Capacity(), Ge(3));
}

TEST(VarArray, Clear) {
  VarArray v;
  v.Resize(3);
  EXPECT_THAT(v.Count(), Eq(3));
  v.Clear();
  EXPECT_THAT(v.Count(), Eq(0));
}

TEST(VarArray, Swap) {
  VarArray v1;
  VarArray v2;
  v1.Resize(3);
  EXPECT_THAT(v1.Count(), Eq(3));
  EXPECT_THAT(v2.Count(), Eq(0));

  v1.Swap(v2);
  EXPECT_THAT(v1.Count(), Eq(0));
  EXPECT_THAT(v2.Count(), Eq(3));
}

TEST(VarArray, PushBack) {
  VarArray v;
  v.PushBack(1);
  v.PushBack(2.0f);
  EXPECT_THAT(v.Count(), Eq(2));
  EXPECT_TRUE(v[0].Is<int>());
  EXPECT_TRUE(v[1].Is<float>());
}

TEST(VarArray, PopBack) {
  VarArray v;
  v.PushBack(1);
  v.PushBack(2.0f);
  EXPECT_THAT(v.Count(), Eq(2));
  EXPECT_TRUE(v[0].Is<int>());
  EXPECT_TRUE(v[1].Is<float>());

  v.PopBack();
  EXPECT_THAT(v.Count(), Eq(1));
  EXPECT_TRUE(v[0].Is<int>());
}

TEST(VarArray, Insert) {
  VarArray v;
  v.PushBack(1);
  v.PushBack(2.0f);
  EXPECT_THAT(v.Count(), Eq(2));
  EXPECT_TRUE(v[0].Is<int>());
  EXPECT_TRUE(v[1].Is<float>());

  v.Insert(1, true);
  EXPECT_THAT(v.Count(), Eq(3));
  EXPECT_TRUE(v[0].Is<int>());
  EXPECT_TRUE(v[1].Is<bool>());
  EXPECT_TRUE(v[2].Is<float>());
}

TEST(VarArray, Erase) {
  VarArray v;
  v.PushBack(1);
  v.PushBack(true);
  v.PushBack(2.0f);
  EXPECT_THAT(v.Count(), Eq(3));
  EXPECT_TRUE(v[0].Is<int>());
  EXPECT_TRUE(v[1].Is<bool>());
  EXPECT_TRUE(v[2].Is<float>());

  v.Erase(1);
  EXPECT_THAT(v.Count(), Eq(2));
  EXPECT_TRUE(v[0].Is<int>());
  EXPECT_TRUE(v[1].Is<float>());
}

}  // namespace
}  // namespace redux
