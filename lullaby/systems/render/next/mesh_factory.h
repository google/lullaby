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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_MESH_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_MESH_FACTORY_H_

#include "fplbase/asset_manager.h"
#include "fplbase/renderer.h"
#include "lullaby/systems/render/next/mesh.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/resource_manager.h"

namespace lull {

// Creates and manages Mesh objects.
//
// Meshes will be automatically released along with the last external reference.
class MeshFactory {
 public:
  MeshFactory(Registry* registry, fplbase::Renderer* renderer,
              std::shared_ptr<fplbase::AssetManager> asset_manager);
  MeshFactory(const MeshFactory&) = delete;
  MeshFactory& operator=(const MeshFactory&) = delete;

  // Loads the mesh with the given |filename|.
  MeshPtr LoadMesh(const std::string& filename);

  // Creates a mesh using the specified data.
  MeshPtr CreateMesh(const MeshData& mesh);

  // Creates a named mesh using the specified data.
  MeshPtr CreateMesh(HashValue key, const MeshData& mesh);

  // Returns the mesh in the cache associated with |key|, else nullptr.
  MeshPtr GetCachedMesh(HashValue key) const;

  // Attempts to add |mesh| to the cache using |key|.
  void CacheMesh(HashValue key, const MeshPtr& mesh);

  // Releases the cached mesh associated with |key|.
  void ReleaseMeshFromCache(HashValue key);

 private:
  Mesh::MeshImplPtr LoadFplMesh(const std::string& name);

  Registry* registry_;
  fplbase::Renderer* fpl_renderer_;
  std::shared_ptr<fplbase::AssetManager> fpl_asset_manager_;
  ResourceManager<Mesh> meshes_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::MeshFactory);

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_MESH_FACTORY_H_
