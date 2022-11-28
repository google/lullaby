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
#include "redux/modules/testing/testing.h"
#include "redux/tools/common/file_utils.h"
#include "redux/tools/common/flatbuffer_utils.h"
#include "redux/tools/common/test_data/schema_generated.h"

namespace redux::tool {
namespace {

using ::testing::Eq;
using ::testing::NotNull;

TEST(FlatbufferUtils, JsonToFlatbuffer) {
  LoadFileFn fn = [](const char* filename) {
    const std::string path = ResolveTestFilePath(filename);
    return DefaultLoadFile(path.c_str());
  };
  SetLoadFileFunction(fn);

  const char* schema_file =
      "redux/tools/common/test_data/schema.fbs";
  const char* schema_name = "Person";
  const char* json =
      "{ "
      "  first_name: 'Jane', "
      "  last_name: 'Smith', "
      "  address: { "
      "    number: 123, "
      "    street: 'somewhere rd', "
      "    postal_code: '90210', "
      "  }, "
      "} ";

  auto fb = JsonToFlatbuffer(json, schema_file, schema_name);
  const Person* person = flatbuffers::GetRoot<Person>(fb.data());
  EXPECT_THAT(person, NotNull());
  EXPECT_THAT(person->first_name()->str(), Eq("Jane"));
  EXPECT_THAT(person->last_name()->str(), Eq("Smith"));

  const Address* address = person->address();
  EXPECT_THAT(address, NotNull());
  EXPECT_THAT(address->number(), Eq(123));
  EXPECT_THAT(address->street()->str(), Eq("somewhere rd"));
  EXPECT_THAT(address->postal_code()->str(), Eq("90210"));
}

}  // namespace
}  // namespace redux::tool
