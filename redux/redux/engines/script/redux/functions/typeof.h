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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_TYPEOF_H_
#define REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_TYPEOF_H_

#include "redux/engines/script/redux/script_env.h"

// This file implements the following script functions:
//
// (nil? [value])
//   Returns true if the variant is empty.
//
// (typeof [value])
//   Returns the TypeId of the value.
//
// (is [value] [symbol])
//   Returns true if the specified value is of the same type as identified by
//   the symbol.  For example, '(is 1.0 float)' will return true.

namespace redux {
namespace script_functions {

inline void IsNilFn(ScriptFrame* frame) {
  const ScriptValue value = frame->EvalNext();
  frame->Return(value.IsNil());
}

inline void TypeOfFn(ScriptFrame* frame) {
  const ScriptValue value = frame->EvalNext();
  frame->Return(value.GetTypeId());
}

inline void IsFn(ScriptFrame* frame) {
  bool match = false;
  const TypeId lhs = frame->EvalNext().GetTypeId();

  const ScriptValue& type_value = frame->Next();
  if (const AstNode* node = type_value.Get<AstNode>()) {
    if (const Symbol* symbol = node->first.Get<Symbol>()) {
      match = (lhs == symbol->value.get());
    }
  }
  frame->Return(match);
}
}  // namespace script_functions
}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_TYPEOF_H_
