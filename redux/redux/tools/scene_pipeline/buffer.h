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

#ifndef REDUX_TOOLS_SCENE_PIPELINE_BUFFER_H_
#define REDUX_TOOLS_SCENE_PIPELINE_BUFFER_H_

#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "redux/tools/scene_pipeline/index.h"
#include "absl/log/check.h"
#include "absl/types/span.h"

namespace redux::tool {

using ByteSpan = absl::Span<const std::byte>;

// A sized buffer of bytes; effectively a std::unique_ptr<byte> + length.
class Buffer {
 public:
  // Creates an empty (zero-length) buffer.
  Buffer() : bytes_(nullptr, Delete), num_bytes_(0) {}

  // Creates an uninitialized buffer of the given size.
  explicit Buffer(int size)
      : bytes_(new std::byte[size], Delete), num_bytes_(size) {}

  // Moves the other buffer into `this`.
  Buffer(Buffer&& other) noexcept
      : bytes_(std::move(other.bytes_)), num_bytes_(other.num_bytes_) {
    other.num_bytes_ = 0;
  }

  // Moves the other buffer into `this`.
  Buffer& operator=(Buffer&& other) noexcept {
    bytes_ = std::move(other.bytes_);
    num_bytes_ = other.num_bytes_;
    other.num_bytes_ = 0;
    return *this;
  }

  Buffer(const Buffer& other) = delete;
  Buffer& operator=(const Buffer& other) = delete;

  // The number of bytes in the buffer.
  int size() const { return num_bytes_; }

  // The data stored in the buffer.
  std::byte* data() { return bytes_.get(); }

  // The data stored in the buffer.
  const std::byte* data() const { return bytes_.get(); }

  // Returns true if the given byte is contained in the buffer.
  bool contains(const std::byte* byte) const {
    return as_ptr(byte) >= as_ptr(data()) &&
           as_ptr(byte) < as_ptr(data() + num_bytes_);
  }

  // The buffer data as a byte span.
  ByteSpan span() const {
    return ByteSpan(bytes_.get(), num_bytes_);
  }

  // A subset of the buffer data as a byte span.
  ByteSpan subspan(int offset, int length) const {
    CHECK(length + offset <= num_bytes_);
    return ByteSpan(&span()[offset], length);
  }

  // The buffer data as a span of the given type.
  template <typename T>
  absl::Span<const T> span_as() const {
    const T* ptr = reinterpret_cast<const T*>(data());
    return absl::Span<const T>(ptr, size() / sizeof(T));
  }

  // Clears the buffer of all data, deallocating memory as needed.
  void reset() {
    bytes_.reset();
    num_bytes_ = 0;
  }

  // Releases the buffer data (similar to std::unique_ptr::release). The caller
  // is responsible for deleting the data.
  std::byte* release() {
    std::byte* ptr = bytes_.release();
    num_bytes_ = 0;
    return ptr;
  }

  // Creates a buffer that takes ownership of the given data. A custom deleter
  // can be provided to handle the deletion of the data. If not provided, the
  // data will be deleted using `delete[]`.
  using Deleter = std::function<void(std::byte*)>;
  static Buffer Own(std::byte* ptr, int num_bytes, Deleter deleter = nullptr) {
    if (deleter == nullptr) {
      deleter = Delete;
    }
    return Buffer(Bytes(ptr, deleter), num_bytes);
  }

  // Creates a Buffer that contains a copy of the given data.
  template <typename T>
  static Buffer Copy(const T* ptr, int num) {
    static_assert(std::is_trivially_copyable_v<T>);
    const int num_bytes = num * sizeof(T);
    auto bytes = new std::byte[num_bytes];
    std::memcpy(bytes, ptr, num_bytes);
    return Own(bytes, num_bytes);
  }

 private:
  using Bytes = std::unique_ptr<std::byte[], Deleter>;

  Buffer(Bytes bytes, int num_bytes)
      : bytes_(std::move(bytes)), num_bytes_(num_bytes) {}

  static void Delete(std::byte* bytes) { delete[] bytes; }

  static const unsigned char* as_ptr(const std::byte* byte) {
    return reinterpret_cast<const unsigned char*>(byte);
  }

  Bytes bytes_;
  int num_bytes_ = 0;
};

using BufferIndex = Index<Buffer>;

}  // namespace redux::tool

#endif  // REDUX_TOOLS_SCENE_PIPELINE_BUFFER_H_
