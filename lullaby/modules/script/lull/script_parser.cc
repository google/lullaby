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

Optional<bool> ToBoolean(string_view str) {
  const bool is_true = (str == "true");
  const bool is_false = (str == "false");
  Optional<bool> result(is_true);
  if (!is_true && !is_false) {
    result.reset();
  }
  return result;
}

Optional<int> ToInteger(string_view str) {
  char* end = nullptr;
  Optional<int> result = static_cast<int>(strtol(str.data(), &end, 10));
  if (str.data() + str.length() != end) {
    result.reset();
  }
  return result;
}

Optional<float> ToFloat(string_view str) {
  char* end = nullptr;
  Optional<float> result = static_cast<float>(strtod(str.data(), &end));
  if (str.data() + str.length() != end) {
    result.reset();
  }
  return result;
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

  ParserCallbacks* callbacks;
};

}  // namespace

void ParseScript(string_view source, ParserCallbacks* callbacks) {
  ScriptParser parser(callbacks);
  parser.Parse(source);
  callbacks->Process(ParserCallbacks::kEof, nullptr, "");
}

void ScriptParser::Parse(string_view source) {
  string_view str = Strip(source);
  if (str.length() == 0) {
    return;
  } else if (str[0] == '(') {
    if (str.length() < 2) {
      callbacks->Error(str, "Expected matching closing parenthesis.");
      return;
    }
    const char end = str[str.length() - 1];
    if (end != ')') {
      callbacks->Error(str, "Expected matching closing parenthesis.");
      return;
    }

    callbacks->Process(ParserCallbacks::kPush, nullptr, "(");
    // Get the code contained inside the parentheses and process its contents
    // recursively using the Split function.
    const string_view sub = str.substr(1, str.length() - 2);
    Split(sub);
    callbacks->Process(ParserCallbacks::kPop, nullptr, ")");
  } else if (str[0] == '\'') {
    if (str.length() < 2) {
      callbacks->Error(str, "Expected matching closing single quote.");
      return;
    }
    const char end = str[str.length() - 1];
    if (end != '\'') {
      callbacks->Error(str, "Expected matching closing single quote.");
      return;
    }

    const string_view sub = str.substr(1, str.length() - 2);
    callbacks->Process(ParserCallbacks::kString, &sub, str);
  } else if (str[0] == '\"') {
    if (str.length() < 2) {
      callbacks->Error(str, "Expected matching closing double quote.");
      return;
    }
    const char end = str[str.length() - 1];
    if (end != '\"') {
      callbacks->Error(str, "Expected matching closing double quote.");
      return;
    }

    const string_view sub = str.substr(1, str.length() - 2);
    callbacks->Process(ParserCallbacks::kString, &sub, str);
  } else if (auto b = ToBoolean(str)) {
    callbacks->Process(ParserCallbacks::kBool, b.get(), str);
  } else if (auto i = ToInteger(str)) {
    callbacks->Process(ParserCallbacks::kInt, i.get(), str);
  } else if (auto f = ToFloat(str)) {
    callbacks->Process(ParserCallbacks::kFloat, f.get(), str);
  } else if (str.length() > 0) {
    HashValue id = Hash(str.data(), str.length());
    callbacks->Process(ParserCallbacks::kSymbol, &id, str);
  } else {
    callbacks->Process(ParserCallbacks::kNil, nullptr, str);
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
