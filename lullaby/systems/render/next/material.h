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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_MATERIAL_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_MATERIAL_H_

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "lullaby/systems/render/next/render_state.h"
#include "lullaby/systems/render/next/uniform.h"
#include "lullaby/systems/render/shader.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/variant.h"
#include "lullaby/generated/material_def_generated.h"

namespace lull {

/// Represents the "look-and-feel" for rendering an object.
///
/// This class contains the shader, the uniforms, the textures, and the render
/// state used for a specific draw call.
class Material {
 public:
  /// Constructs an undefined material.
  Material();

  /// Sets the material's shader.
  void SetShader(const ShaderPtr& shader);

  /// Get the material's shader.
  const ShaderPtr& GetShader() const;

  /// Associates a texture with a specific type of usage.
  void SetTexture(MaterialTextureUsage usage, const TexturePtr& texture);

  /// Returns a texture associated with a usage.
  TexturePtr GetTexture(MaterialTextureUsage usage) const;

  /// Sets a uniform, replacing the existing one.
  template <typename T>
  void SetUniform(HashValue name, ShaderDataType type, Span<T> data);

  /// Sets a uniform using raw byte data, replacing the existing one.
  void SetUniform(HashValue name, ShaderDataType type, Span<uint8_t> data);

  /// Sets uniforms and render state properties from a variant map.
  void ApplyProperties(const VariantMap& properties);

  /// Returns the UniformData associated with the name.
  const UniformData* GetUniformData(HashValue name) const;

  /// Copies the uniforms the |rhs| into this material.
  void CopyUniforms(const Material& rhs);

  /// Binds the uniforms and samplers to the shader and prepares textures for
  /// rendering.
  void Bind(int max_texture_units);

  /// Sets the blend state associated with the material. If |nullptr|, the blend
  /// state will be unset.
  void SetBlendState(const BlendStateT* blend_state);
  /// Sets the cull state associated with the material. If |nullptr|, the cull
  /// state will be unset.
  void SetCullState(const CullStateT* cull_state);
  /// Sets the depth state associated with the material. If |nullptr|, the depth
  /// state will be unset.
  void SetDepthState(const DepthStateT* depth_state);
  /// Sets the point state associated with the material. If |nullptr|, the point
  /// state will be unset.
  void SetPointState(const PointStateT* point_state);
  /// Sets the stencil state associated with the material. If |nullptr|, the
  /// stencil state will be unset.
  void SetStencilState(const StencilStateT* stencil_state);

  /// Returns the blend state associated with the material, or |nullptr| if no
  /// state is set.
  const BlendStateT* GetBlendState() const;
  /// Returns the cull state associated with the material, or |nullptr| if no
  /// state is set.
  const CullStateT* GetCullState() const;
  /// Returns the depth state associated with the material, or |nullptr| if no
  /// state is set.
  const DepthStateT* GetDepthState() const;
  /// Returns the point state associated with the material, or |nullptr| if no
  /// state is set.
  const PointStateT* GetPointState() const;
  /// Returns the stencil state associated with the material, or |nullptr| if no
  /// state is set.
  const StencilStateT* GetStencilState() const;

 private:
  /// Stores the data and the location binding for a single uniform instance.
  struct Uniform {
    Uniform(ShaderDataType type, int count) : data(type, count) {}
    UniformHnd binding;
    UniformData data;
  };

  ShaderPtr shader_ = nullptr;
  std::vector<Uniform> uniforms_;
  std::vector<TexturePtr> textures_;
  std::unordered_map<HashValue, size_t> uniform_index_map_;
  std::unordered_map<MaterialTextureUsage, size_t> sampler_index_map_;

  // Render State.
  Optional<BlendStateT> blend_state_;
  Optional<CullStateT> cull_state_;
  Optional<DepthStateT> depth_state_;
  Optional<PointStateT> point_state_;
  Optional<StencilStateT> stencil_state_;
};

template <typename T>
void Material::SetUniform(HashValue name, ShaderDataType type, Span<T> data) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data());
  const size_t size =
      data.size() * UniformData::ShaderDataTypeToBytesSize(type);
  SetUniform(name, type, {bytes, size});
}

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_MATERIAL_H_
