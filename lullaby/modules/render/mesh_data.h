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

#ifndef LULLABY_MODULES_RENDER_MESH_DATA_H_
#define LULLABY_MODULES_RENDER_MESH_DATA_H_

#include "lullaby/modules/render/vertex.h"
#include "lullaby/util/data_container.h"

namespace lull {

class MeshData {
 public:
  enum PrimitiveType {
    kPoints,
    kLines,
    kTriangles,
    kTriangleFan,
    kTriangleStrip,
  };

  using Index = uint16_t;

  static const Index kInvalidIndex;
  static const Index kMaxValidIndex;

  MeshData() {}

  MeshData(const PrimitiveType primitive_type,
           const VertexFormat& vertex_format, DataContainer&& vertex_data,
           DataContainer&& index_data)
      : primitive_type_(primitive_type),
        vertex_format_(vertex_format),
        vertex_data_(std::move(vertex_data)),
        index_data_(std::move(index_data)),
        // Instantiate num_vertices_ based on the number of vertices that have
        // already been appended into the container.
        num_vertices_(static_cast<Index>(vertex_data_.GetSize() /
                                         vertex_format_.GetVertexSize())) {}

  PrimitiveType GetPrimitiveType() const { return primitive_type_; }

  const VertexFormat& GetVertexFormat() const { return vertex_format_; }

  // Gets a const pointer to the vertex data as bytes. Returns nullptr if
  // the vertex DataContainer does not have read access.
  const uint8_t* GetVertexBytes() const { return vertex_data_.GetReadPtr(); }

  // Returns a mutable pointer to the vertex data as bytes. Returns nullptr if
  // the vertex DataContainer does not have read+write access.
  uint8_t* GetMutableVertexBytes() { return vertex_data_.GetData(); }

  // Gets a const pointer to the vertex data of the mesh. Returns nullptr
  // if Vertex doesn't match format or the vertex DataContainer does not have
  // read access.
  template <typename Vertex>
  const Vertex* GetVertexData() const {
    if (!vertex_format_.Matches<Vertex>()) {
      LOG(DFATAL) << "Requested vertex format does not match mesh format!";
      return nullptr;
    }

    return reinterpret_cast<const Vertex*>(GetVertexBytes());
  }

  // Gets a mutable pointer to the vertex data of the mesh. Returns nullptr
  // if Vertex doesn't match format or the vertex DataContainer does not have
  // read+write access.
  template <typename Vertex>
  Vertex* GetMutableVertexData() {
    if (!vertex_format_.Matches<Vertex>()) {
      LOG(DFATAL) << "Requested vertex format does not match mesh format!";
      return nullptr;
    }

    return reinterpret_cast<Vertex*>(vertex_data_.GetData());
  }

  Index GetNumVertices() const { return num_vertices_; }

  template <typename Vertex>
  Index AddVertex(const Vertex& v) {
    return AddVertices<Vertex>(&v, 1U);
  }

  template <typename Vertex, typename... Args>
  Index AddVertex(Args&&... args) {
    Vertex v = Vertex(std::forward<Args>(args)...);
    return AddVertices<Vertex>(&v, 1U);
  }

  // Copies |count| vertices from |list| into the mesh's data, returning the
  // index of the first vertex added. Returns kInvalidIndex and does not append
  // any data if the Vertex format does not match, if the vertex container does
  // not have write access, or if the vertex container does not have room for
  // all the vertices.
  template <typename Vertex>
  Index AddVertices(const Vertex* list, size_t count) {
    if (!vertex_format_.Matches<Vertex>()) {
      LOG(DFATAL) << "Vertex does not match format!";
      return kInvalidIndex;
    }

    return AddVertices(reinterpret_cast<const uint8_t*>(list), count,
                       sizeof(Vertex));
  }

  // Copies in a list of |count| vertices at |data|, returning the index of the
  // first vertex added. This assumes that the vertex data being passed in
  // matches the vertex format. Returns kInvalidIndex and does not append if the
  // vertex container does not have write access or if it does not have room for
  // all the vertices.
  Index AddVertices(const uint8_t* data, size_t count, size_t vertex_size);

  // Get a const pointer to the index data of the mesh. Returns nullptr if the
  // index DataContainer does not have read access.
  const Index* GetIndexData() const {
    return reinterpret_cast<const Index*>(index_data_.GetReadPtr());
  }

  size_t GetNumIndices() const { return index_data_.GetSize() / sizeof(Index); }

  bool AddIndex(Index index) { return AddIndices({index}); }

  // Adds a list of |count| indices. Returns true if the indices were added
  // successfully. Returns false and does not add any data if the index
  // DataContainer does not have write access, if the index is not in the bounds
  // of the vertex array, or if the mesh does not have room for all the indices.
  bool AddIndices(const Index* list, size_t count);

  // Adds indices to the mesh. Returns true if the indices were added
  // successfully. Returns false and does not add any data if the index
  // DataContainer does not have write access, if the index is not in the bounds
  // of the vertex array, or if the mesh does not have room for all the indices.
  bool AddIndices(std::initializer_list<Index> indices) {
    return AddIndices(indices.begin(), indices.size());
  }

  // Calculates the axis-aligned bounding box from the current vertices by
  // iterating through all the vertex positions and tracking the min and max
  // value found for each vec3 component.  A zero sized Aabb is returned if the
  // vertex count is zero or the vertex data is unreadable.  The index data is
  // unused--all vertices present are assumed to be used and participate in the
  // calculation of the Aabb.
  //
  // *NOTE* While this function is const, it is not thread-safe due to some
  // cached mutable data storing the aabb state.
  Aabb GetAabb() const;

  // Creates and returns a copy with read+write access. If *this doesn't have
  // read access, logs a DFATAL and returns an empty mesh.
  MeshData CreateHeapCopy() const;

 private:
  PrimitiveType primitive_type_ = kTriangles;
  VertexFormat vertex_format_;
  DataContainer vertex_data_;
  DataContainer index_data_;
  // We keep track of the number of vertices that have been added to the mesh
  // in a field so the user can access this info without knowing the vertex
  // format.
  Index num_vertices_ = 0;
  // The mesh aabb is cached when it is computed, so we keep track of a dirty
  // flag, setting it whenever vertices are changed and clearing it whenever
  // the aabb is computed.
  mutable bool aabb_is_dirty_ = true;
  mutable Aabb aabb_;
};

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_MESH_DATA_H_
