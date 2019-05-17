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

#ifndef LULLABY_VIEWER_SRC_JSONNET_WRITER_H_
#define LULLABY_VIEWER_SRC_JSONNET_WRITER_H_

#include <sstream>
#include <string>
#include "lullaby/util/string_view.h"

namespace lull {
namespace tool {

class JsonnetWriter {
 public:
  explicit JsonnetWriter(int indent_level = 0);
  std::string ToString() const;

  void Code(string_view str);

  void Field(string_view name);

  template <typename T>
  void FieldAndValue(string_view field, const T& value, bool add_quotes = false) {
    Field(field);
    Value(value, add_quotes);
  }

  template <typename T>
  void Value(const T& value, bool add_quotes = false) {
    if (stack_.back() == ']') {
      NewLine();
    }
    if (add_quotes) {
      ss_ << "\"" << value << "\",";
    } else {
      ss_ << value << ",";
    }
  }

  void BeginMap();
  void BeginArray();
  void EndArray(string_view comment = string_view());
  void EndMap(string_view comment = string_view());

 private:
  void NewLine();

  std::stringstream ss_;
  std::vector<char> stack_;
  int indent_level_ = 0;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_VIEWER_SRC_JSONNET_WRITER_H_
