/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_UTIL_EXPECTED_H_
#define LULLABY_UTIL_EXPECTED_H_

#include "lullaby/util/error.h"
#include "lullaby/util/logging.h"

#include <string>

namespace lull {

// Expected is a template class to aid in error handling.  It contains either an
// error or valid data, plus a flag to know which of the two cases is present.
template <typename T>
class Expected {
 public:
  Expected(T t) : set_(true) { new (&value_) T(std::move(t)); }
  Expected(Error error) { new (&error_) Error(std::move(error)); }
  Expected(const Expected& rhs) {
    if (rhs) {
      new (&value_) T(rhs.value_);
    } else {
      new (&error_) Error(rhs.error_);
    }
  }
  Expected(Expected&& rhs) {
    if (rhs) {
      new (&value_) T(std::move(rhs.value_));
    } else {
      new (&error_) Error(std::move(rhs.error_));
    }
  }

  ~Expected() {
    if (set_) {
      value_.~T();
    } else {
      error_.~Error();
    }
  }

  Expected& operator=(Expected rhs) {
    if (set_) {
      value_.~T();
    } else {
      error_.~Error();
    }

    if (rhs) {
      new (&value_) T(std::move(rhs.value_));
      set_ = true;
    } else {
      new (&error_) Error(std::move(rhs.error_));
      set_ = false;
    }

    return *this;
  }

  // Cast-to-bool operator allowing the Expected to be used in a conditional
  // statement for brevity of error handling code.
  operator bool() const { return set_; }

  // Returns the T value contained in the Expected<T>.
  T& get() {
    CHECK(set_);
    return value_;
  }

  // Returns the error.
  const Error& GetError() const {
    CHECK(!set_);
    return error_;
  }

  // Dereferences the value like a pointer.
  T* operator->() { return &get(); }

  // Dereferences the value like a pointer.
  const T* operator->() const { return &get(); }

  // Gets a reference to the value.  This function should only be called if the
  // optional has been set.
  T& operator*() {
    CHECK(set_);
    return get();
  }

 private:
  bool set_ = false;

  union {
    T value_;
    Error error_;
  };
};

}  // namespace lull

#endif  // LULLABY_UTIL_EXPECTED_H_
