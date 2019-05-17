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

#ifndef LULLABY_SYSTEMS_RENDER_MESH_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_MESH_FACTORY_H_

#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/systems/render/mesh.h"
#include "lullaby/util/typeid.h"

namespace lull {

/// Provides mechanisms for creating and managing Mesh objects.
///
/// The MeshFactory can be used to create Mesh objects from CPU memory via a
/// MeshData object. It also provides a caching mechanism whereby multiple
/// requests to a mesh identified by a unique name will return the same Mesh
/// object.
class MeshFactory {
 public:
  virtual ~MeshFactory() {}

  /// Caches a mesh for later retrieval. Effectively stores the shared_ptr to
  /// the mesh in an internal cache, allowing all references to be destroyed
  /// without actually destroying the mesh itself.
  virtual void CacheMesh(HashValue name, const MeshPtr& mesh) = 0;

  /// Retrieves a cached mesh by its name hash, or returns nullptr if the mesh
  /// is not cached.
  virtual MeshPtr GetMesh(HashValue name) const = 0;

  /// Releases the cached mesh associated with |name|. If no other references to
  /// the mesh exist, then it will be destroyed.
  virtual void ReleaseMesh(HashValue name) = 0;

  /// Creates a mesh using one or more |mesh_data|.
  virtual MeshPtr CreateMesh(MeshData mesh_data) = 0;
  virtual MeshPtr CreateMesh(MeshData* mesh_datas, size_t len) = 0;

  /// Creates a "named" mesh using one or more |mesh_data|. Subsequent calls to
  /// this function with the same mesh |name| will return the original mesh as
  /// long as any references to that mesh are still valid.
  virtual MeshPtr CreateMesh(HashValue name, MeshData mesh_data) = 0;
  virtual MeshPtr CreateMesh(HashValue name, MeshData* mesh_datas,
                             size_t len) = 0;

  /// Returns an empty mesh.  Intended for use as a placeholder for some other
  /// mesh.  The returned mesh will never be 'loaded' so ReadyToRender checks
  /// will fail when this mesh is set.
  virtual MeshPtr EmptyMesh() = 0;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::MeshFactory);

#endif  // LULLABY_SYSTEMS_RENDER_MESH_FACTORY_H_
