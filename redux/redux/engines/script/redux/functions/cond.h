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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_COND_H_
#define REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_COND_H_

#include "redux/engines/script/redux/script_env.h"

// This file implements the following script functions:
//
// (cond ([condition] [statements]...)
//       ([condition] [statements]...)
//       ...
// )
//   Executes the statements associated with the first condition that is true.
//
// (if [condition] [true-statement] [false-statement])
//   Conditionally evaluates one of two statements based on a boolean condition.
namespace redux {
namespace script_functions {

inline void CondFn(ScriptFrame* frame) {
  while (frame->HasNext()) {
    const ScriptValue& arg = frame->Next();
    const AstNode* node = arg.Get<AstNode>();
    if (!node) {
      frame->Error("Expected AST Node.");
      return;
    }

    ScriptFrame inner(frame->GetEnv(), node->first);
    ScriptValue cond_value = inner.EvalNext();

    const bool* cond = cond_value.Get<bool>();
    if (cond && *cond) {
      while (inner.HasNext()) {
        frame->Return(inner.EvalNext());
      }
      return;
    }
  }
}

inline void IfFn(ScriptFrame* frame) {
  ScriptValue value = frame->EvalNext();
  const bool* condition = value.Get<bool>();
  if (condition && *condition) {
    // Evaluate the first path.
    if (frame->HasNext()) {
      frame->Return(frame->EvalNext());
    }
    // Skip the other path.
    if (frame->HasNext()) {
      frame->Next();
    }
  } else {
    // Skip the first path.
    if (frame->HasNext()) {
      frame->Next();
    }
    // Evaluate the second path.
    if (frame->HasNext()) {
      frame->Return(frame->EvalNext());
    }
  }

  if (frame->HasNext()) {
    frame->Error("if: should only have two paths.");
  }
}
}  // namespace script_functions
}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_COND_H_
