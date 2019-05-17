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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_SHADER_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_SHADER_H_

#include <unordered_map>
#include "filament/Material.h"
#include "lullaby/modules/render/material_info.h"
#include "lullaby/modules/render/shader_description.h"
#include "lullaby/systems/render/filament/filament_utils.h"
#include "lullaby/systems/render/shader.h"

namespace lull {

// Manages a filament::Material and its associated ShaderDescription.
//
// It is referred to as a Shader for legacy reasons.
class Shader {
 public:
  Shader() = default;

  // Returns information about the shader.
  const ShaderDescription& GetDescription() const;

  // Returns the property name for a uniform, or nullptr if no such property.
  const char* GetFilamentPropertyName(HashValue name) const;

  // Returns the property name for a sampler, or nullptr if no such property.
  const char* GetFilamentSamplerName(TextureUsageInfo info) const;

  // Creates a filament MaterialInstance for the managed filament Material.
  filament::MaterialInstance* CreateMaterialInstance();

 private:
  friend class ShaderFactory;
  using FMaterialPtr = FilamentResourcePtr<filament::Material>;

  void Init(FMaterialPtr material, const ShaderDescription& description);

  ShaderDescription description_;
  FMaterialPtr filament_material_;
  std::unordered_map<HashValue, size_t> uniform_name_to_description_index_map_;
  std::unordered_map<HashValue, size_t> sampler_usage_to_description_index_map_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_SHADER_H_
