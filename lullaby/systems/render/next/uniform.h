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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_UNIFORM_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_UNIFORM_H_

#include <memory>
#include "lullaby/systems/render/next/render_handle.h"
#include "lullaby/generated/shader_def_generated.h"

namespace lull {

/// The uniform class represents uniform data of shaders and is used to copy
/// data to the uniform buffers residing on the GPU.
class UniformData {
 public:
  /// Constructs a uniform.
  UniformData(ShaderDataType type, int count = 1);
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

  /// Sets the cached data.
  template <typename T>
  void SetData(const T* data, size_t count) {
    SetData(reinterpret_cast<const void*>(data), sizeof(T) * count);
  }

  /// Sets the cached data.
  void SetData(const void* data, size_t num_bytes);

  /// Returns the size in bytes of this uniform.
  size_t Size() const;
  /// Returns the number of elements being stored based on the Type().
  size_t Count() const;
  /// Returns the ShaderDataType of the data being stored.
  ShaderDataType Type() const;

  /// Returns the size in bytes for a |ShaderDataType|.
  static size_t ShaderDataTypeToBytesSize(ShaderDataType type);

 private:
  uint8_t* Data();
  const uint8_t* Data() const;
  void Init(ShaderDataType type, int count);
  bool IsSmallData() const;


  enum { kSmallDataSize = 16 };

  /// The type of uniform data.
  ShaderDataType type_ = ShaderDataType_Float1;
  /// The number of instances of the data (used for arrays).
  int count_ = 0;
  /// Cached uniform data.
  union {
    uint8_t small_data_[kSmallDataSize];
    uint8_t* heap_data_ = nullptr;
  };
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_UNIFORM_H_
