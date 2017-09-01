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
// (array [value] [value] ... )
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

namespace lull {
namespace {

Optional<int> GetIndex(ScriptArgList* args) {
  Optional<int> result;
  ScriptValue key_value = args->EvalNext();
  if (key_value.Is<HashValue>()) {
    result = static_cast<int>(*key_value.Get<HashValue>());
  } else if (key_value.Is<int>()) {
    result = *key_value.Get<int>();
  }
  return result;
}

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

void ArraySize(ScriptFrame* frame) {
  const ScriptValue array_value = frame->EvalNext();
  const VariantArray* array = array_value.Get<VariantArray>();
  if (array) {
    frame->Return(static_cast<int>(array->size()));
  } else {
    frame->Error("array-size: expected array as first argument");
  }
}

void ArrayEmpty(ScriptFrame* frame) {
  const ScriptValue array_value = frame->EvalNext();
  const VariantArray* array = array_value.Get<VariantArray>();
  if (array) {
    frame->Return(array->empty());
  } else {
    frame->Error("array-empty: expected array as first argument");
  }
}

void ArrayPush(ScriptFrame* frame) {
  ScriptValue array_value = frame->EvalNext();
  VariantArray* array = array_value.Get<VariantArray>();
  if (!array) {
    frame->Error("array-push: expected array as first argument");
    return;
  }
  array->emplace_back(GetValue(frame));
}

void ArrayPop(ScriptFrame* frame) {
  ScriptValue array_value = frame->EvalNext();
  VariantArray* array = array_value.Get<VariantArray>();
  if (!array) {
    frame->Error("array-pop: expected array as first argument");
    return;
  }
  if (!array->empty()) {
    frame->Return(array->back());
    array->pop_back();
  }
}

void ArrayInsert(ScriptFrame* frame) {
  ScriptValue array_value = frame->EvalNext();
  VariantArray* array = array_value.Get<VariantArray>();
  if (!array) {
    frame->Error("array-insert: expected array as first argument");
    return;
  }
  Optional<int> index = GetIndex(frame);
  if (!index) {
    frame->Error("array-insert: expected index as second argument");
    return;
  }
  if (*index >= 0 && *index <= static_cast<int>(array->size())) {
    array->insert(array->begin() + *index, GetValue(frame));
  } else {
    frame->Error("array-insert: index out-of-bounds");
  }
}

void ArrayErase(ScriptFrame* frame) {
  ScriptValue array_value = frame->EvalNext();
  VariantArray* array = array_value.Get<VariantArray>();
  if (!array) {
    frame->Error("array-erase: expected array as first argument");
    return;
  }
  Optional<int> index = GetIndex(frame);
  if (!index) {
    frame->Error("array-erase: expected index as second argument");
    return;
  }
  if (*index >= 0 && *index < static_cast<int>(array->size())) {
    array->erase(array->begin() + *index);
  } else {
    frame->Error("array-erase: index out-of-bounds");
  }
}

void ArrayAt(ScriptFrame* frame) {
  const ScriptValue array_value = frame->EvalNext();
  const VariantArray* array = array_value.Get<VariantArray>();
  if (!array) {
    frame->Error("array-at: expected array as first argument");
    return;
  }
  Optional<int> index = GetIndex(frame);
  if (!index) {
    frame->Error("array-at: expected index as second argument");
    return;
  }
  if (*index >= 0 && *index < static_cast<int>(array->size())) {
    frame->Return(array->at(*index));
  } else {
    frame->Error("array-at: index out-of-bounds");
  }
}

LULLABY_SCRIPT_FUNCTION(ArrayCreate, "array");
LULLABY_SCRIPT_FUNCTION(ArraySize, "array-size");
LULLABY_SCRIPT_FUNCTION(ArrayEmpty, "array-empty");
LULLABY_SCRIPT_FUNCTION(ArrayPush, "array-push");
LULLABY_SCRIPT_FUNCTION(ArrayPop, "array-pop");
LULLABY_SCRIPT_FUNCTION(ArrayInsert, "array-insert");
LULLABY_SCRIPT_FUNCTION(ArrayErase, "array-erase");
LULLABY_SCRIPT_FUNCTION(ArrayAt, "array-at");

}  // namespace
}  // namespace lull
