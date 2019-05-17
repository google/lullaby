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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_MATERIAL_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_MATERIAL_H_

#include <set>
#include <unordered_map>
#include <unordered_set>
#include "lullaby/modules/render/material_info.h"
#include "lullaby/systems/render/detail/uniform_data.h"
#include "lullaby/systems/render/texture.h"

namespace lull {

// Basic information about materials.
class MaterialData {
 public:
  using FeatureSet = std::unordered_set<HashValue>;
  using UniformMap = std::unordered_map<HashValue, detail::UniformData>;
  using TextureMap = std::unordered_map<TextureUsageInfo, TexturePtr,
                                        TextureUsageInfo::Hasher>;

  bool hidden = false;
  FeatureSet features;
  UniformMap uniforms;
  TextureMap textures;
  mathfu::vec4 color = {1, 1, 1, 1};
};

// Merges a set of shader feature flags from the MaterialData.
void AddShaderFeatureFlags(const MaterialData& data,
                           std::set<HashValue>* flags);

// Merges a set of shader environment flags from the MaterialData.
void AddShaderEnvironmentFlags(const MaterialData& data,
                               std::set<HashValue>* flags);

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_MATERIAL_H_
