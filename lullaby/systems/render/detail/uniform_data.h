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

#ifndef LULLABY_SYSTEMS_RENDER_DETAIL_UNIFORM_H_
#define LULLABY_SYSTEMS_RENDER_DETAIL_UNIFORM_H_

#include <memory>
#include "lullaby/generated/shader_def_generated.h"

namespace lull {
namespace detail {

/// The uniform class represents uniform data of shaders and is used to copy
/// data to the uniform buffers residing on the GPU.
class UniformData {
 public:
  /// Constructs a uniform.
  UniformData() {}
  UniformData(const UniformData& rhs);
  UniformData& operator=(const UniformData& rhs);
  UniformData(UniformData&& rhs);
  UniformData& operator=(UniformData&& rhs);
  ~UniformData();

  /// Retrieves the cached uniform data as a specific const type.
  template <typename T>
  const T* GetData() const {
    return reinterpret_cast<const T*>(Data());
  }

  /// Retrieves the cached uniform data as a byte span.
  Span<uint8_t> GetByteSpan() const {
    return {GetData<uint8_t>(), Size()};
  }

  /// Sets the cached data.
  void SetData(ShaderDataType type, Span<uint8_t> data);


  /// Returns the size in bytes of this uniform.
  size_t Size() const;
  /// Returns the number of elements of the type of data stored in this uniform.
  /// This is basically: Size() / ShaderDataTypeToBytesSize(Type()).
  size_t Count() const;
  /// Returns the ShaderDataType of the data being stored.
  ShaderDataType Type() const;

  /// Returns the size in bytes for a |ShaderDataType|.
  static size_t ShaderDataTypeToBytesSize(ShaderDataType type);

 private:
  uint8_t* Data();
  const uint8_t* Data() const;
  void Free();
  void Alloc(size_t size);
  bool IsSmallData() const;

  enum { kSmallDataSize = 16 };

  ShaderDataType type_ = ShaderDataType_Float1;
  size_t size_ = 0;
  size_t capacity_ = kSmallDataSize;
  union {
    uint8_t small_data_[kSmallDataSize];
    uint8_t* heap_data_ = nullptr;
  };
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_DETAIL_UNIFORM_H_
