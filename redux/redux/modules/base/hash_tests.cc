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

#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/base/hash.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Not;

TEST(HashTest, NullString) { EXPECT_THAT(Hash(nullptr), Eq(HashValue(0))); }

TEST(HashTest, EmptyString) { EXPECT_THAT(Hash(""), Eq(HashValue(0))); }

TEST(HashTest, ZeroLength) { EXPECT_THAT(Hash("hello", 0), Eq(HashValue(0))); }

TEST(HashTest, LengthOverflow) {
  EXPECT_THAT(Hash("hello"), Eq(Hash("hello", 10)));
}

TEST(HashTest, CorrectLength) {
  EXPECT_THAT(Hash("hello"), Eq(Hash("hello", 5)));
}

TEST(HashTest, ShortLength) {
  EXPECT_THAT(Hash("hello"), Not(Eq(Hash("hello", 4))));
}

TEST(HashTest, Uniqueness) {
  EXPECT_THAT(Hash("a"), Not(Eq(Hash("b"))));
  EXPECT_THAT(Hash("ab"), Not(Eq(Hash("ba"))));
}

TEST(HashTest, ConstHash) {
  constexpr auto const_hash = ConstHash("hello");
  EXPECT_THAT(const_hash, Eq(Hash("hello")));
}

TEST(HashTest, ConstHashEmpty) {
  constexpr auto const_hash = ConstHash("");
  EXPECT_THAT(const_hash, Eq(HashValue(0)));
}

TEST(HashTest, PrefixAndSuffix) {
  EXPECT_THAT(Hash(Hash("prefix"), "suffix"), Eq(Hash("prefixsuffix")));
}

TEST(HashTest, PrefixNoSuffix) {
  EXPECT_THAT(Hash(Hash("prefix"), ""), Eq(Hash("prefix")));
}

TEST(HashTest, SuffixNoPrefix) {
  EXPECT_THAT(Hash(Hash(""), "suffix"), Eq(Hash("suffix")));
}

TEST(HashTest, StringView) {
  const std::string_view str = "hello";
  EXPECT_THAT(Hash(str), Eq(Hash("hello")));
}

TEST(HashTest, PrefixAndStringViewSuffix) {
  const std::string_view suffix = "suffix";
  EXPECT_THAT(Hash(Hash("prefix"), suffix), Eq(Hash("prefixsuffix")));
}

TEST(HashTest, PrefixNoStringViewSuffix) {
  const std::string_view suffix = "";
  EXPECT_THAT(Hash(Hash("prefix"), suffix), Eq(Hash("prefix")));
}

TEST(HashTest, StringViewSuffixNoPrefix) {
  const std::string_view suffix = "suffix";
  EXPECT_THAT(Hash(Hash(""), suffix), Eq(Hash("suffix")));
}

TEST(HashTest, Combine) {
  auto h1 = Hash("hello");
  auto h2 = Hash("world");
  EXPECT_THAT(Combine(h1, h2), Not(Eq(Combine(h2, h1))));
}

}  // namespace
}  // namespace redux
