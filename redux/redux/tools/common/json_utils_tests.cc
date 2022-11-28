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

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/tools/common/json_utils.h"

namespace redux::tool {
namespace {

using ::testing::Eq;

enum DayEnum {
  Monday,
  Tuesday,
  Wednesday,
  Thursday,
  Friday,
};

struct JsonTestInnerObject {
  float number = 0.f;
  std::string name = "";

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(number, ConstHash("number"));
    archive(name, ConstHash("name"));
  }
};

struct JsonTestObject {
  int number = 0;
  std::string name = "";
  JsonTestInnerObject inner;
  std::vector<std::string> strs;
  std::vector<JsonTestInnerObject> arr;
  DayEnum day = Monday;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(number, ConstHash("number"));
    archive(name, ConstHash("name"));
    archive(inner, ConstHash("inner"));
    archive(strs, ConstHash("strs"));
    archive(arr, ConstHash("arr"));
    archive(day, ConstHash("day"));
  }
};

TEST(JsonUtils, Read) {
  const char* json =
      "{"
      "  \"number\": 123,"
      "  \"name\": \"hello\","
      "  \"day\": \"Friday\","
      "  \"inner\": {"
      "    \"number\": 456,"
      "    \"name\": \"world\""
      "  },"
      "  \"strs\": [\"hi\", \"bye\"],"
      "  \"arr\": [{"
      "    \"number\": 12,"
      "    \"name\": \"ab\""
      "  }, {"
      "    \"number\": 34"
      "  }, {"
      "    \"name\": \"cd\""
      "  }],"
      "}";

  auto obj = ReadJson<JsonTestObject>(json);
  EXPECT_THAT(obj.number, Eq(123));
  EXPECT_THAT(obj.name, Eq("hello"));
  EXPECT_THAT(obj.inner.number, Eq(456.f));
  EXPECT_THAT(obj.inner.name, Eq("world"));
}

}  // namespace
}  // namespace redux::tool
