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

#include "redux/tools/scene_pipeline/index.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace redux::tool {
namespace {

using ::testing::Eq;

struct TestObject {};
using TestIndex = Index<TestObject>;

TEST(Index, DefaultIsInvalid) {
  TestIndex idx;
  EXPECT_FALSE(idx.valid());
}

TEST(Index, Valid) {
  TestIndex idx(123);
  EXPECT_TRUE(idx.valid());
  EXPECT_THAT(idx.value(), Eq(123));
}

TEST(Index, Assign) {
  TestIndex idx1(123);
  TestIndex idx2(456);
  idx1 = idx2;
  EXPECT_THAT(idx1.value(), Eq(456));
}

TEST(Index, ComparisonOperators) {
  TestIndex idx1(123);
  TestIndex idx2(456);
  TestIndex idx3(123);

  EXPECT_TRUE(idx1 != idx2);
  EXPECT_TRUE(idx1 == idx3);
  EXPECT_TRUE(idx1 < idx2);
  EXPECT_TRUE(idx2 > idx1);
  EXPECT_TRUE(idx1 <= idx2);
  EXPECT_TRUE(idx2 >= idx1);
  EXPECT_TRUE(idx1 <= idx3);
  EXPECT_TRUE(idx3 >= idx1);
}

}  // namespace
}  // namespace redux::tool
