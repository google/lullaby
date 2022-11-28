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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_AST_BUILDER_H_
#define REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_AST_BUILDER_H_

#include <vector>

#include "redux/engines/script/redux/script_parser.h"
#include "redux/engines/script/redux/script_types.h"

namespace redux {

// ParserCallback that generates the abstract syntax tree (AST) representation.
class ScriptAstBuilder : public ParserCallbacks {
 public:
  ScriptAstBuilder();

  // Creates an AstNode from the given |type| and associated data.
  void Process(TokenType type, const void* ptr,
               std::string_view token) override;

  // Returns the root of the AST from the processed data.
  const AstNode* GetRoot() const;

  // Sets the internal state to an error state.
  void Error(std::string_view token, std::string_view message) override;

 private:
  struct List {
    ScriptValue head;
    ScriptValue tail;
  };

  void Append(Var value, std::string_view token);
  void Push();
  void Pop();

  std::vector<List> stack_;
  bool error_ = false;
};

}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_SCRIPT_AST_BUILDER_H_
