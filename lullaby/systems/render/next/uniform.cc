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

#include "lullaby/systems/render/next/uniform.h"

#include <cstring>

#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"

namespace lull {

UniformData::UniformData(ShaderDataType type, int count) {
  Init(type, std::max(count, 1));
}

UniformData::UniformData(const UniformData& rhs) {
  Init(rhs.type_, std::max(rhs.count_, 1));
  SetData(rhs.Data(), rhs.Size());
}

UniformData& UniformData::operator=(const UniformData& rhs) {
  if (this != &rhs) {
    Init(rhs.type_, std::max(rhs.count_, 1));
    SetData(rhs.Data(), rhs.Size());
  }
  return *this;
}

UniformData::UniformData(UniformData&& rhs)
    : type_(rhs.type_), count_(rhs.count_), heap_data_(nullptr) {
  using std::swap;
  if (IsSmallData()) {
    std::memcpy(&small_data_, rhs.Data(), rhs.Size());
  } else {
    swap(heap_data_, rhs.heap_data_);
  }
}

UniformData& UniformData::operator=(UniformData&& rhs) {
  using std::swap;
  if (this != &rhs) {
    if (rhs.IsSmallData()) {
      std::memcpy(&small_data_, rhs.Data(), rhs.Size());
    } else {
      swap(heap_data_, rhs.heap_data_);
    }
    swap(type_, rhs.type_);
    swap(count_, rhs.count_);
  }
  return *this;
}

UniformData::~UniformData() {
  Init(type_, 0);
}

void UniformData::Init(ShaderDataType type, int count) {
  const size_t old_size = Size();
  const size_t new_size = ShaderDataTypeToBytesSize(type) * count;

  if (count == 0) {
    if (old_size > kSmallDataSize) {
      delete[] heap_data_;
    }
  } else if (new_size > old_size) {
    if (old_size > kSmallDataSize) {
      delete[] heap_data_;
    }
    if (new_size > kSmallDataSize) {
      heap_data_ = new uint8_t[new_size];
    }
  }
  type_ = type;
  count_ = count;
}

void UniformData::SetData(const void* data, size_t num_bytes) {
  const size_t max_size = Size();
  if (num_bytes > max_size) {
    LOG(DFATAL) << "UniformData buffer overflow. Trying to set " << num_bytes
                << " bytes to a uniform with max size of " << max_size;
    return;
  }
  std::memcpy(Data(), data, num_bytes);
}

uint8_t* UniformData::Data() {
  return IsSmallData() ? &small_data_[0] : heap_data_;
}

const uint8_t* UniformData::Data() const {
  return IsSmallData() ? &small_data_[0] : heap_data_;
}

size_t UniformData::Size() const {
  return ShaderDataTypeToBytesSize(type_) * count_;
}

size_t UniformData::Count() const { return count_; }

ShaderDataType UniformData::Type() const { return type_; }

bool UniformData::IsSmallData() const { return Size() <= kSmallDataSize; }

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
    default:
      LOG(DFATAL) << "Failed to convert uniform type to size.";
      return 1;
  }
}

}  // namespace lull
