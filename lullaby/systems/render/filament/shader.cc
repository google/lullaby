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

#include "lullaby/systems/render/filament/shader.h"

namespace lull {

const ShaderDescription& Shader::GetDescription() const {
  return description_;
}

void Shader::Init(FMaterialPtr material, const ShaderDescription& description) {
  filament_material_ = std::move(material);
  description_ = description;

  for (size_t i = 0; i < description_.uniforms.size(); ++i) {
    const auto& uniforms = description_.uniforms[i];
    uniform_name_to_description_index_map_.emplace(Hash(uniforms.name), i);
  }

  for (size_t i = 0; i < description_.samplers.size(); ++i) {
    const auto& sampler = description_.samplers[i];
    if (sampler.usage_per_channel.empty()) {
      sampler_usage_to_description_index_map_.emplace(
          TextureUsageInfo(sampler.usage).GetHash(), i);
    } else {
      sampler_usage_to_description_index_map_.emplace(
          TextureUsageInfo(sampler.usage_per_channel).GetHash(), i);
    }
  }
}

filament::MaterialInstance* Shader::CreateMaterialInstance() {
  return filament_material_ ? filament_material_->createInstance() : nullptr;
}

const char* Shader::GetFilamentSamplerName(TextureUsageInfo info) const {
  auto iter = sampler_usage_to_description_index_map_.find(info.GetHash());
  if (iter == sampler_usage_to_description_index_map_.end()) {
    return nullptr;
  }
  const auto& sampler = description_.samplers[iter->second];
  if (!filament_material_->hasParameter(sampler.name.c_str())) {
    return nullptr;
  }
  return sampler.name.c_str();
}

const char* Shader::GetFilamentPropertyName(HashValue name) const {
  auto iter = uniform_name_to_description_index_map_.find(name);
  if (iter == uniform_name_to_description_index_map_.end()) {
    return nullptr;
  }
  const auto& uniform = description_.uniforms[iter->second];
  if (!filament_material_->hasParameter(uniform.name.c_str())) {
    return nullptr;
  }
  return uniform.name.c_str();
}

}  // namespace lull
