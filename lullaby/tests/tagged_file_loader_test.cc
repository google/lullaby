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

#include "lullaby/modules/file/tagged_file_loader.h"

#include "gtest/gtest.h"
#include "lullaby/modules/file/test/mock_tagged_file_loader.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

TEST(TaggedFileLoaderTest, NoTaggedFileLoader) {
  std::string transformed_filename;
  bool res = TaggedFileLoader::ApplySettingsToTaggedFilename(
      "file.txt", &transformed_filename);
  EXPECT_FALSE(res);
  EXPECT_EQ(transformed_filename, "file.txt");
}

TEST(TaggedFileLoaderTest, BasicTaggedFilename) {
  MockTaggedFileLoader loader;
  loader.RegisterTag("foo", "bar/");
  loader.RegisterTag("empty", "");
  TaggedFileLoader::SetTaggedFileLoader(&loader);

  std::string transformed_filename;
  bool res = TaggedFileLoader::ApplySettingsToTaggedFilename(
      "file.txt", &transformed_filename);
  EXPECT_FALSE(res);
  EXPECT_EQ(transformed_filename, "file.txt");

  res = TaggedFileLoader::ApplySettingsToTaggedFilename("foo:file.txt",
                                                        &transformed_filename);
  EXPECT_TRUE(res);
  EXPECT_EQ(transformed_filename, "bar/file.txt");

  res = TaggedFileLoader::ApplySettingsToTaggedFilename("empty:file.txt",
                                                        &transformed_filename);
  EXPECT_TRUE(res);
  EXPECT_EQ(transformed_filename, "file.txt");

  res = TaggedFileLoader::ApplySettingsToTaggedFilename("baz:file.txt",
                                                        &transformed_filename);
  EXPECT_FALSE(res);
  EXPECT_EQ(transformed_filename, "baz:file.txt");
}

TEST(TaggedFileLoaderTest, DefaultTag) {
  MockTaggedFileLoader loader;
  loader.RegisterTag("foo", "bar/");
  loader.SetDefaultTag("foo");
  TaggedFileLoader::SetTaggedFileLoader(&loader);

  std::string transformed_filename;
  bool res = TaggedFileLoader::ApplySettingsToTaggedFilename(
      "file.txt", &transformed_filename);
  EXPECT_TRUE(res);
  EXPECT_EQ(transformed_filename, "bar/file.txt");

  res = TaggedFileLoader::ApplySettingsToTaggedFilename("foo:other.txt",
                                                        &transformed_filename);
  EXPECT_TRUE(res);
  EXPECT_EQ(transformed_filename, "bar/other.txt");

  res = TaggedFileLoader::ApplySettingsToTaggedFilename("baz:file.txt",
                                                        &transformed_filename);
  EXPECT_FALSE(res);
  EXPECT_EQ(transformed_filename, "baz:file.txt");

  // Absolute filepaths shouldn't ever contain tags, but could be given to a
  // tagged loader with a default tag.
  res = TaggedFileLoader::ApplySettingsToTaggedFilename("/path/to/file.txt",
                                                        &transformed_filename);
  EXPECT_FALSE(res);
  EXPECT_EQ(transformed_filename, "/path/to/file.txt");
}

TEST(TaggedFileLoaderTest, TagUsed) {
  MockTaggedFileLoader loader;
  loader.RegisterTag("hello", "world/");
  loader.RegisterTag("foo", "bar/");
  loader.SetDefaultTag("foo");
  TaggedFileLoader::SetTaggedFileLoader(&loader);

  // Store the last tag used.
  std::string last_tag_used;
  int count = 0;
  loader.SetFallbackLoadFn(
      [](const char* filename, std::string* dest) { return false; });
  loader.SetMockLoadFn(
      [&](const char* filename, std::string* dest, const std::string& tag) {
        last_tag_used = tag;
        ++count;
        return true;
      });

  bool res = TaggedFileLoader::LoadTaggedFile("file.txt", nullptr);
  EXPECT_TRUE(res);
  EXPECT_EQ(last_tag_used, "foo");
  EXPECT_EQ(count, 1);

  res = TaggedFileLoader::LoadTaggedFile("foo:other.txt", nullptr);
  EXPECT_TRUE(res);
  EXPECT_EQ(last_tag_used, "foo");
  EXPECT_EQ(count, 2);

  res = TaggedFileLoader::LoadTaggedFile("hello:planet.txt", nullptr);
  EXPECT_TRUE(res);
  EXPECT_EQ(last_tag_used, "hello");
  EXPECT_EQ(count, 3);

  res = TaggedFileLoader::LoadTaggedFile("baz:file.txt", nullptr);
  EXPECT_FALSE(res);
  EXPECT_EQ(last_tag_used, "hello");
  EXPECT_EQ(count, 3);
}

TEST(TaggedFileLoaderTest, ReplacementFile) {
  MockTaggedFileLoader loader;
  loader.RegisterTag("foo", "bar/");
  loader.AddReplacementFile("other.txt", "foo:file.txt");
  TaggedFileLoader::SetTaggedFileLoader(&loader);

  std::string transformed_filename;
  bool res = TaggedFileLoader::ApplySettingsToTaggedFilename(
      "other.txt", &transformed_filename);
  EXPECT_TRUE(res);
  EXPECT_EQ(transformed_filename, "bar/file.txt");
}

TEST(TaggedFileLoaderTest, FallbackLoadFn) {
  MockTaggedFileLoader loader;
  loader.RegisterTag("foo", "bar/");
  TaggedFileLoader::SetTaggedFileLoader(&loader);

  // Store the last filename loaded by the fallback function.
  std::string last_fallback_filename;
  loader.SetFallbackLoadFn(
    [&last_fallback_filename](const char* filename, std::string* dest) {
      last_fallback_filename = std::string(filename);
      return true;
    });
  loader.SetMockLoadFn(
    [](const char* filename, std::string* dest, const std::string& tag) {
      return true;
    });

  bool res = TaggedFileLoader::LoadTaggedFile("file.txt", nullptr);
  EXPECT_TRUE(res);
  EXPECT_EQ(last_fallback_filename, "file.txt");

  res = TaggedFileLoader::LoadTaggedFile("foo:other_file.txt", nullptr);
  EXPECT_TRUE(res);
  EXPECT_EQ(last_fallback_filename, "file.txt");
}

TEST(TaggedFileLoaderTest, AltPathForSuffix) {
  MockTaggedFileLoader loader;
  loader.RegisterTag("foo", "bar/");
  loader.RegisterAltPathForSuffix(".doc", "alt/path/");
  TaggedFileLoader::SetTaggedFileLoader(&loader);

  // Store the last filename loaded by the fallback function.
  std::string last_fallback_filename;
  loader.SetFallbackLoadFn(
    [&last_fallback_filename](const char* filename, std::string* dest) {
      last_fallback_filename = std::string(filename);
      return true;
    });
  loader.SetMockLoadFn(
    [](const char* filename, std::string* dest, const std::string& tag) {
      return false;
    });

  bool res = TaggedFileLoader::LoadTaggedFile("foo:file.txt", nullptr);
  EXPECT_FALSE(res);
  EXPECT_EQ(last_fallback_filename, "");

  res = TaggedFileLoader::LoadTaggedFile("foo:file.doc", nullptr);
  EXPECT_TRUE(res);
  EXPECT_EQ(last_fallback_filename, "alt/path/file.doc");
}

TEST(TaggedFileLoaderTest, FailToLoad) {
  MockTaggedFileLoader loader;
  loader.RegisterAltPathForSuffix(".txt", "good/");
  loader.RegisterAltPathForSuffix(".doc", "bad/");
  TaggedFileLoader::SetTaggedFileLoader(&loader);
  loader.SetFallbackLoadFn(
    [](const char* filename, std::string* dest) {
      return strcmp(filename, "good/file.txt") == 0;
    });
  loader.SetMockLoadFn(
    [](const char* filename, std::string* dest, const std::string& tag) {
      return false;
    });

  bool res = TaggedFileLoader::LoadTaggedFile("file.txt", nullptr);
  EXPECT_TRUE(res);
  res = TaggedFileLoader::LoadTaggedFile("file.doc", nullptr);
  EXPECT_FALSE(res);
}

TEST(TaggedFileLoaderDeathTest, NoGlobal) {
  PORT_EXPECT_DEBUG_DEATH(TaggedFileLoader::LoadTaggedFile("file.txt", nullptr),
                          "");
}

}  // namespace
}  // namespace lull
