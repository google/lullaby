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

#ifndef LULLABY_MODULES_SERIALIZE_BUFFER_SERIALIZER_H_
#define LULLABY_MODULES_SERIALIZE_BUFFER_SERIALIZER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "lullaby/modules/serialize/serialize.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/string_view.h"

namespace lull {

// Serializer that writes objects by copying the data into a Buffer.
class SaveToBuffer {
 public:
  using Buffer = std::vector<uint8_t>;  // A Buffer is just a vector of bytes.

  explicit SaveToBuffer(Buffer* buffer)
      : buffer_(CHECK_NOTNULL(buffer)), offset_(0) {
  }

  // Saves types like ints, floats, bools, etc.) to the buffer by directly
  // copying them.
  template <typename T>
  typename std::enable_if<detail::IsSerializeFundamental<T>::kValue, void>::type
  operator()(const T* ptr, lull::HashValue key) {
    Save(ptr, sizeof(T));
  }

  // Saves string_views to the buffer by copying the length and the raw char
  // data to the buffer.
  void operator()(const string_view* ptr, lull::HashValue key) {
    size_t size = ptr->length();
    Save(&size, sizeof(size));
    Save(ptr->data(), size);
  }

  // Saves strings to the buffer by copying the length and the raw char data to
  // the buffer.
  void operator()(const std::string* ptr, lull::HashValue key) {
    size_t size = ptr->size();
    Save(&size, sizeof(size));
    Save(ptr->data(), size);
  }

  // Saves vector data to the buffer by copying the length and then serializing
  // the individual elements to the buffer.
  template <typename T, typename... Args>
  void operator()(const std::vector<T, Args...>* ptr, lull::HashValue key) {
    size_t size = ptr->size();
    Save(&size, sizeof(size));
    for (size_t i = 0; i < size; ++i) {
      Serialize(this, &(*ptr)[i], key);
    }
  }

  // Saves map data to the buffer by copying the count of elements and then
  // serializing the key/value pairs to the buffer.
  template <typename K, typename V, typename... Args>
  void operator()(const std::unordered_map<K, V, Args...>* ptr,
                  lull::HashValue key) {
    size_t size = ptr->size();
    Save(&size, sizeof(size));
    for (auto& iter : *ptr) {
      Serialize(this, const_cast<K*>(&iter.first), key);
      Serialize(this, &iter.second, key);
    }
  }

  // Copies |size| bytes from |ptr| into the end of the internal buffer.
  void Save(const void* ptr, size_t size) {
    buffer_->resize(offset_ + size);
    memcpy(buffer_->data() + offset_, ptr, size);
    offset_ += size;
  }

  // This serializer is only reading the object, the object will not be changed.
  bool IsDestructive() const { return false; }

 private:
  // The buffer being read from or written to.
  Buffer* buffer_ = nullptr;

  // The read/write head of the buffer.
  size_t offset_ = 0;

  SaveToBuffer(const SaveToBuffer& rhs) = delete;
  SaveToBuffer& operator=(const SaveToBuffer& rhs) = delete;
};

// Serializer that reads objects by copying the data from a Buffer.
class LoadFromBuffer {
 public:
  using Buffer = std::vector<uint8_t>;  // A Buffer is just a vector of bytes.

  explicit LoadFromBuffer(const Buffer* buffer)
      : buffer_(CHECK_NOTNULL(buffer)), offset_(0) {
  }

  // Loads types like ints, floats, bools, etc.) from the buffer by directly
  // copying them.
  template <typename T>
  typename std::enable_if<detail::IsSerializeFundamental<T>::kValue, void>::type
  operator()(T* ptr, lull::HashValue key) {
    Load(ptr, sizeof(T));
  }

  // Loads string_views from the buffer by reading the length and then wrapping
  // the raw char data from the buffer.
  void operator()(string_view* ptr, lull::HashValue key) {
    size_t size = 0;
    Load(&size, sizeof(size));
    const uint8_t* data = Advance(size);
    *ptr = string_view(reinterpret_cast<const char*>(data), size);
  }

  // Loads strings from the buffer by copying the length and the raw char data
  // from the buffer.
  void operator()(std::string* ptr, lull::HashValue key) {
    size_t size = 0;
    Load(&size, sizeof(size));
    ptr->resize(size);
    Load(&(*ptr)[0], size);
  }

  // Loads vector data from the buffer by copying the length and then
  // serializing the individual elements to the buffer.
  template <typename T, typename... Args>
  void operator()(std::vector<T, Args...>* ptr, lull::HashValue key) {
    size_t size = 0;
    Load(&size, sizeof(size));
    ptr->resize(size);
    for (size_t i = 0; i < size; ++i) {
      Serialize(this, &(*ptr)[i], key);
    }
  }

  // Loads map data from the buffer by copying the count of elements and then
  // copying each key/value pair.
  template <typename K, typename V, typename... Args>
  void operator()(std::unordered_map<K, V, Args...>* ptr, lull::HashValue key) {
    size_t size = 0;
    Load(&size, sizeof(size));
    ptr->clear();
    for (size_t i = 0; i < size; ++i) {
      std::pair<K, V> elem;
      Serialize(this, &elem.first, key);
      Serialize(this, &elem.second, key);
      ptr->emplace(std::move(elem));
    }
  }

  // Advances the buffer by the specified number of bytes.  Returns the pointer
  // to the buffer at the location prior to the skipping.
  const uint8_t* Advance(size_t size) {
    if (offset_ + size <= buffer_->size()) {
      const uint8_t* ptr = buffer_->data() + offset_;
      offset_ += size;
      return ptr;
    } else {
      DCHECK(false) << "Attempting to load beyond buffer size.";
      return nullptr;
    }
  }

  // This serializer will write into the object, overwriting its current data.
  bool IsDestructive() const { return true; }

 private:
  // Copies |size| bytes of data from the internal buffer to |ptr| and
  // increments the buffer pointer to after the copied bytes.
  void Load(void* ptr, size_t size) {
    const uint8_t* src = Advance(size);
    if (src) {
      memcpy(ptr, src, size);
    }
  }

  // The buffer being read from or written to.
  const Buffer* buffer_ = nullptr;

  // The read/write head of the buffer.
  size_t offset_ = 0;

  LoadFromBuffer(const LoadFromBuffer& rhs) = delete;
  LoadFromBuffer& operator=(const LoadFromBuffer& rhs) = delete;
};

}  // namespace lull

#endif  // LULLABY_MODULES_SERIALIZE_BUFFER_SERIALIZER_H_
