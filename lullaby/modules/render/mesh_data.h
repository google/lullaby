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

#ifndef LULLABY_UTIL_MESH_DATA_H_
#define LULLABY_UTIL_MESH_DATA_H_

#include "lullaby/modules/render/vertex.h"
#include "lullaby/util/data_container.h"
#include "lullaby/util/optional.h"

namespace lull {

// Provides Mesh abstraction over arbitrary data containers.
//
// A mesh can contain three types of data:
//
// - Vertex data. An array of vertices, where each vertex contains data such as
//   positions, normals, colors, etc. The structure of the vertices stored in
//   the Vertex data is defined by the VertexFormat.
//
// - Index data. An array of indices into the vertex data. The usage of the
//   indices is defined by the PrimitiveType. For example, a PrimitiveType of
//   kPoints means each index points to a single Point vertex, whereas a
//   PrimitiveType of kTriangles means that a set of three indices points to
//   the three corner vertices of a Triangle.
//
// - Submesh data. A range within the index data that represents a subsection
//   of the mesh.
//
// A valid mesh can have just "vertex" data, "vertex + index" data, or "vertex +
// index + submesh" data.
//
// The data itself is stored in DataContainer objects moved into the MeshData
// in its constructor.
class MeshData {
 public:
  enum PrimitiveType {
    kPoints,
    kLines,
    kTriangles,
    kTriangleFan,
    kTriangleStrip,
  };

  enum IndexType {
    kIndexU16,
    kIndexU32,
  };

  static const uint32_t kInvalidIndexU32;
  static const uint16_t kMaxValidIndexU16;
  static const uint32_t kMaxValidIndexU32;

  struct IndexRange {
    IndexRange() {}
    IndexRange(uint32_t start, uint32_t end) : start(start), end(end) {}

    uint32_t start = kInvalidIndexU32;
    uint32_t end = kInvalidIndexU32;
  };

  MeshData() {}

  // Represents a Mesh with the specified PrimitiveType and VertexFormat.  Takes
  // ownership of the vertex, index, and submesh data containers (the latter two
  // of which are optional).
  //
  // If non-empty, the data in |vertex_data| must match the |vertex_format|.
  //
  // If non-empty, the data in |index_data| must be an array of IndexType types
  // where each index is a within the range of the number of vertices in the
  // |vertex_data|.  Furthermore, the number of elements in the |index_data|
  // must be valid for the given |primitive_type|.
  //
  // If non-empty, the data in |index_range_data| must be an array of IndexRange
  // types where each IndexRange has a valid start and end value within the
  // |index_data| array.
  MeshData(const PrimitiveType primitive_type,
           const VertexFormat& vertex_format, DataContainer vertex_data,
           IndexType index_type, DataContainer index_data,
           DataContainer index_range_data = DataContainer())
      : primitive_type_(primitive_type),
        vertex_format_(vertex_format),
        index_type_(index_type),
        vertex_data_(std::move(vertex_data)),
        index_data_(std::move(index_data)),
        index_range_data_(std::move(index_range_data)),
        // Instantiate num_vertices_ based on the number of vertices that are
        // already in the vertex_data_.
        num_vertices_(static_cast<uint32_t>(vertex_data_.GetSize() /
                                            vertex_format_.GetVertexSize())),
        num_submeshes_(static_cast<uint32_t>(index_range_data_.GetSize() /
                                             sizeof(IndexRange))) {}

  // Constructs a mesh using only vertex data.
  MeshData(const PrimitiveType primitive_type,
           const VertexFormat& vertex_format, DataContainer vertex_data)
      : MeshData(primitive_type, vertex_format, std::move(vertex_data),
                 kIndexU16, DataContainer()) {}

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

  uint32_t GetNumVertices() const { return num_vertices_; }

  template <typename Vertex>
  Optional<uint32_t> AddVertex(const Vertex& v) {
    return AddVertices<Vertex>(&v, 1U);
  }

  template <typename Vertex, typename... Args>
  Optional<uint32_t> AddVertex(Args&&... args) {
    Vertex v = Vertex(std::forward<Args>(args)...);
    return AddVertices<Vertex>(&v, 1U);
  }

  // Copies |count| vertices from |list| into the mesh's data, returning the
  // index of the first vertex added. Returns an empty value and does not append
  // any data if the Vertex format does not match, if the vertex container does
  // not have write access, or if the vertex container does not have room for
  // all the vertices.
  template <typename Vertex>
  Optional<uint32_t> AddVertices(const Vertex* list, size_t count) {
    if (!vertex_format_.Matches<Vertex>()) {
      LOG(DFATAL) << "Vertex does not match format!";
      return {};
    }
    return AddVertices(reinterpret_cast<const uint8_t*>(list), count,
                       sizeof(Vertex));
  }

  // Copies in a list of |count| vertices at |data|, returning the index of the
  // first vertex added. This assumes that the vertex data being passed in
  // matches the vertex format. Returns an empty value and does not append if
  // the vertex container does not have write access or if it does not have room
  // for all the vertices.
  Optional<uint32_t> AddVertices(const uint8_t* data, size_t count,
                                 size_t vertex_size);

  // Gets a const pointer to the index data as bytes. Returns nullptr if the
  // mesh is not indexed or its index DataContainer does not have read access.
  const uint8_t* GetIndexBytes() const {
    return reinterpret_cast<const uint8_t*>(index_data_.GetReadPtr());
  }

  // Gets a const pointer to the index data of the mesh. Returns nullptr if the
  // index DataContainer does not have read access or IndexT doesn't match the
  // mesh's index type.
  template <typename IndexT>
  const IndexT* GetIndexData() const;

  IndexType GetIndexType() const { return index_type_; }

  static size_t GetIndexSize(IndexType index_type);

  size_t GetIndexSize() const { return GetIndexSize(GetIndexType()); }

  size_t GetNumIndices() const {
    return index_data_.GetSize() / GetIndexSize();
  }

  // Calculates the number of primitives represented by the given number of
  // indices for the specified primitive type.
  static size_t GetNumPrimitives(PrimitiveType type, IndexRange range);
  static size_t GetNumPrimitives(PrimitiveType type, size_t num_indices);

  // Adds a single index, casting to the mesh's index type as necessary. If the
  // mesh has index range data, this also adds a new range for the newly added
  // indices. Returns false and does not add any data for any of:
  // - the index DataContainer does not have write access,
  // - the index DataContainer does not have room for the index, or
  // - the index is not in the bounds of the vertex array.
  bool AddIndex(uint32_t index);

  // Adds a list of |count| indices. If the mesh has index range data, this also
  // adds a new range for the newly added indices. Returns false and does not
  // add any data for any of:
  // - the index type doesn't match that of the mesh,
  // - the index DataContainer does not have write access,
  // - the index DataContainer does not have room for all the indices,
  // - the mesh has submeshes but the submesh DataContainer is full, or
  // - any index is not in the bounds of the vertex array.
  bool AddIndices(const uint16_t* list, size_t count);
  bool AddIndices(const uint32_t* list, size_t count);

  uint32_t GetNumSubMeshes() const;

  // Returns the range of indices that represents the subsection of the mesh
  // specified by |index|.
  IndexRange GetSubMesh(size_t index) const;

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

  // DEPRECATED: do not use.
  // Adds indices to the mesh. Returns true if the indices were added
  // successfully. Returns false and does not add any data if the index
  // DataContainer does not have write access, if the index is not in the bounds
  // of the vertex array, or if the mesh does not have room for all the indices.
  bool AddIndices(std::initializer_list<uint16_t> indices) {
    return AddIndices(indices.begin(), indices.size());
  }

  void SetSubmeshAabbs(std::vector<Aabb> aabbs) {
    submesh_aabbs_ = std::move(aabbs);
  }

  const std::vector<Aabb>& GetSubmeshAabbs() const { return submesh_aabbs_; }

 private:
  template <typename IndexT>
  bool AddIndicesImpl(const IndexT* list, size_t count);

  PrimitiveType primitive_type_ = kTriangles;
  VertexFormat vertex_format_;
  IndexType index_type_ = kIndexU16;
  DataContainer vertex_data_;
  DataContainer index_data_;
  DataContainer index_range_data_;
  std::vector<Aabb> submesh_aabbs_;
  // We keep track of the number of vertices that have been added to the mesh
  // in a field so the user can access this info without knowing the vertex
  // format.
  uint32_t num_vertices_ = 0;
  uint32_t num_submeshes_ = 0;
  // The mesh aabb is cached when it is computed, so we keep track of a dirty
  // flag, setting it whenever vertices are changed and clearing it whenever
  // the aabb is computed.
  mutable bool aabb_is_dirty_ = true;
  mutable Aabb aabb_;
};

template <typename IndexT>
inline const IndexT* MeshData::GetIndexData() const {
  LOG(DFATAL) << "Invalid index type";
  return nullptr;
}

template <>
inline const uint16_t* MeshData::GetIndexData() const {
  if (index_type_ != kIndexU16) {
    LOG(DFATAL) << "Index type mismatch";
    return nullptr;
  }
  return reinterpret_cast<const uint16_t*>(index_data_.GetReadPtr());
}

template <>
inline const uint32_t* MeshData::GetIndexData() const {
  if (index_type_ != kIndexU32) {
    LOG(DFATAL) << "Index type mismatch";
    return nullptr;
  }
  return reinterpret_cast<const uint32_t*>(index_data_.GetReadPtr());
}

}  // namespace lull

#endif  // LULLABY_UTIL_MESH_DATA_H_
