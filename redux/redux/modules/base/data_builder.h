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

#ifndef REDUX_MODULES_BASE_DATA_BUILDER_H_
#define REDUX_MODULES_BASE_DATA_BUILDER_H_

#include <cstddef>
#include <cstring>
#include <utility>

#include "absl/types/span.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/logging.h"

namespace redux {

// Stream-like object for creating DataContainer objects by appending bytes.
class DataBuilder {
 public:
  explicit DataBuilder(std::size_t capacity)
      : data_(new std::byte[capacity]), capacity_(capacity) {}

  ~DataBuilder() { delete[] data_; }

  DataBuilder(const DataBuilder&) = delete;
  DataBuilder& operator=(const DataBuilder&) = delete;

  // Returns pointer to data where |size| bytes can be appended.
  std::byte* GetAppendPtr(std::size_t size) {
    CHECK(offset_ + size <= capacity_);
    std::byte* ptr = data_ + offset_;
    offset_ += size;
    return ptr;
  }

  // Copies |size| bytes at |data| to the end of the container.
  template <typename T>
  void Append(const T* data, std::size_t num) {
    const std::size_t num_bytes = sizeof(T) * num;
    std::byte* ptr = GetAppendPtr(num_bytes);
    std::memcpy(ptr, data, num_bytes);
  }

  template <typename T>
  void Append(absl::Span<const T> data) {
    Append(data.data(), data.size());
  }

  template <typename T>
  void Append(const T& value) {
    Append(&value, 1);
  }

  // Advances the write head of the data container by the specified number of
  // bytes, effectively increasing the size of the data container.
  void Advance(std::size_t size) {
    CHECK(offset_ + size <= capacity_);
    offset_ += size;
  }

  // Returns the current number of bytes appended to the container. Note that
  // this will not count bytes written directly using the mutable data pointer.
  std::size_t GetSize() const { return offset_; }

  // Returns the total number of bytes that can fit into the container.
  std::size_t GetCapacity() const { return capacity_; }

  // Returns a DataContainer around the internal memory buffer, releasing the
  // buffer to prevent further writing.
  DataContainer Release() {
    if (data_ == nullptr || offset_ == 0) {
      delete[] data_;
      data_ = nullptr;
      offset_ = 0;
      capacity_ = 0;
      return DataContainer();
    } else {
      DataContainer::DataPtr ptr(data_,
                                 [](const std::byte* mem) { delete[] mem; });
      const std::size_t num_bytes = offset_;

      data_ = nullptr;
      offset_ = 0;
      capacity_ = 0;
      return DataContainer(std::move(ptr), num_bytes);
    }
  }

 private:
  std::byte* data_ = nullptr;
  std::size_t capacity_ = 0;
  std::size_t offset_ = 0;
};

}  // namespace redux

#endif  // REDUX_MODULES_BASE_DATA_BUILDER_H_
