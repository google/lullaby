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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_MESH_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_MESH_H_

#include <memory>

#include "filament/IndexBuffer.h"
#include "filament/VertexBuffer.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/mesh.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/graphics/mesh_data.h"

namespace redux {

// Manages a filament VertexBuffer and IndexBuffer created using MeshData.
class FilamentMesh : public Mesh, public Readiable {
 public:
  FilamentMesh(Registry* registry, const std::shared_ptr<MeshData>& mesh_data);

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

  // Returns the underlying filament vertex buffer.
  filament::VertexBuffer* GetFilamentVertexBuffer() const;

  // Returns the underlying filament index buffer.
  filament::IndexBuffer* GetFilamentIndexBuffer() const;

 private:
  filament::Engine* fengine_ = nullptr;
  FilamentResourcePtr<filament::VertexBuffer> fvbuffer_;
  FilamentResourcePtr<filament::IndexBuffer> fibuffer_;
  Box bounding_box_;
  size_t num_vertices_ = 0;
  size_t num_primitives_ = 0;
  std::vector<SubmeshData> submeshes_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_MESH_H_
