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

#ifndef LULLABY_SYSTEMS_RENDER_RENDER_HELPERS_H_
#define LULLABY_SYSTEMS_RENDER_RENDER_HELPERS_H_

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"

// This file contains helper functions for dealing with the RenderSystem.
namespace lull {

// Updates the alpha values for an entity and its descendants.
void SetAlphaMultiplierDescendants(Entity entity, float alpha_multiplier,
                                   const TransformSystem* transform_system,
                                   RenderSystem* render_system);

// The default function for calculating the clip_from_model_matrix.
mathfu::mat4 CalculateClipFromModelMatrix(const mathfu::mat4& model,
                                          const mathfu::mat4& projection_view);

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_RENDER_HELPERS_H_
