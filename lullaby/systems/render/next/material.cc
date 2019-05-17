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

#include "lullaby/systems/render/next/material.h"

#include <utility>

#include "lullaby/generated/flatbuffers/render_state_def_generated.h"
#include "lullaby/generated/flatbuffers/shader_def_generated.h"
#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/systems/render/next/next_renderer.h"
#include "lullaby/systems/render/next/shader.h"
#include "lullaby/systems/render/next/texture.h"
#include "lullaby/util/logging.h"

namespace lull {

void Material::SetShader(const ShaderPtr& shader) { shader_ = shader; }

const ShaderPtr& Material::GetShader() const { return shader_; }

void Material::SetTexture(TextureUsageInfo usage, const TexturePtr& texture) {
  textures_[usage] = texture;
}

TexturePtr Material::GetTexture(TextureUsageInfo usage) const {
  auto iter = textures_.find(usage);
  return iter != textures_.end() ? iter->second : nullptr;
}

void Material::SetUniform(HashValue name, ShaderDataType type,
                          Span<uint8_t> data) {
  uniforms_[name].SetData(type, data);
}

const detail::UniformData* Material::GetUniformData(HashValue name) const {
  auto iter = uniforms_.find(name);
  return iter != uniforms_.end() ? &iter->second.GetUniformDataObject()
                                 : nullptr;
}

bool Material::ReadUniformData(HashValue name, size_t length,
                               uint8_t* data_out) const {
  const detail::UniformData* uniform = GetUniformData(name);
  if (uniform == nullptr) {
    return false;
  }
  if (length > uniform->Size()) {
    return false;
  }
  memcpy(data_out, uniform->GetData<uint8_t>(), length);
  return true;
}

void Material::SetIsOpaque(bool is_opaque) {
  BlendStateT blend_state;
  DepthStateT depth_state;
  if (is_opaque) {
    blend_state.enabled = false;
    depth_state.function = RenderFunction_Less;
    depth_state.test_enabled = true;
    depth_state.write_enabled = true;
  } else {
    blend_state.enabled = true;
    blend_state.dst_color = BlendFactor_OneMinusSrcAlpha;
    blend_state.dst_alpha = BlendFactor_OneMinusSrcAlpha;
    depth_state.function = RenderFunction_Less;
    depth_state.test_enabled = true;
    depth_state.write_enabled = false;
  }
  SetBlendState(&blend_state);
  SetDepthState(&depth_state);
}

void Material::SetDoubleSided(bool double_sided) {
  CullStateT cull_state;
  if (double_sided) {
    cull_state.enabled = false;
  } else {
    cull_state.enabled = true;
    cull_state.face = CullFace_Back;
    cull_state.front = FrontFace_CounterClockwise;
  }
  SetCullState(&cull_state);
}

void Material::ApplyProperties(const VariantMap& properties) {
  auto iter = properties.find(ConstHash("IsOpaque"));
  if (iter != properties.end()) {
    SetIsOpaque(iter->second.ValueOr(true));
  }

  iter = properties.find(ConstHash("DoubleSided"));
  if (iter != properties.end()) {
    SetDoubleSided(iter->second.ValueOr(true));
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
    } else if (type == GetTypeId<bool>()) {
      RequestShaderFeature(name);
    }
  }
}

bool Material::IsLoaded() const {
  if (!shader_) {
    return false;
  }
  for (const auto& pair : textures_) {
    const TexturePtr texture = pair.second;
    if (!texture->IsLoaded()) {
      return false;
    }
  }
  return true;
}

void Material::Bind() {
  if (shader_ == nullptr) {
    return;
  }

  for (auto& iter : uniforms_) {
    if (shader_->IsUniformBlock(iter.first)) {
      shader_->BindUniformBlock(iter.first, iter.second.UniformBuffer());
    } else {
      shader_->BindUniform(iter.first, iter.second.Type(), iter.second.Data());
    }
  }

  for (auto& pair : textures_) {
    shader_->BindSampler(pair.first, pair.second);
  }

  // Bind default uniforms and samplers for any data in the shader that was not
  // set explicitly above.
  const ShaderDescription& desc = shader_->GetDescription();
  for (const auto& uniform : desc.uniforms) {
    const HashValue name = Hash(uniform.name);
    if (uniforms_.find(name) == uniforms_.end()) {
      shader_->BindShaderUniformDef(uniform);
    }
  }
  for (const auto& sampler : desc.samplers) {
    const TextureUsageInfo usage = TextureUsageInfo(sampler);
    if (textures_.find(usage) == textures_.end()) {
      shader_->BindShaderSamplerDef(sampler);
    }
  }
}

void Material::CopyUniforms(const Material& rhs) {
  for (const auto& iter : rhs.uniforms_) {
    SetUniform(iter.first, iter.second.Type(), iter.second.Data());
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

const CullStateT* Material::GetCullState() const { return cull_state_.get(); }

const DepthStateT* Material::GetDepthState() const {
  return depth_state_.get();
}

const PointStateT* Material::GetPointState() const {
  return point_state_.get();
}

const StencilStateT* Material::GetStencilState() const {
  return stencil_state_.get();
}

void Material::RequestShaderFeature(HashValue feature) {
  requested_shader_features_.insert(feature);
}

void Material::ClearShaderFeature(HashValue feature) {
  requested_shader_features_.erase(feature);
}

bool Material::IsShaderFeatureRequested(HashValue feature) const {
  return requested_shader_features_.count(feature) > 0;
}

void Material::AddEnvironmentFlags(std::set<HashValue>* environment) const {
  for (const auto& pair : textures_) {
    const TextureUsageInfo& usage_info = pair.first;
    const HashValue hash_value = usage_info.GetHash();
    environment->insert(hash_value);
  }
  for (const auto& pair : uniforms_) {
    const HashValue uniform_name = pair.first;
    environment->insert(uniform_name);
  }
}

void Material::AddFeatureFlags(std::set<HashValue>* features) const {
  for (const auto& pair : textures_) {
    const TextureUsageInfo& usage_info = pair.first;
    const HashValue hash_value = usage_info.GetHash();
    features->insert(hash_value);
  }
  features->insert(requested_shader_features_.cbegin(),
                   requested_shader_features_.cend());
}

Material::Uniform::~Uniform() { DestroyUbo(); }

Span<uint8_t> Material::Uniform::Data() const {
  return uniform_data_.GetByteSpan();
}

ShaderDataType Material::Uniform::Type() const { return uniform_data_.Type(); }

const detail::UniformData& Material::Uniform::GetUniformDataObject() const {
  return uniform_data_;
}

void Material::Uniform::SetData(ShaderDataType type, Span<uint8_t> data) {
  uniform_data_.SetData(type, data);
  dirty_ = true;
}

UniformBufferHnd Material::Uniform::UniformBuffer() {
  if (ubo_.Valid() && dirty_ == false) {
    return ubo_;
  }

  Span<uint8_t> data = Data();
  if (ubo_.Valid()) {
    if (data.size() == ubo_size_) {
      GL_CALL(glBindBuffer(GL_UNIFORM_BUFFER, *ubo_));
      GL_CALL(glBufferSubData(GL_UNIFORM_BUFFER, 0, data.size(), data.data()));
    } else {
      DestroyUbo();
    }
  }

  if (!ubo_.Valid()) {
    GLuint gl_ubo = 0;
    GL_CALL(glGenBuffers(1, &gl_ubo));
    GL_CALL(glBindBuffer(GL_UNIFORM_BUFFER, gl_ubo));
    GL_CALL(glBufferData(GL_UNIFORM_BUFFER, data.size(), data.data(),
                         GL_STATIC_DRAW));
    ubo_ = gl_ubo;
    ubo_size_ = data.size();
  }
  GL_CALL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
  return ubo_;
}

void Material::Uniform::DestroyUbo() {
  if (ubo_.Valid()) {
    GLuint gl_ubo = *ubo_;
    GL_CALL(glDeleteBuffers(1, &gl_ubo));
    ubo_.Reset();
    ubo_size_ = 0;
  }
}

}  // namespace lull
