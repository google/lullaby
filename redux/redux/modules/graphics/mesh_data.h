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
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/graphics/vertex_attribute.h"
#include "redux/modules/graphics/vertex_format.h"
#include "redux/modules/math/bounds.h"

namespace redux {

// Provides a Mesh abstraction over arbitrary chunks of binary data.
//
// A mesh contains three types of data:
//
// - Vertex data. An array of vertices, where each vertex contains data such as
//   positions, normals, colors, etc. The structure of the vertices stored in
//   this data is defined by the VertexFormat.
//
// - Index data. An array of indices into the vertex data. Indices can be
//   specified as either uint16_t or uint32_t integer types.
//
// - Part data. Information that represents a subsection of the mesh. A mesh can
//   have a single part which is the entire mesh itself, or can be composed of
//   multiple parts. A part must refer to a contiguous range of data in the
//   index data or (if not available) then the vertex data.
//
// The usage of the indices is defined by the PrimitiveType. For example, a
// PrimitiveType of kPoints means each index points to a single Point vertex,
// whereas a PrimitiveType of kTriangles means that a set of three indices
// points to the three corner vertices of a Triangle.
//
// The whole mesh as well as individual parts must also specify their bounding
// boxes. This information is used to determine whether or not the mesh will be
// culled during rendering.
class MeshData {
 public:
  // Information that describes a subsection of the MeshData.
  struct PartData {
    HashValue name;
    MeshPrimitiveType primitive_type = MeshPrimitiveType::Triangles;
    std::uint32_t start = kInvalidIndexU32;
    std::uint32_t end = kInvalidIndexU32;
    Box box;
  };

  MeshData() = default;

  // The vertex_data must match the vertex_format. The bounds should account
  // for any potential modifications made by a vertex shader (ie. skinning).
  void SetVertexData(const VertexFormat& vertex_format,
                     DataContainer vertex_data, Box bounds);

  // This should be either an array of uint16_t (if index_type == U16) or
  // uint32_t (if index_type == U32).
  void SetIndexData(MeshIndexType index_type, DataContainer index_data);

  // This should be an array of PartData types. The best way to ensure this is
  // to store the PartData in an std::vector or absl::Span and wrapping it up
  // in a DataContainer.
  void SetParts(DataContainer part_data);

  bool IsValid() const;

  const VertexFormat& GetVertexFormat() const { return vertex_format_; }

  const MeshIndexType GetMeshIndexType() const { return index_type_; }

  std::size_t GetNumVertices() const;

  std::size_t GetNumIndices() const;

  Box GetBoundingBox() const { return bounds_; }

  absl::Span<const std::byte> GetVertexData() const {
    return vertex_data_.GetByteSpan();
  }

  absl::Span<const std::byte> GetIndexData() const {
    return index_data_.GetByteSpan();
  }

  absl::Span<const PartData> GetPartData() const {
    const PartData* ptr = reinterpret_cast<const PartData*>(parts_.GetBytes());
    const std::size_t num = parts_.GetNumBytes() / sizeof(PartData);
    return {ptr, num};
  }

  // Creates and returns a copy of the Mesh. This is useful in cases where the
  // MeshData may have been created on the stack, but needs to be heap allocated
  // in order to pass to a worker thread or the GPU.
  MeshData Clone() const;

  static constexpr std::uint32_t kInvalidIndexU32 =
      static_cast<std::uint32_t>(~0);
  static constexpr std::uint16_t kMaxValidIndexU16 =
      static_cast<std::uint16_t>(~0) - 1;
  static constexpr std::uint32_t kMaxValidIndexU32 =
      static_cast<std::uint32_t>(~0) - 1;

 private:
  VertexFormat vertex_format_;
  DataContainer vertex_data_;
  MeshIndexType index_type_ = MeshIndexType::U16;
  DataContainer index_data_;
  DataContainer parts_;
  Box bounds_;
};
}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_MESH_DATA_H_
