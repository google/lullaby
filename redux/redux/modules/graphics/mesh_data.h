/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_MODULES_GRAPHICS_MESH_DATA_H_
#define REDUX_MODULES_GRAPHICS_MESH_DATA_H_

#include <cstddef>
#include <cstdint>
#include <memory>

#include "absl/types/span.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/vertex_format.h"
#include "redux/modules/math/bounds.h"

namespace redux {

// Provides a Mesh abstraction over arbitrary chunks of binary data.
//
// A mesh is mainly defined by two buffers of data:
//
// - Vertex data. An array of vertices, where each vertex contains data such as
//   positions, normals, colors, etc. The structure of each vertex is defined
//   by the VertexFormat.
//
// - Index data. An array of indices into the vertex data. Indices can be
//   specified as either uint16_t or uint32_t integer types.
//
// The usage of the indices is defined by the PrimitiveType. For example, a
// PrimitiveType of kPoints means each index points to a single Point vertex,
// whereas a PrimitiveType of kTriangles means that a set of three indices
// points to the three corner vertices of a Triangle.
//
// This class also provides the bounding box for a Mesh which can be used, for
// example, to determine whether the mesh can be culled during rendering.
class MeshData {
 public:
  MeshData() = default;

  // The vertex_data must match the vertex_format. The bounds should account
  // for any potential modifications made by a vertex shader (ie. skinning).
  void SetVertexData(const VertexFormat& vertex_format,
                     DataContainer vertex_data, std::size_t num_vertices,
                     Box bounds);

  // As above, the the number of vertices is calculated based on the size of
  // the vertex_data and vertex_format.
  void SetVertexData(const VertexFormat& vertex_format,
                     DataContainer vertex_data, Box bounds);

  // This should be either an array of uint16_t (if index_type == U16) or
  // uint32_t (if index_type == U32).
  void SetIndexData(MeshIndexType index_type, MeshPrimitiveType primitive_type,
                    DataContainer index_data, std::size_t num_indices);

  // As above, but the number of indices is calculated based on the size of the
  // index_data and index_type.
  void SetIndexData(MeshIndexType index_type, MeshPrimitiveType primitive_type,
                    DataContainer index_data);

  // Optionally assigns a name to the data. This is required when a single
  // object will be composed of multiple meshes.
  void SetName(HashValue name);

  // Returns the name assigned to this MeshData.
  HashValue GetName() const { return name_; }

  // Returns the vertex format of the vertex data in the mesh.
  const VertexFormat& GetVertexFormat() const { return vertex_format_; }

  // Returns the type (e.g. U16, U32) of the index data in the mesh.
  MeshIndexType GetMeshIndexType() const { return index_type_; }

  // Returns the type of primitive (e.g. Triangles) defined by the index data.
  MeshPrimitiveType GetPrimitiveType() const { return primitive_type_; }

  std::size_t GetNumVertices() const { return num_vertices_; }

  std::size_t GetNumIndices() const { return num_indices_; }

  Box GetBoundingBox() const { return bounds_; }

  absl::Span<const std::byte> GetVertexData() const {
    return vertex_data_.GetByteSpan();
  }

  absl::Span<const std::byte> GetIndexData() const {
    return index_data_.GetByteSpan();
  }

  // Creates and returns a copy of the Mesh. This is useful in cases where the
  // MeshData may have been created on the stack, but needs to be heap allocated
  // in order to pass to a worker thread or the GPU.
  MeshData Clone() const;

  // Similar to Clone(), but the lifetime of the input MeshData shared_ptr will
  // be tied to the lifetime of the returned MeshData. In other words, both
  // the input and output will share the same underlying buffer data.
  static MeshData WrapDataInSharedPtr(const std::shared_ptr<MeshData>& other);

  static constexpr std::uint32_t kInvalidIndexU32 =
      static_cast<std::uint32_t>(~0);
  static constexpr std::uint16_t kMaxValidIndexU16 =
      static_cast<std::uint16_t>(~0) - 1;
  static constexpr std::uint32_t kMaxValidIndexU32 =
      static_cast<std::uint32_t>(~0) - 1;

 private:
  HashValue name_;
  VertexFormat vertex_format_;
  DataContainer vertex_data_;
  MeshIndexType index_type_ = MeshIndexType::U16;
  MeshPrimitiveType primitive_type_ = MeshPrimitiveType::Triangles;
  DataContainer index_data_;
  Box bounds_;
  std::size_t num_vertices_ = 0;
  std::size_t num_indices_ = 0;
};

// Returns the size (in bytes) needed to store an element of the given type.
std::size_t GetMeshIndexTypeSize(MeshIndexType index_type);

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_MESH_DATA_H_
