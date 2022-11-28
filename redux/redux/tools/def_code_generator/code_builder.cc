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

#include "redux/tools/def_code_generator/code_builder.h"

#include <string>
#include <vector>

#include "absl/strings/str_split.h"

namespace redux::tool {

void CodeBuilder::SetNamespace(std::string_view ns) {
  if (ns != namespace_) {
    if (!namespace_.empty()) {
      Append("}}  // namespace {0}", namespace_);
      AppendBlankLine();
    }
    namespace_ = ns;
    if (!namespace_.empty()) {
      Append("namespace {0} {{", namespace_);
      AppendBlankLine();
    }
  }
}

void CodeBuilder::Indent() { indent_ += "  "; }

void CodeBuilder::Deindent() {
  if (indent_.size() >= 2) {
    indent_.pop_back();
    indent_.pop_back();
  }
}

void CodeBuilder::AppendComment(std::string_view comment) {
  if (!comment.empty()) {
    const std::vector<std::string> lines =
        absl::StrSplit(std::string(comment), '\n');
    if (!lines.empty()) {
      for (size_t i = 0; i < lines.size() - 1; ++i) {
        buffer_ << indent_ << "// " << lines[i] << std::endl;
      }
    }
  }
}

void CodeBuilder::AppendBlankLine() { buffer_ << std::endl; }

std::string CodeBuilder::FlushToString() {
  SetNamespace("");
  std::string code = std::move(buffer_).str();
  buffer_.clear();
  indent_.clear();
  namespace_.clear();
  return code;
}

}  // namespace redux::tool
