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
// (+ [value] [value] ...)
//   Returns the sum of all arguments.  Only valid for integer or floating-point
//   types.
//
// (- [value] [value] ...)
//   Returns the difference of all arguments.  Only valid for integer or
//   floating-point types.
//
// (* [value] [value] ...)
//   Returns the multiplicative product of all arguments.  Only valid for
//   integer or floating-point types.
//
// (/ [value] [value] ...)
//   Returns the divided quotient of all arguments.  Only valid for integer or
//   floating-point types.
//
// (% [value] [value])
//   Returns the modulo of two arguments.  Valid only for integral types.

namespace lull {
namespace {

template <typename T, T (*Fn)(T, T)>
void Accumulate(ScriptFrame* frame, T value) {
  while (frame->HasNext()) {
    const ScriptValue next = frame->EvalNext();
    if (next.Is<T>()) {
      value = Fn(value, *next.Get<T>());
    } else {
      frame->Error("Type mismatch.");
    }
  }
  frame->Return(value);
}

template <typename T>
T AddFn(T lhs, T rhs) {
  return lhs + rhs;
}

template <typename T>
T SubFn(T lhs, T rhs) {
  return lhs - rhs;
}

template <typename T>
T MulFn(T lhs, T rhs) {
  return lhs * rhs;
}

template <typename T>
T DivFn(T lhs, T rhs) {
  return lhs / rhs;
}

template <typename T>
T ModFn(T lhs, T rhs) {
  return lhs % rhs;
}

void Add(ScriptFrame* frame) {
  ScriptValue value = frame->EvalNext();
  if (value.Is<int>()) {
    Accumulate<int, AddFn>(frame, *value.Get<int>());
  } else if (value.Is<float>()) {
    Accumulate<float, AddFn>(frame, *value.Get<float>());
  } else {
    frame->Error("Addition not supported for this type.");
  }
}

void Subtract(ScriptFrame* frame) {
  ScriptValue value = frame->EvalNext();
  if (value.Is<int>()) {
    Accumulate<int, SubFn>(frame, *value.Get<int>());
  } else if (value.Is<float>()) {
    Accumulate<float, SubFn>(frame, *value.Get<float>());
  } else {
    frame->Error("Subtraction not supported for this type.");
  }
}

void Multiply(ScriptFrame* frame) {
  ScriptValue value = frame->EvalNext();
  if (value.Is<int>()) {
    Accumulate<int, MulFn>(frame, *value.Get<int>());
  } else if (value.Is<float>()) {
    Accumulate<float, MulFn>(frame, *value.Get<float>());
  } else {
    frame->Error("Multiplication not supported for this type.");
  }
}

void Divide(ScriptFrame* frame) {
  ScriptValue value = frame->EvalNext();
  if (value.Is<int>()) {
    Accumulate<int, DivFn>(frame, *value.Get<int>());
  } else if (value.Is<float>()) {
    Accumulate<float, DivFn>(frame, *value.Get<float>());
  } else {
    frame->Error("Division not supported for this type.");
  }
}

void Modulo(ScriptFrame* frame) {
  ScriptValue value = frame->EvalNext();
  if (value.Is<int>()) {
    Accumulate<int, ModFn>(frame, *value.Get<int>());
  } else {
    frame->Error("Modulo not supported for this type.");
  }
}

LULLABY_SCRIPT_FUNCTION(Add, "+");
LULLABY_SCRIPT_FUNCTION(Divide, "/");
LULLABY_SCRIPT_FUNCTION(Modulo, "%");
LULLABY_SCRIPT_FUNCTION(Multiply, "*");
LULLABY_SCRIPT_FUNCTION(Subtract, "-");

}  // namespace
}  // namespace lull
