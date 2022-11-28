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

#ifndef REDUX_MODULES_VAR_VAR_H_
#define REDUX_MODULES_VAR_VAR_H_

#include <cstddef>
#include <string>
#include <type_traits>

#include "absl/time/time.h"
#include "redux/modules/base/data_buffer.h"
#include "redux/modules/base/logging.h"
#include "redux/modules/base/typeid.h"

namespace redux {

// Variant type similar to std::any, but constrained to types that have a
// redux::TypeId. Uses small object optimization to store common data types
// (e.g. int, vec3, std::string, etc.) directly without dynamic allocation. Vars
// are not required to hold any value.
//
// A common use-case for Vars is to store a VarArray or a VarTable. To simplify
// these use-cases, operator[] has been overloaded to make it easier to
// "navigate" these data structures. For example:
//
//   var["Name"][0]
//
// is the same as:
//
//   var.ValueOr(VarTable())[(ConstHash("Name")].ValueOr(VarArray())[0]
//
// Similarly, a Count() function returns the size of the containers if a Var
// stores a VarArray or VarTable, otherwise it will return either 0 or 1,
// depending on whether the Var stores a value.
class Var {
 public:
  // Creates an "empty" Var that holds no value.
  Var();
  ~Var();

  Var(const Var& rhs);
  Var(Var&& rhs);
  Var& operator=(const Var& rhs);
  Var& operator=(Var&& rhs);

  // Assigns the value to this.
  template <typename T>
  Var(T&& value) {
    using U = std::decay_t<T>;
    if constexpr (std::is_same_v<Var, U>) {
      SetVar(std::forward<T>(value));
    } else {
      SetValue<U>(value);
    }
  }

  // Assigns the value to this.
  template <typename T>
  Var& operator=(T&& value) {
    using U = std::decay_t<T>;
    if constexpr (std::is_same_v<Var, U>) {
      SetVar(std::forward<T>(value));
    } else {
      SetValue<U>(std::forward<T>(value));
    }
    return *this;
  }

  // Returns whether a value has been assigned.
  bool Empty() const { return type_ == 0; }

  // Clears any assigned value.
  void Clear() { Destroy(); }

  // Returns the TypeId of the assigned value; 0 if unassigned.
  TypeId GetTypeId() const { return type_; }

  // Returns whether a value of type `T` (ignoring any qualifiers) is currently
  // assigned.
  template <typename T>
  bool Is() const {
    using U = std::decay_t<T>;
    return type_ == redux::GetTypeId<U>();
  }

  // Returns a pointer to the assigned value if it is of type `T`, otherwise
  // returns null.
  template <typename T>
  T* Get() {
    if constexpr (std::is_same_v<T, Var>) {
      return this;
    } else {
      return Is<T>() ? reinterpret_cast<T*>(GetDataPtr()) : nullptr;
    }
  }

  // Returns a pointer to the assigned value if it is of type `T`, otherwise
  // returns null.
  template <typename T>
  const T* Get() const {
    if constexpr (std::is_same_v<T, Var>) {
      return this;
    } else {
      return Is<T>() ? reinterpret_cast<const T*>(GetDataPtr()) : nullptr;
    }
  }

  // Returns a copy of the assigned value if it is of type `T`, otherwise
  // returns `default_value` by value. If you don't want the copy, consider
  // calling Get<> instead.
  template <typename T>
  std::decay_t<T> ValueOr(T&& default_value) const {
    using U = std::decay_t<T>;
    const auto* value = Get<U>();
    return value != nullptr ? *value : std::forward<T>(default_value);
  }

  // Returns the number of elements stored in this Var. This is 0 if the Var
  // is empty, or 1 if the Var is not a VarArray or VarTable. In those cases,
  // this will return the number of elements in those containers.
  std::size_t Count() const;

  // If the Var stores a VarArray, returns the n-th element of that array,
  // otherwise returns *this.
  Var& operator[](std::size_t index);
  const Var& operator[](std::size_t index) const;

  // If the Var stores a VarTable, returns the element associated with the `key`
  // in the table, otherwise returns *this.
  Var& operator[](HashValue key);
  const Var& operator[](HashValue key) const;

  // Same as operator[](HashValue), but will do the hashing automatically. Only
  // works with compile-time strings.
  template <std::size_t N>
  Var& operator[](const char (&str)[N]) {
    return (*this)[HashValue(detail::ConstHash(str))];
  }
  template <std::size_t N>
  const Var& operator[](const char (&str)[N]) const {
    return (*this)[HashValue(detail::ConstHash(str))];
  }

 private:
  // Type of operations that may be performed on the stored value.
  enum Operation {
    kCopy,
    kMove,
    kDestroy,
  };

  void SetVar(const Var& rhs);
  void SetVar(Var&& rhs);

  template <typename T>
  void SetValue(const T& value) {
    CHECK(alignof(T) <= kStoreAlign);

    const T* value_ptr = &value;
    Var tmp;
    if (handler_) {
      // We need to protect ourselves from self-assignment. We do this by
      // copying the value into a temporary buffer.
      value_ptr = tmp.CopyIntoHelper(value_ptr);
    }

    // We could be more optimal about swapping buffers rather than performing
    // all these copies, but we'll leave that for an optimization pass.
    Destroy();
    CopyIntoHelper(value_ptr);
  }

  template <typename T>
  void SetValue(T&& value) {
    CHECK(alignof(T) <= kStoreAlign);

    T* value_ptr = &value;
    Var tmp;
    if (handler_) {
      // We need to protect ourselves from self-assignment. We do this by
      // copying the value into a temporary buffer.
      value_ptr = tmp.MoveIntoHelper(value_ptr);
    }

    // We could be more optimal about swapping buffers rather than performing
    // all these copies, but we'll leave that for an optimization pass.
    Destroy();
    MoveIntoHelper(value_ptr);
  }

  template <typename T>
  const T* CopyIntoHelper(const T* ptr) {
    Alloc(sizeof(T));
    type_ = redux::GetTypeId<T>();
    handler_ = HandlerImpl<T>;
    handler_(kCopy, GetDataPtr(), ptr, nullptr);
    return Get<T>();
  }

  template <typename T>
  T* MoveIntoHelper(T* ptr) {
    Alloc(sizeof(T));
    type_ = redux::GetTypeId<T>();
    handler_ = HandlerImpl<T>;
    handler_(kMove, GetDataPtr(), nullptr, ptr);
    return Get<T>();
  }

  // Destroys the stored value.
  void Destroy();

  // Allocates memory of the required size if needed.
  void Alloc(std::size_t size);

  // Frees any allocated memory.
  void Free();

  // Returns true if the small-object memory buffer is being used to store the
  // value.
  bool IsSmallData() const;

  // Returns the pointer to the data for the stored value.
  std::byte* GetDataPtr();
  const std::byte* GetDataPtr() const;

  // Performs type-specific operations on the provided pointers. The parameters
  // are dependent on the type of operation.  Specifically:
  //   kCopy: Object of |Type| is copied from |from| pointer to |to| pointer
  //          using copy constructor.
  //   kMove: Object of |Type| is moved from |victim| pointer to |to| pointer
  //          using move constructor.
  //   kDestroy: Object of |Type| in |victim| is destroyed using destructor.
  template <typename Type>
  static void HandlerImpl(Operation op, void* to, const void* from,
                          void* victim) {
    switch (op) {
      case kCopy:
        ::new (to) Type(*static_cast<const Type*>(from));
        break;
      case kMove:
        ::new (to) Type(std::move(*static_cast<Type*>(victim)));
        break;
      case kDestroy:
        static_cast<Type*>(victim)->~Type();
        break;
    }
  }

  static constexpr std::size_t kStoreSize = 64;
  static constexpr std::size_t kStoreAlign = 16;  // Aligned for simd types.
  using HandlerFn = void(Operation, void*, const void*, void*);

  TypeId type_ = 0;
  std::size_t capacity_ = kStoreSize;
  HandlerFn* handler_ = nullptr;
  union alignas(kStoreAlign) {
    std::byte small_data_[kStoreSize];
    std::byte* heap_data_ = nullptr;
  };
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::Var);

// Register some common types from std and absl.
REDUX_SETUP_TYPEID(std::string);
REDUX_SETUP_TYPEID(std::string_view);
REDUX_SETUP_TYPEID(absl::Time);
REDUX_SETUP_TYPEID(absl::Duration);

#endif  // REDUX_MODULES_VAR_VAR_H_
