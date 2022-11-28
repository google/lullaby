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

#ifndef REDUX_MODULES_DATAFILE_DATAFILE_PARSER_H_
#define REDUX_MODULES_DATAFILE_DATAFILE_PARSER_H_

#include <string_view>

namespace redux {

// Callbacks invoked during parsing of data files; see ParseDatafile() below.
class DatafileParserCallbacks {
 public:
  virtual ~DatafileParserCallbacks() = default;
  virtual void Key(std::string_view value) = 0;
  virtual void BeginObject() = 0;
  virtual void EndObject() = 0;
  virtual void BeginArray() = 0;
  virtual void EndArray() = 0;
  virtual void Null() = 0;
  virtual void Boolean(bool value) = 0;
  virtual void Number(double value) = 0;
  virtual void String(std::string_view value) = 0;
  virtual void Expression(std::string_view value) = 0;
  virtual void ParseError(std::string_view context,
                          std::string_view message) = 0;
};

// Parses the given text for tokens and invokes the appropriate callback.
//
// The parser ignores whitespace and comments as defined by the format.
void ParseDatafile(std::string_view text, DatafileParserCallbacks* cb);

}  // namespace redux

#endif  // REDUX_MODULES_DATAFILE_DATAFILE_PARSER_H_
