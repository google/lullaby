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
// (== [lhs] [rhs])
//   Returns true if two arguments have the same value.  Only valid for integer
//   or floating-point types.
//
// (!= [lhs] [rhs])
//   Returns true if two arguments have different values.  Only valid for
//   integer or floating-point types.
//
// (< [lhs] [rhs])
//   Returns true if first argument's value is less than the second argument's
//   value.  Only valid for integer or floating-point types.
//
// (> [lhs] [rhs])
//   Returns true if first argument's value is greater than the second
//   argument's value.  Only valid for integer or floating-point types.
//
// (<= [lhs] [rhs])
//   Returns true if first argument's value is less than or equal to the second
//   argument's value.  Only valid for integer or floating-point types.
//
// (>= [lhs] [rhs])
//   Returns true if first argument's value is greater than or equal to the
//   second argument's value.  Only valid for integer or floating-point types.

namespace lull {
namespace {

struct ComparisonArgs {
  explicit ComparisonArgs(ScriptFrame* frame) : frame(frame) {
    lhs = frame->EvalNext();
    rhs = frame->EvalNext();
    type = lhs.GetTypeId();
    if (frame->HasNext()) {
      frame->Error("Comparison operator should only have two args.");
    }
    if (type != rhs.GetTypeId()) {
      frame->Error("Both types for comparison should be the same.");
      type = 0;
    }
  }

  template <typename T>
  bool Is() const {
    return type == GetTypeId<T>();
  }

  template <typename T, bool (*Fn)(T, T)>
  void Check() {
    auto ptr1 = lhs.Get<T>();
    auto ptr2 = rhs.Get<T>();
    if (ptr1 && ptr2) {
      frame->Return(Fn(*ptr1, *ptr2));
    }
  }

  TypeId type;
  ScriptFrame* frame;
  ScriptValue lhs;
  ScriptValue rhs;
};

template <typename T>
bool EqFn(T lhs, T rhs) {
  return lhs == rhs;
}

template <typename T>
bool NeFn(T lhs, T rhs) {
  return lhs != rhs;
}

template <typename T>
bool GtFn(T lhs, T rhs) {
  return lhs > rhs;
}

template <typename T>
bool LtFn(T lhs, T rhs) {
  return lhs < rhs;
}

template <typename T>
bool GeFn(T lhs, T rhs) {
  return lhs >= rhs;
}

template <typename T>
bool LeFn(T lhs, T rhs) {
  return lhs <= rhs;
}

void Equal(ScriptFrame* frame) {
  ComparisonArgs args(frame);
  if (args.Is<int>()) {
    args.Check<int, EqFn>();
  } else if (args.Is<float>()) {
    args.Check<float, EqFn>();
  } else {
    frame->Error("Comparison not supported for this type.");
  }
}

void NotEqual(ScriptFrame* frame) {
  ComparisonArgs args(frame);
  if (args.Is<int>()) {
    args.Check<int, NeFn>();
  } else if (args.Is<float>()) {
    args.Check<float, NeFn>();
  } else {
    frame->Error("Comparison not supported for this type.");
  }
}

void LessThan(ScriptFrame* frame) {
  ComparisonArgs args(frame);
  if (args.Is<int>()) {
    args.Check<int, LtFn>();
  } else if (args.Is<float>()) {
    args.Check<float, LtFn>();
  } else {
    frame->Error("Comparison not supported for this type.");
  }
}

void GreaterThan(ScriptFrame* frame) {
  ComparisonArgs args(frame);
  if (args.Is<int>()) {
    args.Check<int, GtFn>();
  } else if (args.Is<float>()) {
    args.Check<float, GtFn>();
  } else {
    frame->Error("Comparison not supported for this type.");
  }
}

void LessThanOrEqual(ScriptFrame* frame) {
  ComparisonArgs args(frame);
  if (args.Is<int>()) {
    args.Check<int, LeFn>();
  } else if (args.Is<float>()) {
    args.Check<float, LeFn>();
  } else {
    frame->Error("Comparison not supported for this type.");
  }
}

void GreaterThanOrEqual(ScriptFrame* frame) {
  ComparisonArgs args(frame);
  if (args.Is<int>()) {
    args.Check<int, GeFn>();
  } else if (args.Is<float>()) {
    args.Check<float, GeFn>();
  } else {
    frame->Error("Comparison not supported for this type.");
  }
}

LULLABY_SCRIPT_FUNCTION(Equal, "==");
LULLABY_SCRIPT_FUNCTION(GreaterThan, ">");
LULLABY_SCRIPT_FUNCTION(GreaterThanOrEqual, ">=");
LULLABY_SCRIPT_FUNCTION(LessThan, "<");
LULLABY_SCRIPT_FUNCTION(LessThanOrEqual, "<=");
LULLABY_SCRIPT_FUNCTION(NotEqual, "!=");

}  // namespace
}  // namespace lull
