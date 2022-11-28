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

#include <algorithm>
#include <limits>

namespace redux {

static size_t GetIndexSize(MeshIndexType index_type) {
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

void MeshData::SetVertexData(const VertexFormat& vertex_format,
                             DataContainer vertex_data, Box bounds) {
  vertex_format_ = vertex_format;
  vertex_data_ = std::move(vertex_data);
  bounds_ = bounds;
}

void MeshData::SetIndexData(MeshIndexType index_type,
                            DataContainer index_data) {
  index_type_ = index_type;
  index_data_ = std::move(index_data);
}

void MeshData::SetParts(DataContainer part_data) {
  parts_ = std::move(part_data);
}

bool MeshData::IsValid() const {
  CHECK_GT(vertex_data_.GetNumBytes(), 0);
  CHECK_GT(parts_.GetNumBytes(), 0);
  CHECK_EQ(vertex_data_.GetNumBytes() % vertex_format_.GetVertexSize(), 0);
  CHECK_EQ(index_data_.GetNumBytes() % GetIndexSize(index_type_), 0);
  CHECK_EQ(parts_.GetNumBytes() % sizeof(PartData), 0);
  return true;
}

std::size_t MeshData::GetNumVertices() const {
  const std::size_t size = vertex_format_.GetVertexSize();
  return size > 0 ? vertex_data_.GetNumBytes() / size : 0;
}

std::size_t MeshData::GetNumIndices() const {
  const std::size_t size = GetIndexSize(index_type_);
  return size > 0 ? index_data_.GetNumBytes() / size : 0;
}

MeshData MeshData::Clone() const {
  MeshData clone;
  clone.SetVertexData(vertex_format_, vertex_data_.Clone(), bounds_);
  clone.SetIndexData(index_type_, index_data_.Clone());
  clone.SetParts(parts_.Clone());
  return clone;
}

}  // namespace redux
