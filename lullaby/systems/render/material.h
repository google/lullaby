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

#ifndef LULLABY_SYSTEMS_RENDER_MATERIAL_H_
#define LULLABY_SYSTEMS_RENDER_MATERIAL_H_

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "lullaby/systems/render/shader.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/systems/render/uniform.h"
#include "lullaby/util/hash.h"

namespace lull {

class Material {
 public:
  using UniformIndex = size_t;

  /// Constructs an undefined material.
  Material() {}
  /// Constructs a material with a shader and initialized uniforms.
  Material(const ShaderPtr& shader,
           const std::vector<Uniform::Description>& uniform_descriptions);

  /// Sets the material's shader.
  void SetShader(const ShaderPtr& shader);
  /// Get the material's shader.
  const ShaderPtr& GetShader() const;

  /// Sets a texture to a sampler index.
  void SetTexture(int index, const TexturePtr& texture);
  /// Returns a texture bound to a sampler index.
  TexturePtr GetTexture(int index) const;

  /// Adds a uniform without any data.
  UniformIndex AddUniform(const Uniform::Description& description);
  /// Adds a uniform as a copy of another uniform and copies its data.
  UniformIndex AddUniform(Uniform uniform);

  /// Clears all uniforms and their descriptions.
  void ClearUniforms();
  /// Updates a uniform description.
  void UpdateUniform(const Uniform::Description& description);

  /// Finds and returns a uniform by its name.
  Uniform* GetUniformByName(string_view name);
  /// Finds and returns a const uniform by its name.
  const Uniform* GetUniformByName(string_view name) const;
  /// Finds and returns a uniform by its index.
  Uniform* GetUniformByIndex(UniformIndex index);
  /// Finds and returns a const uniform by its index.
  const Uniform* GetUniformByIndex(UniformIndex index) const;
  /// Finds and returns a uniform by its hash.
  Uniform* GetUniformByHash(HashValue hash);
  /// Finds and returns a const uniform by its hash.
  const Uniform* GetUniformByHash(HashValue hash) const;

  /// Sets a uniform data block by index and size.
  void SetUniformByIndex(UniformIndex index, const void* data, size_t size,
                         size_t offset = 0);
  /// Sets a uniform type by index and number of instances of the type.
  template <typename T>
  void SetUniformByIndex(UniformIndex index, const T* data, size_t count,
                         size_t offset = 0) {
    SetUniformByIndex(index, reinterpret_cast<const void*>(data),
                      sizeof(T) * count, offset);
  }
  /// Sets a uniform type by hashed name and number of instances of the type.
  template <typename T>
  void SetUniformByHash(HashValue name_hash, const T* data, size_t count,
                        size_t offset = 0) {
    auto iter = name_to_uniform_index_.find(name_hash);
    if (iter != name_to_uniform_index_.end()) {
      SetUniformByIndex(iter->second, data, count, offset);
    }
  }
  /// Sets a uniform type by name and number of instances of the type.
  template <typename T>
  void SetUniformByName(const std::string& name, const T* data, size_t count,
                        size_t offset = 0) {
    size_t index = 0;
    auto iter = name_to_uniform_index_.find(lull::Hash(name));
    if (iter == name_to_uniform_index_.end()) {
      index = uniforms_.size();
      Uniform::Description desc;
      desc.name = name;
      desc.type =
          (sizeof(T) == 4) ? Uniform::Type::kFloats : Uniform::Type::kMatrix;
      desc.num_bytes = sizeof(T) * count;
      AddUniform(desc);
    } else {
      index = iter->second;
    }
    SetUniformByIndex(index, data, count, offset);
  }

  /// Returns all uniforms.
  const std::vector<Uniform>& GetUniforms() const;
  /// Returns all uniforms. (This is intended to be used by the renderer to bind
  /// the uniforms).
  std::vector<Uniform>& GetUniforms();
  /// Returns all textures. (This is intended to be used by the renderer to bind
  /// the textures).
  const std::unordered_map<int, TexturePtr>& GetTextures() const;

 private:
  ShaderPtr shader_ = nullptr;
  std::unordered_map<int, TexturePtr> textures_;
  std::vector<Uniform> uniforms_;
  std::unordered_map<HashValue, UniformIndex> name_to_uniform_index_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_MATERIAL_H_
