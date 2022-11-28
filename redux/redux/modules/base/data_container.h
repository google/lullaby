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

#ifndef REDUX_MODULES_BASE_DATA_CONTAINER_H_
#define REDUX_MODULES_BASE_DATA_CONTAINER_H_

#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <utility>

#include "absl/types/span.h"
#include "redux/modules/base/logging.h"

namespace redux {

// Basically a unique_ptr<byte[]> + size.
//
// The unique_ptr has a custom deleter which allows for more interesting
// ownership models. At its simplest, the unique_ptr will behave "normally"
// by deleting the byte array. A more interesting use-case is to have the
// deleter destroy the object which owns the memory referenced in the container.
// A null deleter effectively makes this class behave like a span<std::byte>.
//
// We explicitly limit this container to std::byte as it meant to wrap binary
// data that can be copied as needed.
class DataContainer {
 public:
  using Deleter = std::function<void(const std::byte*)>;
  using DataPtr = std::unique_ptr<const std::byte[], Deleter>;

  DataContainer() = default;

  DataContainer(DataPtr data, size_t num_bytes)
      : data_(std::move(data)), num_bytes_(num_bytes) {}

  DataContainer(std::byte* data, size_t num_bytes, Deleter deleter = nullptr)
      : data_(data, std::move(deleter)), num_bytes_(num_bytes) {}

  void Reset() {
    data_.reset();
    num_bytes_ = 0;
  }

  const std::byte* GetBytes() const { return data_.get(); }

  size_t GetNumBytes() const { return num_bytes_; }

  absl::Span<const std::byte> GetByteSpan() const {
    return {GetBytes(), GetNumBytes()};
  }

  // Returns a new DataContainer containing a copy of the data stored in `this`
  // DataContainer. The new data is allocated on the heap and will be released
  // along with the new DataContainer.
  DataContainer Clone() const {
    if (data_ == nullptr || num_bytes_ == 0) {
      return DataContainer();
    }
    std::byte* bytes = new std::byte[num_bytes_];
    std::memcpy(bytes, data_.get(), num_bytes_);
    auto deleter = [](const std::byte* mem) { delete[] mem; };
    return DataContainer(bytes, num_bytes_, std::move(deleter));
  }

  // Returns a DataContainer which points to a span of bytes. The DataContainer
  // does not assume ownership of the data. As such, it is up to the caller to
  // ensure the lifetime of data exceeds that of the returned DataContainer.
  template <typename T>
  static DataContainer WrapData(absl::Span<const T> span) {
    return WrapData(span.data(), span.size());
  }

  // As above, with the raw pointer and size to the data to be wrapped.
  template <typename T>
  static DataContainer WrapData(const T* ptr, size_t size) {
    const std::byte* bytes = reinterpret_cast<const std::byte*>(ptr);
    DataPtr data(bytes, [](const std::byte*) {});
    return DataContainer(std::move(data), size * sizeof(T));
  }

  // Creates a DataContainer that uses dynamically allocated memory.
  static DataContainer Allocate(std::size_t num_bytes) {
    std::byte* bytes = new std::byte[num_bytes];
    auto deleter = [](const std::byte* mem) { delete[] mem; };
    return DataContainer(bytes, num_bytes, std::move(deleter));
  }

  // Imagine you want to create a DataWrapper around a subset of data that is
  // owned by an object. In order to ensure the data in the DataWrapper remains
  // valid, we need to ensure that the object's lifetime exceeds that of the
  // DataWrapper. If your object is stored in a shared_ptr, then we can tie its
  // lifetime to a DataContainer.
  template <typename T, typename U>
  static DataContainer WrapDataInSharedPtr(absl::Span<const T> span,
                                           std::shared_ptr<U> ptr) {
    return WrapDataInSharedPtr(span.data(), span.size(), std::move(ptr));
  }

  template <typename T, typename U>
  static DataContainer WrapDataInSharedPtr(const T* ptr, size_t size,
                                           std::shared_ptr<U> shared_pointer) {
    const std::byte* bytes = reinterpret_cast<const std::byte*>(ptr);
    DataPtr data(bytes,
                 [=](const std::byte*) mutable { shared_pointer.reset(); });
    return DataContainer(std::move(data), size * sizeof(T));
  }

 private:
  DataPtr data_ = nullptr;
  size_t num_bytes_ = 0;
};

}  // namespace redux

#endif  // REDUX_MODULES_BASE_DATA_CONTAINER_H_
