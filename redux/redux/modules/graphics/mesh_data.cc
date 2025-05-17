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

#include "redux/modules/graphics/mesh_data.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include "absl/log/check.h"
#include "absl/log/log.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/graphics_enums_generated.h"
#include "redux/modules/graphics/vertex_format.h"

namespace redux {

size_t GetMeshIndexTypeSize(MeshIndexType index_type) {
  switch (index_type) {
    case MeshIndexType::U16:
      return sizeof(std::uint16_t);
    case MeshIndexType::U32:
      return sizeof(std::uint32_t);
    default:
      LOG(FATAL) << "Invalid index type " << ToString(index_type);
      return 0;
  }
}

void MeshData::SetName(HashValue name) { name_ = name; }

void MeshData::SetVertexData(const VertexFormat& vertex_format,
                             DataContainer vertex_data,
                             std::size_t num_vertices, Box bounds) {
  vertex_format_ = vertex_format;
  vertex_data_ = std::move(vertex_data);
  num_vertices_ = num_vertices;
  bounds_ = bounds;

  CHECK_GE(vertex_data_.GetNumBytes(),
           vertex_format_.GetVertexSize() * num_vertices_);
}

void MeshData::SetVertexData(const VertexFormat& vertex_format,
                             DataContainer vertex_data, Box bounds) {
  const std::size_t num_vertices =
      vertex_data.GetNumBytes() / vertex_format.GetVertexSize();
  SetVertexData(vertex_format, std::move(vertex_data), num_vertices, bounds);
}

void MeshData::SetIndexData(MeshIndexType index_type,
                            MeshPrimitiveType primitive_type,
                            DataContainer index_data, std::size_t num_indices) {
  index_type_ = index_type;
  primitive_type_ = primitive_type;
  index_data_ = std::move(index_data);
  num_indices_ = num_indices;

  CHECK_GE(index_data_.GetNumBytes(),
           GetMeshIndexTypeSize(index_type_) * num_indices_);
}

void MeshData::SetIndexData(MeshIndexType index_type,
                            MeshPrimitiveType primitive_type,
                            DataContainer index_data) {
  const std::size_t num_indices =
      index_data.GetNumBytes() / GetMeshIndexTypeSize(index_type);
  SetIndexData(index_type, primitive_type, std::move(index_data), num_indices);
}

MeshData MeshData::Clone() const {
  MeshData clone;
  clone.SetVertexData(vertex_format_, vertex_data_.Clone(), num_vertices_,
                      bounds_);
  clone.SetIndexData(index_type_, primitive_type_, index_data_.Clone(),
                     num_indices_);
  return clone;
}

MeshData MeshData::WrapDataInSharedPtr(const std::shared_ptr<MeshData>& other) {
  MeshData clone;
  clone.SetName(other->GetName());

  DataContainer vertices =
      DataContainer::WrapDataInSharedPtr(other->GetVertexData(), other);
  clone.SetVertexData(other->GetVertexFormat(), std::move(vertices),
                      other->GetNumVertices(), other->GetBoundingBox());
  if (!other->GetIndexData().empty()) {
    DataContainer indices =
        DataContainer::WrapDataInSharedPtr(other->GetIndexData(), other);
    clone.SetIndexData(other->GetMeshIndexType(), other->GetPrimitiveType(),
                       std::move(indices), other->GetNumIndices());
  }
  return clone;
}

}  // namespace redux
