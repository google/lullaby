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

#ifndef REDUX_ENGINES_RENDER_MESH_H_
#define REDUX_ENGINES_RENDER_MESH_H_

#include <functional>
#include <memory>
#include <vector>

#include "redux/modules/graphics/mesh_data.h"

namespace redux {

// Geometry (eg. vertices, indicies, primitive type) used in a draw call.
//
// A mesh can be composed of multiple submeshes, each of which can be drawn
// independently of each other.
class Mesh {
 public:
  virtual ~Mesh() = default;

  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;

  // Information about a submesh.
  struct SubmeshData {
    HashValue name;
    VertexFormat vertex_format;
    MeshPrimitiveType primitive_type = MeshPrimitiveType::Triangles;
    MeshIndexType index_type = MeshIndexType::U16;
    size_t range_start = 0;
    size_t range_end = 0;
    Box box;
  };

  // Returns the number of vertices contained in the mesh.
  size_t GetNumVertices() const;

  // Returns the number of primitives (eg. points, lines, triangles, etc.)
  // contained in the mesh.
  size_t GetNumPrimitives() const;

  // Gets the bounding box for the mesh.
  Box GetBoundingBox() const;

  // Returns the number of submeshes in the mesh.
  size_t GetNumSubmeshes() const;

  // Returns information about submesh of the mesh.
  const SubmeshData& GetSubmeshData(size_t index) const;

 protected:
  Mesh() = default;
};

using MeshPtr = std::shared_ptr<Mesh>;

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_MESH_H_
