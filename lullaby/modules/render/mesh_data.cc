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

#include "lullaby/modules/render/mesh_data.h"

#include <limits>

namespace lull {

const uint32_t MeshData::kInvalidIndexU32 = static_cast<uint32_t>(~0);
const uint16_t MeshData::kMaxValidIndexU16 = static_cast<uint16_t>(~0) - 1;
const uint32_t MeshData::kMaxValidIndexU32 = static_cast<uint32_t>(~0) - 1;

Optional<uint32_t> MeshData::AddVertices(const uint8_t* data, size_t count,
                                         size_t vertex_size) {
  const size_t stride = vertex_format_.GetVertexSize();
  if (vertex_size != stride) {
    LOG(DFATAL) << "Invalid vertex size: " << vertex_size << " != " << stride;
    return {};
  }
  // Set the number of vertices after appending, regardless of
  // whether or not appending succeeded, to guarantee that num_vertices_
  // will be the correct value.
  const size_t total_size = count * vertex_size;
  const bool appended = vertex_data_.Append(data, total_size);
  const uint32_t first_vertex_index = num_vertices_;

  const size_t max_num_vertices =
      static_cast<size_t>(std::numeric_limits<decltype(num_vertices_)>::max());
  const size_t num_vertices =
      static_cast<size_t>(vertex_data_.GetSize() / stride);

  if (num_vertices > max_num_vertices) {
    LOG(DFATAL) << "Vertices size exceeds limit. Vertices size: "
                << num_vertices << ", Limit: " << max_num_vertices;
    return {};
  }

  num_vertices_ = static_cast<uint32_t>(num_vertices);

  if (!appended) {
    LOG(DFATAL) << "Could not append vertices to mesh.";
    return {};
  }

  aabb_is_dirty_ = true;

  return first_vertex_index;
}

bool MeshData::AddIndex(uint32_t index) {
  if (index >= num_vertices_) {
    LOG(DFATAL) << "Index (" << index << ") cannot be greater than or equal to "
                << "the number of vertices (" << num_vertices_ << ")";
    return false;
  }

  bool appended = false;
  if (index_type_ == kIndexU16) {
    if (index > kMaxValidIndexU16) {
      LOG(DFATAL) << "Index (" << index << ") exceeds max index value";
      return false;
    }

    const uint16_t index16 = static_cast<uint16_t>(index);
    appended = index_data_.Append(reinterpret_cast<const uint8_t*>(&index16),
                                  sizeof(index16));
  } else {
    appended = index_data_.Append(reinterpret_cast<const uint8_t*>(&index),
                                  sizeof(index));
  }

  if (!appended) {
    LOG(DFATAL) << "Could not append indices to mesh.";
    return false;
  }

  bool indexed = false;
  if (index_range_data_.GetCapacity() > 0) {
    const uint32_t start = static_cast<uint32_t>(GetNumIndices() - 1);
    const uint32_t end = static_cast<uint32_t>(start + 1);
    const IndexRange range(start, end);
    indexed = index_range_data_.Append(reinterpret_cast<const uint8_t*>(&range),
                                       sizeof(range));
    if (!indexed) {
      LOG(DFATAL) << "Could not append ranges to mesh.";
      return false;
    }
  }

  if (appended && indexed) {
    ++num_submeshes_;
  }

  return true;
}

template <typename IndexT>
bool MeshData::AddIndicesImpl(const IndexT* list, size_t count) {
  if (sizeof(IndexT) != GetIndexSize()) {
    LOG(DFATAL) << "Index type mismatch";
    return false;
  }

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

  // Attempt to add this new range of indices to the range data if it is
  // supported.
  bool indexed = false;
  if (index_range_data_.GetCapacity() > 0) {
    const uint32_t start = static_cast<uint32_t>(GetNumIndices());
    const uint32_t end = static_cast<uint32_t>(start + count);
    const IndexRange range(start, end);
    indexed = index_range_data_.Append(reinterpret_cast<const uint8_t*>(&range),
                                       sizeof(range));
    if (!indexed) {
      LOG(DFATAL) << "Could not append indices to mesh.";
      return false;
    }
  }

  const bool appended = index_data_.Append(
      reinterpret_cast<const uint8_t*>(list), count * sizeof(IndexT));
  if (!appended) {
    LOG(DFATAL) << "Could not append indices to mesh.";
    return false;
  }

  if (appended && indexed) {
    ++num_submeshes_;
  }
  return true;
}

bool MeshData::AddIndices(const uint16_t* list, size_t count) {
  return AddIndicesImpl(list, count);
}

bool MeshData::AddIndices(const uint32_t* list, size_t count) {
  return AddIndicesImpl(list, count);
}

uint32_t MeshData::GetNumSubMeshes() const {
  return std::max(1U, num_submeshes_);
}

MeshData::IndexRange MeshData::GetSubMesh(size_t index) const {
  if (index == 0 && num_submeshes_ == 0) {
    return IndexRange(0, static_cast<uint32_t>(GetNumIndices()));
  }
  if (index >= num_submeshes_) {
    return IndexRange();
  }
  const IndexRange* ptr =
      reinterpret_cast<const IndexRange*>(index_range_data_.GetReadPtr());
  return ptr[index];
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

    DCHECK(vertex_format_.GetAttributeAt(0)->usage() ==
           VertexAttributeUsage_Position);
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
                index_type_, index_data_.CreateHeapCopy(),
                index_range_data_.CreateHeapCopy());
  copy.aabb_is_dirty_ = aabb_is_dirty_;
  copy.aabb_ = aabb_;
  return copy;
}

size_t MeshData::GetNumPrimitives(PrimitiveType type, IndexRange range) {
  return GetNumPrimitives(type, range.end - range.start);
}

size_t MeshData::GetNumPrimitives(PrimitiveType type, size_t num_indices) {
  switch (type) {
    case MeshData::kPoints:
      return num_indices;
    case MeshData::kLines:
      return num_indices / 2;
    case MeshData::kTriangles:
      return num_indices / 3;
    case MeshData::kTriangleFan:
      return num_indices - 2;
    case MeshData::kTriangleStrip:
      return num_indices - 2;
    default:
      return num_indices;
  }
}

size_t MeshData::GetIndexSize(IndexType index_type) {
  switch (index_type) {
    case kIndexU16:
      return sizeof(uint16_t);
    case kIndexU32:
      return sizeof(uint32_t);
    default:
      LOG(DFATAL) << "Invalid index type " << index_type;
      return 0;
  }
}

}  // namespace lull
