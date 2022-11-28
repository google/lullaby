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
#include "redux/modules/base/filepath.h"

namespace redux {
namespace {

TEST(Filepath, EndsWith) {
  EXPECT_TRUE(EndsWith("test.ext", ".ext"));
  EXPECT_TRUE(EndsWith("test.ext.test.ext", ".ext"));
  EXPECT_FALSE(EndsWith("test.ext.", ".ext"));
  EXPECT_FALSE(EndsWith("test.ext ", ".ext"));
}

TEST(Filepath, GetBasepath) {
  EXPECT_EQ(GetBasepath("foo\\bar\\test.ext"), "test.ext");
  EXPECT_EQ(GetBasepath("\\foo\\bar\\test.ext"), "test.ext");
  EXPECT_EQ(GetBasepath("foo\\test"), "test");
  EXPECT_EQ(GetBasepath("foo/bar/test.ext"), "test.ext");
  EXPECT_EQ(GetBasepath("/foo/bar/test.ext"), "test.ext");
  EXPECT_EQ(GetBasepath("foo/test"), "test");
  EXPECT_EQ(GetBasepath("test.ext"), "test.ext");
  EXPECT_EQ(GetBasepath("Not A Path"), "Not A Path");
}

TEST(Filepath, GetExtension) {
  EXPECT_EQ(GetExtension("foo/bar/test.ext"), ".ext");
  EXPECT_EQ(GetExtension("/foo/bar/test.txt"), ".txt");
  EXPECT_EQ(GetExtension("test.fplmesh"), ".fplmesh");
  EXPECT_EQ(GetExtension("foo/bar/text.temp.0.ext"), ".ext");
  EXPECT_EQ(GetExtension("foo/test."), ".");
  EXPECT_EQ(GetExtension("foo/test"), "");
  EXPECT_EQ(GetExtension("Not A Path"), "");
}

TEST(Filepath, RemoveExtension) {
  EXPECT_EQ(RemoveExtension("foo/bar/test.ext"), "foo/bar/test");
  EXPECT_EQ(RemoveExtension("/foo/bar/test.txt"), "/foo/bar/test");
  EXPECT_EQ(RemoveExtension("test.fplmesh"), "test");
  EXPECT_EQ(RemoveExtension("foo/bar/text.temp.0.ext"), "foo/bar/text.temp.0");
  EXPECT_EQ(RemoveExtension("foo/test."), "foo/test");
  EXPECT_EQ(RemoveExtension("foo/test"), "foo/test");
}

TEST(Filepath, GetDirectory) {
  EXPECT_EQ(GetDirectory("foo/bar/test.ext"), "foo/bar");
  EXPECT_EQ(GetDirectory("/foo/bar/test.txt"), "/foo/bar");
  EXPECT_EQ(GetDirectory("test.fplmesh"), "");
  EXPECT_EQ(GetDirectory("foo/bar/text.temp.0.ext"), "foo/bar");
  EXPECT_EQ(GetDirectory("foo/test."), "foo");
  EXPECT_EQ(GetDirectory("foo/test"), "foo");
  EXPECT_EQ(GetDirectory("Not A Path"), "");
}

TEST(Filepath, JoinPath) {
  EXPECT_EQ(JoinPath("foo/bar", "test.ext"), "foo/bar/test.ext");
  EXPECT_EQ(JoinPath("foo/bar/", "test.ext"), "foo/bar/test.ext");
  EXPECT_EQ(JoinPath("foo/bar/", "/test.ext"), "foo/bar/test.ext");
  EXPECT_EQ(JoinPath("", "/test.ext"), "/test.ext");
  EXPECT_EQ(JoinPath("", "test.ext"), "test.ext");
  EXPECT_EQ(JoinPath(".", "test.ext"), "test.ext");
}

}  // namespace
}  // namespace redux
