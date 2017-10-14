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

namespace lull {
namespace {

Variant GetValue(ScriptArgList* args) {
  const ScriptValue value = args->EvalNext();
  return value.IsNil() ? Variant() : *value.GetVariant();
}

void ArrayCreate(ScriptFrame* frame) {
  VariantArray array;
  while (frame->HasNext()) {
    array.emplace_back(GetValue(frame));
  }
  frame->Return(array);
}

int ArraySize(const VariantArray* array) {
  return static_cast<int>(array->size());
}

bool ArrayEmpty(const VariantArray* array) { return array->empty(); }

void ArrayPush(VariantArray* array, const Variant* value) {
  array->emplace_back(*value);
}

Variant ArrayPop(VariantArray* array) {
  Variant value;
  if (!array->empty()) {
    value = array->back();
    array->pop_back();
  }
  return value;
}

void ArrayInsert(ScriptFrame* frame, VariantArray* array, int index,
                 const Variant* value) {
  if (index >= 0 && index <= static_cast<int>(array->size())) {
    array->insert(array->begin() + index, *value);
  } else {
    std::stringstream msg;
    msg << "array-insert: tried to insert an item at index " << index
        << " of an array of size " << array->size();
    frame->Error(msg.str().c_str());
  }
}

void ArrayErase(ScriptFrame* frame, VariantArray* array, int index) {
  if (index >= 0 && index < static_cast<int>(array->size())) {
    array->erase(array->begin() + index);
  } else {
    std::stringstream msg;
    msg << "array-erase: tried to erase an item at index " << index
        << " of an array of size " << array->size();
    frame->Error(msg.str().c_str());
  }
}

Variant ArrayAt(ScriptFrame* frame, const VariantArray* array, int index) {
  if (index >= 0 && index < static_cast<int>(array->size())) {
    return array->at(index);
  }
  std::stringstream msg;
  msg << "array-at: tried to get an item at index " << index
      << " of an array of size " << array->size();
  frame->Error(msg.str().c_str());
  return Variant();
}

Variant ArraySet(ScriptFrame* frame, VariantArray* array, int index,
                 const Variant* value) {
  if (index >= 0 && index < static_cast<int>(array->size())) {
    return (*array)[index] = *value;
  }
  std::stringstream msg;
  msg << "array-set: tried to set an item at index " << index
      << " of an array of size " << array->size();
  frame->Error(msg.str().c_str());
  return Variant();
}

void ArrayForeach(ScriptFrame* frame) {
  if (!frame->HasNext()) {
    frame->Error(
        "array-foreach: expect [array] [initial-value] ([args]) [body].");
    return;
  }
  ScriptValue array_arg = frame->EvalNext();
  if (!array_arg.Is<VariantArray>()) {
    frame->Error("array-foreach: first argument should be an array.");
    return;
  }
  const VariantArray* array = array_arg.Get<VariantArray>();

  const AstNode* node = frame->GetArgs().Get<AstNode>();
  if (!node) {
    frame->Error("array-foreach: expected parameters after array.");
    return;
  }

  ScriptEnv* env = frame->GetEnv();
  const AstNode* params = node->first.Get<AstNode>();
  const Symbol* index = params ? params->first.Get<Symbol>() : nullptr;
  params = params ? params->rest.Get<AstNode>() : nullptr;
  const Symbol* value = params ? params->first.Get<Symbol>() : nullptr;

  if (!index) {
    frame->Error("array-foreach: should be at least 1 symbol parameter");
    return;
  }
  if (!value) {
    // Only 1 parameter -- treat it as value.
    std::swap(index, value);
  }

  // Now iterate the array elements, evaluating body.
  ScriptValue result;
  for (size_t i = 0; i < array->size(); ++i) {
    if (index) {
      env->SetValue(*index, env->Create(static_cast<int>(i)));
    }
    env->SetValue(*value, env->Create(array->at(i)));
    ScriptValue iter = node->rest;
    while (auto* node = iter.Get<AstNode>()) {
      result = env->Eval(iter);
      // Maybe implement continue/break here?
      iter = node->rest;
    }
  }
  frame->Return(result);
}

LULLABY_SCRIPT_FUNCTION(ArrayCreate, "make-array");
LULLABY_SCRIPT_FUNCTION_WRAP(ArraySize, "array-size");
LULLABY_SCRIPT_FUNCTION_WRAP(ArrayEmpty, "array-empty");
LULLABY_SCRIPT_FUNCTION_WRAP(ArrayPush, "array-push");
LULLABY_SCRIPT_FUNCTION_WRAP(ArrayPop, "array-pop");
LULLABY_SCRIPT_FUNCTION_WRAP(ArrayInsert, "array-insert");
LULLABY_SCRIPT_FUNCTION_WRAP(ArrayErase, "array-erase");
LULLABY_SCRIPT_FUNCTION_WRAP(ArraySet, "array-set");
LULLABY_SCRIPT_FUNCTION_WRAP(ArrayAt, "array-at");
LULLABY_SCRIPT_FUNCTION(ArrayForeach, "array-foreach");

}  // namespace
}  // namespace lull
