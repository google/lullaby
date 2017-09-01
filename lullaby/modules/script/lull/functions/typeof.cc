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

#include "lullaby/modules/script/lull/functions/functions.h"
#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/modules/script/lull/script_frame.h"
#include "lullaby/modules/script/lull/script_types.h"

// This file implements the following script functions:
//
// (typeof [value])
//   Returns the TypeId of the value.
//
// (is [value] [symbol])
//   Returns true if the specified value is of the same type as identified by
//   the symbol.  For example, '(is 1.0 float)' will return true.

namespace lull {
namespace {

void TypeOf(ScriptFrame* frame) {
  frame->Return(frame->EvalNext().GetTypeId());
}

void Is(ScriptFrame* frame) {
  bool match = false;
  const TypeId lhs = frame->EvalNext().GetTypeId();

  ScriptValue type_value = frame->Next();
  if (const AstNode* node = type_value.Get<AstNode>()) {
    if (const Symbol* symbol = node->first.Get<Symbol>()) {
      match = (lhs == symbol->value);
    }
  }
  frame->Return(match);
}

LULLABY_SCRIPT_FUNCTION(Is, "is");
LULLABY_SCRIPT_FUNCTION(TypeOf, "typeof");

}  // namespace
}  // namespace lull
