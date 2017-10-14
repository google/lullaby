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

#include "lullaby/systems/render/render_helpers.h"

#include "mathfu/glsl_mappings.h"

namespace lull {

void SetAlphaMultiplierDescendants(Entity entity, float alpha_multiplier,
                                   const TransformSystem* transform_system,
                                   RenderSystem* render_system) {
  auto fn = [alpha_multiplier, render_system](Entity child) {
    mathfu::vec4 color;
    const bool use_default = !render_system->GetColor(child, &color);
    const mathfu::vec4 default_color = render_system->GetDefaultColor(child);
    color[0] = use_default ? default_color[0] : color[0];
    color[1] = use_default ? default_color[1] : color[1];
    color[2] = use_default ? default_color[2] : color[2];
    color[3] = default_color[3] * alpha_multiplier;
    render_system->SetColor(child, color);
  };

  transform_system->ForAllDescendants(entity, fn);
}

mathfu::mat4 CalculateClipFromModelMatrix(const mathfu::mat4& model,
                                          const mathfu::mat4& projection_view) {
  return projection_view * model;
}

}  // namespace lull
