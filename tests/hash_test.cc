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

#include "lullaby/util/hash.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/string_view.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::Not;

TEST(Hash, NullString) { EXPECT_THAT(Hash(nullptr), Eq(HashValue(0))); }

TEST(Hash, EmptyString) { EXPECT_THAT(Hash(""), Eq(HashValue(0))); }

TEST(Hash, ZeroLength) { EXPECT_THAT(Hash("hello", 0), Eq(HashValue(0))); }

TEST(Hash, LengthOverflow) {
  EXPECT_THAT(Hash("hello"), Eq(Hash("hello", 10)));
}

TEST(Hash, CorrectLength) { EXPECT_THAT(Hash("hello"), Eq(Hash("hello", 5))); }

TEST(Hash, ShortLength) {
  EXPECT_THAT(Hash("hello"), Not(Eq(Hash("hello", 4))));
}

TEST(Hash, Uniqueness) {
  EXPECT_THAT(Hash("a"), Not(Eq(Hash("b"))));
  EXPECT_THAT(Hash("ab"), Not(Eq(Hash("ba"))));
}

TEST(Hash, CaseInsensitive) {
  EXPECT_THAT(HashCaseInsensitive("hello_world", 5),
              Eq(HashCaseInsensitive("HELLO_World", 5)));
}

TEST(Hash, ConstHash) { EXPECT_THAT(ConstHash("Hello"), Eq(Hash("Hello"))); }

TEST(Hash, ConstHashEmpty) { EXPECT_THAT(ConstHash(""), Eq(HashValue(0))); }

TEST(Hash, StringView) {
  EXPECT_THAT(Hash(string_view("Hello")), Eq(Hash("Hello")));
}

TEST(Hash, Hasher) {
  EXPECT_THAT(Hasher<string_view>()("Hello"), Eq(Hash("Hello")));
}

TEST(Hash, Basis) {
  EXPECT_THAT(Hash("prefixSuffix"), Eq(Hash(Hash("prefix"), "Suffix")));
  EXPECT_THAT(Hash("prefixOther"), Eq(Hash(Hash("prefix"), "Other")));
  EXPECT_THAT(Hash("Other"), Eq(Hash(Hash(""), "Other")));
}

}  // namespace
}  // namespace lull
