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

#ifndef REDUX_TOOLS_DEF_CODE_GENERATOR_CODE_BUILDER_H_
#define REDUX_TOOLS_DEF_CODE_GENERATOR_CODE_BUILDER_H_

#include <sstream>
#include <string>
#include <string_view>

#include "fmt/format.h"

namespace redux::tool {

// Like a stringstream, but with utilities specifically for generating code
// (such as tracking indent levels and adding comments).
class CodeBuilder {
 public:
  // Appends a line to the stream, using libfmt-type formatting. Automatically
  // adds a line-return to the end the line.
  template <typename... T>
  void Append(fmt::format_string<T...> format_string, T&&... args) {
    std::string str = fmt::format(format_string, std::forward<T>(args)...);
    buffer_ << indent_ << str << std::endl;
  }

  // Appends a blank line.
  void AppendBlankLine();

  // Adds a line as a comment (with the appropriate comment marker prefix).
  void AppendComment(std::string_view comment);

  // Increases the indentation level by one. All lines added after this point
  // will be indented.
  void Indent();

  // Decreases the indentation level by one.
  void Deindent();

  // Sets the active namespace for all subsequent code that will be appended.
  void SetNamespace(std::string_view ns);

  // Returns a string containing the generated code and clears the contents of
  // the internal buffer.
  std::string FlushToString();

 private:
  std::stringstream buffer_;
  std::string namespace_;
  std::string indent_;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_DEF_CODE_GENERATOR_CODE_BUILDER_H_
