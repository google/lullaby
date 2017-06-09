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

#ifndef LULLABY_UTIL_HEAP_DYNAMIC_MESH_H_
#define LULLABY_UTIL_HEAP_DYNAMIC_MESH_H_

#include <memory>
#include <utility>
#include <vector>

#include "lullaby/util/mesh_data.h"
#include "lullaby/util/vertex_format.h"

namespace lull {

// Stores mesh data by allocating vertex and index buffers from the heap.
class HeapDynamicMesh {
 public:
  using PrimitiveType = MeshData::PrimitiveType;
  using Index = MeshData::Index;

  HeapDynamicMesh(PrimitiveType primitive_type, const VertexFormat& format,
                  size_t max_vertices, size_t max_indices)
      : primitive_type_(primitive_type),
        vertex_data_(max_vertices * format.GetVertexSize()),
        vertex_format_(format),
        vertex_data_size_(max_vertices * format.GetVertexSize()),
        num_vertices_(0),
        index_data_(max_indices),
        max_indices_(max_indices),
        num_indices_(0) {}

  const VertexFormat& GetVertexFormat() const { return vertex_format_; }

  PrimitiveType GetPrimitiveType() const { return primitive_type_; }

  const uint8_t* GetVertexData() const { return vertex_data_.data(); }

  Index GetNumVertices() const { return num_vertices_; }

  const Index* GetIndexData() const { return index_data_.data(); }

  size_t GetNumIndices() const { return num_indices_; }

  template <typename Vertex>
  Index AddVertex(const Vertex& v);

  template <typename Vertex, typename... Args>
  Index AddVertex(Args&&... args);

  template <typename Vertex>
  Index AddVertices(const Vertex* list, size_t count);

  Index AddVertices(const uint8_t* data, size_t count, size_t vertex_size);

  void AddIndex(Index index);

  void AddIndices(const Index* list, size_t count);

  void AddIndices(std::initializer_list<Index> list);

 private:
  const PrimitiveType primitive_type_;

  std::vector<uint8_t> vertex_data_;
  const VertexFormat vertex_format_;
  const size_t vertex_data_size_;
  Index num_vertices_;

  std::vector<Index> index_data_;
  const size_t max_indices_;
  size_t num_indices_;
};

template <typename Vertex>
inline HeapDynamicMesh::Index HeapDynamicMesh::AddVertex(const Vertex& v) {
  return AddVertices<Vertex>(&v, 1);
}

template <typename Vertex, typename... Args>
inline HeapDynamicMesh::Index HeapDynamicMesh::AddVertex(Args&&... args) {
  if (!vertex_format_.Matches<Vertex>()) {
    LOG(DFATAL) << "Vertex does not match format!";
    return MeshData::kInvalidIndex;
  }
  if ((num_vertices_ + 1) * sizeof(Vertex) > vertex_data_size_) {
    LOG(DFATAL) << "Can't be greater than vertex data size "
                << vertex_data_size_;
    return MeshData::kInvalidIndex;
  }
  const Index index = num_vertices_;
  Vertex& dst = reinterpret_cast<Vertex*>(vertex_data_.data())[index];
  dst = Vertex(std::forward<Args>(args)...);
  ++num_vertices_;
  return index;
}

inline HeapDynamicMesh::Index HeapDynamicMesh::AddVertices(const uint8_t* data,
                                                           size_t count,
                                                           size_t vertex_size) {
  const size_t stride = vertex_format_.GetVertexSize();
  if (vertex_size != stride) {
    LOG(DFATAL) << "Invalid vertex size: " << vertex_size << " != " << stride;
    return MeshData::kInvalidIndex;
  }
  const size_t total_size = count * vertex_size;
  if (num_vertices_ * stride + total_size > vertex_data_size_) {
    LOG(DFATAL) << "Can't be greater than vertex data size "
                << vertex_data_size_;
    return MeshData::kInvalidIndex;
  }
  const Index index = num_vertices_;
  std::memcpy(vertex_data_.data() + index * stride, data, total_size);
  num_vertices_ = static_cast<Index>(num_vertices_ + count);
  return index;
}

template <typename Vertex>
inline HeapDynamicMesh::Index HeapDynamicMesh::AddVertices(const Vertex* list,
                                                           size_t count) {
  if (!vertex_format_.Matches<Vertex>()) {
    LOG(DFATAL) << "Vertex does not match format!";
    return MeshData::kInvalidIndex;
  }
  return AddVertices(reinterpret_cast<const uint8_t*>(list), count,
                     sizeof(Vertex));
}

inline void HeapDynamicMesh::AddIndex(Index index) { AddIndices(&index, 1); }

inline void HeapDynamicMesh::AddIndices(const Index* list, size_t count) {
  if (num_indices_ + count > max_indices_) {
    LOG(DFATAL) << "Number of indices cannot exceed " << max_indices_;
    return;
  }
#if !defined(NDEBUG) || ION_DEBUG
  for (size_t i = 0; i < count; ++i) {
    if (list[i] >= num_vertices_) {
      LOG(DFATAL) << "Index cannot exceed number of vertices (" << list[i]
                  << " >= " << num_vertices_ << ").";
      return;
    }
  }
#endif  // !defined(NDEBUG) || ION_DEBUG

  std::memcpy(index_data_.data() + num_indices_, list, count * sizeof(Index));
  num_indices_ += count;
}

inline void HeapDynamicMesh::AddIndices(std::initializer_list<Index> list) {
  AddIndices(list.begin(), list.size());
}

}  // namespace lull

#endif  // LULLABY_UTIL_HEAP_DYNAMIC_MESH_H_
