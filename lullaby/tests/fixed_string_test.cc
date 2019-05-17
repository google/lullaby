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

#include "lullaby/util/fixed_string.h"

#include <sstream>
#include <unordered_map>

#include "gtest/gtest.h"

namespace lull {

TEST(FixedStringTest, DefaultCtor) {
  FixedString<3> str;
  EXPECT_EQ(0u, str.size());
  EXPECT_EQ(0u, str.length());
  EXPECT_TRUE(str.empty());
}

TEST(FixedStringTest, CstrCtor) {
  const char* cstr = "abc";
  FixedString<3> str(cstr);
  EXPECT_EQ(3u, str.size());
  EXPECT_EQ(3u, str.length());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ('a', str[0]);
  EXPECT_EQ('b', str[1]);
  EXPECT_EQ('c', str[2]);
  EXPECT_EQ('\0', str[3]);
}

TEST(FixedStringTest, CstrOverflowCtor) {
  const char* cstr = "Hello World";
  FixedString<5> str(cstr);
  EXPECT_EQ(5u, str.size());
  EXPECT_EQ(5u, str.length());
  EXPECT_FALSE(str.empty());
}

TEST(FixedStringTest, StringCtor) {
  std::string string = "abc";
  FixedString<3> str(string);
  EXPECT_EQ(3u, str.size());
  EXPECT_EQ(3u, str.length());
  EXPECT_FALSE(str.empty());
  EXPECT_EQ('a', str[0]);
  EXPECT_EQ('b', str[1]);
  EXPECT_EQ('c', str[2]);
  EXPECT_EQ('\0', str[3]);
}

TEST(FixedStringTest, Iteration) {
  std::string out;
  for (char c : FixedString<5>("Hello")) {
    out += c;
  }
  FixedString<5> fstr("World");
  std::string out2;
  for (auto it = fstr.begin(); it != fstr.end(); ++it) {
    out2 += *it;
  }
  EXPECT_EQ("Hello", out);
  EXPECT_EQ("World", out2);
}

TEST(FixedStringTest, ReverseIteration) {
  std::string out;
  FixedString<7> fstr("live on");
  for (auto rit = fstr.rbegin(); rit != fstr.rend(); ++rit) {
    out += *rit;
  }
  EXPECT_EQ("no evil", out);
}

TEST(FixedStringTest, Capacity) {
  std::string string = "abc";
  FixedString<3> str(string);
  EXPECT_EQ(3u, str.size());
  EXPECT_EQ(3u, str.length());
  EXPECT_EQ(3u, str.max_size());
  EXPECT_EQ(4u, str.capacity());
  EXPECT_FALSE(str.empty());
  str.clear();
  EXPECT_TRUE(str.empty());
}

TEST(FixedStringTest, ElelmentAccessBounding) {
  std::string string = "abc";
  FixedString<3> str(string);
  EXPECT_EQ('\0', str[3]);
  EXPECT_EQ('\0', str.at(4));
  EXPECT_EQ('a', str.front());
  EXPECT_EQ('c', str.back());
}

TEST(FixedStringTest, AppendCstr) {
  FixedString<5> fstr("abc");
  char str[2] = "d";
  fstr.append(str);
  EXPECT_EQ(4u, fstr.size());
  EXPECT_EQ(4u, fstr.length());
  EXPECT_FALSE(fstr.empty());
  EXPECT_EQ('a', fstr[0]);
  EXPECT_EQ('b', fstr[1]);
  EXPECT_EQ('c', fstr[2]);
  EXPECT_EQ('d', fstr[3]);
  EXPECT_EQ('\0', fstr[4]);
}

TEST(FixedStringTest, AppendCstrOverflow) {
  FixedString<5> fstr("abc");
  char str[40] = "deeeeeeeeeeeeee";
  fstr.append(str);
  EXPECT_EQ(5u, fstr.size());
  EXPECT_EQ(5u, fstr.length());
  EXPECT_FALSE(fstr.empty());
  EXPECT_EQ('a', fstr[0]);
  EXPECT_EQ('b', fstr[1]);
  EXPECT_EQ('c', fstr[2]);
  EXPECT_EQ('d', fstr[3]);
  EXPECT_EQ('e', fstr[4]);
  EXPECT_EQ('\0', fstr[5]);
}

TEST(FixedStringTest, AppendString) {
  FixedString<5> fstr("abc");
  std::string str = "defg";
  fstr.append(str);
  EXPECT_EQ(5u, fstr.size());
  EXPECT_EQ(5u, fstr.length());
  EXPECT_FALSE(fstr.empty());
  EXPECT_EQ('a', fstr[0]);
  EXPECT_EQ('b', fstr[1]);
  EXPECT_EQ('c', fstr[2]);
  EXPECT_EQ('d', fstr[3]);
  EXPECT_EQ('e', fstr[4]);
  EXPECT_EQ('\0', fstr[5]);
}

TEST(FixedStringTest, PushBack) {
  FixedString<3> fstr;
  char str[4] = "abc";
  for (char c : str) {
    fstr.push_back(c);
  }
  EXPECT_EQ(3u, fstr.size());
  EXPECT_EQ(3u, fstr.length());
  EXPECT_FALSE(fstr.empty());
  EXPECT_EQ('a', fstr[0]);
  EXPECT_EQ('b', fstr[1]);
  EXPECT_EQ('c', fstr[2]);
  EXPECT_EQ('\0', fstr[3]);
}

TEST(FixedStringTest, PushBackOverflow) {
  FixedString<4> fstr("abc");
  char str[4] = "def";
  for (char c : str) {
    fstr.push_back(c);
  }
  EXPECT_EQ(4u, fstr.size());
  EXPECT_EQ(4u, fstr.length());
  EXPECT_FALSE(fstr.empty());
  EXPECT_EQ('a', fstr[0]);
  EXPECT_EQ('b', fstr[1]);
  EXPECT_EQ('c', fstr[2]);
  EXPECT_EQ('d', fstr[3]);
  EXPECT_EQ('\0', fstr[4]);
}

TEST(FixedStringTest, ToString) {
  FixedString<3> str;
  EXPECT_EQ(std::string(""), str.to_string());
  str = "abc";
  EXPECT_EQ(std::string("abc"), str.to_string());
  std::string string = (std::string)str;
  EXPECT_EQ(std::string("abc"), string);
}

TEST(FixedStringTest, SubStr) {
  FixedString<11> str;
  EXPECT_EQ("", str.substr(1, 3));
  str = "Hello World";
  string_view substr = str.substr(6, 5);
  EXPECT_EQ("World", substr);
  EXPECT_EQ(5u, substr.size());
  str = "hi lullaby";
  EXPECT_EQ("lullaby", str.substr(3));
}

TEST(FixedStringTest, CompareFstrings) {
  FixedString<32> str1 = "hello";
  FixedString<64> str2 = "world";
  FixedString<64> str3 = "hello";
  EXPECT_FALSE(str1 == str2);
  EXPECT_FALSE(str2 == str3);
  EXPECT_TRUE(str1 == str3);
  EXPECT_TRUE(str1 != str2);
  EXPECT_FALSE(str1 != str3);
  EXPECT_TRUE(str1 < str2);
  EXPECT_FALSE(str1 < str3);
  EXPECT_TRUE(str1 <= str3);
  EXPECT_FALSE(str2 <= str1);
  EXPECT_TRUE(str2 > str1);
  EXPECT_FALSE(str3 > str1);
  EXPECT_TRUE(str3 >= str1);
  EXPECT_FALSE(str1 >= str2);
}

TEST(FixedStringTest, CompareStrings) {
  FixedString<3> str = "def";
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

TEST(FixedStringTest, Operators) {
  FixedString<5> str = "Hello";
  EXPECT_EQ(5u, str.length());
  EXPECT_EQ("Hello", str.to_string());
  FixedString<6> str2 = " world";
  FixedString<11> str3 = str + str2;
  EXPECT_EQ(11u, str3.length());
  EXPECT_EQ("Hello world", str3.to_string());
  EXPECT_EQ("Hello", str.to_string());
  EXPECT_EQ(" world", str2.to_string());
  str+=" world";
  EXPECT_EQ(5u, str.length());
  EXPECT_EQ("Hello", str.to_string());
  FixedString<12> str4 = "Hello";
  str4+=str2;
  EXPECT_EQ(11u, str4.length());
  EXPECT_EQ("Hello world", str4.to_string());
  EXPECT_EQ("Hello world.", str4 + ".");
  FixedString<10> str5 = "Hello";
  auto str6 = str5 + str5;
  EXPECT_EQ(10u, str6.max_size());
  EXPECT_EQ("HelloHello", str6.to_string());
}

TEST(FixedStringTest, Ostream) {
  std::stringstream o;
  FixedString<5> str = "world";
  o << "Hello" << FixedString<5>() << " " << str;
  EXPECT_EQ("Hello world", o.str());
}

TEST(FixedStringTest, Format) {
  FixedString<50> fstr;
  fstr.format("Hello %s! Pi is %.2f", "world", 3.14);
  EXPECT_FALSE(fstr.empty());
  EXPECT_EQ("Hello world! Pi is 3.14", fstr.to_string());
}

TEST(FixedStringTest, Hash) {
  const char* test1 = "Hello";
  FixedString<5> test2 = "Hello";
  std::unordered_map<FixedString<5>, int, FixedString<5>::Hash> map;
  map[test2] = 5;
  EXPECT_EQ(map[test2], 5);
  EXPECT_EQ(map[test1], 5);
}

}  // namespace lull
