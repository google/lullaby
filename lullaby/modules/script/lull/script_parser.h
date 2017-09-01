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
  enum TokenType {
    kEof,        // End of parsing stream.
    kPush,       // Start of a new scope block, eg. "("
    kPop,        // End of a scope block, eg. ")"
    kPushArray,  // Start of a new array block, eg. "["
    kPopArray,   // End of an array block, eg. "]"
    kPushMap,    // Start of a new map block, eg. "{"
    kPopMap,     // End of a map block, eg. "}"
    kBool,       // A boolean constant, eg. "true" or "false"
    kInt8,       // An 8-bit signed integral constant.
    kUint8,      // An 8-bit unsigned integral constant.
    kInt16,      // A 16-bit signed integral constant.
    kUint16,     // A 16-bit unsigned integral constant.
    kInt32,      // A 32-bit signed integral constant, eg. "123"
    kUint32,     // A 32-bit unsigned integral constant, eg. "123u"
    kInt64,      // A 64-bit signed integral constant, eg. "123l"
    kUint64,     // A 64-bit unsigned integral constant, eg. "123ul"
    kFloat,      // A 32-bit floating-point constant, eg. "123.456f"
    kDouble,     // A 64-bit floating-point constant, eg. "123.456"
    kHashValue,  // A hash of a string literal that was prefixed with a colon.
    kString,     // A literal string constant (eg. text enclosed in either
                 // single- or double-quotes.
    kSymbol,     // A hash of a string literal, basically anything that isn't
                 // one of the above.
  };

  virtual ~ParserCallbacks() {}
  virtual void Process(TokenType type, const void* ptr, string_view token) = 0;
  virtual void Error(string_view token, string_view message) = 0;
};

// Parses LullScript source code, invoking the callback function as individual
// tokens are extracted.
void ParseScript(string_view source, ParserCallbacks* callbacks);

}  // namespace lull

#endif  // LULLABY_MODULES_SCRIPT_LULL_SCRIPT_PARSER_H_
