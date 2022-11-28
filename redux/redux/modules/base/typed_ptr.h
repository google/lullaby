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

#ifndef REDUX_MODULES_BASE_TYPED_PTR_H_
#define REDUX_MODULES_BASE_TYPED_PTR_H_

#include <type_traits>

#include "redux/modules/base/typeid.h"

namespace redux {

// A type-safe wrapper around a void pointer.
//
// The actual resource being referenced must outlive the lifetime of the
// TypedPtr. This is basically an any_view; a "type-erased, non-owning,
// object pointer."
class TypedPtr {
 public:
  // Constructs a null TypedPtr object.
  TypedPtr() = default;

  // Wraps the specified object.
  template <typename T>
  explicit TypedPtr(T* obj) : ptr_(reinterpret_cast<void*>(obj)) {
    static_assert(!std::is_const_v<T>, "Cannot assign a const pointer!");
    static_assert(!std::is_volatile_v<T>, "Cannot assign a volatile pointer!");
    if (ptr_ != nullptr) {
      type_ = redux::GetTypeId<T>();
    }
  }

  // Returns true if an object is wrapped, and false otherwise.
  explicit operator bool() const { return type_ != 0; }

  // Returns true if an object is wrapped, and false otherwise.
  bool Empty() const { return type_ == 0; }

  // Resets the TypedPtr to an empty state.
  void Reset() {
    type_ = 0;
    ptr_ = nullptr;
  }

  template <typename T>
  bool Is() const {
    return type_ == redux::GetTypeId<T>();
  }

  // Returns the TypeId of the wrapped object.
  TypeId GetTypeId() const { return type_; }

  // Returns the pointer to the wrapped object if it is actually of the type
  // requested, or nullptr otherwise.
  template <typename T>
  T* Get() {
    return Is<T>() ? reinterpret_cast<T*>(ptr_) : nullptr;
  }

  // Returns the const pointer to the wrapped object if it is actually of the
  // type requested, or nullptr otherwise.
  template <typename T>
  const T* Get() const {
    return Is<T>() ? reinterpret_cast<const T*>(ptr_) : nullptr;
  }

 private:
  void* ptr_ = nullptr;
  TypeId type_ = 0;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::TypedPtr);

#endif  // REDUX_MODULES_BASE_TYPED_PTR_H_
