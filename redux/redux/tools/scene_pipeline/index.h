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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_INDEX_H_
#define REDUX_TOOLS_SCENE_PIPELINE_INDEX_H_

#include <cassert>
#include <utility>

#include "absl/log/check.h"

namespace redux::tool {

// Base class for a strongly-typed index.
template <typename T>
class Index {
 public:
  Index() : value_(-1) {}

  explicit Index(int value) : value_(value) {
    CHECK(valid());
  }

  int value() const { return value_; }

  bool valid() const { return value_ >= 0; }

 private:
  friend bool operator==(const Index& lhs, const Index& rhs) {
    return lhs.value_ == rhs.value_;
  }
  friend bool operator!=(const Index& lhs, const Index& rhs) {
    return lhs.value_ != rhs.value_;
  }
  friend bool operator<(const Index& lhs, const Index& rhs) {
    return lhs.value_ < rhs.value_;
  }
  friend bool operator<=(const Index& lhs, const Index& rhs) {
    return lhs.value_ <= rhs.value_;
  }
  friend bool operator>(const Index& lhs, const Index& rhs) {
    return lhs.value_ > rhs.value_;
  }
  friend bool operator>=(const Index& lhs, const Index& rhs) {
    return lhs.value_ >= rhs.value_;
  }
  template <typename H>
  friend H AbslHashValue(H h, const Index& idx) {
    return H::combine(std::move(h), idx.value_);
  }

  int value_;
};

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_INDEX_H_
