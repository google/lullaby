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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_MESH_FACTORY_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_MESH_FACTORY_H_

#include "lullaby/systems/render/mesh_factory.h"
#include "lullaby/systems/render/next/mesh.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/resource_manager.h"

namespace lull {

// Creates and manages Mesh objects.
//
// Meshes will be automatically released along with the last external reference.
class MeshFactoryImpl : public MeshFactory {
 public:
  explicit MeshFactoryImpl(Registry* registry);

  MeshFactoryImpl(const MeshFactoryImpl&) = delete;
  MeshFactoryImpl& operator=(const MeshFactoryImpl&) = delete;

  // Returns the mesh in the cache associated with |name|, else nullptr.
  MeshPtr GetMesh(HashValue name) const override;

  // Attempts to add |mesh| to the cache using |name|.
  void CacheMesh(HashValue name, const MeshPtr& mesh) override;

  // Releases the cached mesh associated with |name|.
  void ReleaseMesh(HashValue name) override;

  // Creates a mesh using the specified data.
  MeshPtr CreateMesh(MeshData mesh_data) override;
  MeshPtr CreateMesh(MeshData* mesh_datas, size_t len) override;

  // Creates a named mesh using the specified data.
  MeshPtr CreateMesh(HashValue name, MeshData mesh_data) override;
  MeshPtr CreateMesh(HashValue name, MeshData* mesh_datas, size_t len) override;

  // Returns an empty mesh.
  MeshPtr EmptyMesh() override;

  // DEPRECATED. Loads the fplmesh with the given |filename|.
  MeshPtr LoadMesh(const std::string& filename);

  // DEPRECATED. Old RenderSystem API passes MeshData by const reference which
  // can be redirected here.
  MeshPtr CreateMesh(const MeshData* mesh_data);
  MeshPtr CreateMesh(HashValue name, const MeshData* mesh_data);

 private:
  Registry* registry_;
  ResourceManager<Mesh> meshes_;
  MeshPtr empty_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::MeshFactoryImpl);

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_MESH_FACTORY_H_
