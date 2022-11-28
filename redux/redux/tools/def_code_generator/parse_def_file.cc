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

#include "redux/tools/def_code_generator/parse_def_file.h"

#include <string>
#include <utility>

#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/datafile/datafile_parser.h"

namespace redux::tool {

constexpr const char kInclude[] = "include";
constexpr const char kNamespace[] = "namespace";
constexpr const char kEnum[] = "enum";
constexpr const char kStruct[] = "struct";
constexpr const char kComment[] = "#";
constexpr const char kStartScope[] = "{";
constexpr const char kEndScope[] = "}";
constexpr const char kEqual[] = "=";

static const bool IsQuote(char c) { return c == '"' || c == '\''; }

static const bool IsNewline(char c) { return c == '\r' || c == '\n'; }

static const bool IsSpace(char c) { return absl::ascii_isspace(c); }

static const bool IsEscape(char c) { return c == '\\'; }

static const bool IsValidName(std::string_view name) {
  if (name.empty()) {
    return false;
  }
  if (!absl::ascii_isalpha(name.front())) {
    return false;
  }
  for (auto c : name) {
    if (!absl::ascii_isalnum(c) && c != '_') {
      return false;
    }
  }
  return true;
}

// Removes whitespace from around the text.
static std::string_view StripWhitespace(std::string_view txt) {
  int len = static_cast<int>(txt.length());
  const char* ptr = txt.data();
  while (len > 0 && IsSpace(ptr[0])) {
    ++ptr;
    --len;
  }
  while (len > 0 && IsSpace(ptr[len - 1])) {
    --len;
  }
  return std::string_view(ptr, len);
}

// Returns the first token in the text. A token is a sequence of non-whitespace
// characters, or any sequence of characters surrounded by quotes.
static std::string_view PeekToken(std::string_view txt) {
  size_t start = 0;
  while (start < txt.length() && IsSpace(txt[start])) {
    ++start;
  }
  if (start >= txt.length()) {
    return txt.substr(start);
  }

  if (IsQuote(txt[start])) {
    // Quoted string, keep reading until we hit the end quote.
    char quote = txt[start];
    for (std::size_t end = start + 1; end < txt.length(); ++end) {
      const char prev = txt[end - 1];
      const char curr = txt[end];
      if (curr == quote && !IsEscape(prev)) {
        // +1 to include the ending quote.
        return txt.substr(start, end - start + 1);
      }
    }
  } else {
    for (std::size_t end = start; end < txt.length(); ++end) {
      if (IsSpace(txt[end])) {
        return txt.substr(start, end - start);
      }
    }
  }
  // Hit EOF before able to read a full token, so treat the entirety of the
  // rest of the text as a token.
  return txt.substr(start);
}

static std::string_view ReadToken(std::string_view* txt) {
  *txt = StripWhitespace(*txt);
  std::string_view token = PeekToken(*txt);
  *txt = txt->substr(token.length());
  return token;
}

static std::string_view ReadLine(std::string_view* txt) {
  size_t index = 0;
  while (index < txt->length() && !IsNewline((*txt)[index])) {
    ++index;
  }
  std::string_view line = txt->substr(0, index);
  while (index < txt->length() && IsNewline((*txt)[index])) {
    ++index;
  }
  *txt = txt->substr(index);
  return line;
}

class DefParser {
 public:
  absl::StatusOr<DefDocument> Parse(std::string_view txt);

 private:
  void TryParseComment(std::string_view* txt);
  void ParseInclude(std::string_view* txt);
  void ParseNamespace(std::string_view* txt);
  void ParseStruct(std::string_view* txt);
  void ParseEnum(std::string_view* txt);

  DefDocument doc_;

  std::string active_namespace_;
  std::string comment_;
  absl::Status status_;
};

absl::StatusOr<DefDocument> DefParser::Parse(std::string_view txt) {
  txt = StripWhitespace(txt);
  while (!txt.empty() && status_.ok()) {
    TryParseComment(&txt);

    std::string_view token = PeekToken(txt);
    if (token == kInclude) {
      ParseInclude(&txt);
    } else if (token == kNamespace) {
      ParseNamespace(&txt);
    } else if (token == kEnum) {
      ParseEnum(&txt);
    } else if (token == kStruct) {
      ParseStruct(&txt);
    } else {
      status_ = absl::InternalError(absl::StrCat("Unknown token: ", token));
      break;
    }
  }

  if (!status_.ok()) {
    return status_;
  }
  return std::move(doc_);
}

void DefParser::TryParseComment(std::string_view* txt) {
  comment_.clear();

  std::string_view marker = PeekToken(*txt);
  while (marker == kComment) {
    // Consume the comment marker.
    ReadToken(txt);

    // Conume everything until the end of the line as part of the comment.
    std::string line(ReadLine(txt));

    // We drop the leading space because that's not really part of the comment
    // itself but rather there just for the purposes of formatting within a
    // def file.
    if (!line.empty() && line.front() == ' ') {
      line = line.substr(1);
    }

    comment_.append(line);
    comment_.append("\n");
    marker = PeekToken(*txt);
  }
}

void DefParser::ParseInclude(std::string_view* txt) {
  std::string_view directive = ReadToken(txt);
  CHECK(directive == kInclude);

  std::string_view path = ReadToken(txt);
  if (path.empty()) {
    status_ = absl::InternalError(
        absl::StrCat("Expected include path (in quotes): ", path));
    return;
  }
  doc_.includes.emplace_back(path);

  std::string line(StripWhitespace(ReadLine(txt)));
  if (!line.empty() && line.front() != '#') {
    status_ =
        absl::InternalError("Unexpected characters after include statement.");
  }
}

void DefParser::ParseNamespace(std::string_view* txt) {
  std::string_view directive = ReadToken(txt);
  CHECK(directive == kNamespace);

  std::string_view ns = ReadToken(txt);
  active_namespace_ = std::string(ns);

  std::string line(StripWhitespace(ReadLine(txt)));
  if (!line.empty() && line.front() != '#') {
    status_ =
        absl::InternalError("Unexpected characters after namespace statement.");
  }
}

void DefParser::ParseEnum(std::string_view* txt) {
  std::string_view directive = ReadToken(txt);
  CHECK(directive == kEnum);

  std::string_view enum_name = ReadToken(txt);
  if (enum_name.empty()) {
    status_ = absl::InternalError("Type name required for enum.");
    return;
  } else if (!IsValidName(enum_name)) {
    status_ =
        absl::InternalError(absl::StrCat("Invalid type name: ", enum_name));
    return;
  }

  EnumMetadata info;
  info.name = std::string(enum_name);
  info.description = comment_;
  info.name_space = active_namespace_;

  std::string_view marker = ReadToken(txt);
  if (marker != kStartScope) {
    status_ = absl::InternalError("Expected '{' after enum declaration");
    return;
  }

  while (true) {
    TryParseComment(txt);

    std::string_view name = ReadToken(txt);
    if (name == kEndScope) {
      break;
    } else if (name.empty()) {
      status_ = absl::InternalError("Expected an enumerator name.");
      return;
    }

    // Ignore trailing commas.
    if (name.back() == ',') {
      name = name.substr(0, name.length() - 1);
    }

    if (!IsValidName(name)) {
      status_ =
          absl::InternalError(absl::StrCat("Invalid enumerator name: ", name));
      return;
    }

    EnumeratorMetadata e;
    e.description = comment_;
    e.name = std::string(name);
    info.enumerators.push_back(std::move(e));
  }

  doc_.enums.push_back(std::move(info));
}

void DefParser::ParseStruct(std::string_view* txt) {
  std::string_view directive = ReadToken(txt);
  CHECK(directive == kStruct);

  std::string_view struct_name = ReadToken(txt);
  if (struct_name.empty()) {
    status_ = absl::InternalError("Type name required for struct.");
    return;
  } else if (!IsValidName(struct_name)) {
    status_ =
        absl::InternalError(absl::StrCat("Invalid type name: ", struct_name));
    return;
  }

  StructMetadata info;
  info.name = std::string(struct_name);
  info.description = comment_;
  info.name_space = active_namespace_;

  std::string_view marker = ReadToken(txt);
  if (marker != kStartScope) {
    status_ = absl::InternalError("Expected '{' after enum declaration");
    return;
  }

  while (true) {
    // name: type = value { .. }

    TryParseComment(txt);

    FieldMetadata f;
    f.description = comment_;

    std::string_view name = ReadToken(txt);
    if (name == kEndScope) {
      break;
    } else if (name.empty()) {
      status_ = absl::InternalError("Expected an field name.");
      return;
    }

    // Ignore trailing colons.
    if (name.back() == ':') {
      name = name.substr(0, name.length() - 1);
    }

    if (!IsValidName(name)) {
      status_ = absl::InternalError(absl::StrCat("Invalid field name: ", name));
      return;
    }

    f.name = std::string(name);

    std::string_view type = ReadToken(txt);
    if (type == ":") {
      type = ReadToken(txt);
    }

    if (type.empty()) {
      status_ = absl::InternalError(
          absl::StrCat("Expected a type for field: ", name));
      return;
    } else if (!IsValidName(type)) {
      status_ = absl::InternalError(absl::StrCat("Invalid type name: ", type));
      return;
    }

    f.type = std::string(type);

    std::string_view has_equal = PeekToken(*txt);
    if (has_equal == kEqual) {
      ReadToken(txt);
      std::string_view value = ReadToken(txt);
      if (value.empty()) {
        status_ = absl::InternalError(
            absl::StrCat("Expected value after '=' for field: ", name));
        return;
      }

      f.attributes[FieldMetadata::kDefaulValue] = std::string(value);
    }
    info.fields.push_back(f);
  }
  doc_.structs.push_back(std::move(info));
}

absl::StatusOr<DefDocument> ParseDefFile(std::string_view txt) {
  return DefParser().Parse(txt);
}

}  // namespace redux::tool
