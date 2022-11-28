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

#ifndef REDUX_MODULES_BASE_DATA_BUFFER_H_
#define REDUX_MODULES_BASE_DATA_BUFFER_H_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <utility>

#include "absl/types/span.h"
#include "redux/modules/base/typeid.h"

namespace redux {

// A dynamic buffer of std::byte with small size optimization.
class DataBuffer {
 public:
  static constexpr std::size_t kSmallDataSize = 32;

  DataBuffer() = default;

  DataBuffer(const DataBuffer& rhs) : DataBuffer() {
    Assign(rhs.GetByteSpan());
  }

  DataBuffer(DataBuffer&& rhs) : DataBuffer() {
    if (rhs.IsSmallData()) {
      Assign(rhs.GetByteSpan());
      rhs.size_ = 0;
    } else {
      heap_data_ = std::exchange(rhs.heap_data_, nullptr);
      capacity_ = std::exchange(rhs.capacity_, kSmallDataSize);
      size_ = std::exchange(rhs.size_, 0);
    }
  }

  DataBuffer& operator=(const DataBuffer& rhs) {
    if (this != &rhs) {
      Assign(rhs.GetByteSpan());
    }
    return *this;
  }

  DataBuffer& operator=(DataBuffer&& rhs) {
    if (this != &rhs) {
      if (rhs.IsSmallData()) {
        Assign(rhs.GetByteSpan());
        rhs.size_ = 0;
      } else {
        using std::swap;
        swap(heap_data_, rhs.heap_data_);
        swap(capacity_, rhs.capacity_);
        swap(size_, rhs.size_);
        rhs.Free();
      }
    }
    return *this;
  }

  ~DataBuffer() { Free(); }

  // Empties the buffer of all bytes.
  void Clear() { size_ = 0; }

  // Copies the given byte span into the buffer.
  void Assign(absl::Span<const std::byte> data) {
    if (!data.empty()) {
      Alloc(data.size());
      std::memcpy(GetData(), data.data(), data.size());
    }
    size_ = data.size();
  }

  // Returns the number of bytes stored in the buffer.
  std::size_t GetNumBytes() const { return size_; }

  // Returns a view of the bytes stored in the buffer.
  absl::Span<const std::byte> GetByteSpan() const {
    return absl::Span<const std::byte>(GetData(), GetNumBytes());
  }

 private:
  bool IsSmallData() const { return capacity_ <= kSmallDataSize; }

  std::byte* GetData() { return IsSmallData() ? &small_data_[0] : heap_data_; }

  const std::byte* GetData() const {
    return IsSmallData() ? &small_data_[0] : heap_data_;
  }

  void Alloc(std::size_t size) {
    if (size <= capacity_) {
      // We already have enough space.
      return;
    }
    Free();
    if (size > kSmallDataSize) {
      heap_data_ = new std::byte[size];
      capacity_ = size;
    }
  }

  void Free() {
    if (!IsSmallData()) {
      delete[] heap_data_;
    }
    capacity_ = kSmallDataSize;
  }

  std::size_t size_ = 0;
  std::size_t capacity_ = kSmallDataSize;
  union {
    std::byte small_data_[kSmallDataSize];
    std::byte* heap_data_ = nullptr;
  };
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::DataBuffer);

#endif  // REDUX_MODULES_BASE_DATA_BUFFER_H_
