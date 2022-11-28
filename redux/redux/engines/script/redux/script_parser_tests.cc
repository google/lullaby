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
#include "redux/engines/script/redux/script_parser.h"
#include "redux/engines/script/redux/script_types.h"
#include "redux/modules/var/var.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Ge;

struct Token {
  ParserCallbacks::TokenType type;
  Var value;
  std::string token;
};

template <typename T>
bool Compare(const Var& lhs, const Var& rhs) {
  const T* a = lhs.Get<T>();
  const T* b = rhs.Get<T>();
  if constexpr (std::is_same_v<T, Symbol>) {
    return a && b && a->value == b->value;
  } else {
    return a && b && *a == *b;
  }
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
    case ParserCallbacks::kPush:
      return true;
    case ParserCallbacks::kPop:
      return true;
    case ParserCallbacks::kPushArray:
      return true;
    case ParserCallbacks::kPopArray:
      return true;
    case ParserCallbacks::kPushMap:
      return true;
    case ParserCallbacks::kPopMap:
      return true;
    case ParserCallbacks::kNull:
      return lhs.value.Empty() && rhs.value.Empty();
    case ParserCallbacks::kBool:
      return Compare<bool>(lhs.value, rhs.value);
    case ParserCallbacks::kInt8:
      return Compare<int8_t>(lhs.value, rhs.value);
    case ParserCallbacks::kUint8:
      return Compare<uint8_t>(lhs.value, rhs.value);
    case ParserCallbacks::kInt16:
      return Compare<int16_t>(lhs.value, rhs.value);
    case ParserCallbacks::kUint16:
      return Compare<uint16_t>(lhs.value, rhs.value);
    case ParserCallbacks::kInt32:
      return Compare<int32_t>(lhs.value, rhs.value);
    case ParserCallbacks::kUint32:
      return Compare<uint32_t>(lhs.value, rhs.value);
    case ParserCallbacks::kInt64:
      return Compare<int64_t>(lhs.value, rhs.value);
    case ParserCallbacks::kUint64:
      return Compare<uint64_t>(lhs.value, rhs.value);
    case ParserCallbacks::kFloat:
      return Compare<float>(lhs.value, rhs.value);
    case ParserCallbacks::kDouble:
      return Compare<double>(lhs.value, rhs.value);
    case ParserCallbacks::kHashValue:
      return Compare<HashValue>(lhs.value, rhs.value);
    case ParserCallbacks::kSymbol:
      return Compare<Symbol>(lhs.value, rhs.value);
    case ParserCallbacks::kString:
      return Compare<std::string>(lhs.value, rhs.value);
  }
}

struct TestParserCallbacks : ParserCallbacks {
  void Expect(TokenType type, std::string token = "", Var var = Var()) {
    expected.push_back({type, std::move(var), std::move(token)});
  }

  void Process(TokenType type, const void* ptr,
               std::string_view token) override {
    Var var;
    switch (type) {
      case kEof:
        break;
      case kPush:
        break;
      case kPop:
        break;
      case kPushArray:
        break;
      case kPopArray:
        break;
      case kPushMap:
        break;
      case kPopMap:
        break;
      case kNull:
        break;
      case kBool:
        var = *reinterpret_cast<const bool*>(ptr);
        break;
      case kInt8:
        var = *reinterpret_cast<const int8_t*>(ptr);
        break;
      case kUint8:
        var = *reinterpret_cast<const uint8_t*>(ptr);
        break;
      case kInt16:
        var = *reinterpret_cast<const int16_t*>(ptr);
        break;
      case kUint16:
        var = *reinterpret_cast<const uint16_t*>(ptr);
        break;
      case kInt32:
        var = *reinterpret_cast<const int32_t*>(ptr);
        break;
      case kUint32:
        var = *reinterpret_cast<const uint32_t*>(ptr);
        break;
      case kInt64:
        var = *reinterpret_cast<const int64_t*>(ptr);
        break;
      case kUint64:
        var = *reinterpret_cast<const uint64_t*>(ptr);
        break;
      case kFloat:
        var = *reinterpret_cast<const float*>(ptr);
        break;
      case kDouble:
        var = *reinterpret_cast<const double*>(ptr);
        break;
      case kHashValue:
        var = *reinterpret_cast<const HashValue*>(ptr);
        break;
      case kSymbol:
        var = *reinterpret_cast<const Symbol*>(ptr);
        break;
      case kString:
        var = std::string(*reinterpret_cast<const std::string_view*>(ptr));
        break;
    }
    parsed.push_back({type, var, std::string(token)});
  }

  void Error(std::string_view token, std::string_view message) override {
    LOG(ERROR) << message;
    errors.emplace_back(token);
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

TEST(ScriptParserTest, MapArrayBlocks) {
  TestParserCallbacks callbacks;

  ParseScript("([{[()]}])", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kPushArray, "[");
  callbacks.Expect(ParserCallbacks::kPushMap, "{");
  callbacks.Expect(ParserCallbacks::kPushArray, "[");
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kPopArray, "]");
  callbacks.Expect(ParserCallbacks::kPopMap, "}");
  callbacks.Expect(ParserCallbacks::kPopArray, "]");
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, NullParsing) {
  TestParserCallbacks callbacks;
  ParseScript("(null null)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kNull, "null");
  callbacks.Expect(ParserCallbacks::kNull, "null");
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, BoolParsing) {
  TestParserCallbacks callbacks;
  ParseScript("(true false)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kBool, "true", true);
  callbacks.Expect(ParserCallbacks::kBool, "false", false);
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, Int32Parsing) {
  TestParserCallbacks callbacks;
  ParseScript("(123 -321)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kInt32, "123", int32_t(123));
  callbacks.Expect(ParserCallbacks::kInt32, "-321", int32_t(-321));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, UInt32Parsing) {
  TestParserCallbacks callbacks;
  ParseScript("(123u 321u)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kUint32, "123u", uint32_t(123));
  callbacks.Expect(ParserCallbacks::kUint32, "321u", uint32_t(321));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, Int64Parsing) {
  TestParserCallbacks callbacks;
  ParseScript("(123l -321l)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kInt64, "123l", int64_t(123));
  callbacks.Expect(ParserCallbacks::kInt64, "-321l", int64_t(-321));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, UInt64Parsing) {
  TestParserCallbacks callbacks;
  ParseScript("(123ul 321ul)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kUint64, "123ul", uint64_t(123));
  callbacks.Expect(ParserCallbacks::kUint64, "321ul", uint64_t(321));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, FloatParsing) {
  TestParserCallbacks callbacks;
  ParseScript("(456.123f 789.f -987.f -654.321f)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kFloat, "456.123f", 456.123f);
  callbacks.Expect(ParserCallbacks::kFloat, "789.f", 789.f);
  callbacks.Expect(ParserCallbacks::kFloat, "-987.f", -987.f);
  callbacks.Expect(ParserCallbacks::kFloat, "-654.321f", -654.321f);
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, DoubleParsing) {
  TestParserCallbacks callbacks;
  ParseScript("(456.123 789. -987. -654.321)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kDouble, "456.123", 456.123);
  callbacks.Expect(ParserCallbacks::kDouble, "789.", 789.);
  callbacks.Expect(ParserCallbacks::kDouble, "-987.", -987.);
  callbacks.Expect(ParserCallbacks::kDouble, "-654.321", -654.321);
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, StringParsing) {
  TestParserCallbacks callbacks;
  ParseScript("('hello' \"world\")", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kString, "'hello'", std::string("hello"));
  callbacks.Expect(ParserCallbacks::kString, "\"world\"", std::string("world"));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, HashParsing) {
  TestParserCallbacks callbacks;
  ParseScript("(:hello :world)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kHashValue, ":hello", ConstHash("hello"));
  callbacks.Expect(ParserCallbacks::kHashValue, ":world", ConstHash("world"));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, SymbolParsing) {
  TestParserCallbacks callbacks;
  ParseScript("(hello world)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kSymbol, "hello",
                   Symbol(ConstHash("hello")));
  callbacks.Expect(ParserCallbacks::kSymbol, "world",
                   Symbol(ConstHash("world")));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);
  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, Comments) {
  TestParserCallbacks callbacks;

  ParseScript("(123 ; comment\n; line\n456 ; another\n)", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kInt32, "123", 123);
  callbacks.Expect(ParserCallbacks::kInt32, "456", 456);
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, TrailingComments) {
  TestParserCallbacks callbacks;

  ParseScript("(123 ; comment\n; line\n456 ; another\n)\n; trailing",
              &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kInt32, "123", 123);
  callbacks.Expect(ParserCallbacks::kInt32, "456", 456);
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
  EXPECT_THAT(callbacks.errors.size(), Eq(size_t(0)));
}

TEST(ScriptParserTest, ClosingDelimiterInTrailingComment) {
  TestParserCallbacks callbacks;

  ParseScript("(123 ; comment\n; line\n456 ; another\n)\n; trailing)",
              &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kInt32, "123", 123);
  callbacks.Expect(ParserCallbacks::kInt32, "456", 456);
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
  EXPECT_THAT(callbacks.errors.size(), Eq(size_t(0)));
}

TEST(ScriptParserTest, Nesting) {
  TestParserCallbacks callbacks;

  ParseScript("(1 [2.0f (true {false} 'hello') world])", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kInt32, "1", 1);
  callbacks.Expect(ParserCallbacks::kPushArray, "[");
  callbacks.Expect(ParserCallbacks::kFloat, "2.0f", 2.f);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kBool, "true", true);
  callbacks.Expect(ParserCallbacks::kPushMap, "{");
  callbacks.Expect(ParserCallbacks::kBool, "false", false);
  callbacks.Expect(ParserCallbacks::kPopMap, "}");
  callbacks.Expect(ParserCallbacks::kString, "'hello'", std::string("hello"));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kSymbol, "world",
                   Symbol(ConstHash("world")));
  callbacks.Expect(ParserCallbacks::kPopArray, "]");
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

TEST(ScriptParserTest, MismatchNesting) {
  TestParserCallbacks callbacks;

  ParseScript("(1 [(2]))", &callbacks);
  EXPECT_THAT(callbacks.errors.size(), Ge(size_t(1)));
}

TEST(ScriptParserTest, MismatchQuoting) {
  TestParserCallbacks callbacks;

  ParseScript("('hello\")", &callbacks);
  EXPECT_THAT(callbacks.errors.size(), Ge(size_t(1)));
}

TEST(ScriptParserTest, UnendedQuoting) {
  TestParserCallbacks callbacks;

  ParseScript("('hello)", &callbacks);
  EXPECT_THAT(callbacks.errors.size(), Ge(size_t(1)));
}

TEST(ScriptParserTest, MixedQuotes) {
  TestParserCallbacks callbacks;

  ParseScript("(\"'\" '\"')", &callbacks);
  callbacks.Expect(ParserCallbacks::kPush, "(");
  callbacks.Expect(ParserCallbacks::kString, "\"'\"", std::string("'"));
  callbacks.Expect(ParserCallbacks::kString, "'\"'", std::string("\""));
  callbacks.Expect(ParserCallbacks::kPop, ")");
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}


}  // namespace
}  // namespace redux
