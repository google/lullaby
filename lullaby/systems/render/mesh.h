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

#ifndef LULLABY_SYSTEMS_RENDER_MESH_H_
#define LULLABY_SYSTEMS_RENDER_MESH_H_

#include <memory>

#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/vertex_format.h"

namespace lull {

class Mesh;
using MeshPtr = std::shared_ptr<Mesh>;

// Allows a mesh's vertex buffer object, vertex attribute object, index buffer
// object to be overridden to the given (remotely owned) handles. This
// deallocates any existing GPU buffers in the mesh but inhibits further
// deallocation of the passed-in buffers via ~Mesh.
void SetGpuBuffers(const MeshPtr& mesh, uint32_t vbo, uint32_t vao,
                   uint32_t ibo);

// Returns the vertex format of the specified submesh index of the geometry, or
// an empty VertexFormat if the index is invalid.
VertexFormat GetVertexFormat(const MeshPtr& mesh, size_t submesh_index);

// Returns true if the specified mesh is fully loaded, false otherwise.
bool IsMeshLoaded(const MeshPtr& mesh);

// Returns the number of submeshes contained in the mesh.
size_t GetNumSubmeshes(const MeshPtr& mesh);

// Replaces a specific submesh index within a mesh with new MeshData. This may
// fail if the existing mesh has submeshes based on index ranges instead of
// multiple MeshDatas.
//
// Note that this function will affect all Entities that currently use the
// MeshPtr.
void ReplaceSubmesh(MeshPtr mesh, size_t submesh_index,
                    const MeshData& mesh_data);

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_MESH_H_
