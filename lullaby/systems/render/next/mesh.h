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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_MESH_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_MESH_H_

#include <functional>
#include <vector>
#include "lullaby/modules/file/asset.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/systems/render/mesh.h"
#include "lullaby/systems/render/next/render_handle.h"
#include "lullaby/systems/rig/rig_system.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/generated/shader_def_generated.h"

namespace lull {

// Represents a mesh used for rendering.
class Mesh {
 public:
  // Deprecated. Use RigSystem instead for storing skeletal data.
  struct SkeletonData {
    RigSystem::BoneIndices parent_indices;
    RigSystem::Pose inverse_bind_pose;
    RigSystem::BoneIndices shader_indices;
    Span<string_view> bone_names;
  };

  Mesh();
  ~Mesh();

  // Creates the actual mesh from the provided MeshData.
  void Init(const MeshData& mesh,
            const SkeletonData& skeleton = SkeletonData());

  // Returns if this mesh has been loaded into OpenGL.
  bool IsLoaded() const;

  // Returns the number of vertices contained in the mesh.
  int GetNumVertices() const;

  // Returns the number of triangles contained in the mesh.
  int GetNumTriangles() const;

  // Returns the number of submeshes in the mesh.
  size_t GetNumSubmeshes() const;

  // Gets the axis-aligned bounding box for the mesh.
  Aabb GetAabb() const;

  // Updates the RigSystem with the skeleton information in mesh.
  size_t TryUpdateRig(RigSystem* rig_system, Entity entity);

  // Draws the mesh.
  void Render();

  // Draws a portion of the mesh.
  void RenderSubmesh(size_t submesh);

  // If the mesh is still loading, this adds a function that will be called when
  // it finishes loading. If the mesh is already loaded, |callback| is
  // immediately invoked.
  void AddOrInvokeOnLoadCallback(const std::function<void()>& callback);

 private:
  void DrawArrays();
  void DrawElements(size_t index);
  void BindAttributes();
  void UnbindAttributes();

  BufferHnd vbo_;
  BufferHnd vao_;
  BufferHnd ibo_;
  Aabb aabb_;
  size_t num_vertices_ = 0;
  size_t num_indices_ = 0;
  std::vector<MeshData::IndexRange> submeshes_;
  VertexFormat vertex_format_;
  MeshData::PrimitiveType primitive_type_ = MeshData::kTriangles;
  MeshData::IndexType index_type_ = MeshData::kIndexU16;
  std::vector<std::function<void()>> on_load_callbacks_;

  // Deprecated. Store locally for now until RigSystem migration is complete.
  std::vector<uint8_t> bone_parents_;
  std::vector<uint8_t> shader_bone_indices_;
  std::vector<std::string> bone_names_;
  std::vector<mathfu::AffineTransform> bone_transform_inverses_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_MESH_H_
