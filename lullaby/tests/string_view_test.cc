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

#include "lullaby/util/string_view.h"

#include <sstream>
#include <type_traits>
#include <unordered_map>

#include "gtest/gtest.h"
#include "lullaby/util/hash.h"

namespace lull {

TEST(StringViewTest, DefaultCtor) {
  string_view str;
  EXPECT_EQ(0u, str.size());
  EXPECT_EQ(0u, str.length());
  EXPECT_TRUE(str.empty());
  EXPECT_EQ(nullptr, str.data());
  EXPECT_EQ("", str);
}

TEST(StringViewTest, CstrCtor) {
  const char* cstr = "abc";
  string_view str(cstr);
  EXPECT_EQ(3u, str.size());
  EXPECT_EQ(3u, str.length());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ('b', str[1]);
  EXPECT_EQ(cstr, str.data());
  EXPECT_EQ("abc", str);
}

TEST(StringViewTest, CstrLenCtor) {
  const char* cstr = "Hello World";
  string_view str(cstr, 5);
  EXPECT_EQ(5u, str.size());
  EXPECT_EQ(5u, str.length());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ('H', str[0]);
  EXPECT_EQ(cstr, str.data());
  EXPECT_EQ("Hello", str);
}

TEST(StringViewTest, StringCtor) {
  std::string string = "Blah";
  string_view str(string);
  EXPECT_EQ(4u, str.size());
  EXPECT_EQ(4u, str.length());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ('h', str[3]);
  EXPECT_EQ(string.data(), str.data());
  EXPECT_EQ("Blah", str);
}

TEST(StringViewTest, Iteration) {
  std::string out;
  for (char c : string_view("Thing")) {
    out += c;
  }
  EXPECT_EQ("Thing", out);
}

TEST(StringViewTest, ToString) {
  string_view str;
  EXPECT_EQ(std::string(""), std::string(str));
  str = "abc";
  EXPECT_EQ(std::string("abc"), std::string(str));
}

TEST(StringViewTest, SubStr) {
  string_view str;
  EXPECT_EQ("", str.substr(1, 3));
  str = "Moar COFFEE!!!";
  EXPECT_EQ("COFFEE", str.substr(5, 6));
  str = "eat cake";
  EXPECT_EQ("cake", str.substr(4));
}

TEST(StringViewTest, Compare) {
  string_view str = "def";
  EXPECT_EQ(-1, str.compare("ghi"));
  EXPECT_EQ(1, str.compare("abc"));
  EXPECT_EQ(-1, str.compare("defg"));
  EXPECT_EQ(1, str.compare("de"));
  EXPECT_EQ(0, str.compare("def"));
  EXPECT_TRUE(str == "def");
  EXPECT_FALSE(str == "abc");
  EXPECT_TRUE(str != "abc");
  EXPECT_FALSE(str != "def");
  EXPECT_TRUE(str < "ghi");
  EXPECT_FALSE(str < "def");
  EXPECT_TRUE(str <= "def");
  EXPECT_FALSE(str <= "abc");
  EXPECT_TRUE(str > "abc");
  EXPECT_FALSE(str > "def");
  EXPECT_TRUE(str >= "def");
  EXPECT_FALSE(str >= "ghi");
}

TEST(StringViewTest, Ostream) {
  std::stringstream o;
  string_view str = "jumble";
  o << "bumble" << string_view() << " " << str;
  EXPECT_EQ("bumble jumble", o.str());
}

TEST(StringViewTest, Hash) {
  const char* test1 = "IAmAClass";
  string_view test2 = "IAmAClass";
  std::unordered_map<string_view, int, Hasher> map;
  map[test2] = 5;
  EXPECT_EQ(map[test2], 5);
  EXPECT_EQ(map[test1], 5);
}

TEST(StringViewTest, Add) {
  string_view view = "View";
  std::string str = "String";
  const char* chars = "Chars";
  auto view_str = view + str;
  auto str_view = str + view;
  auto view_chars = view + chars;
  auto chars_view = chars + view;
  bool view_str_same = std::is_same<decltype(view_str), std::string>::value;
  bool str_view_same = std::is_same<decltype(str_view), std::string>::value;
  bool view_chars_same = std::is_same<decltype(view_chars), std::string>::value;
  bool chars_view_same = std::is_same<decltype(chars_view), std::string>::value;
  EXPECT_TRUE(view_str_same);
  EXPECT_TRUE(str_view_same);
  EXPECT_TRUE(view_chars_same);
  EXPECT_TRUE(chars_view_same);
  EXPECT_EQ(view_str, std::string("ViewString"));
  EXPECT_EQ(str_view, std::string("StringView"));
  EXPECT_EQ(view_chars, std::string("ViewChars"));
  EXPECT_EQ(chars_view, std::string("CharsView"));
}

}  // namespace lull
