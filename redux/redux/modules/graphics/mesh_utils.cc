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

#include "redux/modules/graphics/mesh_utils.h"

#include <cstddef>

#include "absl/log/check.h"
#include "absl/types/span.h"
#include "redux/modules/base/data_builder.h"
#include "redux/modules/graphics/vertex_utils.h"

namespace redux {
namespace {

// Allows iteration over untyped, strided data from a MeshData object.
template <typename T>
class MeshDataAccessor {
 public:
  MeshDataAccessor(const std::byte* base, std::size_t count,
                   std::size_t stride = 0, std::size_t offset = 0)
      : base_(base + offset), stride_(stride), count_(count) {}

  bool empty() const { return count_ == 0; }

  std::size_t size() const { return count_; }

  const T& operator[](std::size_t index) const {
    const auto* ptr = base_ + (index * stride_);
    return *reinterpret_cast<const T*>(ptr);
  }

 private:
  const std::byte* base_;
  std::size_t stride_;
  std::size_t count_;
};
}

// Returns an accessor to the index data withing the mesh. Ensures that T
// matches the type of the index data.
template <typename T>
static MeshDataAccessor<T> IndicesAccessor(const MeshData& mesh) {
  const MeshIndexType type = mesh.GetMeshIndexType();
  const std::size_t size = GetMeshIndexTypeSize(type);
  CHECK_EQ(size, sizeof(T));

  const std::byte* data = mesh.GetIndexData().data();
  const std::size_t num_indices = mesh.GetNumIndices();
  return MeshDataAccessor<T>(data, num_indices, size);
}

// Returns an accessor to the vertex data withing the mesh with the given
// attribute. Ensures that T matches the type of the attribute data, otherwise
// will return an empty accessor.
template <typename T>
static MeshDataAccessor<T> VerticesAccessor(const MeshData& mesh,
                                            VertexAttribute attrib) {
  const VertexFormat& vertex_format = mesh.GetVertexFormat();
  for (std::size_t i = 0; i < vertex_format.GetNumAttributes(); ++i) {
    const auto* ptr = vertex_format.GetAttributeAt(i);
    if (*ptr == attrib) {
      const std::size_t stride = vertex_format.GetStrideOfAttributeAt(i);
      const std::size_t offset = vertex_format.GetOffsetOfAttributeAt(i);

      const std::byte* data = mesh.GetVertexData().data();
      const std::size_t num_vertices = mesh.GetNumVertices();
      return MeshDataAccessor<T>(data, num_vertices, stride, offset);
    }
  }
  return MeshDataAccessor<T>(nullptr, 0, 0);
}

Box ComputeBounds(const MeshData& mesh) {
  auto positions =
      VerticesAccessor<vec3>(mesh, {VertexUsage::Position, VertexType::Vec3f});
  Box bounds = Box::Empty();
  for (std::size_t i = 0; i < positions.size(); ++i) {
    bounds = bounds.Included(positions[i]);
  }
  return bounds;
}

MeshData ComputeOrientations(const MeshDataAccessor<vec3>& normals) {
  if (normals.empty()) {
    return {};
  }

  VertexFormat vertex_format;
  vertex_format.AppendAttribute({VertexUsage::Orientation, VertexType::Vec4f});

  DataBuilder builder(vertex_format.GetVertexSize() * normals.size());
  for (std::size_t i = 0; i < normals.size(); ++i) {
    const vec4 orientation = CalculateOrientation(normals[i]);
    builder.Append<float>(orientation.data, 4);
  }

  MeshData data;
  data.SetVertexData(vertex_format, builder.Release(), normals.size(), {});
  return data;
}

MeshData ComputeOrientations(const MeshDataAccessor<vec3>& normals,
                             MeshDataAccessor<vec4>& tangents) {
  CHECK_EQ(normals.size(), tangents.size());
  if (normals.empty()) {
    return {};
  }

  VertexFormat vertex_format;
  vertex_format.AppendAttribute({VertexUsage::Orientation, VertexType::Vec4f});

  DataBuilder builder(vertex_format.GetVertexSize() * normals.size());
  for (std::size_t i = 0; i < normals.size(); ++i) {
    const vec4 orientation = CalculateOrientation(normals[i], tangents[i]);
    builder.Append<float>(orientation.data, 4);
  }

  MeshData data;
  data.SetVertexData(vertex_format, builder.Release(), normals.size(), {});
  return data;
}

MeshData ComputeOrientations(const MeshData& mesh) {
  auto normals =
      VerticesAccessor<vec3>(mesh, {VertexUsage::Normal, VertexType::Vec3f});
  auto tangents =
      VerticesAccessor<vec4>(mesh, {VertexUsage::Tangent, VertexType::Vec4f});

  if (!normals.empty() && !tangents.empty()) {
    return ComputeOrientations(normals, tangents);
  } else if (!normals.empty()) {
    return ComputeOrientations(normals);
  } else {
    return {};
  }
}

}  // namespace redux
