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

#ifndef LULLABY_UTIL_TYPED_POINTER_H_
#define LULLABY_UTIL_TYPED_POINTER_H_

#include "lullaby/util/typeid.h"

namespace lull {

/// A type-safe wrapper around a void pointer.
///
/// The actual resource being referenced must outlive the lifetime of the
/// TypedPointer.
class TypedPointer {
 public:
  /// Constructs an empty TypedPointer object.
  TypedPointer() {}

  /// Wraps the specified object.
  template <typename T>
  explicit TypedPointer(T* obj) : ptr_(obj) {
    if (ptr_) {
      type_ = lull::GetTypeId<T>();
    }
  }

  /// Returns true if an object is wrapped, and false otherwise.
  explicit operator bool() const {
    return type_ != 0;
  }

  /// Returns true if an object is wrapped, and false otherwise.
  bool Empty() const {
    return type_ != 0;
  }

  /// Resets the TypedPointer to an empty state.
  void Reset() {
    type_ = 0;
    ptr_ = nullptr;
  }

  /// Returns the pointer to the wrapped object if it is actually of the type
  /// requested, or nullptr otherwise.
  template <typename T>
  T* Get() {
    return type_ == lull::GetTypeId<T>() ? reinterpret_cast<T*>(ptr_) : nullptr;
  }

  /// Returns the const pointer to the wrapped object if it is actually of the
  /// type requested, or nullptr otherwise.
  template <typename T>
  const T* Get() const {
    return type_ == lull::GetTypeId<T>() ? reinterpret_cast<const T*>(ptr_)
                                     : nullptr;
  }

  /// Returns the TypeId of the wrapped object.
  TypeId GetTypeId() const { return type_; }

 private:
  void* ptr_ = nullptr;
  TypeId type_ = 0;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TypedPointer);

#endif  // LULLABY_UTIL_TYPED_POINTER_H_
