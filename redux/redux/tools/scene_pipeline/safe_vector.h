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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_SAFE_VECTOR_H_
#define REDUX_TOOLS_SCENE_PIPELINE_SAFE_VECTOR_H_

#include <vector>

#include "redux/tools/scene_pipeline/index.h"
#include "absl/log/check.h"

namespace redux::tool {

// A simpler wrapper around std::vector, but operator[] only accepts an
// Index<T> as the argument (instead of size_t/int).
//
// This provides a basic level of type safety by ensuring the vector can only
// be accessed by a given Index. Additionally, operator[] checks bounds and
// fails if the Index is out of range.
template <typename T>
class SafeVector {
 public:
  SafeVector() = default;

  // Returns a reference to the element at the given Index.
  T& operator[](Index<T> idx) {
    CHECK(idx.value() < size());
    return data_[idx.value()];
  }

  // Returns a const reference to the element at the given Index.
  const T& operator[](Index<T> idx) const {
    CHECK(idx.value() < size());
    return data_[idx.value()];
  }

  // The remaining functions are perfect-forwarding pass-throughs to the
  // underlying std::vector functions.

  auto size() const { return data_.size(); }

  T& front() { return data_.front(); }

  T& back() { return data_.back(); }

  template <typename... Args>
  T& emplace_back(Args&&... args) {
    return data_.emplace_back(std::forward<Args>(args)...);
  }

 private:
  std::vector<T> data_;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_SAFE_VECTOR_H_
