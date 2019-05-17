/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/util/expected.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/tests/portable_test_macros.h"

#include <string>

namespace lull {
namespace {

Expected<std::string> Good() { return std::string("Hooray!"); }

Expected<std::string> Bad() {
  return LULL_ERROR(kErrorCode_OutOfRange, "Uh-oh!");
}

TEST(ExpectedTest, Return) {
  auto result = Good();
  EXPECT_TRUE(result);
  EXPECT_THAT(*result, testing::Eq("Hooray!"));

  result = Bad();
  EXPECT_FALSE(result);
  EXPECT_THAT(result.GetError().GetErrorCode(),
              testing::Eq(kErrorCode_OutOfRange));
#ifndef NDEBUG
  EXPECT_THAT(result.GetError().GetErrorMessage(),
              testing::Eq(std::string("Uh-oh!")));
#else
  EXPECT_THAT(result.GetError().GetErrorMessage(),
              testing::Eq(std::string("")));
#endif
}

TEST(ExpectedTest, IsSet) {
  Expected<int> is_set(42);
  EXPECT_TRUE(is_set);
}

TEST(ExpectedTest, IsNotSet) {
  Expected<int> is_not_set(LULL_ERROR(kErrorCode_Unknown, "fail"));
  EXPECT_FALSE(is_not_set);
}

TEST(ExpectedTest, GoodValue) {
  Expected<int> good_value(42);
  EXPECT_EQ(good_value.get(), 42);
}

TEST(ExpectedTest, GoodValueDereference) {
  Expected<int> good_value(42);
  EXPECT_EQ(*good_value, 42);
}

TEST(ExpectedTest, GoodValueArrow) {
  struct BasicStruct {
    int member;
  };

  Expected<BasicStruct> good_value(BasicStruct{42});
  EXPECT_EQ(good_value->member, 42);
}

TEST(ExpectedDeathTest, BadValue) {
  Expected<int> bad_value(LULL_ERROR(kErrorCode_Unknown, "fail"));
  PORT_EXPECT_DEATH(bad_value.get(), "");
}

}  // namespace
}  // namespace lull
