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

#include "lullaby/systems/render/next/material.h"

#include <utility>

#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/systems/render/next/shader.h"
#include "lullaby/systems/render/next/texture.h"
#include "lullaby/util/logging.h"

namespace lull {

Material::Material() {
  textures_.resize(MaterialTextureUsage_MAX + 1);
}

void Material::SetShader(const ShaderPtr& shader) {
  shader_ = shader;

  // Update bindings for all uniforms based on the new shader.
  for (auto& iter : uniform_index_map_) {
    Uniform& uniform = uniforms_[iter.second];
    if (shader_) {
      uniform.binding = shader_->FindUniform(iter.first);
    } else {
      uniform.binding.Reset();
    }
  }

  // Update bindings for all samplers based on the new shader.
  for (auto& iter : sampler_index_map_) {
    Uniform& uniform = uniforms_[iter.second];
    if (shader_ ) {
      const Shader::Sampler sampler = shader_->GetSampler(iter.first);
      uniform.data.SetData(&sampler.texture_unit, 1);
      uniform.binding = sampler.uniform;
    } else {
      uniform.binding.Reset();
    }
  }
}

const ShaderPtr& Material::GetShader() const { return shader_; }

void Material::SetTexture(MaterialTextureUsage usage,
                          const TexturePtr& texture) {
  if (usage < MaterialTextureUsage_MIN || usage > MaterialTextureUsage_MAX) {
    LOG(DFATAL) << "Invalid usage: " << usage;
    return;
  }

  textures_[usage] = texture;

  // Ether create or find a new uniform and assign it the appropriate sampler
  // from the current shader.
  size_t index = uniforms_.size();
  auto iter = sampler_index_map_.find(usage);
  if (iter != sampler_index_map_.end()) {
    index = iter->second;
  } else {
    // Samplers are 1-dimensional integers referencing the texture unit.
    uniforms_.emplace_back(ShaderDataType_Int1, 1);
    sampler_index_map_[usage] = index;
  }

  Uniform& uniform = uniforms_[index];
  if (shader_) {
    const Shader::Sampler sampler = shader_->GetSampler(usage);
    uniform.data.SetData(&sampler.texture_unit, 1);
    uniform.binding = sampler.uniform;
  }
}

TexturePtr Material::GetTexture(MaterialTextureUsage usage) const {
  if (usage < MaterialTextureUsage_MIN || usage > MaterialTextureUsage_MAX) {
    LOG(DFATAL) << "Invalid usage: " << usage;
    return nullptr;
  }
  return textures_[usage];
}

void Material::SetUniform(HashValue name, ShaderDataType type,
                          Span<uint8_t> data) {
  const size_t bytes_per_element = UniformData::ShaderDataTypeToBytesSize(type);
  const size_t count = data.size() / bytes_per_element;

  // Ether create or find a new uniform and assign it the provided data value
  // and appropriate uniform location from the current shader.
  size_t index = uniforms_.size();
  auto iter = uniform_index_map_.find(name);
  if (iter != uniform_index_map_.end()) {
    index = iter->second;
  } else {
    uniforms_.emplace_back(type, static_cast<int>(count));
    uniform_index_map_[name] = index;
  }

  Uniform& uniform = uniforms_[index];
  if (uniform.data.Type() != type || uniform.data.Count() != count) {
    uniforms_[index].data = UniformData(type, static_cast<int>(count));
  }

  uniform.data.SetData(data.data(), data.size());
  uniform.binding = shader_ ? shader_->FindUniform(name).Get() : UniformHnd();
}

const UniformData* Material::GetUniformData(HashValue name) const {
  auto iter = uniform_index_map_.find(name);
  if (iter == uniform_index_map_.end()) {
    return nullptr;
  }
  return &uniforms_[iter->second].data;
}

void Material::ApplyProperties(const VariantMap& properties) {
  auto iter = properties.find(ConstHash("IsOpaque"));
  if (iter != properties.end()) {
    const Variant& is_opaque = iter->second;
    if (is_opaque.ValueOr(true)) {
      BlendStateT blend_state;
      blend_state.enabled = false;
      SetBlendState(&blend_state);

      DepthStateT depth_state;
      depth_state.function = RenderFunction_Less;
      depth_state.test_enabled = true;
      depth_state.write_enabled = true;
      SetDepthState(&depth_state);
    } else {
      BlendStateT blend_state;
      blend_state.enabled = true;
      blend_state.dst_color = BlendFactor_OneMinusSrcAlpha;
      blend_state.dst_alpha = BlendFactor_OneMinusSrcAlpha;
      SetBlendState(&blend_state);

      DepthStateT depth_state;
      depth_state.function = RenderFunction_Less;
      depth_state.test_enabled = true;
      depth_state.write_enabled = false;
      SetDepthState(&depth_state);
    }
  }

  for (const auto& iter : properties) {
    const HashValue name = iter.first;
    const Variant& var = iter.second;
    const TypeId type = var.GetTypeId();
    if (type == GetTypeId<float>()) {
      SetUniform<float>(name, ShaderDataType_Float1, {var.Get<float>(), 1});
    } else if (type == GetTypeId<mathfu::vec2>()) {
      SetUniform<float>(name, ShaderDataType_Float2,
                        {&var.Get<mathfu::vec2>()->x, 1});
    } else if (type == GetTypeId<mathfu::vec3>()) {
      SetUniform<float>(name, ShaderDataType_Float3,
                        {&var.Get<mathfu::vec3>()->x, 1});
    } else if (type == GetTypeId<mathfu::vec4>()) {
      SetUniform<float>(name, ShaderDataType_Float4,
                        {&var.Get<mathfu::vec4>()->x, 1});
    }
  }
}

template <typename T>
static Span<uint8_t> ToSpan(ShaderDataType type, const T& data) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data());
  const size_t num_bytes =
      data.size() * UniformData::ShaderDataTypeToBytesSize(type);
  return {bytes, num_bytes};
}

void Material::Bind(int max_texture_units) {
  if (shader_ == nullptr) {
    return;
  }

  for (const Uniform& uniform : uniforms_) {
    shader_->BindUniform(uniform.binding, uniform.data);
  }

  for (size_t i = 0; i < textures_.size(); ++i) {
    TexturePtr texture = textures_[i];

    const MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(i);
    const int texture_unit = shader_->GetSampler(usage).texture_unit;
    if (texture_unit < 0 || texture_unit >= max_texture_units) {
      if (texture) {
        LOG(ERROR) << "Invalid unit for texture: " << texture->GetName();
      }
      continue;
    }

    if (texture) {
      texture->Bind(texture_unit);
    } else {
      GL_CALL(glActiveTexture(GL_TEXTURE0 + texture_unit));
      GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
    }
  }

  const Shader::Description& desc = shader_->GetDescription();
  for (const auto& shader_uniform : desc.uniforms) {
    const HashValue name = Hash(shader_uniform.name);

    // This uniform has been set by the material, so no need to set a default
    // value for it.
    if (uniform_index_map_.find(name) != uniform_index_map_.end()) {
      continue;
    }

    if (!shader_uniform.values.empty()) {
      const UniformHnd binding = shader_->FindUniform(name);
      const Span<uint8_t> data =
          ToSpan(shader_uniform.type, shader_uniform.values);
      shader_->BindUniform(binding, shader_uniform.type, data);
    } else if (!shader_uniform.values_int.empty()) {
      const UniformHnd binding = shader_->FindUniform(name);
      const Span<uint8_t> data =
          ToSpan(shader_uniform.type, shader_uniform.values_int);
      shader_->BindUniform(binding, shader_uniform.type, data);
    }
  }
}

void Material::CopyUniforms(const Material& rhs) {
  for (const auto& iter : rhs.uniform_index_map_) {
    const Uniform& uniform = rhs.uniforms_[iter.second];
    const uint8_t* bytes = uniform.data.GetData<uint8_t>();
    const size_t size = uniform.data.Size();
    SetUniform(iter.first, uniform.data.Type(), {bytes, size});
  }
}

void Material::SetBlendState(const BlendStateT* blend_state) {
  if (blend_state) {
    blend_state_ = *blend_state;
  } else {
    blend_state_.reset();
  }
}

void Material::SetCullState(const CullStateT* cull_state) {
  if (cull_state) {
    cull_state_ = *cull_state;
  } else {
    cull_state_.reset();
  }
}

void Material::SetDepthState(const DepthStateT* depth_state) {
  if (depth_state) {
    depth_state_ = *depth_state;
  } else {
    depth_state_.reset();
  }
}

void Material::SetPointState(const PointStateT* point_state) {
  if (point_state) {
    point_state_ = *point_state;
  } else {
    point_state_.reset();
  }
}

void Material::SetStencilState(const StencilStateT* stencil_state) {
  if (stencil_state) {
    stencil_state_ = *stencil_state;
  } else {
    stencil_state_.reset();
  }
}

const BlendStateT* Material::GetBlendState() const {
  return blend_state_.get();
}

const CullStateT* Material::GetCullState() const {
  return cull_state_.get();
}

const DepthStateT* Material::GetDepthState() const {
  return depth_state_.get();
}

const PointStateT* Material::GetPointState() const {
  return point_state_.get();
}

const StencilStateT* Material::GetStencilState() const {
  return stencil_state_.get();
}

}  // namespace lull
