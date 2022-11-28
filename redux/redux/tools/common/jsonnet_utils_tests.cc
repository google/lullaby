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
#include "redux/tools/common/jsonnet_utils.h"

namespace redux::tool {
namespace {

using ::testing::Eq;

TEST(JsonnetUtils, JsonnetToJson) {
  LoadFileFn fn = [](const char* filename) {
    const std::string path = ResolveTestFilePath(filename);
    return DefaultLoadFile(path.c_str());
  };
  SetLoadFileFunction(fn);

  JsonnetVarMap vars;
  vars["arg"] = "1";

  const char* jsonnet =
      "local sum = import "
      "'redux/tools/common/test_data/sum.jsonnet';\n"
      "{ "
      "  value: sum.sum(std.parseInt(std.extVar('arg')), 2), "
      "} ";
  const std::string json = JsonnetToJson(jsonnet, "local", vars);
  EXPECT_THAT(json, Eq("{\n   \"value\": 3\n}\n"));
}

}  // namespace
}  // namespace redux::tool
