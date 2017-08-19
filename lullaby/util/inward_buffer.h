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

#ifndef LULLABY_UTIL_INWARD_BUFFER_H_
#define LULLABY_UTIL_INWARD_BUFFER_H_

#include <cstring>
#include <memory>

namespace lull {

// A buffer that supports writing from either end towards the middle.
//
// The buffer owns a block of memory into which it writes data.  The writing can
// be done either from the front towards the back, or from the back towards the
// front.  If the two write-heads are about to overlap, the buffer attempts to
// reallocate to allow for the new data to be written.
//
// The primary use case for this class is to help write Flatbuffers.
// Flatbuffers need to be created in a "bottom-up" fashion and the data itself
// needs to be written from back to front (ie. high memory to low memory).  The
// process of creating Flatbuffers also requires some temporary data to also
// be written to a secondary buffer before being copied into Flatbuffer.  Using
// an InwardBuffer allows the temporary data to be written to the "front" memory
// while the final data is written to "back" memory.  This allows Flatbuffers
// to be created using a single block of memory, minimizing unnecessary
// allocations.
class InwardBuffer {
 public:
  explicit InwardBuffer(size_t capacity);

  InwardBuffer(const InwardBuffer&) = delete;
  InwardBuffer& operator=(const InwardBuffer&) = delete;

  InwardBuffer(InwardBuffer&&);
  InwardBuffer& operator=(InwardBuffer&&);

  // Resets both the front and back write-heads to the ends of the buffer.
  void Reset();

  // Returns the size (in bytes) that has been written to the front of the
  // buffer.
  size_t FrontSize() const;

  // Returns the size (in bytes) that has been written to the back of the
  // buffer.
  size_t BackSize() const;

  // Returns a pointer to current location of the front write-head and then
  // advances the write-head by |len| bytes.  Callers are free to write directly
  // into the returned memory up to |len| bytes.  The memory returned by this
  // function may be invalidated (eg. due to a reallocation of the buffer), so
  // it should be used immediately.
  void* AllocFront(size_t len);

  // Returns a pointer to current location of the back write-head and then
  // advances the write-head by |len| bytes.  Callers are free to write directly
  // into the returned memory up to |len| bytes.  The memory returned by this
  // function may be invalidated (eg. due to a reallocation of the buffer), so
  // it should be used immediately.
  void* AllocBack(size_t len);

  // Copies the specified |num| of |bytes| to the front of the buffer.
  void WriteFront(const void* bytes, size_t num);

  // Copies the specified |num| of |bytes| to the back of the buffer.
  void WriteBack(const void* bytes, size_t num);

  // Specialized version of push_front() that avoids memcpy call for small data.
  template <typename T>
  void WriteFront(const T& value) {
    void* dest = AllocFront(sizeof(T));
    *reinterpret_cast<T*>(dest) = value;
  }

  // Specialized version of push_back() that avoids memcpy call for small data.
  template <typename T>
  void WriteBack(const T& value) {
    void* dest = AllocBack(sizeof(T));
    *reinterpret_cast<T*>(dest) = value;
  }

  // Erases |num_bytes| of data from the front.
  void EraseFront(size_t num_bytes);

  // Erases |num_bytes| of data from the back.
  void EraseBack(size_t num_bytes);

  // Returns the actual memory address in the buffer referred to by the given
  // offset.  This memory may get invalidated, so should only be used when
  // the buffer is no longer being mutated.
  void* FrontAt(size_t offset);
  const void* FrontAt(size_t offset) const;

  // Returns the actual memory address in the buffer referred to by the given
  // offset.  This memory may get invalidated, so should only be used when
  // the buffer is no longer being mutated.
  void* BackAt(size_t offset);
  const void* BackAt(size_t offset) const;

 private:
  static size_t EnsureAligned(size_t capacity);

  void ReallocateIfNeeded(size_t requested);

  void DoReallocate(size_t requested);

  std::unique_ptr<uint8_t[]> mem_;
  uint8_t* back_ = nullptr;
  uint8_t* front_ = nullptr;
  size_t capacity_ = 0;
};

inline void InwardBuffer::Reset() {
  front_ = mem_.get();
  back_ = mem_.get() + capacity_;
}

inline size_t InwardBuffer::FrontSize() const {
  return static_cast<size_t>(front_ - mem_.get());
}

inline size_t InwardBuffer::BackSize() const {
  return static_cast<size_t>(mem_.get() + capacity_ - back_);
}

inline void* InwardBuffer::AllocFront(size_t len) {
  ReallocateIfNeeded(len);
  front_ += len;
  return reinterpret_cast<void*>(front_ - len);
}

inline void* InwardBuffer::AllocBack(size_t len) {
  ReallocateIfNeeded(len);
  back_ -= len;
  return reinterpret_cast<void*>(back_);
}

inline void InwardBuffer::WriteFront(const void* bytes, size_t num) {
  void* dest = AllocFront(num);
  memcpy(dest, bytes, num);
}

inline void InwardBuffer::WriteBack(const void* bytes, size_t num) {
  void* dest = AllocBack(num);
  memcpy(dest, bytes, num);
}

inline void InwardBuffer::EraseFront(size_t num_bytes) { front_ -= num_bytes; }

inline void InwardBuffer::EraseBack(size_t num_bytes) { back_ += num_bytes; }

inline void* InwardBuffer::FrontAt(size_t offset) {
  return mem_ ? mem_.get() + offset : nullptr;
}

inline const void* InwardBuffer::FrontAt(size_t offset) const {
  return mem_ ? mem_.get() + offset : nullptr;
}

inline void* InwardBuffer::BackAt(size_t offset) {
  return mem_ ? mem_.get() + capacity_ - offset : nullptr;
}

inline const void* InwardBuffer::BackAt(size_t offset) const {
  return mem_ ? mem_.get() + capacity_ - offset : nullptr;
}

inline void InwardBuffer::ReallocateIfNeeded(size_t requested) {
  if (requested > static_cast<size_t>(back_ - front_)) {
    DoReallocate(requested);
  }
}

}  // namespace lull

#endif  // LULLABY_UTIL_INWARD_BUFFER_H_
