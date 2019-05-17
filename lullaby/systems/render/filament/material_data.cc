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

#include "lullaby/systems/render/filament/material_data.h"

namespace lull {

void AddShaderFeatureFlags(const MaterialData& data,
                           std::set<HashValue>* flags) {
  for (const auto& pair : data.textures) {
    flags->insert(pair.first.GetHash());
  }
  flags->insert(data.features.cbegin(), data.features.cend());
}

void AddShaderEnvironmentFlags(const MaterialData& data,
                               std::set<HashValue>* flags) {
  for (const auto& pair : data.textures) {
    flags->insert(pair.first.GetHash());
  }
  for (const auto& pair : data.uniforms) {
    flags->insert(pair.first);
  }
}

}  // namespace lull
