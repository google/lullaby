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

#ifndef REDUX_MODULES_FLATBUFFERS_VAR_H_
#define REDUX_MODULES_FLATBUFFERS_VAR_H_

#include <cstddef>
#include <vector>

#include "redux/modules/base/data_buffer.h"
#include "redux/modules/flatbuffers/var_generated.h"
#include "redux/modules/var/var.h"
#include "redux/modules/var/var_array.h"
#include "redux/modules/var/var_table.h"

namespace redux {

// Converts a VarArrayDef into a VarArray.
bool TryReadFbs(const fbs::VarArrayDef* in, VarArray* out);

// Converts a VarTableDef into a VarTable.
bool TryReadFbs(const fbs::VarTableDef* in, VarTable* out);

// Because a VarDef is a flatbuffer union, we need to pass the type separately
// from the data payload which is represented as a void pointer.
bool TryReadFbs(fbs::VarDef type, const void* in, Var* out);

// VarDefs that store DataBytes will be converted into ByteVectors using the
// above ReadFbs functions.
using ByteVector = std::vector<std::byte>;

// Converts the VarDef type to a string (useful for debugging).
inline const char* ToString(fbs::VarDef e) { return fbs::EnumNameVarDef(e); }

}  // namespace redux

REDUX_SETUP_TYPEID(redux::ByteVector);

#endif  // REDUX_MODULES_FLATBUFFERS_VAR_H_
