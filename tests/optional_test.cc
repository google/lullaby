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

#include <memory>

#include "lullaby/util/optional.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

using ::testing::Eq;

struct OptionalTestClass {
  OptionalTestClass(int key, int value) : key(key), value(value) {
    ++ctor_count;
  }

  OptionalTestClass(const OptionalTestClass& rhs)
      : key(rhs.key), value(rhs.value) {
    ++ctor_count;
  }

  OptionalTestClass& operator=(const OptionalTestClass& rhs) {
    key = rhs.key;
    value = rhs.value;
    ++ctor_count;
    return *this;
  }

  ~OptionalTestClass() { ++dtor_count; }

  int key;
  int value;
  static int ctor_count;
  static int dtor_count;
};

int OptionalTestClass::ctor_count = 0;
int OptionalTestClass::dtor_count = 0;

class OptionalTest : public testing::Test {
 public:
  void SetUp() override {
    OptionalTestClass::ctor_count = 0;
    OptionalTestClass::dtor_count = 0;
  }
};

TEST_F(OptionalTest, EmplaceAndValueOr) {
  Optional<int> opt;
  EXPECT_FALSE(opt);

  opt.emplace(1);
  EXPECT_TRUE(opt);
  EXPECT_THAT(*opt, Eq(1));

  opt = 2;
  EXPECT_TRUE(opt);
  EXPECT_THAT(*opt, Eq(2));

  opt.emplace(3);
  EXPECT_THAT(opt.value_or(0), Eq(3));

  opt.reset();
  EXPECT_FALSE(opt);
  EXPECT_THAT(opt.value_or(4), Eq(4));

  const int x = 5;
  EXPECT_THAT(opt.value_or(x), Eq(5));
}

TEST_F(OptionalTest, CopyAssign) {
  Optional<int> opt1 = 5;
  EXPECT_THAT(*opt1, Eq(5));

  Optional<int> opt2 = opt1;
  EXPECT_THAT(*opt2, Eq(5));

  Optional<int> opt3 = std::move(opt1);
  EXPECT_FALSE(opt1);
  EXPECT_THAT(*opt3, Eq(5));

  opt2.emplace(6);
  EXPECT_THAT(*opt2, Eq(6));

  opt3 = opt2;
  EXPECT_THAT(*opt3, Eq(6));

  opt2.emplace(7);
  EXPECT_THAT(*opt2, Eq(7));

  opt1 = std::move(opt2);
  EXPECT_FALSE(opt2);
  EXPECT_THAT(*opt1, Eq(7));

  opt1 = 8;
  EXPECT_THAT(*opt1, Eq(8));
}

TEST_F(OptionalTest, TestClass) {
  Optional<OptionalTestClass> opt1;
  EXPECT_FALSE(opt1);

  opt1.emplace(1, 2);
  EXPECT_THAT(opt1->key, Eq(1));
  EXPECT_THAT(opt1->value, Eq(2));

  Optional<OptionalTestClass> opt2 = opt1;
  EXPECT_THAT(opt2->key, Eq(1));
  EXPECT_THAT(opt2->value, Eq(2));
}

TEST_F(OptionalTest, Move) {
  Optional<std::unique_ptr<int>> opt1;
  EXPECT_FALSE(opt1);

  opt1.emplace(new int(123));
  EXPECT_TRUE(opt1);
  EXPECT_THAT(**opt1, Eq(123));

  Optional<std::unique_ptr<int>> opt2 = std::move(opt1);
  EXPECT_FALSE(opt1);
  EXPECT_TRUE(opt2);
  EXPECT_THAT(**opt2, Eq(123));

  opt2.reset();
  EXPECT_FALSE(opt2);
}

TEST_F(OptionalTest, Raii) {
  Optional<OptionalTestClass> opt1;
  EXPECT_THAT(OptionalTestClass::ctor_count, Eq(0));
  EXPECT_THAT(OptionalTestClass::dtor_count, Eq(0));

  opt1.emplace(1, 2);
  EXPECT_THAT(OptionalTestClass::ctor_count, Eq(1));
  EXPECT_THAT(OptionalTestClass::dtor_count, Eq(0));

  Optional<OptionalTestClass> opt2 = opt1;
  EXPECT_THAT(OptionalTestClass::ctor_count, Eq(2));
  EXPECT_THAT(OptionalTestClass::dtor_count, Eq(0));

  opt1.reset();
  opt2.reset();
  EXPECT_THAT(OptionalTestClass::ctor_count, Eq(2));
  EXPECT_THAT(OptionalTestClass::dtor_count, Eq(2));
}

TEST_F(OptionalTest, Equality) {
  Optional<int> opt1 = 1;
  Optional<int> opt2 = 1;
  Optional<int> opt3 = 2;
  Optional<int> opt4;
  Optional<int> opt5;
  EXPECT_THAT(opt1 == opt2, Eq(true));
  EXPECT_THAT(opt1 == opt3, Eq(false));
  EXPECT_THAT(opt1 == opt4, Eq(false));
  EXPECT_THAT(opt4 == opt5, Eq(true));

  EXPECT_THAT(opt1 != opt2, Eq(false));
  EXPECT_THAT(opt1 != opt3, Eq(true));
  EXPECT_THAT(opt1 != opt4, Eq(true));
  EXPECT_THAT(opt4 != opt5, Eq(false));
}

TEST_F(OptionalTest, NullOpt) {
  Optional<int> opt;
  EXPECT_FALSE(opt);

  opt = 1;
  EXPECT_TRUE(opt);

  opt = NullOpt;
  EXPECT_FALSE(opt);
}

}  // namespace
}  // namespace lull

