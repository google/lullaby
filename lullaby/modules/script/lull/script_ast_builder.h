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

#ifndef LULLABY_MODULES_SCRIPT_LULL_SCRIPT_AST_BUILDER_H_
#define LULLABY_MODULES_SCRIPT_LULL_SCRIPT_AST_BUILDER_H_

#include "lullaby/modules/script/lull/script_parser.h"
#include "lullaby/modules/script/lull/script_types.h"

namespace lull {

class ScriptEnv;

// ParserCallback that generates the abstract syntax tree (AST) representation.
class ScriptAstBuilder : public ParserCallbacks {
 public:
  explicit ScriptAstBuilder(ScriptEnv* env);

  // Creates an AstNode from the given |type| and associated data.
  void Process(TokenType type, const void* ptr, string_view token) override;

  // Returns the root of the AST from the processed data.
  AstNode GetRoot() const;

  // Sets the internal state to an error state.
  void Error(string_view token, string_view message) override;

 private:
  struct ScriptValueList {
    ScriptValue head;
    ScriptValue tail;
  };

  void Append(const ScriptValue& value, string_view token);
  void Push();
  void Pop();

  ScriptEnv* env_;
  std::vector<ScriptValueList> stack_;
  bool error_ = false;
};

}  // namespace lull

#endif  // LULLABY_MODULES_SCRIPT_LULL_SCRIPT_AST_BUILDER_H_
