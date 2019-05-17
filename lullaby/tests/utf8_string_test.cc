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

#include "lullaby/util/utf8_string.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

static const char foo[] =
    "\xC3\x8E"
    "\xC3\xB1"
    "\xC5\xA3"
    "\xC3\xA9"
    "\x72"
    "\xC3\xB1"
    "\xC3\xA5"
    "\xE1\x82\xA0"
    "\xF0\xAF\xA5\x80";

static const char bar[] =
    "\xC3\x8E"
    "\xC3\xA9"
    "\x72"
    "\xC3\xB1"
    "\xC3\xA5"
    "\xE1\x82\xA0";

TEST(UTF8StringTest, Simple) {
  UTF8String utf8_string;

  EXPECT_TRUE(utf8_string.empty());

  utf8_string.Append(foo);
  EXPECT_EQ(static_cast<size_t>(9), utf8_string.CharSize());
  EXPECT_FALSE(utf8_string.empty());

  utf8_string.DeleteLast();
  EXPECT_EQ(static_cast<size_t>(8), utf8_string.CharSize());

  utf8_string.DeleteChars(1, 2);
  EXPECT_EQ(static_cast<size_t>(6), utf8_string.CharSize());
  UTF8String utf8_bar(bar);
  EXPECT_EQ(utf8_bar, utf8_string);
  utf8_string.Insert(0, foo);
  UTF8String utf8_foo(foo);
  UTF8String utf8_foo_bar(foo);
  utf8_foo_bar.Append(bar);
  EXPECT_EQ(utf8_foo_bar, utf8_string);

  std::string utf8_char = utf8_foo.CharAt(0);
  EXPECT_EQ("\xC3\x8E", utf8_char);
  utf8_char = utf8_foo.CharAt(4);
  EXPECT_EQ("\x72", utf8_char);

  // Test the deletion is updating character offsets correctly by deleting
  // some characters and then trying to insert into the middle of the string.
  UTF8String cats("cats");
  cats.DeleteChars(0, 2);  // "ts"
  cats.Insert(1, "o");     // "tos"
  UTF8String tos("tos");
  EXPECT_EQ(tos, cats);
}

}  // namespace
}  // namespace lull
