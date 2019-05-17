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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_MESH_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_MESH_FACTORY_H_

#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/systems/render/filament/mesh.h"
#include "lullaby/systems/render/filament/renderer.h"
#include "lullaby/systems/render/mesh_factory.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/resource_manager.h"

namespace lull {

// Creates and manages Mesh objects.
//
// Meshes will be automatically released along with the last external reference.
class MeshFactoryImpl : public MeshFactory {
 public:
  MeshFactoryImpl(Registry* registry, filament::Engine* engine);
  MeshFactoryImpl(const MeshFactoryImpl&) = delete;
  MeshFactoryImpl& operator=(const MeshFactoryImpl&) = delete;

  // Creates a mesh using the specified data.
  MeshPtr CreateMesh(MeshData mesh_data) override;
  MeshPtr CreateMesh(MeshData* mesh_datas, size_t len) override;

  // Creates a named mesh using the specified data.
  MeshPtr CreateMesh(HashValue name, MeshData mesh_data) override;
  MeshPtr CreateMesh(HashValue name, MeshData* mesh_datas, size_t len) override;

  // Returns the mesh in the cache associated with |name|, else nullptr.
  MeshPtr GetMesh(HashValue name) const override;

  /// Caches a mesh for later retrieval. Effectively stores the shared_ptr to
  /// the mesh in an internal cache, allowing all references to be destroyed
  /// without actually destroying the mesh itself.
  void CacheMesh(HashValue name, const MeshPtr& mesh) override;

  // Returns an empty mesh.
  MeshPtr EmptyMesh() override;

  // Releases the cached mesh associated with |name|.
  void ReleaseMesh(HashValue name) override;

 private:
  void Init(MeshPtr mesh, MeshData* mesh_datas, size_t len);

  Registry* registry_;
  ResourceManager<Mesh> meshes_;
  filament::Engine* engine_;
  MeshPtr empty_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::MeshFactoryImpl);

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_MESH_FACTORY_H_
