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

#include "lullaby/viewer/src/jsonnet_writer.h"

namespace lull {
namespace tool {

JsonnetWriter::JsonnetWriter(int indent_level) : indent_level_(indent_level) {
}

std::string JsonnetWriter::ToString() const {
  return ss_.str();
}

void JsonnetWriter::Code(string_view str) {
  NewLine();
  ss_ << str;
}

void JsonnetWriter::Field(string_view name) {
  NewLine();
  ss_ << name << ": ";
}

void JsonnetWriter::BeginMap() {
  NewLine();
  ss_ << '{';
  stack_.push_back('}');
  ++indent_level_;
}

void JsonnetWriter::BeginArray() {
  NewLine();
  ss_ << '[';
  stack_.push_back(']');
  ++indent_level_;
}

void JsonnetWriter::EndMap(string_view comment) {
  if (!stack_.empty() && stack_.back() == '}') {
    stack_.pop_back();
    --indent_level_;
    NewLine();
    if (indent_level_ == 0) {
      ss_ << "}";
    } else {
      ss_ << "},";
    }
    if (comment.length()) {
      ss_ << "  // " << comment;
    }
  }
}

void JsonnetWriter::EndArray(string_view comment) {
  if (!stack_.empty() && stack_.back() == ']') {
    stack_.pop_back();
    --indent_level_;
    NewLine();
    ss_ << "],";
    if (comment.length()) {
      ss_ << "  // " << comment;
    }
  }
}

void JsonnetWriter::NewLine() {
  ss_ << std::endl;
  for (int i = 0; i < indent_level_; ++i) {
    ss_ << "  ";
  }
}

}  // namespace tool
}  // namespace lull
