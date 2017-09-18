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

#ifndef LULLABY_SCRIPT_LULL_FUNCTIONS_CONVERTER_H_
#define LULLABY_SCRIPT_LULL_FUNCTIONS_CONVERTER_H_

#include "lullaby/modules/script/lull/script_value.h"
#include "lullaby/util/typeid.h"

namespace lull {
namespace detail {

// ScriptConverter is a wrapper around the normal Variant based conversion to
// allow some special cases.
template <typename T>
struct ScriptConverter {
  static bool Convert(ScriptValue src, T* dest) {
    T* value_ptr = src.Get<T>();
    if (!value_ptr) return false;
    *dest = *value_ptr;
    return true;
  }

  static const char* TypeName() { return GetTypeName<T>(); }
};

// Pointer overloads allow modifying the input value, or just avoiding copies.
// For example, in the map and array manipulation functions.
template <typename T>
struct ScriptConverter<T*> {
  static bool Convert(ScriptValue src, T** dest) {
    T* value_ptr = src.Get<T>();
    if (!value_ptr) return false;
    *dest = value_ptr;
    return true;
  }

  static const char* TypeName() { return GetTypeName<T>(); }
};

template <typename T>
struct ScriptConverter<const T*> {
  static bool Convert(ScriptValue src, const T** dest) {
    T* value_ptr = src.Get<T>();
    if (!value_ptr) return false;
    *dest = value_ptr;
    return true;
  }

  static const char* TypeName() { return GetTypeName<T>(); }
};

// Variant overload allows passing in any type as a function argument. For
// example in the map and array manipulation functions.
template <>
struct ScriptConverter<const Variant*> {
  static bool Convert(const ScriptValue& src, const Variant** dest) {
    const Variant* value_ptr = src.GetVariant();
    if (!value_ptr) return false;
    *dest = value_ptr;
    return true;
  }

  static const char* TypeName() { return "any type"; }
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SCRIPT_LULL_FUNCTIONS_CONVERTER_H_
