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

#ifndef LULLABY_UTIL_TYPEID_H_
#define LULLABY_UTIL_TYPEID_H_

#include <assert.h>
#include "lullaby/util/hash.h"

// A type identification system.
//
// Any C++ data type can be registered with the system by adding
// LULLABY_SETUP_TYPEID(Type) after its declaration.  This macro uses the Hash
// function on the Type's name to generate a unique ID is created for each
// registered type.  A presubmit script (check_for_collisions.py) is run to
// ensure that no hash collisions exist for any specified types.
//
// Call GetTypeId<T>() to get the TypeId of a registered type.  Likewise, call
// GetTypeName<T>() to get the name of a registered type.
//
// Use the TypeId system to store and access objects in a type-safe manner.
// A typical pattern is to use a pair of TypeId and void*. Then the TypeId can
// be used to check that the pointer points to an object of the correct type,
// before casting it to that type.
//
// The TypeId system is not a complete replacement for C++ RTTI.  Specifically,
// it does not provide any dynamic_cast-like functionality.  However, it does
// have a few benefits:
//
// TypeId is safe to serialize because it is a hash of the type's name.  The
// C++ std::type_info is not serializable because its value is not guaranteed
// to be the same every time.
//
// The TypeId system is opt-in.  Only types that are explicitly registered with
// the LULLABY_SETUP_TYPEID macro can be used when calling GetTypeId<T>().  As a
// result, the TypeId system does not need to be enabled/disabled at a compiler
// level.
//
// The TypeId is not stored in each object (requiring each object instance to
// have a vtable pointer).  Instead, TypeId can be stored externally to the
// object with which it belongs on an as-needed basis.
//
// TODO(b/26013482): The presubmit script needs to be re-introduced and fixed to
// be able to check for collisions without specifying the hash value in the
// macro.

namespace lull {

using TypeId = unsigned int;

template<typename T>
const char* GetTypeName() {
  assert(false);
  // If you get a compiler error that complains about TYPEID_NOT_SETUP, then
  // you have not added LULLABY_SETUP_TYPEID(T) such that the compilation unit
  // being compiled uses it.
  return T::TYPEID_NOT_SETUP;
}

template<typename T>
TypeId GetTypeId() {
  assert(false);
  return T::TYPEID_NOT_SETUP;
}

}  // namespace lull

#define LULLABY_SETUP_TYPEID(Type)                       \
  namespace lull {                                       \
  template <>                                            \
  inline const char* GetTypeName<Type>() {               \
    return #Type;                                        \
  }                                                      \
  template <>                                            \
  inline TypeId GetTypeId<Type>() {                      \
    static constexpr TypeId id = lull::ConstHash(#Type); \
    return id;                                           \
  }                                                      \
  }  // namespace lull

#endif  // LULLABY_UTIL_TYPEID_H_
