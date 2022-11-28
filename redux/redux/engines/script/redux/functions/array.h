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

#ifndef REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_ARRAY_H_
#define REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_ARRAY_H_

#include "redux/engines/script/redux/script_env.h"

// This file implements the following script functions:
//
// (make-array [value] [value] ... )
//   Creates an array with the optional list of values. The values can be of
//   any supported type.
//
// (array-size [array])
//   Returns the number of elements in the array.
//
// (array-empty [array])
//   Returns true if the array is empty (ie. contains no elements).
//
// (array-push [array] [value])
//   Adds a new value to the end of the array.
//
// (array-pop [array])
//   Removes and returns a value from the end of the array (or does nothing if
//   the array is empty).
//
// (array-insert [array] [index] [value])
//   Inserts the value into the array at the given index, pushing all elements
//   after the index "backwards".  The index must be an integer type.
//
// (array-erase [array] [index])
//   Removes the element in the array at the specified index, moving all
//   elements after the index "forwards".  The index must be an integer type.
//
// (array-at [array] [index])
//   Returns the value at the specified index in the array.  The index must be
//   an integer type.
//
// (array-set [array] [index] [value])
//   Sets the value of the specified index in the array.  The index must be an
//   integer type.
//
// (array-foreach [array] ([index-name?] [value-name]) [expressions...])
//   Passes each element of the array to expressions with the value bound to
//   [value-name] and if supplied, index bound to [index-name].
//
// Note that redux script parsing uses [ and ] as a short-cut for make-array.
// In other words, the following are equivalent:
//   (make-array 1 2 3)
//   [1 2 3]

#include "redux/modules/var/var_array.h"

namespace redux {
namespace script_functions {

inline void ArrayCreateFn(ScriptFrame* frame) {
  VarArray array;
  while (frame->HasNext()) {
    array.PushBack(*frame->EvalNext().Get<Var>());
  }
  frame->Return(array);
}

inline int ArraySizeFn(const VarArray* array) {
  return static_cast<int>(array->Count());
}

inline bool ArrayEmptyFn(const VarArray* array) { return array->Count() == 0; }

inline void ArrayPushFn(VarArray* array, const Var* value) {
  array->PushBack(*value);
}

inline Var ArrayPopFn(VarArray* array) {
  Var value;
  if (array->Count() > 0) {
    value = array->At(array->Count() - 1);
    array->PopBack();
  }
  return value;
}

inline void ArrayInsertFn(VarArray* array, int index, const Var* value) {
  array->Insert(index, *value);
}

inline void ArrayEraseFn(VarArray* array, int index) { array->Erase(index); }

inline Var ArrayAtFn(const VarArray* array, int index) {
  return array->At(index);
}

inline Var ArraySetFn(VarArray* array, int index, const Var* value) {
  return (*array)[index] = *value;
}

inline void ArrayForEachFn(ScriptFrame* frame) {
  if (!frame->HasNext()) {
    frame->Error("array-foreach: expect [array] ([args]) [body].");
    return;
  }
  ScriptValue array_arg = frame->EvalNext();
  if (!array_arg.Is<VarArray>()) {
    frame->Error("array-foreach: first argument should be an array.");
    return;
  }
  const VarArray* array = array_arg.Get<VarArray>();

  const AstNode* node = frame->GetArgs().Get<AstNode>();
  if (!node) {
    frame->Error("array-foreach: expected parameters after array.");
    return;
  }

  const AstNode* params = node->first.Get<AstNode>();
  const Symbol* index = params ? params->first.Get<Symbol>() : nullptr;
  params = params && params->rest ? params->rest.Get<AstNode>() : nullptr;
  const Symbol* value = params ? params->first.Get<Symbol>() : nullptr;

  if (!index) {
    frame->Error("array-foreach: should be at least 1 symbol parameter");
    return;
  }
  if (!value) {
    // Only 1 parameter -- treat it as value.
    std::swap(index, value);
  }

  ScriptEnv* env = frame->GetEnv();

  // Now iterate the array elements, evaluating body.
  ScriptValue result;
  for (size_t i = 0; i < array->Count(); ++i) {
    if (index) {
      env->SetValue(index->value, static_cast<int>(i));
    }
    env->SetValue(value->value, array->At(i));

    const ScriptValue* iter = &node->rest;
    while (auto* node = iter->Get<AstNode>()) {
      result = env->Eval(*iter);
      // Maybe implement continue/break here?
      iter = &node->rest;
      if (iter->IsNil()) {
        break;
      }
    }
  }
  frame->Return(std::move(result));
}
}  // namespace script_functions
}  // namespace redux

#endif  // REDUX_ENGINES_SCRIPT_REDUX_FUNCTIONS_ARRAY_H_
