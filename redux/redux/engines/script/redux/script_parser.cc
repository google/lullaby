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

#include "redux/engines/script/redux/script_parser.h"

#include <cctype>

#include "absl/strings/numbers.h"
#include "redux/engines/script/redux/script_types.h"
#include "redux/modules/base/hash.h"

namespace redux {
namespace {

bool IsScopeDelimiter(const char c) {
  return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}';
}

bool IsWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool IsSeperator(const char c) {
  return IsWhitespace(c) || c == ';' || c == ',';
}

bool HasSuffix(std::string_view str, char c1) {
  const std::size_t len = str.length();
  return len >= 1 && str[len - 1] == c1;
}

bool HasSuffix(std::string_view str, char c1, char c2) {
  const std::size_t len = str.length();
  return len >= 2 && str[len - 2] == c1 && str[len - 1] == c2;
}

std::string_view Strip(std::string_view str) {
  int len = static_cast<int>(str.length());
  const char* ptr = str.data();
  while (len > 0 && IsWhitespace(ptr[0])) {
    ++ptr;
    --len;
  }
  while (len > 0 && IsWhitespace(ptr[len - 1])) {
    --len;
  }
  while (len > 0 && *ptr == ';') {
    ++ptr;
    --len;
    while (len > 0 && *ptr != '\r' && *ptr != '\n') {
      ++ptr;
      --len;
    }
    while (len > 0 && IsWhitespace(*ptr)) {
      ++ptr;
      --len;
    }
  }
  return std::string_view(ptr, len);
}

template <typename T>
std::optional<T> ParseNumberAs(std::string_view str) {
  T value = 0;
  bool ok = true;
  if constexpr (std::is_same_v<T, float>) {
    ok = absl::SimpleAtof(str, &value);
  } else if constexpr (std::is_same_v<T, double>) {
    ok = absl::SimpleAtod(str, &value);
  } else {
    ok = absl::SimpleAtoi<T>(str, &value);
  }
  if (ok) {
    return value;
  } else {
    return std::nullopt;
  }
}

std::optional<int32_t> ParseInt32(std::string_view str) {
  return ParseNumberAs<int32_t>(str);
}

std::optional<uint32_t> ParseUint32(std::string_view str) {
  const size_t len = str.length();
  if (HasSuffix(str, 'u')) {
    return ParseNumberAs<uint32_t>(str.substr(0, len - 1));
  } else {
    return std::nullopt;
  }
}

std::optional<int64_t> ParseInt64(std::string_view str) {
  const size_t len = str.length();
  if (HasSuffix(str, 'l')) {
    return ParseNumberAs<int64_t>(str.substr(0, len - 1));
  } else {
    return std::nullopt;
  }
}

std::optional<uint64_t> ParseUint64(std::string_view str) {
  const size_t len = str.length();
  if (HasSuffix(str, 'u', 'l')) {
    return ParseNumberAs<uint64_t>(str.substr(0, len - 2));
  } else {
    return std::nullopt;
  }
}

std::optional<float> ParseFloat(std::string_view str) {
  const size_t len = str.length();
  if (HasSuffix(str, 'f')) {
    return ParseNumberAs<float>(str.substr(0, len - 1));
  } else {
    return std::nullopt;
  }
}

std::optional<double> ParseDouble(std::string_view str) {
  return ParseNumberAs<double>(str);
}

std::optional<bool> ParseBoolean(std::string_view str) {
  if (str == "true") {
    return true;
  } else if (str == "false") {
    return false;
  } else {
    return std::nullopt;
  }
}

struct ScriptParser {
  explicit ScriptParser(ParserCallbacks* callbacks) : callbacks(callbacks) {}

  // Parses the given |source|, invoking the appropriate callbacks as needed.
  std::string_view Parse(std::string_view source);

  // Returns the next token and if not null sets 'rest' to the portion of source
  // following the token.
  std::string_view NextToken(std::string_view source, std::string_view* rest);

  std::string_view ParseBlock(std::string_view token, std::string_view rest,
                              std::string_view src);
  ParserCallbacks* callbacks;
};

}  // namespace

void ParseScript(std::string_view source, ParserCallbacks* callbacks) {
  ScriptParser parser(callbacks);

  // We split the source to separate the first block from any trailing
  // comments or others.
  std::string_view remaining = parser.Parse(source);

  // Check whether there are any tokens following the script.
  std::string_view token = parser.NextToken(remaining, nullptr);
  if (token.length() > 0) {
    callbacks->Error(remaining, "Unexpected content after script.");
  }

  callbacks->Process(ParserCallbacks::kEof, nullptr, "");
}

std::string_view ScriptParser::ParseBlock(std::string_view token,
                                          std::string_view rest,
                                          std::string_view src) {
  if (rest.length() < 1) {
    callbacks->Error(token, "Expected delimited block.");
    return rest;
  }

  char close;
  std::string_view error_msg;
  ParserCallbacks::TokenType push_code = ParserCallbacks::kEof;
  ParserCallbacks::TokenType pop_code = ParserCallbacks::kEof;

  switch (token[0]) {
    case '(':
      close = ')';
      push_code = ParserCallbacks::kPush;
      pop_code = ParserCallbacks::kPop;
      error_msg = "Expected closing ')'";
      break;
    case '[':
      close = ']';
      push_code = ParserCallbacks::kPushArray;
      pop_code = ParserCallbacks::kPopArray;
      error_msg = "Expected closing ']'";
      break;
    case '{':
      close = '}';
      push_code = ParserCallbacks::kPushMap;
      pop_code = ParserCallbacks::kPopMap;
      error_msg = "Expected closing '}'";
      break;
    default:
      callbacks->Error(src, "Invalid delimiter.");
      return rest;
  }

  callbacks->Process(push_code, nullptr, token);
  bool closed = false;
  while (rest.length() > 0) {
    // Peek at the next token to see if it's our closing delimiter.
    if (rest[0] == close) {
      closed = true;
      break;
    }
    rest = Parse(rest);
  }

  if (closed) {
    callbacks->Process(pop_code, nullptr, std::string_view(&close, 1));
    return Strip(rest.substr(1));  // consume the closing delimiter.
  } else {
    callbacks->Error(src, error_msg);
    return rest;
  }
}

std::string_view ScriptParser::Parse(std::string_view source) {
  std::string_view rest;
  std::string_view token = NextToken(source, &rest);
  if (token.length() == 0) {
    return token;
  } else if (token[0] == '(' || token[0] == '[' || token[0] == '{') {
    rest = ParseBlock(token, rest, source);
  } else if (token.front() == '\'' || token.front() == '"') {
    if (token.length() < 2 || token.front() != token.back()) {
      callbacks->Error(token, "Expected matching closing quote.");
    } else {
      const std::string_view str = token.substr(1, token.length() - 2);
      callbacks->Process(ParserCallbacks::kString, &str, token);
    }
  } else if (token[0] == ':') {
    if (token.length() == 1) {
      callbacks->Error(token, "Hash value is empty");
    } else {
      const HashValue id = Hash(token.substr(1));
      callbacks->Process(ParserCallbacks::kHashValue, &id, token);
    }
  } else if (token == "null") {
    callbacks->Process(ParserCallbacks::kNull, nullptr, token);
  } else if (auto b = ParseBoolean(token)) {
    callbacks->Process(ParserCallbacks::kBool, &b.value(), token);
  } else if (auto i = ParseUint64(token)) {
    callbacks->Process(ParserCallbacks::kUint64, &i.value(), token);
  } else if (auto i = ParseInt64(token)) {
    callbacks->Process(ParserCallbacks::kInt64, &i.value(), token);
  } else if (auto i = ParseUint32(token)) {
    callbacks->Process(ParserCallbacks::kUint32, &i.value(), token);
  } else if (auto i = ParseInt32(token)) {
    callbacks->Process(ParserCallbacks::kInt32, &i.value(), token);
  } else if (auto f = ParseFloat(token)) {
    callbacks->Process(ParserCallbacks::kFloat, &f.value(), token);
  } else if (auto f = ParseDouble(token)) {
    callbacks->Process(ParserCallbacks::kDouble, &f.value(), token);
  } else if (token.length() > 0) {
    Symbol id = Symbol(Hash(token));
    callbacks->Process(ParserCallbacks::kSymbol, &id, token);
  } else {
    callbacks->Error(token, "Unknown token type.");
  }
  return rest;
}

std::string_view ScriptParser::NextToken(std::string_view source,
                                         std::string_view* rest) {
  source = Strip(source);
  if (source.length() == 0) {
    if (rest) {
      *rest = source;
    }
    return std::string_view();
  }

  const char* str = source.data();
  const char* end = source.data() + source.length();

  if (IsScopeDelimiter(*str)) {
    if (rest) {
      *rest = Strip(std::string_view(str + 1, end - str - 1));
    }
    return std::string_view(str, 1);
  }

  char quote = 0;
  if (*str == '"') {
    quote = '"';
  } else if (*str == '\'') {
    quote = '\'';
  }

  const char* ptr = str;
  if (quote != 0) {
    ++ptr;
  }

  bool escape = false;
  while (ptr < end) {
    char c = *ptr;

    if (escape) {
      escape = false;
    } else if (c == '\\' && escape == false) {
      escape = true;
    } else if (c == quote) {
      ++ptr;
      break;
    } else if (quote != 0) {
      // nothing
    } else if (IsSeperator(c) || IsScopeDelimiter(c)) {
      break;
    }
    ++ptr;
  }

  if (rest) {
    *rest = Strip(std::string_view(ptr, end - ptr));
  }
  return std::string_view(str, ptr - str);
}

}  // namespace redux
