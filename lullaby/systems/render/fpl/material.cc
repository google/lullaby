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

#include "lullaby/systems/render/fpl/material.h"

#include <utility>

#include "lullaby/util/logging.h"

namespace lull {

Material::Material(
    const ShaderPtr& shader,
    const std::vector<Uniform::Description>& uniform_descriptions) {
  SetShader(shader);
  for (const auto& it : uniform_descriptions) {
    AddUniform(it);
  }
}

void Material::SetShader(const ShaderPtr& shader) {
  shader_ = shader;
  for (auto& uniform : uniforms_) {
    uniform.GetDescription().binding = -1;
  }
}
const ShaderPtr& Material::GetShader() const { return shader_; }

void Material::SetTexture(int index, const TexturePtr& texture) {
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(index, 0);
  DCHECK_LE(index, 255);

  if (texture == nullptr) {
    textures_.erase(index);
  } else {
    textures_[index] = texture;
  }
}
TexturePtr Material::GetTexture(int index) const {
  const auto it = textures_.find(index);
  if (it == textures_.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}

Material::UniformIndex Material::AddUniform(
    const Uniform::Description& description) {
  const UniformIndex index = uniforms_.size();

  uniforms_.emplace_back(description);
  name_to_uniform_index_[Hash(description.name)] = index;

  return index;
}

Material::UniformIndex Material::AddUniform(Uniform uniform) {
  const UniformIndex index = static_cast<UniformIndex>(uniforms_.size());
  Uniform::Description& desc = uniform.GetDescription();
  desc.binding = -1;

  name_to_uniform_index_[Hash(desc.name)] = index;
  uniforms_.emplace_back(std::move(uniform));

  return index;
}

void Material::ClearUniforms() {
  name_to_uniform_index_.clear();
  uniforms_.clear();
}

Uniform* Material::GetUniformByName(string_view name) {
  return GetUniformByHash(Hash(name));
}

const Uniform* Material::GetUniformByName(string_view name) const {
  return GetUniformByHash(Hash(name));
}

Uniform* Material::GetUniformByIndex(UniformIndex index) {
  if (index < uniforms_.size()) {
    return &uniforms_[index];
  }

  return nullptr;
}

const Uniform* Material::GetUniformByIndex(UniformIndex index) const {
  if (index < uniforms_.size()) {
    return &uniforms_[index];
  }

  return nullptr;
}

Uniform* Material::GetUniformByHash(HashValue hash) {
  const auto it = name_to_uniform_index_.find(hash);
  if (it == name_to_uniform_index_.end()) {
    return nullptr;
  }

  return GetUniformByIndex(it->second);
}

const Uniform* Material::GetUniformByHash(HashValue hash) const {
  const auto it = name_to_uniform_index_.find(hash);
  if (it == name_to_uniform_index_.end()) {
    return nullptr;
  }

  return GetUniformByIndex(it->second);
}

void Material::SetUniform(Uniform uniform) {
  const HashValue name_hash = Hash(uniform.GetDescription().name);
  auto iter = name_to_uniform_index_.find(name_hash);
  if (iter != name_to_uniform_index_.end()) {
    uniforms_[iter->second] = std::move(uniform);
  } else {
    name_to_uniform_index_[name_hash] = uniforms_.size();
    uniforms_.emplace_back(std::move(uniform));
  }
}

const std::vector<Uniform>& Material::GetUniforms() const { return uniforms_; }
std::vector<Uniform>& Material::GetUniforms() { return uniforms_; }
const std::unordered_map<int, TexturePtr>& Material::GetTextures() const {
  return textures_;
}

void Material::SetBlendState(const fplbase::BlendState* blend_state) {
  if (blend_state) {
    blend_state_ = *blend_state;
  } else {
    blend_state_.reset();
  }
}

void Material::SetCullState(const fplbase::CullState* cull_state) {
  if (cull_state) {
    cull_state_ = *cull_state;
  } else {
    cull_state_.reset();
  }
}

void Material::SetDepthState(const fplbase::DepthState* depth_state) {
  if (depth_state) {
    depth_state_ = *depth_state;
  } else {
    depth_state_.reset();
  }
}

void Material::SetPointState(const fplbase::PointState* point_state) {
  if (point_state) {
    point_state_ = *point_state;
  } else {
    point_state_.reset();
  }
}

void Material::SetStencilState(const fplbase::StencilState* stencil_state) {
  if (stencil_state) {
    stencil_state_ = *stencil_state;
  } else {
    stencil_state_.reset();
  }
}

const fplbase::BlendState* Material::GetBlendState() const {
  return blend_state_.get();
}

const fplbase::CullState* Material::GetCullState() const {
  return cull_state_.get();
}

const fplbase::DepthState* Material::GetDepthState() const {
  return depth_state_.get();
}

const fplbase::PointState* Material::GetPointState() const {
  return point_state_.get();
}

const fplbase::StencilState* Material::GetStencilState() const {
  return stencil_state_.get();
}

}  // namespace lull
