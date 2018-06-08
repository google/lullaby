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

#include "lullaby/systems/render/shader_uniform_initializer.h"

#include "lullaby/systems/render/render_system.h"

namespace lull {

namespace {
constexpr int kMaxDimension = 16;  // Per lull::RenderSystem docs.
RenderSystem* GetRenderSystem(Registry* registry) {
  return CHECK_NOTNULL(CHECK_NOTNULL(registry)->Get<RenderSystem>());
}
}  // namespace

void InitializeUniform(Registry* registry, Entity entity,
                       const char* uniform_name, int dimension) {
  CHECK(dimension <= kMaxDimension);
  float dummy[kMaxDimension];
  auto* render_system = GetRenderSystem(registry);
  if (!render_system->GetUniform(entity, uniform_name, dimension, dummy)) {
    constexpr float kZeros[kMaxDimension] = { 0.0f };
    render_system->SetUniform(entity, uniform_name, kZeros, dimension);
  }
}

void InitializeUniform(Registry* registry, Entity entity,
                       const char* uniform_name, int dimension, int count) {
  const int total_floats = dimension * count;
  std::vector<float> zeros(total_floats);
  auto* render_system = GetRenderSystem(registry);
  if (!render_system->GetUniform(entity, uniform_name, total_floats,
                                 zeros.data())) {
    // zeros was likely untouched, but just to be safe...
    std::fill(zeros.begin(), zeros.end(), 0.0f);
    render_system->SetUniform(entity, uniform_name, zeros.data(), dimension,
                                count);
  }
}

void InitializeUniform(Registry* registry, Entity entity,
                       const char* uniform_name,
                       std::initializer_list<float> values) {
  const int dimension = static_cast<int>(values.size());
  CHECK(dimension <= kMaxDimension);
  float dummy[kMaxDimension];
  auto* render_system = GetRenderSystem(registry);
  if (!render_system->GetUniform(entity, uniform_name, dimension, dummy)) {
    render_system->SetUniform(entity, uniform_name, std::begin(values),
                              dimension);
  }
}

}  // namespace lull
