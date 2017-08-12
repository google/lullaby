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

#ifndef LULLABY_SYSTEMS_RENDER_UNIFORM_H_
#define LULLABY_SYSTEMS_RENDER_UNIFORM_H_

#include <string>
#include <vector>

namespace lull {

/// The uniform class represents uniform data of shaders and is used to copy
/// data to the uniform buffers residing on the GPU.
class Uniform {
 public:
  /// Types of uniforms supported.
  enum class Type {
    kFloats,  /// Represents a uniform of float or vector data.
    kMatrix   /// Represents a uniform of matrix type.
  };

  /// Description of a uniform.
  struct Description {
    /// The uniform name identifier.
    std::string name;
    /// The type of uniform data.
    Type type = Type::kFloats;
    /// The size of the uniform data in bytes.
    size_t num_bytes = 0;
    /// The number of instances of the data (used for arrays).
    int count = 1;
    /// The uniform binding handle at the shader (internal, do not use).
    int binding = -1;

    Description() = default;
    Description(std::string name, Type type, size_t num_bytes, int count = 1,
                int binding = -1)
        : name(std::move(name)),
          type(type),
          num_bytes(num_bytes),
          count(count),
          binding(binding) {}
  };

  /// Constructs a uniform from a uniform description.
  explicit Uniform(const Description& desc);
  /// Returns the uniform description.
  Description& GetDescription();
  /// Returns the uniform description.
  const Description& GetDescription() const;

  /// Retrieves the cached uniform data as a specific type.
  template <typename T>
  T* GetData() {
    return reinterpret_cast<T*>(data_.data());
  }
  /// Retrieves the cached uniform data as a specific const type.
  template <typename T>
  const T* GetData() const {
    return reinterpret_cast<const T*>(data_.data());
  }

  /// Sets the cached data. This does not immediately copy to the GPU.
  void SetData(const void* data, size_t num_bytes, size_t bytes_offset = 0);
  template <typename T>
  /// Sets the cached data. This does not immediately copy to the GPU.
  void SetData(const T* data, size_t count, size_t bytes_offset = 0) {
    SetData(reinterpret_cast<const void*>(data), sizeof(T) * count,
            bytes_offset);
  }

 private:
  /// Description of the uniform.
  Description description_;
  /// Cached uniform data.
  std::vector<char> data_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_UNIFORM_H_
