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

#ifndef LULLABY_MODULES_SCRIPT_LULL_SCRIPT_PARSER_H_
#define LULLABY_MODULES_SCRIPT_LULL_SCRIPT_PARSER_H_

#include "lullaby/util/string_view.h"

namespace lull {

// Interface that provides a function that will be called during the parsing
// of a script source code.
struct ParserCallbacks {
  enum CodeType {
    kEof,
    kNil,
    kPush,
    kPop,
    kBool,
    kInt,
    kFloat,
    kSymbol,
    kString,
  };

  virtual ~ParserCallbacks() {}
  virtual void Process(CodeType type, const void* ptr, string_view token) = 0;
  virtual void Error(string_view token, string_view message) = 0;
};

// Parses LullScript source code, invoking the callback function as individual
// tokens are extracted.
void ParseScript(string_view source, ParserCallbacks* callbacks);

}  // namespace lull

#endif  // LULLABY_MODULES_SCRIPT_LULL_SCRIPT_PARSER_H_
