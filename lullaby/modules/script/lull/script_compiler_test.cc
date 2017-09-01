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

#include "lullaby/modules/script/lull/script_compiler.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

using ::testing::Eq;

struct Entry {
  ParserCallbacks::TokenType type;
  Variant value;
};

template <typename T>
bool Compare(const Variant& lhs, const Variant& rhs) {
  const T* a = lhs.Get<T>();
  const T* b = rhs.Get<T>();
  const bool result = a && b && *a == *b;
  return result;
}

bool operator==(const Entry& lhs, const Entry& rhs) {
  if (lhs.type != rhs.type) {
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
      return Compare<HashValue>(lhs.value, rhs.value);
    case ParserCallbacks::kString:
      return Compare<std::string>(lhs.value, rhs.value);
  }
}

struct TestParserCallbacks : ParserCallbacks {
  void Expect(TokenType type, Variant var = Variant()) {
    expected.push_back({type, std::move(var)});
  }

  void Process(TokenType type, const void* ptr, string_view token) override {
    Variant var;
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
      case kSymbol:
        var = *reinterpret_cast<const HashValue*>(ptr);
        break;
      case kHashValue:
        var = *reinterpret_cast<const HashValue*>(ptr);
        break;
      case kString:
        var = reinterpret_cast<const string_view*>(ptr)->to_string();
        break;
    }
    parsed.push_back({type, var});
  }

  void Error(string_view token, string_view message) override {
    errors.push_back(token.to_string());
  }

  std::vector<Entry> parsed;
  std::vector<Entry> expected;
  std::vector<std::string> errors;
};

TEST(ScriptCompilerTest, CompileAndBuild) {
  std::vector<uint8_t> buffer;

  ScriptCompiler saver(&buffer);
  ParseScript("(1 (2.0f (true (false) 'hello') world))", &saver);

  ScriptCompiler loader(&buffer);

  TestParserCallbacks callbacks;
  loader.Build(&callbacks);

  callbacks.Expect(ParserCallbacks::kPush, Variant());
  callbacks.Expect(ParserCallbacks::kInt32, 1);
  callbacks.Expect(ParserCallbacks::kPush);
  callbacks.Expect(ParserCallbacks::kFloat, 2.f);
  callbacks.Expect(ParserCallbacks::kPush);
  callbacks.Expect(ParserCallbacks::kBool, true);
  callbacks.Expect(ParserCallbacks::kPush);
  callbacks.Expect(ParserCallbacks::kBool, false);
  callbacks.Expect(ParserCallbacks::kPop);
  callbacks.Expect(ParserCallbacks::kString, std::string("hello"));
  callbacks.Expect(ParserCallbacks::kPop);
  callbacks.Expect(ParserCallbacks::kSymbol, Hash("world"));
  callbacks.Expect(ParserCallbacks::kPop);
  callbacks.Expect(ParserCallbacks::kPop);
  callbacks.Expect(ParserCallbacks::kEof);

  EXPECT_THAT(callbacks.parsed, Eq(callbacks.expected));
}

}  // namespace
}  // namespace lull
