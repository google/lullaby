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
#include "lullaby/util/optional.h"

namespace lull {
namespace {

string_view Strip(string_view str) {
  int len = static_cast<int>(str.length());
  const char* ptr = str.data();
  while (std::isspace(ptr[0]) && len > 0) {
    ++ptr;
    --len;
  }
  while (std::isspace(ptr[len - 1]) && len > 0) {
    --len;
  }
  while (*ptr == '#') {
    ++ptr;
    --len;
    while (*ptr != '\r' && *ptr != '\n' && len > 0) {
      ++ptr;
      --len;
    }
    while (std::isspace(*ptr) && len > 0) {
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
  void Parse(string_view source);

  // Splits the |source| into two parts, first and rest, using whitespace as
  // a delimeter.  Whitespace inside code blocks (ie. code enclosed in
  // parentheses) or text (ie. code enclosed in quotes) is ignored during the
  // splitting process.  The |first| element is then passed to the Parse()
  // function for processing, while the |rest| of the source is recursively
  // passed back to Split.
  void Split(string_view source);

  void ParseBlock(string_view str);
  void ParseString(string_view str);

  ParserCallbacks* callbacks;
};

}  // namespace

void ParseScript(string_view source, ParserCallbacks* callbacks) {
  ScriptParser parser(callbacks);
  parser.Parse(source);
  callbacks->Process(ParserCallbacks::kEof, nullptr, "");
}

void ScriptParser::ParseBlock(string_view str) {
  if (str.length() < 2) {
    callbacks->Error(str, "Expected matching closing parenthesis.");
    return;
  }

  const char open = str[0];
  const char close = str[str.length() - 1];
  ParserCallbacks::TokenType push_code = ParserCallbacks::kEof;
  ParserCallbacks::TokenType pop_code = ParserCallbacks::kEof;

  if (open == '(') {
    if (close != ')') {
      callbacks->Error(str, "Expected matching closing parenthesis.");
      return;
    }
    push_code = ParserCallbacks::kPush;
    pop_code = ParserCallbacks::kPop;
  } else if (open == '[') {
    if (close != ']') {
      callbacks->Error(str, "Expected matching closing parenthesis.");
      return;
    }
    push_code = ParserCallbacks::kPushArray;
    pop_code = ParserCallbacks::kPopArray;
  } else if (open == '{') {
    if (close != '}') {
      callbacks->Error(str, "Expected matching closing parenthesis.");
      return;
    }
    push_code = ParserCallbacks::kPushMap;
    pop_code = ParserCallbacks::kPopMap;
  } else {
    callbacks->Error(str, "Invalid parenthesis type.");
    return;
  }

  callbacks->Process(push_code, nullptr, string_view(&open, 1));
  // Get the code contained inside the parentheses and process its contents
  // recursively using the Split function.
  const string_view sub = str.substr(1, str.length() - 2);
  Split(sub);
  callbacks->Process(pop_code, nullptr, string_view(&close, 1));
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

void ScriptParser::Parse(string_view source) {
  string_view str = Strip(source);
  if (str.length() == 0) {
    return;
  } else if (str[0] == '(' || str[0] == '[' || str[0] == '{') {
    ParseBlock(str);
  } else if (str[0] == '\'' || str[0] == '"') {
    ParseString(str);
  } else if (str[0] == ':') {
    HashValue id = Hash(str.substr(1));
    callbacks->Process(ParserCallbacks::kHashValue, &id, str);
  } else if (auto b = ParseBoolean(str)) {
    callbacks->Process(ParserCallbacks::kBool, b.get(), str);
  } else if (auto i = ParseUint64(str)) {
    callbacks->Process(ParserCallbacks::kUint64, i.get(), str);
  } else if (auto i = ParseInt64(str)) {
    callbacks->Process(ParserCallbacks::kInt64, i.get(), str);
  } else if (auto i = ParseUint32(str)) {
    callbacks->Process(ParserCallbacks::kUint32, i.get(), str);
  } else if (auto i = ParseInt32(str)) {
    callbacks->Process(ParserCallbacks::kInt32, i.get(), str);
  } else if (auto f = ParseFloat(str)) {
    callbacks->Process(ParserCallbacks::kFloat, f.get(), str);
  } else if (auto f = ParseDouble(str)) {
    callbacks->Process(ParserCallbacks::kDouble, f.get(), str);
  } else if (str.length() > 0) {
    HashValue id = Hash(str);
    callbacks->Process(ParserCallbacks::kSymbol, &id, str);
  } else {
    callbacks->Error(str, "Unknown token type.");
  }
}

void ScriptParser::Split(string_view in) {
  if (Strip(in).length() == 0) {
    return;
  }

  const char* str = in.data();
  const char* end = in.data() + in.length();

  char quote = 0;
  char open = 0;
  char close = 0;
  int stack = 0;
  if (*str == '(') {
    open = '(';
    close = ')';
  } else if (*str == '[') {
    open = '[';
    close = ']';
  } else if (*str == '{') {
    open = '{';
    close = '}';
  } else if (*str == '"') {
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
    } else if (c == open) {
      ++stack;
    } else if (c == close) {
      --stack;
      if (stack == 0) {
        ++ptr;
        break;
      }
    } else if (quote != 0 || stack > 0) {
      // nothing
    } else if (std::isspace(c)) {
      break;
    }
    ++ptr;
  }

  const string_view word(str, ptr - str);
  Parse(Strip(word));

  const string_view rest(ptr, end - ptr);
  Split(Strip(rest));
}

}  // namespace lull
