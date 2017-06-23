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

#ifndef LULLABY_UTIL_TRIANGLE_MESH_H_
#define LULLABY_UTIL_TRIANGLE_MESH_H_

#include <stddef.h>
#include <stdint.h>

#include <utility>
#include <vector>

#include "mathfu/glsl_mappings.h"
#include "lullaby/util/deform.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/mesh_util.h"
#include "lullaby/util/mesh_data.h"
#include "lullaby/util/vertex.h"

namespace lull {

// TriangleMesh is a list of vertices and indices arranged in triangle format
// (3 indices = 1 triangle; ie not strips).  Its contents are stored in CPU
// memory, so it does not provide efficient rendering performance.
template <typename Vertex>
class TriangleMesh {
 public:
  using Index = uint16_t;

  TriangleMesh() {}

  // Default implementation of move constructor.  MSVC toolchain currently does
  // not support =default for move constructors, so define it explicitly.
  // TODO(b/28276908) Remove after switch to MSVC 2015.
  TriangleMesh(TriangleMesh&& rhs)
      : vertices_(std::move(rhs.vertices_)),
        indices_(std::move(rhs.indices_)) {}

  // Default implementation of move assignment.  MSVC toolchain currently does
  // not support =default for move assignment, so define it explicitly.
  // TODO(b/28276908) Remove after switch to MSVC 2015.
  TriangleMesh& operator=(TriangleMesh<Vertex>&& rhs) {
    vertices_ = std::move(rhs.vertices_);
    indices_ = std::move(rhs.indices_);
    return *this;
  }

  // Queries whether the mesh is completely empty.
  bool IsEmpty() const { return (vertices_.empty() && indices_.empty()); }

  // Returns the list of vertices.
  const std::vector<Vertex>& GetVertices() const { return vertices_; }

  // Returns the list of vertices.
  std::vector<Vertex>& GetVertices() { return vertices_; }

  // Adds |v| to the end of the vertex list and returns its index.
  Index AddVertex(const Vertex& v) {
    const Index index = static_cast<Index>(vertices_.size());
    vertices_.emplace_back(v);
    return index;
  }

  // Adds a vertex by constructing it in place and returns its index.
  template <typename... Args>
  Index AddVertex(Args&&... args) {
    const Index index = static_cast<Index>(vertices_.size());
    vertices_.emplace_back(std::forward<Args>(args)...);
    return index;
  }

  // Adds |count| vertices from |list| and returns the index of the first new
  // vertex.
  Index AddVertices(const Vertex* list, size_t count) {
    const Index index = static_cast<Index>(vertices_.size());
    for (size_t i = 0; i < count; ++i) {
      vertices_.emplace_back(list[i]);
    }
    return index;
  }

  // Returns the list of indices.
  const std::vector<Index>& GetIndices() const { return indices_; }

  // Returns the list of indices.
  std::vector<Index>& GetIndices() { return indices_; }

  // Adds a triangle using vertices at indices |v0|, |v1|, and |v2|.
  void AddTriangle(Index v0, Index v1, Index v2) {
    if (v0 > vertices_.size() || v1 > vertices_.size() ||
        v2 > vertices_.size()) {
      LOG(DFATAL) << "Index out of bounds!";
      return;
    }
    indices_.push_back(v0);
    indices_.push_back(v1);
    indices_.push_back(v2);
  }

  // Adds |count| indices from |list|.
  void AddIndices(const Index* list, size_t count) {
    if (count % 3 != 0) {
      LOG(DFATAL) << "Too many indices in the list!";
      return;
    }
    for (size_t i = 0; i < count; i += 3) {
      AddTriangle(list[i], list[i + 1], list[i + 2]);
    }
  }

  // Clears all vertices and indices.
  void Clear() {
    vertices_.clear();
    indices_.clear();
  }

  // Calculates the axis-aligned bounding box from the current vertices.
  Aabb GetAabb() const {
    Aabb box;
    if (vertices_.empty()) {
      return box;
    }

    box.min = box.max = GetPosition(vertices_[0]);

    for (size_t i = 1; i < vertices_.size(); ++i) {
      const mathfu::vec3 pos = GetPosition(vertices_[i]);
      box.min = mathfu::vec3::Min(box.min, pos);
      box.max = mathfu::vec3::Max(box.max, pos);
    }

    return box;
  }

  // Sets the mesh to a quad using the tesselated quad functions from
  // util/mesh.h.  Clears any existing data.
  void SetQuad(float size_x, float size_y, int num_verts_x, int num_verts_y,
               float corner_radius, int corner_verts, CornerMask corner_mask) {
    vertices_ = CalculateTesselatedQuadVertices<Vertex>(
        size_x, size_y, num_verts_x, num_verts_y, corner_radius, corner_verts,
        corner_mask);
    indices_ =
        CalculateTesselatedQuadIndices(num_verts_x, num_verts_y, corner_verts);
    if (vertices_.empty() || indices_.empty()) {
      LOG(DFATAL) << "Failed to set the mesh to quad!";
      vertices_.clear();
      indices_.clear();
    }
  }

  // Creates and returns a MeshData with read+write access.
  MeshData CreateMeshData() const {
    const size_t vertex_data_size = vertices_.size() * sizeof(Vertex);
    DataContainer vertex_data =
        DataContainer::CreateHeapDataContainer(vertex_data_size);

    const size_t index_data_size = indices_.size() * sizeof(Index);
    DataContainer index_data =
        DataContainer::CreateHeapDataContainer(index_data_size);

    if (!vertex_data.Append(reinterpret_cast<const uint8_t*>(vertices_.data()),
                            vertex_data_size) ||
        !index_data.Append(reinterpret_cast<const uint8_t*>(indices_.data()),
                           index_data_size)) {
      LOG(DFATAL) << "Failed to write mesh";
    }

    return MeshData(MeshData::kTriangles, Vertex::kFormat,
                    std::move(vertex_data), std::move(index_data));
  }

 private:
  std::vector<Vertex> vertices_;
  std::vector<Index> indices_;

  TriangleMesh(const TriangleMesh& rhs) = delete;
  TriangleMesh& operator=(const TriangleMesh& rhs) = delete;
};

}  // namespace lull

#endif  // LULLABY_UTIL_TRIANGLE_MESH_H_
