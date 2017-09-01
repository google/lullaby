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
// (int8 [number])
//   Casts the number to a int8_t.
//
// (uint8 [number])
//   Casts the number to a uint8_t.
//
// (int16 [number])
//   Casts the number to a int16_t.
//
// (uint16 [number])
//   Casts the number to a uint16_t.
//
// (int32 [number])
//   Casts the number to a int32_t.
//
// (uint32 [number])
//   Casts the number to a uint32_t.
//
// (int64 [number])
//   Casts the number to a int64_t.
//
// (uint64 [number])
//   Casts the number to a uint64_t.
//
// (float [number])
//   Casts the number to a float.
//
// (double [number])
//   Casts the number to a double.
//

namespace lull {
namespace {

// Helper function that evaluates the first argument in the frame and attempts
// to perform a NumericCast on the value for type T.
template <typename T>
void DoConvert(ScriptFrame* frame) {
  const ScriptValue value = frame->EvalNext();
  auto res = value.NumericCast<T>();
  if (res) {
    frame->Return(*res);
  } else {
    frame->Error("Could not convert value.");
  }
}

// Types and macros to implement a "constructor" for all numeric types.
using float_t = float;
using double_t = double;
#define TYPES  \
  TYPE(int32)  \
  TYPE(float)  \
  TYPE(uint32) \
  TYPE(uint64) \
  TYPE(int8)   \
  TYPE(uint8)  \
  TYPE(int16)  \
  TYPE(uint16) \
  TYPE(int64)  \
  TYPE(double)

#define TYPE(U)                                                     \
  void Convert_##U(ScriptFrame* frame) { DoConvert<U##_t>(frame); } \
  LULLABY_SCRIPT_FUNCTION(Convert_##U, #U);
TYPES
#undef TYPE

}  // namespace
}  // namespace lull
