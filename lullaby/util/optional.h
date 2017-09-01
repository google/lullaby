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

#ifndef LULLABY_UTIL_OPTIONAL_H_
#define LULLABY_UTIL_OPTIONAL_H_

#include <utility>
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"

namespace lull {

// Wrapper around an instance of type |T| that may or may not be constructed.
//
// This class is similar to std::unique_ptr, but rather than allocate memory
// from the heap for the object, it provides the storage for the object
// internally (using std::aligned_storage).
//
// This class implements a subset of C++17's std::optional, so that once that
// class is standardized we can easily switch to it.

// A helper class and global variable that can be used to specify an empty
// optional.
struct NullOptT {
  struct DetailInit {};
  static DetailInit init;
  explicit constexpr NullOptT(DetailInit&) {}
};
extern const NullOptT NullOpt;

template <typename T>
class Optional {
 public:
  using ValueType = T;

  // Default constructor, no value set.
  Optional() : set_(false) {}

  // Constructor that takes a NullOpt to indicate an empty optional.
  Optional(NullOptT) : set_(false) {}

  // Copy constructor, copies value from |rhs| if set.
  Optional(const Optional& rhs) : set_(false) {
    if (rhs) {
      emplace(*rhs);
    }
  }

  // Move constructor, moves value from |rhs| if set.
  Optional(Optional&& rhs) : set_(false) {
    if (rhs) {
      emplace(std::move(*rhs));
      rhs.reset();
    }
  }

  // Sets the internal value by copying |rhs| value.
  Optional(const T& value) : set_(false) {
    emplace(value);
  }

  // Sets the internal value by moving |rhs| value.
  Optional(T&& value) : set_(false) {
    emplace(std::move(value));
  }

  // Destructor, ensures the internal value is properly destroyed.
  ~Optional() { reset(); }

  // Copies value from |rhs| if set.
  Optional& operator=(const Optional& rhs) {
    if (this == &rhs) {
      // Self-assignment so do nothing.
    } else if (set_ && rhs.set_) {
      *get() = *rhs;
    } else if (rhs.set_) {
      emplace(*rhs);
    } else {
      reset();
    }
    return *this;
  }

  // Moves value from |rhs| if set.
  Optional& operator=(Optional&& rhs) {
    if (this == &rhs) {
      // Self-assignment so do nothing.
    } else if (set_ && rhs.set_) {
      *get() = std::move(*rhs);
      rhs.reset();
    } else if (rhs.set_) {
      emplace(std::move(*rhs));
      rhs.reset();
    } else {
      reset();
    }
    return *this;
  }

  // Sets the internal value by copying |value|.
  Optional& operator=(const T& value) {
    emplace(value);
    return *this;
  }

  // Sets the internal value by moving |value|.
  Optional& operator=(T&& value) {
    emplace(std::move(value));
    return *this;
  }

  // Returns true if a value is set, false otherwise.
  explicit operator bool() const { return set_; }

  // Resets the optional back to an unset state, destroying the internal value
  // if it had been set.
  void reset() {
    if (set_) {
      reinterpret_cast<T*>(&value_)->~T();
      set_ = false;
    }
  }

  // Sets the value of the object using its constructor with the specified
  // arguments.
  template <typename... Args>
  void emplace(Args&&... args) {
    reset();
    new (&value_) T(std::forward<Args>(args)...);
    set_ = true;
  }

  // Gets the set value or nullptr if unset.
  T* get() { return set_ ? reinterpret_cast<T*>(&value_) : nullptr; }

  // Gets the set value or nullptr if unset.
  const T* get() const {
    return set_ ? reinterpret_cast<const T*>(&value_) : nullptr;
  }

  // Gets the set value.  This function should only be called if the
  // optional has been set.
  T& value() {
    CHECK(set_);
    return *get();
  }

  // Gets the set value.  This function should only be called if the
  // optional has been set.
  const T& value() const {
    CHECK(set_);
    return *get();
  }

  // Gets the set value or the provided |default_value| if unset.
  const T& value_or(const T& default_value) const {
    return set_ ? *get() : default_value;
  }

  // Dereferences the value like a pointer.
  T* operator->() { return get(); }

  // Dereferences the value like a pointer.
  const T* operator->() const { return get(); }

  // Gets a reference to the value.  This function should only be called if the
  // optional has been set.
  T& operator*() {
    CHECK(set_);
    return *get();
  }

  // Gets a reference to the value.  This function should only be called if the
  // optional has been set.
  const T& operator*() const {
    CHECK(set_);
    return *get();
  }

  // Returns true if both optionals are unset or are set to the same value.
  bool operator==(const Optional& rhs) const {
    if (set_ && rhs.set_) {
      return value() == rhs.value();
    } else {
      return set_ == rhs.set_;
    }
  }

  // Returns false if both optionals are unset or are set to the same value.
  bool operator!=(const Optional& rhs) const {
    return !operator==(rhs);
  }

  template <typename Archive>
  void Serialize(Archive archive) {
    if (archive.IsDestructive()) {
      reset();
    }

    bool set = set_;
    archive(&set, Hash("set"));

    if (set) {
      if (archive.IsDestructive()) {
        emplace();
      }
      archive(get(), Hash("value"));
    }
  }

 private:
  enum {
    kSize = sizeof(T),
    kAlign = alignof(T),
  };
  using StorageType = typename std::aligned_storage<kSize, kAlign>::type;

  StorageType value_;  // Memory buffer for value that can be set.
  bool set_;           // Indicates whether the value is set or not.
};

}  // namespace lull

#endif  // LULLABY_UTIL_OPTIONAL_H_
