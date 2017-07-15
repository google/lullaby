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

#ifndef LULLABY_UTIL_SPAN_H_
#define LULLABY_UTIL_SPAN_H_

#include <array>
#include <vector>

namespace lull {

// A span points to an array of const data that is does not own itself.

// It can be used when you need to pass an array of data to a function, whether
// its a C-style array, std::array, or std::vector.  It is similar to
// string_view, but for arbitrary (non-char) data.
template <typename T = uint8_t>
class Span {
 public:
  // Constructors.
  Span() : data_(nullptr), size_(0) {}

  Span(const T* data, size_t size) : data_(data), size_(size) {}

  Span(const std::vector<T>& vec)
      : data_(vec.data()), size_(vec.size()) {}

  template <size_t N>
  Span(const T (&data)[N])
      : data_(data), size_(N) {}

  template <size_t N>
  Span(const std::array<T, N>& arr)
      : data_(arr.data()), size_(arr.size()) {}

  // Returns the number of elements in the span.
  size_t size() const { return size_; }

  // Returns whether the span is empty.
  bool empty() const { return size_ == 0; }

  // Returns an element from the span. Does not do bounds checking.
  const T& operator[](size_t i) const { return data_[i]; }

  // Get the raw data of the span.
  const T* data() const { return data_; }

  // Iteration methods.
  const T* begin() const { return data_; }
  const T* end() const { return data_ + size_; }

 private:
  const T* data_ = nullptr;
  size_t size_ = 0;
};

}  // namespace lull

#endif  // LULLABY_UTIL_SPAN_H_
