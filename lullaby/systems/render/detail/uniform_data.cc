/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/systems/render/detail/uniform_data.h"

#include <cstring>
#include "lullaby/util/logging.h"

namespace lull {
namespace detail {

UniformData::UniformData(const UniformData& rhs) {
  SetData(rhs.Type(), {rhs.Data(), rhs.Size()});
}

UniformData& UniformData::operator=(const UniformData& rhs) {
  if (this != &rhs) {
    SetData(rhs.Type(), {rhs.Data(), rhs.Size()});
  }
  return *this;
}

UniformData::UniformData(UniformData&& rhs)
    : type_(rhs.type_),
      size_(rhs.size_),
      capacity_(rhs.capacity_),
      heap_data_(nullptr) {
  using std::swap;
  if (rhs.IsSmallData()) {
    std::memcpy(&small_data_, rhs.Data(), rhs.Size());
  } else {
    swap(heap_data_, rhs.heap_data_);
    rhs.size_ = 0;
    rhs.capacity_ = 0;
  }
}

UniformData& UniformData::operator=(UniformData&& rhs) {
  using std::swap;
  if (this != &rhs) {
    // The rhs is small, just copy the data. Otherwise, swap memory buffers.
    if (rhs.IsSmallData()) {
      Free();
      std::memcpy(&small_data_, rhs.Data(), rhs.Size());
      type_ = rhs.type_;
      size_ = rhs.size_;
      capacity_ = rhs.capacity_;
    } else {
      swap(heap_data_, rhs.heap_data_);
      swap(type_, rhs.type_);
      swap(size_, rhs.size_);
      swap(capacity_, rhs.capacity_);
    }
  }
  return *this;
}

UniformData::~UniformData() {
  Free();
}

void UniformData::SetData(ShaderDataType type, Span<uint8_t> data) {
  type_ = type;

  if (data.empty()) {
    Free();
    size_ = 0;
  } else {
    Alloc(data.size());
    std::memcpy(Data(), data.data(), data.size());
    size_ = data.size();
  }
}

uint8_t* UniformData::Data() {
  return IsSmallData() ? &small_data_[0] : heap_data_;
}

const uint8_t* UniformData::Data() const {
  return IsSmallData() ? &small_data_[0] : heap_data_;
}

size_t UniformData::Size() const { return size_; }

size_t UniformData::Count() const {
  if (type_ == ShaderDataType_BufferObject) {
    return 1;
  } else {
    const size_t bytes_per_element = ShaderDataTypeToBytesSize(type_);
    return size_ / bytes_per_element;
  }
}

ShaderDataType UniformData::Type() const { return type_; }

bool UniformData::IsSmallData() const { return capacity_ <= kSmallDataSize; }

void UniformData::Alloc(size_t size) {
  if (size <= capacity_) {
    // We already have enough space.
    return;
  }

  Free();
  if (size > kSmallDataSize) {
    heap_data_ = new uint8_t[size];
    capacity_ = size;
  }
}

void UniformData::Free() {
  if (!IsSmallData()) {
    delete[] heap_data_;
    capacity_ = kSmallDataSize;
  }
}
size_t UniformData::ShaderDataTypeToBytesSize(ShaderDataType type) {
  switch (type) {
    case ShaderDataType_Float1:
      return sizeof(float);
    case ShaderDataType_Float2:
      return sizeof(float) * 2;
    case ShaderDataType_Float3:
      return sizeof(float) * 3;
    case ShaderDataType_Float4:
      return sizeof(float) * 4;
    case ShaderDataType_Float2x2:
      return sizeof(float) * 4;
    case ShaderDataType_Float3x3:
      return sizeof(float) * 9;
    case ShaderDataType_Float4x4:
      return sizeof(float) * 16;
    case ShaderDataType_Int1:
      return sizeof(int);
    case ShaderDataType_Int2:
      return sizeof(int) * 2;
    case ShaderDataType_Int3:
      return sizeof(int) * 3;
    case ShaderDataType_Int4:
      return sizeof(int) * 4;
    case ShaderDataType_BufferObject:
      return 1;
    default:
      LOG(DFATAL) << "Failed to convert uniform type to size.";
      return 1;
  }
}

}  // namespace detail
}  // namespace lull
