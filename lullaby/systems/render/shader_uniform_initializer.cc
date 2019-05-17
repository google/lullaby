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

#include "lullaby/systems/render/shader_uniform_initializer.h"

#include "lullaby/systems/render/mesh.h"
#include "lullaby/systems/render/render_system.h"

namespace lull {

namespace {
constexpr int kMaxDimension = 16;  // Per lull::RenderSystem docs.
// Maps valid dimensions to Lullaby's shader data types.
ShaderDataType GetType(int dimension) {
  CHECK_LE(dimension, kMaxDimension);
  switch (dimension) {
    case 1:
      return ShaderDataType_Float1;
    case 2:
      return ShaderDataType_Float2;
    case 3:
      return ShaderDataType_Float3;
    case 4:
      return ShaderDataType_Float4;
    case 9:
      return ShaderDataType_Float3x3;
    case 16:
      return ShaderDataType_Float4x4;
  }
  LOG(FATAL) << "Unsupported shader uniform dimension: " << dimension;
  return ShaderDataType_MAX;
}
// Shared implementation for setting uniforms that aren't already set in the
// given entity, across all passes and submeshes.
void InitializeUniformHelper(Registry* registry, Entity entity,
                             const char* uniform_name, ShaderDataType type,
                             const Span<uint8_t>& span, int count) {
  auto* render_system =
      CHECK_NOTNULL(CHECK_NOTNULL(registry)->Get<RenderSystem>());
  const auto passes = render_system->GetRenderPasses(entity);
  uint8_t dummy;
  for (const auto& pass : passes) {
    MeshPtr mesh = render_system->GetMesh({entity, pass});
    const int submeshes = static_cast<int>(GetNumSubmeshes(mesh));
    for (int submesh = 0; submesh < submeshes; ++submesh) {
      if (!render_system->GetUniform({entity, pass, submesh}, uniform_name,
                                     sizeof(dummy), &dummy)) {
        render_system->SetUniform({entity, pass, submesh}, uniform_name, type,
                                  span, count);
      }
    }
  }
  // Sets defaults for the default material in case the above loops iterate over
  // nothing (e.g. entity not fully loaded yet).
  if (!render_system->GetUniform(entity, uniform_name,
                                 sizeof(dummy), &dummy)) {
    render_system->SetUniform(entity, uniform_name, type,
                              span, count);
  }
}
}  // namespace

void InitializeUniform(Registry* registry, Entity entity,
                       const char* uniform_name, int dimension) {
  InitializeUniform(registry, entity, uniform_name, dimension, 1);
}

void InitializeUniform(Registry* registry, Entity entity,
                       const char* uniform_name, int dimension, int count) {
  const auto type = GetType(dimension);
  const int total_floats = dimension * count;
  const std::vector<float> zeros(total_floats);
  const Span<uint8_t> span(reinterpret_cast<const uint8_t*>(zeros.data()),
                           total_floats * sizeof(float));
  InitializeUniformHelper(registry, entity, uniform_name, type, span, count);
}

void InitializeUniform(Registry* registry, Entity entity,
                       const char* uniform_name,
                       std::initializer_list<float> values) {
  const int dimension = static_cast<int>(values.size());
  const auto type = GetType(dimension);
  const Span<uint8_t> span(reinterpret_cast<const uint8_t*>(std::begin(values)),
                           dimension * sizeof(float));
  InitializeUniformHelper(registry, entity, uniform_name, type, span, 1);
}

}  // namespace lull
