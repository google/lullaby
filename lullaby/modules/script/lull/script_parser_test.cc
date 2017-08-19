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

#include "lullaby/modules/script/lull/script_parser.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/common_types.h"
#include "lullaby/util/variant.h"

namespace lull {
namespace {

using ::testing::Eq;

struct Token {
  ParserCallbacks::CodeType type;
  Variant value;
  std::string token;
};

template <typename T>
bool Compare(const Variant& lhs, const Variant& rhs) {
  const T* a = lhs.Get<T>();
  const T* b = rhs.Get<T>();
  const bool result = a && b && *a == *b;
  return result;
}

bool operator==(const Token& lhs, const Token& rhs) {
  if (lhs.type != rhs.type) {
    return false;
  } else if (lhs.token != rhs.token) {
    return false;
  } else if (lhs.value.GetTypeId() != rhs.value.GetTypeId()) {
    return false;
  }
  switch (lhs.type) {
    case ParserCallbacks::kEof:
      return true;
    case ParserCallbacks::kNil:
      return true;
    case ParserCallbacks::kPush:
      return true;
    case ParserCallbacks::kPop:
      return true;
    case ParserCallbacks::kBool:
      return Compare<bool>(lhs.value, rhs.value);
    case ParserCallbacks::kInt:
      return Compare<int>(lhs.value, rhs.value);
    case ParserCallbacks::kFloat:
      return Compare<float>(lhs.value, rhs.value);
    case ParserCallbacks::kSymbol:
      return Compare<HashValue>(lhs.value, rhs.value);
    case ParserCallbacks::kString:
      return Compare<std::string>(lhs.value, rhs.value);
  }
}

struct TestParserCallbacks : ParserCallbacks {
  void Expect(CodeType type, std::string token = "", Variant var = Variant()) {
    expected.push_back({type, std::move(var), std::move(token)});
  }

  void Process(CodeType type, const void* ptr, string_view token) override {
    Variant var;
    switch (type) {
      case kEof:
        break;
      case kNil:
        break;
      case kPush:
        break;
      case kPop:
        break;
      case kBool:
        var = *reinterpret_cast<const bool*>(ptr);
        break;
      case kInt:
        var = *reinterpret_cast<const int*>(ptr);
        break;
      case kFloat:
        var = *reinterpret_cast<const float*>(ptr);
        break;
      case kSymbol:
        var = *reinterpret_cast<const HashValue*>(ptr);
        break;
      case kString:
        var = reinterpret_cast<const string_view*>(ptr)->to_string();
        break;
    }
    parsed.push_back({type, var, token.to_string()});
  }

  void Error(string_view token, string_view message) override {
    errors.push_back(token.to_string());
  }

  std::vector<Token> parsed;
  std::vector<Token> expected;
  std::vector<std::string> errors;
};

TEST(ScriptParserTest, Eof) {
  TestParserCallbacks callbacks;

  ParseScript("", &callbacks);
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, Empty) {
  TestParserCallbacks callbacks;

  ParseScript("()", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, Comments) {
  TestParserCallbacks callbacks;

  ParseScript("(123 # comment \n 456)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kInt, "123", 123);
  callbacks.Expect(ParserCallbacks::kInt, "456", 456);
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}


TEST(ScriptParserTest, Types) {
  TestParserCallbacks callbacks;

  ParseScript("(123 -321 456.123 789. -987. -654.321 true false 'hello' world)",
              &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kInt, "123", 123);
  callbacks.Expect(ParserCallbacks::kInt, "-321", -321);
  callbacks.Expect(ParserCallbacks::kFloat, "456.123", 456.123f);
  callbacks.Expect(ParserCallbacks::kFloat, "789.", 789.f);
  callbacks.Expect(ParserCallbacks::kFloat, "-987.", -987.f);
  callbacks.Expect(ParserCallbacks::kFloat, "-654.321", -654.321f);
  callbacks.Expect(ParserCallbacks::kBool, "true", true);
  callbacks.Expect(ParserCallbacks::kBool, "false", false);
  callbacks.Expect(ParserCallbacks::kString, "'hello'", std::string("hello"));
  callbacks.Expect(ParserCallbacks::kSymbol, "world", Hash("world"));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, Nesting) {
  TestParserCallbacks callbacks;

  ParseScript("(1 (2.0 (true (false) 'hello') world))", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kInt, "1", 1);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kFloat, "2.0", 2.f);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kBool, "true", true);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kBool, "false", false);
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kString, "'hello'", std::string("hello"));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kSymbol, "world", Hash("world"));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

}  // namespace
}  // namespace lull
