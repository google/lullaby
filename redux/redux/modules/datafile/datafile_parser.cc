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

#include "redux/modules/datafile/datafile_parser.h"

#include "redux/modules/base/logging.h"

namespace redux {

static bool IsWhitespace(char c) {
  // We treat commas as whitespace to simplify our logic when parsing things
  // like:
  //   [ { key1 : value, key2 : value2, }, { key3 : value3 } ],
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == ',';
}

static bool IsNewline(char c) { return c == '\n' || c == '\r'; }

static bool IsBracket(char c) {
  return c == '{' || c == '}' || c == '[' || c == ']';
}

static bool IsParenthesis(char c) { return c == '(' || c == ')'; }

static bool IsQuote(char c) { return c == '"' || c == '\''; }

static bool IsCommentMarker(char c) { return c == ';'; }

static bool IsKeyValueSeparator(char c) { return c == ':'; }

static bool IsReservedCharacter(char c) {
  return IsBracket(c) || IsParenthesis(c) || IsQuote(c) || IsCommentMarker(c) ||
         IsKeyValueSeparator(c);
}

static bool IsQuoteDelimitedToken(std::string_view token) {
  return token.length() > 2 && IsQuote(token.front()) &&
         token.front() == token.back();
}

static bool IsParenthesisDelimitedToken(std::string_view token) {
  return token.length() > 2 && token.front() == '(' && token.back() == ')';
}

// Returns the substring of the text with all whitespace removed from the front.
static std::string_view StripFront(std::string_view txt) {
  while (!txt.empty() && IsWhitespace(txt.front())) {
    txt = txt.substr(1);
  }
  return txt;
}

// Returns the substring of the text with all whitespace removed from the back.
static std::string_view StripBack(std::string_view txt) {
  while (!txt.empty() && IsWhitespace(txt.back())) {
    txt.remove_suffix(1);
  }
  return txt;
}

// Returns the substring of the text with all whitespace removed from the front
// and back.
static std::string_view Strip(std::string_view txt) {
  return StripFront(StripBack(txt));
}

static std::string_view StripQuotes(std::string_view txt) {
  CHECK(IsQuoteDelimitedToken(txt));
  return txt.substr(1, txt.length() - 2);
}

static bool IsKeyValid(std::string_view key) {
  for (char c : key) {
    if (IsWhitespace(c) || IsReservedCharacter(c)) {
      return false;
    }
  }
  return true;
}

// Returns the substring of the text starting at the first non-whitespace
// character after the comments.
static std::string_view SkipComments(std::string_view txt) {
  txt = StripFront(txt);
  while (IsCommentMarker(txt.front())) {
    std::size_t i = 0;
    for (; i < txt.length(); ++i) {
      if (IsNewline(txt[i])) {
        // Consume all characters until the end of the line, as well as all
        // non-whitespace at the start of the next line.
        txt = StripFront(txt.substr(i));
      }
    }

    if (i == txt.length()) {
      // There are no more characters in the text. This may happen if the
      // comment line ends with an EOF rather than a new line.
      return txt.substr(i);
    }
  }
  return txt;
}

// Given a stream that starts with a quote, returns the snippet of the text
// from the starting quote until the ending quote. If unable to find the
// ending quote, returns an empty string pointing to the end of the stream.
static std::string_view TryReadString(std::string_view txt) {
  char quote = txt.front();
  CHECK(IsQuote(quote))
      << "Should only be called with text starting with quote";

  for (std::size_t index = 1; index < txt.length(); ++index) {
    const char p = txt[index - 1];
    const char c = txt[index];
    if (c == quote && p != '\\') {
      return txt.substr(0, index + 1);
    }
  }
  // No closing quote found, return the empty string.
  return txt.substr(txt.length());
}

// Given a stream that starts with an open parenthesis, returns the snippet of
// the text from the starting parenthesis until the ending parenthesis, taking
// any nesting into account. If unable to find the ending parenthesis, returns
// an empty string pointing to the end of the stream.
static std::string_view TryReadExpression(std::string_view txt) {
  CHECK_EQ(txt.front(), '(')
      << "Should only be called with text starting with open parenthesis";

  std::size_t index = 0;
  int count = 0;
  while (index < txt.length()) {
    const char c = txt[index];
    if (c == '(') {
      ++count;
    } else if (c == ')') {
      --count;
    }

    if (IsQuote(c)) {
      std::string_view str = TryReadString(txt.substr(index));
      if (str.empty()) {
        return str;
      }
      index += str.length();
    } else {
      ++index;
    }
    if (count == 0) {
      return txt.substr(0, index);
    }
  }
  // Return an empty string pointing at the end of the input text to indicate
  // an error.
  return txt.substr(txt.length());
}

struct TokenOrError {
  std::string_view token;
  std::string_view error;
};

// Attempts to read the next token from the stream.
// Returns a non-empty error if there was a problem during parsing. Otherwise,
// returns the token (which may or may not be empty).
// The `txt` will be updated to a position after the read token.
static TokenOrError ReadNextToken(std::string_view txt) {
  TokenOrError result;

  if (txt.empty()) {
    return result;
  }

  std::size_t index = 0;
  if (IsBracket(txt.front())) {
    index = 1;
  } else if (IsQuote(txt.front())) {
    std::string_view str = TryReadString(txt);
    if (str.empty()) {
      result.error = "Error parsing string.";
      return result;
    }

    index = str.length();
  } else if (txt.front() == '(') {
    std::string_view expr = TryReadExpression(txt);
    if (expr.empty()) {
      result.error = "Error parsing expression.";
      return result;
    }

    index = expr.length();
  } else {
    while (index < txt.length()) {
      char c = txt[index];
      if (IsWhitespace(c) || IsReservedCharacter(c)) {
        break;
      }
      ++index;
    }
  }

  result.token = txt.substr(0, index);
  return result;
}

namespace {

// Tracks the scope of the parser (i.e. inside an object or an array). Also
// helps ensure that objects/arrays are enclosed correctly.
class Scope {
 public:
  enum Type { kNone, kObject, kArray };

  Scope() { scopes_[index_] = kNone; }

  void Push(Type type) {
    CHECK(index_ < kMaxScopes) << "Too many scopes, increase kMaxScopes count";
    scopes_[index_] = type;
    ++index_;
  }

  bool Pop(Type type) {
    if (index_ > 0) {
      --index_;
      return scopes_[index_] == type;
    } else {
      return false;
    }
  }

  bool Is(Type type) const {
    return index_ > 0 ? (scopes_[index_ - 1] == type) : false;
  }

  bool Empty() const { return index_ == 0; }

 private:
  static constexpr int kMaxScopes = 128;
  Type scopes_[kMaxScopes];
  std::size_t index_ = 0;
};

}  // namespace

void ParseDatafile(std::string_view text, DatafileParserCallbacks* cb) {
  Scope scope;

  // Tracks whether the next non-delimeter token
  bool expect_key = false;

  std::string_view rest = Strip(text);
  while (!rest.empty()) {
    // Ignore any comments at the current read head.
    rest = SkipComments(rest);

    const auto result = ReadNextToken(rest);

    // Unable to read a token, forward the parsing error.
    if (!result.error.empty()) {
      cb->ParseError(rest, result.error);
      return;
    }

    // No more tokens to be read, all done.
    const std::string_view token = result.token;
    if (token.empty()) {
      break;
    }

    // Consume the token from the stream.
    rest = StripFront(rest.substr(token.length()));

    const char c = token.front();

    // Ensure that all documents start with an object or an array.
    if (scope.Empty()) {
      if (c != '{' && c != '[') {
        cb->ParseError(token, "Document must start with object or array.");
        return;
      }
    }

    if (c == '{') {
      CHECK_EQ(token.length(), 1) << "ReadNextToken parsing error.";
      if (expect_key) {
        cb->ParseError(token, "Cannot have an object as a key.");
        return;
      }

      scope.Push(Scope::kObject);
      cb->BeginObject();
      expect_key = true;
    } else if (c == '}') {
      CHECK_EQ(token.length(), 1) << "ReadNextToken parsing error.";
      if (!scope.Pop(Scope::kObject)) {
        cb->ParseError(token, "Expected ], got }");
        return;
      }

      cb->EndObject();
      expect_key = scope.Is(Scope::kObject);
    } else if (c == '[') {
      CHECK_EQ(token.length(), 1) << "ReadNextToken parsing error.";
      if (expect_key) {
        cb->ParseError(token, "Cannot have an array as a key.");
        return;
      }

      scope.Push(Scope::kArray);
      cb->BeginArray();
      expect_key = false;
    } else if (c == ']') {
      CHECK_EQ(token.length(), 1) << "ReadNextToken parsing error.";
      if (!scope.Pop(Scope::kArray)) {
        cb->ParseError(token, "Expected }, got ]");
        return;
      }

      cb->EndArray();
      expect_key = scope.Is(Scope::kObject);
    } else if (c == '(') {
      if (!IsParenthesisDelimitedToken(token)) {
        cb->ParseError(token, "Unable to parse expression.");
        return;
      }

      cb->Expression(token);
      expect_key = scope.Is(Scope::kObject);
    } else if (expect_key) {
      std::string_view key = token;
      if (IsQuoteDelimitedToken(key)) {
        // If the key is quoted, we allow it to contain special characters,
        // so only check IsKeyValid in the non-quoted case.
        key = StripQuotes(token);
      } else if (!IsKeyValid(key)) {
        cb->ParseError(key, "Invalid key.");
        return;
      }
      cb->Key(key);

      // Consume the separator character after the key.
      if (rest.empty() || !IsKeyValueSeparator(rest.front())) {
        cb->ParseError(rest, "Expecting separator between key and value.");
        return;
      }
      rest = StripFront(rest.substr(1));
      expect_key = false;
    } else if (IsQuote(c)) {
      if (!IsQuoteDelimitedToken(token)) {
        cb->ParseError(token, "Unable to parse string.");
        return;
      }

      cb->String(StripQuotes(token));
      expect_key = scope.Is(Scope::kObject);
    } else if (token == "true") {
      cb->Boolean(true);
      expect_key = scope.Is(Scope::kObject);
    } else if (token == "false") {
      cb->Boolean(false);
      expect_key = scope.Is(Scope::kObject);
    } else if (token == "null") {
      cb->Null();
      expect_key = scope.Is(Scope::kObject);
    } else {
      char* end = nullptr;
      const double result = strtod(token.data(), &end);
      // Ensure we consumed the entire string as part of the parsing.
      const bool success = (end == &*token.end());
      if (!success) {
        cb->ParseError(token, "Unable to parse value.");
        return;
      }

      cb->Number(result);
      expect_key = scope.Is(Scope::kObject);
    }
  }

  if (!scope.Empty()) {
    cb->ParseError(text.substr(text.length()), "Unexpected end of stream");
  }
}

}  // namespace redux
