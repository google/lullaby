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

#ifndef LULLABY_SYSTEMS_RENDER_RENDER_HELPERS_H_
#define LULLABY_SYSTEMS_RENDER_RENDER_HELPERS_H_

#include "lullaby/util/entity.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"

// This file contains helper functions for dealing with the RenderSystem.
namespace lull {

// Updates the alpha values for an entity and its descendants.
void SetAlphaMultiplierDescendants(Entity entity, float alpha_multiplier,
                                   const TransformSystem* transform_system,
                                   RenderSystem* render_system);

// Updates the render passes for an entity and its descendants.
void SetRenderPassDescendants(Entity entity, RenderPass pass,
                              const TransformSystem* transform_system,
                              RenderSystem* render_system);

// The default function for calculating the clip_from_model_matrix.
mathfu::mat4 CalculateClipFromModelMatrix(const mathfu::mat4& model,
                                          const mathfu::mat4& projection_view);

/// Attempts to ensure the RenderPass value is valid and fixes it for rendering.
HashValue FixRenderPass(HashValue pass);

// Maps a numerical value to the corresponding shader data type with the
// appropriate dimension.  (eg. 3 -> ShaderDataType_Float3).
// Note: when dimensions is 4, returns the Vec4 type, not the Mat2x2 type.
ShaderDataType FloatDimensionsToUniformType(int dimensions);
ShaderDataType IntDimensionsToUniformType(int dimensions);

// Calls UpdateDynamicMesh with common parameters for rendering quads with the
// VertexPT format.
inline void UpdateDynamicMeshQuadsPT(
    Entity entity, size_t quad_count,
    const std::function<void(lull::MeshData*)>& update_mesh,
    RenderSystem* render_system) {
  const int kVertsPerQuad = 4;
  const int kIndicesPerQuad = 6;
  render_system->UpdateDynamicMesh(
      entity, RenderSystem::PrimitiveType::kTriangles,
      VertexPT::kFormat, kVertsPerQuad * quad_count,
      kIndicesPerQuad * quad_count, update_mesh);
}

// Sets a specific number of bone transforms to the identity transform.
void ClearBoneTransforms(RenderSystem* render_system, Entity entity,
                         int bone_count);

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_RENDER_HELPERS_H_
