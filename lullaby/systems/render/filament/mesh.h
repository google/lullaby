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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_MESH_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_MESH_H_

#include "filament/Engine.h"
#include "filament/IndexBuffer.h"
#include "filament/VertexBuffer.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/systems/render/filament/filament_utils.h"
#include "lullaby/systems/render/mesh.h"

namespace lull {

// Geometry data used for rendering.
//
// Effectively a wrapper around Filament's Vertex and Index Buffer objects with
// some additional functionality.
class Mesh {
 public:
  explicit Mesh(filament::Engine* engine);

  // Replaces the submesh at a specific index with the provided MeshData and the
  // buffers it references. Fails if multiple submeshes reference the same GBU
  // buffers.
  void ReplaceSubmesh(size_t index, MeshData mesh);

  // Returns the vertex format of the specified submesh index of the geometry,
  // or an empty VertexFormat if the index is invalid.
  VertexFormat GetVertexFormat(size_t submesh_index) const;

  // Returns the number of submeshes contained the geometry.
  size_t GetNumSubMeshes() const;

  // Returns the bounding box for a specific submesh of the geometry, or the
  // bounding box for the entire geometry if the index is out of range.
  Aabb GetSubMeshAabb(size_t index) const;

  // Returns the range (start, end) of the indices of the submesh specified by
  // |index|.
  MeshData::IndexRange GetSubMeshRange(size_t index) const;

  // Returns true if the Mesh is actually loaded into filament, false otherwise
  // (eg. mesh data is still loading asynchronously).
  bool IsLoaded() const;

  // Registers a callback that will be invoked when the mesh is fully loaded.
  // If the mesh is already loaded, the callback is invoked immediately.
  void AddOrInvokeOnLoadCallback(const std::function<void()>& callback);

  // Returns the underlying filament VertexBuffer object for a specific submesh
  // index.
  filament::VertexBuffer* GetVertexBuffer(size_t index);

  // Returns the underlying filament IndexBuffer object for a specific submesh
  // index.
  filament::IndexBuffer* GetIndexBuffer(size_t index);

 private:
  friend class MeshFactoryImpl;
  using FIndexPtr = FilamentResourcePtr<filament::IndexBuffer>;
  using FVertexPtr = FilamentResourcePtr<filament::VertexBuffer>;

  // Initializes a Mesh from a list of Filament vertex buffers and index
  // buffers and the MeshDatas they were created from. All three must be equal
  // length.
  void Init(FVertexPtr* vertex_buffers, FIndexPtr* index_buffers,
            MeshData* mesh_datas, size_t len);

  // Creates Submesh constructs using a MeshData that references particular
  // Filament vertex and index buffers. There will be one Submesh per,
  // MeshData::GetNumSubMeshes(), or a single Submesh if the MeshData had no
  // submeshes.
  void CreateSubmeshes(FVertexPtr vertex_buffer, FIndexPtr index_buffer,
                       MeshData mesh_data);

  // Replaces the contents of the vertex buffer at |index| with the vertex data
  // in |mesh|.
  void ReplaceVertexBuffer(int index, const MeshData& mesh);

  // Replaces the contents of the index buffer at |index| with the index data in
  // |mesh|.
  void ReplaceIndexBuffer(int index, const MeshData& mesh);

  // A Mesh consists of one or more sub-meshes, each of which may or may not
  // reference the same buffer and array objects.
  struct Submesh {
    // Indices into the global Mesh lists of buffer objects.
    int vertex_buffer_index = -1;
    int index_buffer_index = -1;
    int mesh_data_index = -1;
    Aabb aabb;
    VertexFormat vertex_format;
    MeshData::IndexRange index_range;
  };

  std::vector<FVertexPtr> vertex_buffers_;
  std::vector<FIndexPtr> index_buffers_;
  std::vector<Submesh> submeshes_;
  // Hold onto the MeshDatas since Filament reads from them after a Mesh is
  // Init().
  std::vector<MeshData> mesh_datas_;
  std::vector<std::function<void()>> on_load_callbacks_;
  filament::Engine* engine_ = nullptr;
  bool index_range_submeshes_ = false;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_MESH_H_
