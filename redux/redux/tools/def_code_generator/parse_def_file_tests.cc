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
#include "redux/tools/def_code_generator/parse_def_file.h"

namespace redux::tool {
namespace {

using ::testing::Eq;

TEST(ParseDefFileTest, EmptyFile) {
  const char* txt = "";
  absl::StatusOr<DefDocument> doc = ParseDefFile(txt);
  EXPECT_TRUE(doc.ok());
}

TEST(ParseDefFileTest, Includes) {
  const char* txt = "include a/b/c";
  absl::StatusOr<DefDocument> doc = ParseDefFile(txt);
  EXPECT_TRUE(doc.ok()) << doc.status();
  EXPECT_THAT(doc.value().includes.size(), Eq(1));
  EXPECT_THAT(doc.value().includes.front(), Eq("a/b/c"));
}

TEST(ParseDefFileTest, Enum) {
  const char* txt =
      "# e\n"
      "enum TestEnum { \n"
      "  # a\n"
      "  Alpha,        \n"
      "  # b\n"
      "  Beta          \n"
      "  # c\n"
      "  Gamma         \n"
      "}\n";
  absl::StatusOr<DefDocument> doc = ParseDefFile(txt);
  EXPECT_TRUE(doc.ok()) << doc.status();
  EXPECT_THAT(doc.value().enums.size(), Eq(1));

  const EnumMetadata& e = doc.value().enums.front();
  EXPECT_THAT(e.name, Eq("TestEnum"));
  EXPECT_THAT(e.description, Eq("e\n"));

  EXPECT_THAT(e.enumerators.size(), Eq(3));

  EXPECT_THAT(e.enumerators[0].name, Eq("Alpha"));
  EXPECT_THAT(e.enumerators[0].description, Eq("a\n"));

  EXPECT_THAT(e.enumerators[1].name, Eq("Beta"));
  EXPECT_THAT(e.enumerators[1].description, Eq("b\n"));

  EXPECT_THAT(e.enumerators[2].name, Eq("Gamma"));
  EXPECT_THAT(e.enumerators[2].description, Eq("c\n"));
}

TEST(ParseDefFileTest, Struct) {
  const char* txt =
      "# s\n"
      "struct TestStruct {        \n"
      "  # a\n"
      "  Alpha: int = 0           \n"
      "  # b\n"
      "  Beta: string = \"hello\" \n"
      "}\n";
  absl::StatusOr<DefDocument> doc = ParseDefFile(txt);
  EXPECT_TRUE(doc.ok()) << doc.status();
  EXPECT_THAT(doc.value().structs.size(), Eq(1));

  const StructMetadata& s = doc.value().structs.front();
  EXPECT_THAT(s.name, Eq("TestStruct"));
  EXPECT_THAT(s.description, Eq("s\n"));

  EXPECT_THAT(s.fields.size(), Eq(2));

  EXPECT_THAT(s.fields[0].name, Eq("Alpha"));
  EXPECT_THAT(s.fields[0].type, Eq("int"));

  EXPECT_THAT(s.fields[1].name, Eq("Beta"));
  EXPECT_THAT(s.fields[1].type, Eq("string"));

  EXPECT_THAT(s.fields[1].attributes.begin()->first,
              Eq(FieldMetadata::kDefaulValue));
  EXPECT_THAT(s.fields[1].attributes.begin()->second, Eq("\"hello\""));
}


}  // namespace
}  // namespace redux::tool
