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

#include "gtest/gtest.h"

#include "lullaby/modules/file/file.h"

namespace lull {
namespace {
TEST(File, EndsWith) {
  EXPECT_TRUE(EndsWith("test.txt", ".txt"));
  EXPECT_TRUE(EndsWith("test.txt.test.txt", ".txt"));
  EXPECT_FALSE(EndsWith("text.txt.", ".txt"));
  EXPECT_FALSE(EndsWith("text.txt ", ".txt"));
}

TEST(File, GetBasenameFromFilename) {
  EXPECT_EQ(GetBasenameFromFilename("foo\\bar\\test.ttf"), "test.ttf");
  EXPECT_EQ(GetBasenameFromFilename("\\foo\\bar\\test.ttf"), "test.ttf");
  EXPECT_EQ(GetBasenameFromFilename("foo\\test"), "test");
  EXPECT_EQ(GetBasenameFromFilename("foo/bar/test.ttf"), "test.ttf");
  EXPECT_EQ(GetBasenameFromFilename("/foo/bar/test.ttf"), "test.ttf");
  EXPECT_EQ(GetBasenameFromFilename("foo/test"), "test");
  EXPECT_EQ(GetBasenameFromFilename("test.ttf"), "test.ttf");
  EXPECT_EQ(GetBasenameFromFilename("Not A Path"), "Not A Path");
}

TEST(File, GetExtensionFromFilename) {
  EXPECT_EQ(GetExtensionFromFilename("foo/bar/test.ttf"), ".ttf");
  EXPECT_EQ(GetExtensionFromFilename("/foo/bar/test.mpeg"), ".mpeg");
  EXPECT_EQ(GetExtensionFromFilename("test.fplmesh"), ".fplmesh");
  EXPECT_EQ(GetExtensionFromFilename("foo/bar/text.temp.0.txt"), ".txt");
  EXPECT_EQ(GetExtensionFromFilename("foo/test."), ".");
  EXPECT_EQ(GetExtensionFromFilename("foo/test"), "");
  EXPECT_EQ(GetExtensionFromFilename("Not A Path"), "");
}

TEST(File, RemoveExtensionFromFilename) {
  EXPECT_EQ(RemoveExtensionFromFilename("foo/bar/test.ttf"), "foo/bar/test");
  EXPECT_EQ(RemoveExtensionFromFilename("/foo/bar/test.mpeg"), "/foo/bar/test");
  EXPECT_EQ(RemoveExtensionFromFilename("test.fplmesh"), "test");
  EXPECT_EQ(RemoveExtensionFromFilename("foo/bar/text.temp.0.txt"),
            "foo/bar/text.temp.0");
  EXPECT_EQ(RemoveExtensionFromFilename("foo/test."), "foo/test");
  EXPECT_EQ(RemoveExtensionFromFilename("foo/test"), "foo/test");
}

TEST(File, GetDirectoryFromFilename) {
  EXPECT_EQ(GetDirectoryFromFilename("foo/bar/test.ttf"), "foo/bar");
  EXPECT_EQ(GetDirectoryFromFilename("/foo/bar/test.mpeg"), "/foo/bar");
  EXPECT_EQ(GetDirectoryFromFilename("test.fplmesh"), "");
  EXPECT_EQ(GetDirectoryFromFilename("foo/bar/text.temp.0.txt"), "foo/bar");
  EXPECT_EQ(GetDirectoryFromFilename("foo/test."), "foo");
  EXPECT_EQ(GetDirectoryFromFilename("foo/test"), "foo");
  EXPECT_EQ(GetDirectoryFromFilename("Not A Path"), "");
}

}  // namespace
}  // namespace lull
