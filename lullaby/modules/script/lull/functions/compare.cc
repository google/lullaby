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
// (or [args...])
//   Returns true if any of the arguments is true, false otherwise.
//
// (and [args...])
//   Returns false if any of the arguments is false, true otherwise.
//
// (not [arg])
//   Returns false if arg is true or true if arg is false.
//

namespace lull {
namespace {

void And(ScriptFrame* frame) {
  bool result = true;
  while (result && frame->HasNext()) {
    ScriptValue arg = frame->EvalNext();
    if (!arg.Is<bool>()) {
      frame->Error("and: argument should have type bool.");
      return;
    }
    result &= *arg.Get<bool>();
  }
  frame->Return(result);
}

void Or(ScriptFrame* frame) {
  bool result = false;
  while (!result && frame->HasNext()) {
    ScriptValue arg = frame->EvalNext();
    if (!arg.Is<bool>()) {
      frame->Error("or: argument should have type bool.");
      return;
    }
    result |= *arg.Get<bool>();
  }
  frame->Return(result);
}

bool Not(bool arg) { return !arg; }

LULLABY_SCRIPT_FUNCTION(And, "and");
LULLABY_SCRIPT_FUNCTION(Or, "or");
LULLABY_SCRIPT_FUNCTION_WRAP(Not, "not");

}  // namespace
}  // namespace lull
