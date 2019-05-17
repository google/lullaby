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

#include "lullaby/util/hash.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/string_view.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::Not;

TEST(Hash, NullString) { EXPECT_THAT(Hash(nullptr), Eq(HashValue(0))); }

TEST(Hash, EmptyString) {
  EXPECT_THAT(Hash(""), Eq(HashValue(0)));
  EXPECT_THAT(Unhash(0), Eq(""));
}

TEST(Hash, ZeroLength) { EXPECT_THAT(Hash("Apple", 0), Eq(HashValue(0))); }

TEST(Hash, LengthOverflow) {
  HashValue hash = Hash("Banana");
  EXPECT_THAT(hash, Eq(Hash("Banana", 10)));
  EXPECT_THAT(Unhash(hash), Eq("Banana"));
}

TEST(Hash, CorrectLength) {
  HashValue hash = Hash("Carrot");
  EXPECT_THAT(hash, Eq(Hash("Carrot", 6)));
  EXPECT_THAT(Unhash(hash), Eq("Carrot"));
}

TEST(Hash, ShortLength) {
  HashValue hash = Hash("Dragon fruit", 6);
  EXPECT_THAT(Hash("Dragon fruit"), Not(Eq(hash)));
  EXPECT_THAT(Unhash(hash), Eq("Dragon"));
}

TEST(Hash, Uniqueness) {
  EXPECT_THAT(Hash("a"), Not(Eq(Hash("b"))));
  EXPECT_THAT(Hash("ab"), Not(Eq(Hash("ba"))));
  EXPECT_THAT(Unhash(Hash("a")), Eq("a"));
  EXPECT_THAT(Unhash(Hash("b")), Eq("b"));
  EXPECT_THAT(Unhash(Hash("ab")), Eq("ab"));
  EXPECT_THAT(Unhash(Hash("ab")), Eq("ab"));
}

TEST(Hash, CaseInsensitive) {
  HashValue hash = HashCaseInsensitive("EgGPlaNt", 8);
  EXPECT_THAT(HashCaseInsensitive("eggplant____", 8), Eq(hash));
  EXPECT_THAT(Unhash(hash), Eq("eggplant"));
}

TEST(Hash, ConstHash) {
  HashValue const_hash = ConstHash("Fennel");
  EXPECT_THAT(Unhash(const_hash), Eq(""));
  HashValue hash = Hash("Fennel");
  EXPECT_THAT(const_hash, Eq(hash));
  EXPECT_THAT(Unhash(const_hash), Eq("Fennel"));
}

TEST(Hash, ConstHashEmpty) { EXPECT_THAT(ConstHash(""), Eq(HashValue(0))); }

TEST(Hash, StringView) {
  EXPECT_THAT(Hash(string_view("Hello")), Eq(Hash("Hello")));
}

TEST(Hash, Hasher) {
  EXPECT_THAT(Hasher()("Hello"), Eq(Hash("Hello")));
}

TEST(Hash, Basis) {
  EXPECT_THAT(Hash("prefixSuffix"), Eq(Hash(Hash("prefix"), "Suffix")));
  EXPECT_THAT(Hash("prefixOther"), Eq(Hash(Hash("prefix"), "Other")));
  EXPECT_THAT(Hash("Other"), Eq(Hash(Hash(""), "Other")));
}

}  // namespace
}  // namespace lull
