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

#include "lullaby/systems/blend_shape/blend_shape_system.h"

#include "lullaby/generated/blend_shape_def_generated.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/model_asset/model_asset_system.h"
#include "lullaby/systems/render/render_system.h"
#include "mathfu/utilities.h"

namespace lull {

namespace {

// Name of our system when we register with Lullaby.
constexpr HashValue kBlendShapeDefHash = ConstHash("BlendShapeDef");

// Helper function that calls the 16-bit or 32-bit version of Mesh::AddIndices.
void AddIndices(const uint8_t* indices, size_t index_size,
                const MeshData::IndexRange& range, MeshData* mesh) {
  const uint8_t* address = indices + range.start * index_size;
  const size_t indices_count = range.end - range.start;
  if (index_size == sizeof(uint16_t)) {
    mesh->AddIndices(reinterpret_cast<const uint16_t*>(address), indices_count);
  } else {
    mesh->AddIndices(reinterpret_cast<const uint32_t*>(address), indices_count);
  }
}

}  // namespace

BlendShapeSystem::BlendData::BlendData(const std::shared_ptr<ModelAsset>& asset)
    : asset(asset),
      mesh_data(asset->GetMutableMeshData()),
      vertex_format(mesh_data.GetVertexFormat()),
      vertex_count(mesh_data.GetNumVertices()),
      submesh_count(mesh_data.GetNumSubMeshes()) {
  // Stage a version of the mesh that we can modify.
  const size_t vertex_memory_needed =
      mesh_data.GetNumVertices() * vertex_format.GetVertexSize();
  computed_vertices.resize(vertex_memory_needed);
  memcpy(computed_vertices.data(), mesh_data.GetVertexBytes(),
         vertex_memory_needed);

  // Record the offset into the vertex for the attributes that we will be
  // changing (vertex and normal only).
  for (int i = 0; i < vertex_format.GetNumAttributes(); ++i) {
    auto attribute = vertex_format.GetAttributeAt(i);
    switch (attribute->usage()) {
      case VertexAttributeUsage_Position:
        position_offset = vertex_format.GetAttributeOffset(attribute);
        break;
      case VertexAttributeUsage_Normal:
        normal_offset = vertex_format.GetAttributeOffset(attribute);
        break;
      default:
        break;
    }
  }

  // Store all of the pointers to the blend vertices.
  auto* flatbuffer_blend_shapes =
      asset->GetModelDef()->lods()->Get(0)->blend_shapes();
  const size_t blend_shape_count = flatbuffer_blend_shapes->Length();
  blend_shapes.reserve(blend_shape_count);
  blend_names.reserve(blend_shape_count);
  for (int blend_index = 0; blend_index < blend_shape_count; ++blend_index) {
    auto blend_shape_def = flatbuffer_blend_shapes->Get(blend_index);
    blend_shapes.push_back(blend_shape_def->vertex_data()->data());
    blend_names.push_back(blend_shape_def->name());
  }
}

void BlendShapeSystem::BlendData::UpdateDynamicMesh(
    Entity entity, RenderSystem* render_system, const Span<float>& weights) {
  render_system->UpdateDynamicMesh(
      entity, MeshData::PrimitiveType::kTriangles, vertex_format, vertex_count,
      mesh_data.GetNumIndices(), mesh_data.GetIndexType(), submesh_count,
      [this, &weights](MeshData* mesh) { UpdateMesh(weights, mesh); });
}

void BlendShapeSystem::BlendData::WriteComputedVertex(
    size_t index, const mathfu::vec3& position, const mathfu::vec3& normal) {
  const size_t vertex_offset = index * vertex_format.GetVertexSize();
  memcpy(computed_vertices.data() + vertex_offset + position_offset,
         &position[0], sizeof(float) * 3);
  memcpy(computed_vertices.data() + vertex_offset + normal_offset, &normal[0],
         sizeof(float) * 3);
}

bool BlendShapeSystem::BlendData::ReadVertex(const uint8_t* vertices,
                                             size_t vertex_index,
                                             mathfu::vec3* out_position,
                                             mathfu::vec3* out_normal) const {
  const size_t vertex_offset = vertex_index * vertex_format.GetVertexSize();
  if (vertex_index >= vertex_count) {
    return false;
  }
  *out_position = mathfu::vec3(reinterpret_cast<const float*>(
      vertices + vertex_offset + position_offset));
  *out_normal = mathfu::vec3(
      reinterpret_cast<const float*>(vertices + vertex_offset + normal_offset));
  return true;
}

void BlendShapeSystem::BlendData::WriteIndices(MeshData* output_mesh) {
  const uint8_t* neutral_indices = mesh_data.GetIndexBytes();
  for (int submesh_counter = 0; submesh_counter < submesh_count;
       ++submesh_counter) {
    MeshData::IndexRange submesh = mesh_data.GetSubMesh(submesh_counter);
    AddIndices(neutral_indices, mesh_data.GetIndexSize(), submesh, output_mesh);
  }
}

void BlendShapeSystem::BlendData::UpdateMesh(const Span<float>& weights,
                                             MeshData* output_mesh) {
  BlendVertexParams blend_params;

  for (size_t vertex_loop = 0; vertex_loop < vertex_count; ++vertex_loop) {
    // Get the original position and normal for this vertex.
    ReadVertex(mesh_data.GetVertexBytes(), vertex_loop,
               &blend_params.neutral_pos, &blend_params.neutral_norm);

    // The calculated position will start at the neutral position and will be
    // deflected by each applicable blend shape.
    blend_params.calculated_pos = blend_params.neutral_pos;

    // If we knew the total weight available to distribute across the blends, we
    // could properly weight the neutral_norm.  But we don't, so we assume the
    // neutral norm goes unused unless no other normal is considered.
    blend_params.calculated_norm = {0, 0, 0};

    bool blend_shapes_used = false;
    for (size_t blend_index = 0; blend_index < blend_shapes.size();
         ++blend_index) {
      blend_params.weight = mathfu::Clamp(weights[blend_index], 0.f, 1.f);

      // Don't waste cycles if clamped weight is zero.
      if (blend_params.weight == 0) {
        continue;
      }

      // Perform the blend on each vertex in turn.
      ReadVertex(blend_shapes[blend_index], vertex_loop,
                 &blend_params.blend_pos, &blend_params.blend_norm);
      // The result will be in blend_params.calculated_pos and
      // blend_params.calculated_norm.
      blend_shapes_used |= BlendVertex(&blend_params);
    }

    // If no other blend shape's normal was considered, use the neutral normal.
    if (!blend_shapes_used) {
      blend_params.calculated_norm = blend_params.neutral_norm;
    }
    blend_params.calculated_norm.Normalize();

    // Store the vertex into our computed_vertices.
    WriteComputedVertex(vertex_loop, blend_params.calculated_pos,
                        blend_params.calculated_norm);
  }

  output_mesh->AddVertices(computed_vertices.data(), vertex_count,
                           vertex_format.GetVertexSize());

  // RenderSystemNext will create a new range for every invocation of
  // AddIndices, so although it's a bit awkward, we need to add in batches.
  WriteIndices(output_mesh);
}

HashValue BlendShapeSystem::BlendData::GetBlendName(size_t index) const {
  return blend_names[index];
}

Optional<size_t> BlendShapeSystem::BlendData::FindBlendIndex(
    HashValue name) const {
  size_t diff = std::find(blend_names.begin(), blend_names.end(), name) -
                blend_names.begin();
  if (diff == blend_names.size()) {
    return NullOpt;
  }
  return diff;
}

BlendShapeSystem::BlendShapeSystem(Registry* registry)
    : System(registry),
      models_(ResourceManager<ModelAsset>::kCacheFullyOnCreate) {
  RegisterDef(this, kBlendShapeDefHash);
  RegisterDependency<ModelAssetSystem>(this);
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
  const MeshData& mesh_data = iter->second.asset->GetMutableMeshData();
  return mesh_data.GetNumVertices();
}

bool BlendShapeSystem::ReadVertex(Entity entity, size_t vertex_index,
                                  mathfu::vec3* out_position,
                                  mathfu::vec3* out_normal) const {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    return false;
  }
  return iter->second.ReadVertex(iter->second.computed_vertices.data(),
                                 vertex_index, out_position, out_normal);
}

void BlendShapeSystem::OnAssetsLoaded(Entity entity, HashValue key) {
  // Retrieve the asset that we just created.
  std::shared_ptr<ModelAsset> asset = models_.Find(key);
  blends_.emplace(entity, BlendData(asset));
}

void BlendShapeSystem::PostCreateInit(Entity entity, HashValue type,
                                      const Def* def) {
  if (def == nullptr || type != kBlendShapeDefHash) {
    return;
  }
  const auto* blend_def = ConvertDef<BlendShapeDef>(def);

  // Blend system requires a ModelAssetDef to supply the mesh and blends.
  auto filename = blend_def->model()->filename()->c_str();
  registry_->Get<ModelAssetSystem>()->CreateModel(entity, filename,
                                                  blend_def->model());

  // Load the model, which calls OnAssetsLoaded when async operations are done.
  const HashValue key = Hash(filename);
  models_.Create(key, [this, entity, key, filename]() {
    auto* asset_loader = registry_->Get<AssetLoader>();
    return asset_loader->LoadAsync<ModelAsset>(
        filename, [this, entity, key]() { OnAssetsLoaded(entity, key); });
  });
}

void BlendShapeSystem::Destroy(Entity entity) { blends_.erase(entity); }

bool BlendShapeSystem::BlendVertex(BlendVertexParams* params) {
  DCHECK(params->weight >= 0.f && params->weight <= 1.f);
  if (params->weight == 0) {
    return false;
  }

  // Add the deflection to the neutral position.
  const mathfu::vec3 delta_pos =
      mathfu::Lerp(params->neutral_pos, params->blend_pos, params->weight) -
      params->neutral_pos;
  params->calculated_pos += delta_pos;

  // Normals are treated differently than positions. Normals are added together
  // and normalized at the end.
  params->calculated_norm +=
      mathfu::Lerp(params->neutral_norm, params->blend_norm, params->weight);
  return true;
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
    return HashValue(0);
  }
  return iter->second.GetBlendName(index);
}

Optional<size_t> BlendShapeSystem::FindBlendIndex(Entity entity,
                                                  HashValue name) const {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    return NullOpt;
  }
  return iter->second.FindBlendIndex(name);
}

void BlendShapeSystem::UpdateWeights(Entity entity,
                                     const Span<float>& weights) {
  auto iter = blends_.find(entity);
  if (iter == blends_.end()) {
    return;
  }
  iter->second.UpdateDynamicMesh(entity, registry_->Get<RenderSystem>(),
                                 weights);
}
}  // namespace lull
