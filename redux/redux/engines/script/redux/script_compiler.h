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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_COMPILER_H_
#define REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_COMPILER_H_

#include <vector>

#include "redux/engines/script/redux/script_parser.h"
#include "redux/engines/script/redux/script_types.h"

namespace redux {

class ScriptEnv;

// ParserCallback that generates a binary block of data that can then saved to
// disk or built into the AST.
//
// Specifically, source code can be compiled into a byte array by passing a
// ScriptCompiler to the ParseScript function as part of the build process.
// The byte array can then be converted (again using the ScriptCompiler) to
// the appropriate runtime structure by calling ScriptCompiler::Build and
// passing it another set of ParserCallbacks.
class ScriptCompiler : public ParserCallbacks {
 public:
  using CodeBuffer = std::vector<uint8_t>;
  explicit ScriptCompiler(CodeBuffer* code);

  // Stores the |type| and associated data into the byte array buffer.
  void Process(TokenType type, const void* ptr,
               std::string_view token) override;

  // Processes the stored byte array buffer into another sequence of
  // ParserCallbacks.
  void Build(ParserCallbacks* builder);

  // Sets the internal state to an error state.
  void Error(std::string_view token, std::string_view message) override;

  // Determines if the specified array of bytes is actually ScriptByteCode.
  static bool IsByteCode(absl::Span<const uint8_t> bytes);

 private:
  CodeBuffer* code_ = nullptr;
  bool error_ = false;
};

}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_COMPILER_H_
