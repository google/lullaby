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

#include "lullaby/tools/shader_pipeline/process_shader_source.h"

#include <string>
#include "lullaby/util/logging.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/string_view.h"
#include "lullaby/tools/common/file_utils.h"

namespace lull {
namespace tool {
namespace {
Optional<std::string> LoadIncludeFile(string_view filename) {
  const std::string filename_string(filename);
  std::string file_content;
  if (!LoadFile(filename_string.c_str(), false, &file_content)) {
    return NullOpt;
  }

  // Ensure there's a linefeed at eof.
  if (!file_content.empty() && file_content.back() != '\n') {
    file_content.append("\n");
  }

  // Process the coder in place.
  if (ProcessShaderSource(&file_content)) {
    return file_content;
  }
  return NullOpt;
}

size_t FindWhitespaceEnd(const std::string& string, size_t offset) {
  // Whitespace which doesn't end a line: space, horizontal & vertical tabs.
  // GLSL ES spec: https://www.khronos.org/files/opengles_shading_language.pdf
  return string.find_first_not_of(" \t\v", offset);
}

size_t FindNextLine(const std::string& string, size_t offset) {
  // Newlines are \n, \r, \r\n or \n\r, except when immediately preceded by a
  // backslash.
  static const char kNewlineCharacterSet[] = "\n\r";

  // Find offset to the next newline character.
  offset = string.find_first_of(kNewlineCharacterSet, offset);
  if (offset != std::string::npos) {
    // If the previous character is a backslash, keep going.
    while (string[offset - 1] == '\\') {
      // Find the offset beyond the newline character.
      offset = string.find_first_not_of(kNewlineCharacterSet, offset);
      // Find offset to the next newline character.
      offset = string.find_first_of(kNewlineCharacterSet, offset);
    }
  }
  // Find offset beyond the newline character.
  offset = string.find_first_not_of(kNewlineCharacterSet, offset);
  return offset;
}
}  // namespace

bool ProcessShaderSource(std::string* source) {
  static const std::string kIncludeStatement = "#include";

  if (!source) {
    LOG(ERROR) << "ProcessShaderSourceHelper called without source value.";
    return false;
  }

  size_t cursor = 0;
  // Parse only lines that are include statements, skipping everything else.
  while (cursor != std::string::npos) {
    // Skip white spaces which mean nothing.
    cursor = FindWhitespaceEnd(*source, cursor);
    // See if the new line starts with an include statement.
    if (source->compare(cursor, kIncludeStatement.size(), kIncludeStatement) ==
        0) {
      const size_t include_pos = cursor;
      cursor += kIncludeStatement.size();

      // Find quotes including the include file name.
      cursor = FindWhitespaceEnd(*source, cursor);
      if ((*source)[cursor] == '\"') {
        ++cursor;

        // Find the end quote.
        const size_t quote_end_pos = source->find_first_of("\"\n\r", cursor);
        if ((*source)[quote_end_pos] == '\"') {
          // Filename for the include.
          const std::string include_filename =
              source->substr(cursor, quote_end_pos - cursor);
          // Remove the include statement.
          source->erase(include_pos, quote_end_pos - include_pos + 1);

          // Load and parse the include file.
          Optional<std::string> include_string =
              LoadIncludeFile(include_filename);
          if (!include_string) {
            LOG(ERROR) << "Failed to include file: " << include_filename;
            return false;
          }

          source->insert(include_pos, *include_string);
          cursor = include_pos + include_string->size();
        }
      }
    }
    // Move to the next line.
    cursor = FindNextLine(*source, cursor);
  }

  return true;
}

}  // namespace tool
}  // namespace lull
