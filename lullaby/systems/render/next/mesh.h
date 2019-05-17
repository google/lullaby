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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_MESH_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_MESH_H_

#include <functional>
#include <limits>
#include <vector>

#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/systems/render/mesh.h"
#include "lullaby/systems/render/next/render_handle.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/generated/shader_def_generated.h"

namespace lull {

// Represents a mesh used for rendering.
class Mesh {
 public:
  Mesh();
  ~Mesh();

  // Creates the actual mesh from the provided MeshData.
  void Init(const MeshData* mesh_datas, size_t len);

  // Returns if this mesh has been loaded into OpenGL.
  bool IsLoaded() const;

  // Returns the number of vertices contained in the mesh.
  int GetNumVertices() const;

  // Returns the number of primitives (eg. points, lines, triangles, etc.)
  // contained in the mesh.
  int GetNumPrimitives() const;

  // Returns the number of submeshes in the mesh.
  size_t GetNumSubmeshes() const;

  // Replaces the submesh at a specific index with the provided MeshData. Fails
  // if multiple submeshes reference the same GBU buffers.
  void ReplaceSubmesh(size_t index, const MeshData& mesh);

  // Gets the axis-aligned bounding box for the specified submesh.
  Aabb GetSubmeshAabb(size_t index) const;

  // Gets the axis-aligned bounding box for the mesh.
  Aabb GetAabb() const;

  // Draws the mesh.
  void Render();

  // Draws a portion of the mesh.
  void RenderSubmesh(size_t index);

  // Returns the vertex format of the specified submesh index of the geometry,
  // or an empty VertexFormat if the index is invalid.
  VertexFormat GetVertexFormat(size_t submesh_index) const;

  // If the mesh is still loading, this adds a function that will be called when
  // it finishes loading. If the mesh is already loaded, |callback| is
  // immediately invoked.
  void AddOrInvokeOnLoadCallback(const std::function<void()>& callback);

  // Allows custom geometry processors to specify their own buffers.
  void SetGpuBuffers(BufferHnd vbo, BufferHnd vao, BufferHnd ibo);

 private:
  // Creates Submesh constructs using a MeshData, including creation of buffer
  // and array objects.
  void CreateSubmeshes(const MeshData& mesh_data);

  // A Mesh consists of one or more sub-meshes, each of which may or may not
  // reference the same buffer and array objects.
  struct Submesh {
    // Indices into the global mesh lists of array and buffer objects.
    int vbo_index = -1;
    int vao_index = -1;
    int ibo_index = -1;
    // Other properties of this individual submesh.
    Aabb aabb;
    size_t num_vertices = 0;
    MeshData::IndexRange index_range;
    VertexFormat vertex_format;
    MeshData::PrimitiveType primitive_type = MeshData::kTriangles;
    MeshData::IndexType index_type = MeshData::kIndexU16;
  };

  void DrawArrays(const Submesh& submesh);
  void DrawElements(const Submesh& submesh);
  void BindAttributes(const Submesh& submesh);
  void UnbindAttributes(const Submesh& submesh);

  // Creates a GL buffer or array object and returns its index in the Mesh's
  // lists of objects.
  int CreateVbo(const MeshData& mesh);
  int CreateVao(const MeshData& mesh, int vbo_index);
  int CreateIbo(const MeshData& mesh);

  // Fills an already-allocated GL buffer or array object based on its index in
  // the Mesh's list of objects.
  void FillVbo(int vbo_index, const MeshData& mesh);
  void FillVao(int vao_index, const MeshData& mesh, int vbo_index);
  void FillIbo(int ibo_index, const MeshData& mesh);

  // Properly releases all BufferHnds below and clears their vectors.
  void ReleaseGpuResources();

  // Buffer and array objects used by geometry.
  std::vector<BufferHnd> vbos_;
  std::vector<BufferHnd> vaos_;
  std::vector<BufferHnd> ibos_;
  std::vector<Submesh> submeshes_;
  // Aabb encompassing all submeshes. Initialize to (max, min) so that the first
  // merge will effectively replace it.
  Aabb aabb_ = Aabb(mathfu::vec3(std::numeric_limits<float>::max()),
                    mathfu::vec3(std::numeric_limits<float>::min()));
  // Total vertex and primitive counts, primarily for profiling.
  size_t num_vertices_ = 0;
  size_t num_primitives_ = 0;
  std::vector<std::function<void()>> on_load_callbacks_;
  bool remote_gpu_buffers_ = false;
  bool index_range_submeshes_ = false;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_MESH_H_
