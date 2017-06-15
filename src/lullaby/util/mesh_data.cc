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

#include "lullaby/util/mesh_data.h"

namespace lull {

using Index = MeshData::Index;

const Index MeshData::kInvalidIndex = static_cast<Index>(~0);

Index MeshData::AddVertices(const uint8_t* data, size_t count,
                            size_t vertex_size) {
  const size_t stride = vertex_format_.GetVertexSize();
  if (vertex_size != stride) {
    LOG(DFATAL) << "Invalid vertex size: " << vertex_size << " != " << stride;
    return kInvalidIndex;
  }
  // Set the number of vertices after appending, regardless of
  // whether or not appending succeeded, to guarantee that num_vertices_
  // will be the correct value.
  const size_t total_size = count * vertex_size;
  const bool appended = vertex_data_.Append(data, total_size);
  const Index first_vertex_index = num_vertices_;
  num_vertices_ = static_cast<Index>(vertex_data_.GetSize() / stride);

  if (!appended) {
    LOG(DFATAL) << "Could not append vertices to mesh.";
    return kInvalidIndex;
  }

  aabb_is_dirty_ = true;

  return first_vertex_index;
}

bool MeshData::AddIndices(const Index* list, size_t count) {
  // Verify that all the indices are in-bounds before doing any appending,
  // so we don't add any bad data to the mesh.
  for (size_t i = 0; i < count; ++i) {
    if (list[i] >= num_vertices_) {
      LOG(DFATAL) << "Index (" << list[i] << ") cannot be greater than or "
                  << "equal to the number of vertices (" << num_vertices_
                  << ")";
      return false;
    }
  }

  const bool appended = index_data_.Append(
      reinterpret_cast<const uint8_t*>(list), count * sizeof(Index));
  if (!appended) {
    LOG(DFATAL) << "Could not append indices to mesh.";
    return false;
  }

  return true;
}

Aabb MeshData::GetAabb() const {
  if (aabb_is_dirty_) {
    aabb_is_dirty_ = false;

    if (num_vertices_ == 0) {
      aabb_ = Aabb();
      return aabb_;
    }

    if (!vertex_data_.IsReadable()) {
      LOG(DFATAL) << "Can't compute aabb for MeshData with no read access";
      aabb_ = Aabb();
      return aabb_;
    }

    auto* vertices = vertex_data_.GetReadPtr();

    DCHECK(vertex_format_.GetAttributeAt(0).usage ==
           VertexAttribute::kPosition);
    mathfu::vec3 first_position;
    memcpy(&first_position, vertices, sizeof(first_position));
    aabb_.min = first_position;
    aabb_.max = first_position;

    ForEachVertexPosition(vertices + vertex_format_.GetVertexSize(),
                          num_vertices_ - 1, vertex_format_,
                          [this](const mathfu::vec3& position) {
                            aabb_.min = mathfu::vec3::Min(aabb_.min, position);
                            aabb_.max = mathfu::vec3::Max(aabb_.max, position);
                          });
  }

  return aabb_;
}

MeshData MeshData::CreateHeapCopy() const {
  MeshData copy(primitive_type_, vertex_format_, vertex_data_.CreateHeapCopy(),
                index_data_.CreateHeapCopy());
  copy.aabb_is_dirty_ = aabb_is_dirty_;
  copy.aabb_ = aabb_;
  return copy;
}

}  // namespace lull
