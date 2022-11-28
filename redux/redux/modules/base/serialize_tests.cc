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

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/types/any.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/base/serialize.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Not;

struct TestSerializer {
  void Begin(HashValue) {}
  void End() {}

  template <typename T>
  void operator()(T& value, HashValue key) {
    keys.emplace_back(key);
    values.emplace_back(value);
  }

  std::vector<HashValue> keys;
  std::vector<absl::any> values;
};

struct TestObject {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(int_value, ConstHash("int_value"));
    archive(float_value, ConstHash("float_value"));
  }

  int int_value = 0;
  float float_value = 0.0f;
};

TEST(SerializeTest, Test) {
  TestObject obj;
  obj.int_value = 1;
  obj.float_value = 2.0f;

  TestSerializer serializer;
  Serialize(serializer, obj);

  EXPECT_THAT(serializer.keys.size(), Eq(2));
  EXPECT_THAT(serializer.values.size(), Eq(2));
  EXPECT_THAT(serializer.keys[0], Eq(ConstHash("int_value")));
  EXPECT_THAT(serializer.keys[1], Eq(ConstHash("float_value")));
  EXPECT_THAT(absl::any_cast<int>(&serializer.values[0]), Not(Eq(nullptr)));
  EXPECT_THAT(absl::any_cast<float>(&serializer.values[1]), Not(Eq(nullptr)));
}

}  // namespace
}  // namespace redux
