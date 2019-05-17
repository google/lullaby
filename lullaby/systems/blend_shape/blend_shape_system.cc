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

#include "lullaby/systems/blend_shape/blend_shape_system.h"

#include "lullaby/generated/blend_shape_def_generated.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/render/render_system.h"
#include "mathfu/constants.h"
#include "mathfu/utilities.h"

namespace lull {
namespace {

int GetAttributeOffset(const VertexFormat& format, VertexAttributeUsage usage) {
  auto* attribute = format.GetAttributeWithUsage(usage);
  if (attribute) {
    return static_cast<int>(format.GetAttributeOffset(attribute));
  } else {
    return -1;
  }
}

int GetPositionAttributeOffset(const VertexFormat& format) {
  return GetAttributeOffset(format, VertexAttributeUsage_Position);
}

int GetNormalAttributeOffset(const VertexFormat& format) {
  return GetAttributeOffset(format, VertexAttributeUsage_Normal);
}

int GetTangentAttributeOffset(const VertexFormat& format) {
  return GetAttributeOffset(format, VertexAttributeUsage_Tangent);
}

int GetOrientationAttributeOffset(const VertexFormat& format) {
  return GetAttributeOffset(format, VertexAttributeUsage_Orientation);
}

}  // namespace

void BlendShapeSystem::BlendData::UpdateMeshVertex(
    size_t index, const BlendableVertex& vertex) {
  const size_t vertex_offset = index * mesh.GetVertexFormat().GetVertexSize();
  uint8_t* mesh_vertex = mesh.GetMutableVertexBytes() + vertex_offset;

  // Only update attributes that are actually modified by the blend data as
  // indicated by a non-negative mesh and blend offset.
  if (mesh_offsets.position >= 0 && blend_offsets.position >= 0) {
    memcpy(mesh_vertex + mesh_offsets.position, &vertex.position[0],
           sizeof(float) * 3);
  }
  if (mesh_offsets.normal >= 0 && blend_offsets.normal >= 0) {
    memcpy(mesh_vertex + mesh_offsets.normal, &vertex.normal[0],
           sizeof(float) * 3);
  }
  if (mesh_offsets.tangent >= 0 && blend_offsets.tangent >= 0) {
    memcpy(mesh_vertex + mesh_offsets.tangent, &vertex.tangent[0],
           sizeof(float) * 3);
  }
  if (mesh_offsets.orientation >= 0 && blend_offsets.orientation >= 0) {
    const mathfu::vec4 packed(vertex.orientation.vector(),
                              vertex.orientation.scalar());
    memcpy(mesh_vertex + mesh_offsets.orientation, &packed[0],
           sizeof(float) * 4);
  }
}

void BlendShapeSystem::BlendData::ReadVertex(
    const uint8_t* vertices, size_t vertex_size, size_t vertex_index,
    const BlendableAttributeOffsets& offsets,
    BlendableVertex* out_vertex) const {
  const size_t vertex_offset = vertex_index * vertex_size;
  const uint8_t* vertex = vertices + vertex_offset;

  if (offsets.position >= 0) {
    out_vertex->position =
        mathfu::vec3(reinterpret_cast<const float*>(vertex + offsets.position));
  }
  if (offsets.normal >= 0) {
    out_vertex->normal =
        mathfu::vec3(reinterpret_cast<const float*>(vertex + offsets.normal));
  }
  if (offsets.tangent >= 0) {
    out_vertex->tangent =
        mathfu::vec3(reinterpret_cast<const float*>(vertex + offsets.tangent));
  }
  if (offsets.orientation >= 0) {
    mathfu::vec4 packed(
        reinterpret_cast<const float*>(vertex + offsets.orientation));
    out_vertex->orientation =
        mathfu::quat(packed[3], packed[0], packed[1], packed[2]);
  }
}

void BlendShapeSystem::BlendData::UpdateMesh(Span<float> weights) {
  if (weights.size() < blend_shapes.size()) {
    LOG(WARNING) << "Not enough weights specified, missing weights will "
                 << "default to 0.";
  }
  BlendVertexParams blend_params;

  for (size_t index = 0; index < mesh.GetNumVertices(); ++index) {
    // Get the original position, normal, and orientation for this vertex.
    ReadVertex(base_shape.GetReadPtr(), blend_vertex_size, index, blend_offsets,
               &blend_params.neutral);

    // The calculated position will start at the neutral position and will be
    // deflected by each applicable blend shape.
    blend_params.calculated.position = blend_params.neutral.position;

    if (mode == kInterpolate) {
      // If we knew the total weight available to distribute across the blends,
      // we could properly weight the neutral_norm. But we don't, so we assume
      // the neutral norm goes unused unless no other normal is considered.
      blend_params.calculated.normal = mathfu::kZeros3f;
      blend_params.calculated.tangent = mathfu::kZeros3f;
      blend_params.calculated.orientation = mathfu::quat::identity;
    } else if (mode == kDisplacement) {
      // Displacement mode simply displaces the normal and tangent and then
      // re-normalizes them.
      blend_params.calculated.normal = blend_params.neutral.normal;
      blend_params.calculated.tangent = blend_params.neutral.tangent;
    }

    bool blend_shapes_used = false;
    for (size_t blend_index = 0; blend_index < blend_shapes.size();
         ++blend_index) {
      // Don't waste cycles if no weight is present.
      if (blend_index >= weights.size()) {
        continue;
      }

      blend_params.weight = mathfu::Clamp(weights[blend_index], 0.f, 1.f);

      // Don't waste cycles if clamped weight is zero.
      if (blend_params.weight == 0.f) {
        continue;
      }
      blend_shapes_used = true;

      // Perform the blend on each vertex in turn. The result will be in
      // blend_params.calculated.
      ReadVertex(blend_shapes[blend_index].GetReadPtr(), blend_vertex_size,
                 index, blend_offsets, &blend_params.blend);
      BlendVertex(&blend_params, mode);
    }

    // If no blend shapes were considered, use the neutral normal and
    // orientation.
    if (!blend_shapes_used) {
      blend_params.calculated.normal = blend_params.neutral.normal;
      blend_params.calculated.tangent = blend_params.neutral.tangent;
      blend_params.calculated.orientation = blend_params.neutral.orientation;
    }

    // Only normalize attributes that will actually be used.
    if (mesh_offsets.normal >= 0 && blend_offsets.normal >= 0) {
      blend_params.calculated.normal.Normalize();
    }
    if (mesh_offsets.tangent >= 0 && blend_offsets.tangent >= 0) {
      blend_params.calculated.tangent.Normalize();
    }
    if (mesh_offsets.orientation >= 0 && blend_offsets.orientation >= 0) {
      blend_params.calculated.orientation.Normalize();
    }

    // Store the vertex into our computed_vertices.
    UpdateMeshVertex(index, blend_params.calculated);
  }
  current_weights.assign(weights.begin(), weights.end());
}

BlendShapeSystem::BlendShapeSystem(Registry* registry) : System(registry) {}

void BlendShapeSystem::Destroy(Entity entity) { blends_.erase(entity); }

void BlendShapeSystem::InitBlendShape(Entity entity, MeshData mesh,
                                      const VertexFormat& blend_format,
                                      DataContainer base_shape,
                                      BlendMode mode) {
  BlendData& blend = blends_[entity];
  blend.mode = mode;
  blend.mesh = std::move(mesh);
  blend.base_shape = std::move(base_shape);

  const VertexFormat& mesh_format = blend.mesh.GetVertexFormat();
  blend.mesh_offsets.position = GetPositionAttributeOffset(mesh_format);
  blend.mesh_offsets.normal = GetNormalAttributeOffset(mesh_format);
  blend.mesh_offsets.tangent = GetTangentAttributeOffset(mesh_format);

  blend.blend_offsets.position = GetPositionAttributeOffset(blend_format);
  blend.blend_offsets.normal = GetNormalAttributeOffset(blend_format);
  blend.blend_offsets.tangent = GetTangentAttributeOffset(blend_format);
  blend.blend_vertex_size = blend_format.GetVertexSize();

  // TODO: fix orientation blends.
  if (GetOrientationAttributeOffset(mesh_format) != -1 &&
      GetOrientationAttributeOffset(blend_format) != -1) {
    LOG(WARNING) << "Orientation blends are currently disabled.";
  }
}

void BlendShapeSystem::AddBlendShape(Entity entity, HashValue name,
                                     DataContainer blend_shape) {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    LOG(DFATAL) << "No blend data associated with entity: " << entity;
    return;
  }

  BlendData& blend = iter->second;
  blend.blend_names.emplace_back(name);
  blend.blend_shapes.emplace_back(std::move(blend_shape));
}

bool BlendShapeSystem::IsReady(Entity entity) const {
  auto iter = blends_.find(entity);
  return iter != blends_.end();
}

size_t BlendShapeSystem::GetVertexCount(Entity entity) const {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    return 0;
  }
  return iter->second.mesh.GetNumVertices();
}

bool BlendShapeSystem::ReadVertex(Entity entity, size_t vertex_index,
                                  BlendableVertex* out_vertex) const {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    return false;
  }
  if (vertex_index >= iter->second.mesh.GetNumVertices()) {
    return false;
  }
  iter->second.ReadVertex(iter->second.mesh.GetVertexBytes(),
                          iter->second.mesh.GetVertexFormat().GetVertexSize(),
                          vertex_index, iter->second.mesh_offsets, out_vertex);
  return true;
}

void BlendShapeSystem::BlendVertex(BlendVertexParams* params, BlendMode mode) {
  DCHECK(params->weight >= 0.f && params->weight <= 1.f);
  if (params->weight == 0) {
    return;
  }

  // TODO: only compute blend values for attributes that are
  // actually used in the mesh.
  switch (mode) {
    case kInterpolate:
      // Add the deflection from the neutral position to the calculated
      // position.
      params->calculated.position +=
          mathfu::Lerp(params->neutral.position, params->blend.position,
                       params->weight) -
          params->neutral.position;

      // Normals and tangents are treated differently than positions. They are
      // added together and normalized at the end.
      params->calculated.normal += mathfu::Lerp(
          params->neutral.normal, params->blend.normal, params->weight);
      params->calculated.tangent += mathfu::Lerp(
          params->neutral.tangent, params->blend.tangent, params->weight);

      // TODO: fix orientation blends.
      // Adding two quaternions works as expected, but multiplying a quaternion
      // by a scalar does not, so the Lerp needs to be broken down into the
      // scalar portion and vector portion.
      params->calculated.orientation +=
          mathfu::quat(mathfu::Lerp(params->neutral.orientation.scalar(),
                                    params->blend.orientation.scalar(),
                                    params->weight),
                       mathfu::Lerp(params->neutral.orientation.vector(),
                                    params->blend.orientation.vector(),
                                    params->weight));
      break;
    case kDisplacement:
      params->calculated.position += params->blend.position * params->weight;
      params->calculated.normal += params->blend.normal * params->weight;
      params->calculated.tangent += params->blend.tangent * params->weight;
      // TODO: fix orientation blends.
      params->calculated.orientation +=
          mathfu::quat(params->blend.orientation.scalar() * params->weight,
                       params->blend.orientation.vector() * params->weight);
      break;
  }
}

size_t BlendShapeSystem::GetBlendCount(Entity entity) const {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    return 0;
  }
  return iter->second.blend_shapes.size();
}

HashValue BlendShapeSystem::GetBlendName(Entity entity, size_t index) const {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    return 0;
  }
  const BlendData& blend = iter->second;
  if (index >= blend.blend_names.size()) {
    return 0;
  }
  return blend.blend_names[index];
}

Optional<size_t> BlendShapeSystem::FindBlendIndex(Entity entity,
                                                  HashValue name) const {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    return NullOpt;
  }
  const BlendData& blend = iter->second;
  for (size_t i = 0; i < blend.blend_names.size(); ++i) {
    if (blend.blend_names[i] == name) {
      return i;
    }
  }
  return NullOpt;
}

void BlendShapeSystem::UpdateWeights(Entity entity, Span<float> weights) {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    return;
  }
  iter->second.UpdateMesh(weights);
  auto* render_system = registry_->Get<RenderSystem>();
  render_system->SetMesh(entity, iter->second.mesh);
}

Span<float> BlendShapeSystem::GetWeights(Entity entity) {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    return {};
  }
  return iter->second.current_weights;
}

}  // namespace lull
