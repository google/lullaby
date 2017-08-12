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

#include "lullaby/util/inward_buffer.h"
#include <algorithm>

namespace lull {

InwardBuffer::InwardBuffer(size_t capacity)
    : mem_(nullptr), back_(nullptr), front_(nullptr), capacity_(capacity) {
  capacity_ = EnsureAligned(capacity_);
  mem_.reset(new uint8_t[capacity_]);
  Reset();
}

InwardBuffer::InwardBuffer(InwardBuffer&& rhs)
    : mem_(nullptr), back_(nullptr), front_(nullptr), capacity_(0) {
  std::swap(mem_, rhs.mem_);
  std::swap(back_, rhs.back_);
  std::swap(front_, rhs.front_);
  std::swap(capacity_, rhs.capacity_);
}

InwardBuffer& InwardBuffer::operator=(InwardBuffer&& rhs) {
  if (this != &rhs) {
    mem_ = std::move(rhs.mem_);
    back_ = rhs.back_;
    front_ = rhs.front_;
    capacity_ = rhs.capacity_;
    rhs.back_ = nullptr;
    rhs.front_ = nullptr;
    rhs.capacity_ = 0;
  }
  return *this;
}

size_t InwardBuffer::EnsureAligned(size_t capacity) {
  // We can assume the memory will be allocated with a reasonable alignment, but
  // we need to make sure that the end of the buffer is also correctly aligned.
  constexpr size_t align = alignof(uint64_t);
  // Invert the alignment bits which we'll apply as a bitmask on the final
  // capacity value.  This will ensure that the capacity will have an end
  // alignment that works correctly with data elements that have an alignment
  // requirement the same as a uint64_t.
  const size_t mask = ~(align - 1);
  // Increase the capacity by a little bit under the target alignment to ensure
  // that the masking process will not result in a smaller capacity than
  // originally requested.
  const size_t min_capacity = capacity + align - 1;
  return min_capacity & mask;
}

void InwardBuffer::DoReallocate(size_t requested) {
  const size_t num_bytes_back = BackSize();
  const size_t num_bytes_front = FrontSize();

  // Calculate the required size for the new buffer which should at least be
  // large enough to accomodate the requested allocation size.
  capacity_ = EnsureAligned(capacity_ + std::max(requested, capacity_));

  // Allocate the new buffer, copy the commited front and back blocks of
  // memory from the current buffer to the new buffer, and deallocate the
  // old buffer.
  uint8_t* new_buffer = new uint8_t[capacity_];
  uint8_t* end_buffer = new_buffer + capacity_;

  if (num_bytes_front > 0) {
    memcpy(new_buffer, mem_.get(), num_bytes_front);
  }
  if (num_bytes_back > 0) {
    memcpy(end_buffer - num_bytes_back, back_, num_bytes_back);
  }

  // Update the members to point to the correct locations in the new buffer.
  mem_.reset(new_buffer);
  front_ = new_buffer + num_bytes_front;
  back_ = end_buffer - num_bytes_back;
}

}  // namespace lull
