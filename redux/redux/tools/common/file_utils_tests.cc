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

namespace redux::tool {
namespace {

using ::testing::Eq;

std::string TestFilePath(const char* name) {
  return ResolveTestFilePath(
      "redux/tools/common/test_data", name);
}

std::string TempFilePath(const char* name) {
  return ::testing::TempDir() + "/" + name;
}

TEST(FileUtils, FileExists) {
  const std::string f = TestFilePath("hello.txt");
  EXPECT_TRUE(FileExists(f.c_str()));
}

TEST(FileUtils, FileDoesntExist) {
  const std::string f = TestFilePath("invalid.txt");
  EXPECT_FALSE(FileExists(f.c_str()));
}

TEST(FileUtils, LoadFile) {
  const std::string f = TestFilePath("hello.txt");

  auto data = LoadFile(f.c_str());
  EXPECT_TRUE(data.ok());

  const std::string contents(reinterpret_cast<const char*>(data->GetBytes()),
                             data->GetNumBytes());
  EXPECT_THAT(contents, Eq("hello\n"));
}

TEST(FileUtils, LoadFileAsString) {
  const std::string f = TestFilePath("hello.txt");

  std::string contents = LoadFileAsString(f.c_str());
  EXPECT_THAT(contents, Eq("hello\n"));
}

TEST(FileUtils, SaveFile) {
  const std::string f = TempFilePath("new.txt");

  EXPECT_TRUE(SaveFile("hello\n", 6, f.c_str(), true));

  std::string contents = LoadFileAsString(f.c_str());
  EXPECT_THAT(contents, Eq("hello\n"));
}

TEST(FileUtils, LoadMissingFile) {
  const std::string f = TestFilePath("invalid.txt");

  auto data = LoadFile(f.c_str());
  EXPECT_FALSE(data.ok());
}

TEST(FileUtils, CopyFile) {
  const std::string src = TestFilePath("hello.txt");
  const std::string cpy = TempFilePath("copy.txt");
  EXPECT_TRUE(CopyFile(cpy.c_str(), src.c_str()));

  std::string contents = LoadFileAsString(cpy.c_str());
  EXPECT_THAT(contents, Eq("hello\n"));
}

TEST(FileUtils, CreateFolder) {
  const std::string src = TestFilePath("hello.txt");
  const std::string dir = TempFilePath("newdir");
  const std::string cpy = dir + "/copy.txt";

  EXPECT_FALSE(CopyFile(cpy.c_str(), src.c_str()));
  EXPECT_TRUE(CreateFolder(dir.c_str()));
  EXPECT_TRUE(CopyFile(cpy.c_str(), src.c_str()));

  std::string contents = LoadFileAsString(cpy.c_str());
  EXPECT_THAT(contents, Eq("hello\n"));
}

TEST(FileUtils, SetLoadFileFunction) {
  LoadFileFn fn = [](const char* filename) {
    return DataContainer::WrapData("goodbye\n", 8);
  };
  SetLoadFileFunction(fn);

  const std::string f = TestFilePath("hello.txt");

  std::string contents = LoadFileAsString(f.c_str());
  EXPECT_THAT(contents, Eq("goodbye\n"));
}

}  // namespace
}  // namespace redux::tool
