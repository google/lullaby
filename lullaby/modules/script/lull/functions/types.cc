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

#define CONVERT(U, name)                                      \
  Variant Convert_##U(ScriptFrame* frame, const Variant* v) { \
    Variant w = v->NumericCast<U>();                          \
    if (w.Empty()) {                                          \
      frame->Error("Can't cast arg to " name);                \
    }                                                         \
    return w;                                                 \
  }                                                           \
  LULLABY_SCRIPT_FUNCTION_WRAP(Convert_##U, name)

// Types and macros to implement a "constructor" for all numeric types.
CONVERT(int32_t, "int32")
CONVERT(uint32_t, "uint32")
CONVERT(uint64_t, "uint64")
CONVERT(int8_t, "int8")
CONVERT(uint8_t, "uint8")
CONVERT(int16_t, "int16")
CONVERT(uint16_t, "uint16")
CONVERT(int64_t, "int64")
CONVERT(float, "float")
CONVERT(double, "double")

#undef CONVERT

}  // namespace
}  // namespace lull
