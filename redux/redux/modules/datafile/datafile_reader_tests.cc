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
#include "redux/modules/datafile/datafile_reader.h"

namespace redux {
namespace {

using ::testing::Eq;

struct DatafileTestInnerClass {
  float value = 0.f;
  std::string str = "";

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(value, ConstHash("value"));
    archive(str, ConstHash("str"));
  }
};

struct DatafileTestClass {
  int value = 0;
  std::string str = "";
  DatafileTestInnerClass inner;
  std::vector<float> arr;
  std::vector<DatafileTestInnerClass> objs;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(value, ConstHash("value"));
    archive(str, ConstHash("str"));
    archive(inner, ConstHash("inner"));
    archive(arr, ConstHash("arr"));
    archive(objs, ConstHash("objs"));
  }
};

TEST(Datafile, ReadDatafile) {
  const char* txt =
      "{"
      "  value : 123,"
      "  str : 'hello',"
      "  inner : {"
      "    value : 456.789,"
      "    str : 'world',"
      "  },"
      "  arr : [1, 2, 3, (+ 2 2)],"
      "  objs : [{"
      "    value : 12,"
      "    str : 'ab'"
      "  }, {"
      "    value : 34"
      "  }, {"
      "    str : 'cd'"
      "  }]"
      "}";

  ScriptEnv env;
  DatafileTestClass test = ReadDatafile<DatafileTestClass>(txt, &env);
  EXPECT_THAT(test.value, Eq(123));
  EXPECT_THAT(test.str, Eq("hello"));
  EXPECT_THAT(test.inner.value, Eq(456.789f));
  EXPECT_THAT(test.inner.str, Eq("world"));
  EXPECT_THAT(test.arr.size(), Eq(4));
  EXPECT_THAT(test.arr[0], Eq(1));
  EXPECT_THAT(test.arr[1], Eq(2));
  EXPECT_THAT(test.arr[2], Eq(3));
  EXPECT_THAT(test.arr[3], Eq(4));
  EXPECT_THAT(test.objs.size(), Eq(3));
  EXPECT_THAT(test.objs[0].value, Eq(12.f));
  EXPECT_THAT(test.objs[0].str, Eq("ab"));
  EXPECT_THAT(test.objs[1].value, Eq(34.f));
  EXPECT_THAT(test.objs[1].str, Eq(""));
  EXPECT_THAT(test.objs[2].value, Eq(0.f));
  EXPECT_THAT(test.objs[2].str, Eq("cd"));
}

}  // namespace
}  // namespace redux
