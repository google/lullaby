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

#ifndef LULLABY_MODULES_SCRIPT_LULL_SCRIPT_COMPILER_H_
#define LULLABY_MODULES_SCRIPT_LULL_SCRIPT_COMPILER_H_

#include <vector>
#include "lullaby/modules/serialize/buffer_serializer.h"
#include "lullaby/modules/script/lull/script_parser.h"
#include "lullaby/modules/script/lull/script_types.h"
#include "lullaby/util/span.h"

namespace lull {

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
  explicit ScriptCompiler(ScriptByteCode* code);

  // Stores the |type| and associated data into the byte array buffer.
  void Process(TokenType type, const void* ptr, string_view token) override;

  // Processes the stored byte array buffer into another sequence of
  // ParserCallbacks.
  void Build(ParserCallbacks* builder);

  // Sets the internal state to an error state.
  void Error(string_view token, string_view message) override;

  // Determines if the specified array of bytes is actually ScriptByteCode.
  static bool IsByteCode(Span<uint8_t> bytes);

 private:
  ScriptByteCode* code_;
  SaveToBuffer writer_;
  bool error_ = false;
};

}  // namespace lull

#endif  // LULLABY_MODULES_SCRIPT_LULL_SCRIPT_COMPILER_H_
