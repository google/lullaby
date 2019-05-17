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

#include "lullaby/util/make_unique.h"

#include <string>

#include "gtest/gtest.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace {

int destructor_counter = 0;

class TestObject {
 public:
  TestObject() {}
  explicit TestObject(int value) : value_(value) {}
  TestObject(int value, const std::string& str) : value_(value), str_(str) {}

  ~TestObject() { ++destructor_counter; }

  int value() const { return value_; }
  const std::string& string() const { return str_; }

 private:
  int value_ = 0;
  std::string str_;
};

}  // namespace

TEST(MakeUnique, Scalar) {
  destructor_counter = 0;

  {
    std::unique_ptr<TestObject> test_default = MakeUnique<TestObject>();
    CHECK(test_default != nullptr);
    EXPECT_EQ(test_default->value(), 0);
    EXPECT_TRUE(test_default->string().empty());
  }
  EXPECT_EQ(destructor_counter, 1);

  std::unique_ptr<TestObject> test1 = MakeUnique<TestObject>(5);
  CHECK(test1 != nullptr);
  EXPECT_EQ(test1->value(), 5);
  EXPECT_TRUE(test1->string().empty());
  test1.reset();
  EXPECT_EQ(destructor_counter, 2);

  std::unique_ptr<TestObject> test2 = MakeUnique<TestObject>(8, "test string");
  CHECK(test2 != nullptr);
  EXPECT_EQ(test2->value(), 8);
  EXPECT_EQ(test2->string(), "test string");
}

TEST(MakeUnique, Array) {
  constexpr size_t kCount = 10;
  std::unique_ptr<TestObject[]> test = MakeUnique<TestObject[]>(kCount);
  CHECK(test != nullptr);

  for (size_t i = 0; i < kCount; ++i) {
    EXPECT_EQ(test[i].value(), 0);
  }
}

}  // namespace lull
