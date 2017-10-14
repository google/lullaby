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

#include <cctype>
#include "lullaby/modules/script/lull/script_types.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/optional.h"

namespace lull {
namespace {

bool is_delimiter(const char c) {
  return (c == '(') || (c == ')') || (c == '[') || (c == ']') || (c == '{') ||
         (c == '}');
}

bool is_separator(const char c) { return std::isspace(c) || (c == ';'); }

string_view Strip(string_view str) {
  int len = static_cast<int>(str.length());
  const char* ptr = str.data();
  while (len > 0 && std::isspace(ptr[0])) {
    ++ptr;
    --len;
  }
  while (len > 0 && std::isspace(ptr[len - 1])) {
    --len;
  }
  while (len > 0 && *ptr == ';') {
    ++ptr;
    --len;
    while (len > 0 && *ptr != '\r' && *ptr != '\n') {
      ++ptr;
      --len;
    }
    while (len > 0 && std::isspace(*ptr)) {
      ++ptr;
      --len;
    }
  }
  return string_view(ptr, len);
}

Optional<int32_t> ToInt32(const char* str, size_t len) {
  char* end = nullptr;
  Optional<int32_t> result =
      static_cast<int32_t>(strtol(str, &end, 10));
  if (str + len != end) {
    result.reset();
  }
  return result;
}

Optional<uint32_t> ToUint32(const char* str, size_t len) {
  char* end = nullptr;
  Optional<uint32_t> result =
      static_cast<uint32_t>(strtoul(str, &end, 10));
  if (str + len != end) {
    result.reset();
  }
  return result;
}

Optional<int64_t> ToInt64(const char* str, size_t len) {
  char* end = nullptr;
  Optional<int64_t> result = strtoll(str, &end, 10);
  if (str + len != end) {
    result.reset();
  }
  return result;
}

Optional<uint64_t> ToUint64(const char* str, size_t len) {
  char* end = nullptr;
  Optional<uint64_t> result = strtoull(str, &end, 10);
  if (str + len != end) {
    result.reset();
  }
  return result;
}

Optional<float> ToFloat(const char* str, size_t len) {
  char* end = nullptr;
  Optional<float> result = strtof(str, &end);
  if (str + len != end) {
    result.reset();
  }
  return result;
}

Optional<double> ToDouble(const char* str, size_t len) {
  char* end = nullptr;
  Optional<double> result = strtod(str, &end);
  if (str + len != end) {
    result.reset();
  }
  return result;
}

Optional<bool> ParseBoolean(string_view str) {
  const bool is_true = (str == "true");
  const bool is_false = (str == "false");
  Optional<bool> result(is_true);
  if (!is_true && !is_false) {
    result.reset();
  }
  return result;
}

Optional<int32_t> ParseInt32(string_view str) {
  return ToInt32(str.data(), str.length());
}

Optional<uint32_t> ParseUint32(string_view str) {
  const size_t len = str.length();
  if (len < static_cast<size_t>(2)) {
    return Optional<uint32_t>();
  } else if (str[len - 1] != 'u') {
    return Optional<uint32_t>();
  } else {
    return ToUint32(str.data(), len - 1);
  }
}

Optional<int64_t> ParseInt64(string_view str) {
  const size_t len = str.length();
  if (len < static_cast<size_t>(2)) {
    return Optional<int64_t>();
  } else if (str[len - 1] != 'l') {
    return Optional<int64_t>();
  } else {
    return ToInt64(str.data(), len - 1);
  }
}

Optional<uint64_t> ParseUint64(string_view str) {
  const size_t len = str.length();
  if (len < static_cast<size_t>(3)) {
    return Optional<uint64_t>();
  } else if (str[len - 2] != 'u') {
    return Optional<uint64_t>();
  } else if (str[len - 1] != 'l') {
    return Optional<uint64_t>();
  } else {
    return ToUint64(str.data(), len - 2);
  }
}

Optional<float> ParseFloat(string_view str) {
  const size_t len = str.length();
  if (len < static_cast<size_t>(2)) {
    return Optional<float>();
  } else if (str[len - 1] != 'f') {
    return Optional<float>();
  } else {
    return ToFloat(str.data(), len - 1);
  }
}

Optional<double> ParseDouble(string_view str) {
  return ToDouble(str.data(), str.length());
}

struct ScriptParser {
  explicit ScriptParser(ParserCallbacks* callbacks) : callbacks(callbacks) {}

  // Parses the given |source|, invoking the appropriate callbacks as needed.
  string_view Parse(string_view source);

  // Returns the next token and if not null sets 'rest' to the portion of source
  // following the token.
  string_view NextToken(string_view source, string_view* rest);

  string_view ParseBlock(string_view token, string_view rest, string_view src);
  void ParseString(string_view str);

  ParserCallbacks* callbacks;
};

}  // namespace

void ParseScript(string_view source, ParserCallbacks* callbacks) {
  ScriptParser parser(callbacks);
  // We split the source to separate the first block from any trailing
  // comments or others.
  string_view remaining = parser.Parse(source);
  // Check whether there are any tokens following the script.
  string_view token = parser.NextToken(remaining, nullptr);
  if (token.length() > 0) {
    callbacks->Error(remaining, "Unexpected content after script.");
  }
  callbacks->Process(ParserCallbacks::kEof, nullptr, "");
}

string_view ScriptParser::ParseBlock(string_view token, string_view rest,
                                     string_view src) {
  if (rest.length() < 1) {
    callbacks->Error(token, "Expected delimited block.");
    return rest;
  }

  char close;
  string_view error_msg;
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
  if (!closed) {
    callbacks->Error(src, error_msg);
    return rest;
  }
  callbacks->Process(pop_code, nullptr, string_view(&close, 1));
  return Strip(rest.substr(1));  // consume the closing delimiter.
}

void ScriptParser::ParseString(string_view str) {
  if (str.length() < 2) {
    callbacks->Error(str, "Expected matching closing quote.");
    return;
  } else if (str[0] != str[str.length() - 1]) {
    callbacks->Error(str, "Expected matching closing quote.");
    return;
  }

  const string_view sub = str.substr(1, str.length() - 2);
  callbacks->Process(ParserCallbacks::kString, &sub, str);
}

string_view ScriptParser::Parse(string_view source) {
  string_view rest;
  string_view token = NextToken(source, &rest);
  if (token.length() == 0) {
    return token;
  } else if (token[0] == '(' || token[0] == '[' || token[0] == '{') {
    rest = ParseBlock(token, rest, source);
  } else if (token[0] == '\'' || token[0] == '"') {
    ParseString(token);
  } else if (token[0] == ':') {
    HashValue id = Hash(token.substr(1));
    callbacks->Process(ParserCallbacks::kHashValue, &id, token);
  } else if (auto b = ParseBoolean(token)) {
    callbacks->Process(ParserCallbacks::kBool, b.get(), token);
  } else if (auto i = ParseUint64(token)) {
    callbacks->Process(ParserCallbacks::kUint64, i.get(), token);
  } else if (auto i = ParseInt64(token)) {
    callbacks->Process(ParserCallbacks::kInt64, i.get(), token);
  } else if (auto i = ParseUint32(token)) {
    callbacks->Process(ParserCallbacks::kUint32, i.get(), token);
  } else if (auto i = ParseInt32(token)) {
    callbacks->Process(ParserCallbacks::kInt32, i.get(), token);
  } else if (auto f = ParseFloat(token)) {
    callbacks->Process(ParserCallbacks::kFloat, f.get(), token);
  } else if (auto f = ParseDouble(token)) {
    callbacks->Process(ParserCallbacks::kDouble, f.get(), token);
  } else if (token.length() > 0) {
    Symbol id = Symbol(token);
    callbacks->Process(ParserCallbacks::kSymbol, &id, token);
  } else {
    callbacks->Error(token, "Unknown token type.");
  }
  return rest;
}

string_view ScriptParser::NextToken(string_view source, string_view* rest) {
  source = Strip(source);
  if (source.length() == 0) {
    if (rest) {
      *rest = source;
    }
    return string_view();
  }

  const char* str = source.data();
  const char* end = source.data() + source.length();

  if (is_delimiter(*str)) {
    if (rest) {
      *rest = Strip(string_view(str + 1, end - str - 1));
    }
    return string_view(str, 1);
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
    } else if (is_separator(c) || is_delimiter(c)) {
      break;
    }
    ++ptr;
  }

  if (rest) {
    *rest = Strip(string_view(ptr, end - ptr));
  }
  return string_view(str, ptr - str);
}

}  // namespace lull
