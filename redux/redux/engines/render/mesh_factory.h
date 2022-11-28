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

#ifndef REDUX_ENGINES_RENDER_MESH_FACTORY_H_
#define REDUX_ENGINES_RENDER_MESH_FACTORY_H_

#include "redux/engines/render/mesh.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/resource_manager.h"
#include "redux/modules/graphics/mesh_data.h"

namespace redux {

// Creates and manages Mesh objects.
//
// Meshes will be automatically released along with the last external reference.
class MeshFactory {
 public:
  explicit MeshFactory(Registry* registry);

  MeshFactory(const MeshFactory&) = delete;
  MeshFactory& operator=(const MeshFactory&) = delete;

  // Returns the mesh in the cache associated with |name|, else nullptr.
  MeshPtr GetMesh(HashValue name) const;

  // Attempts to add |mesh| to the cache using |name|.
  void CacheMesh(HashValue name, const MeshPtr& mesh);

  // Releases the cached mesh associated with |name|.
  void ReleaseMesh(HashValue name);

  // Returns an empty mesh.
  MeshPtr EmptyMesh();

  // Creates a mesh using the specified data.
  MeshPtr CreateMesh(MeshData mesh_data);

  // Creates a named mesh using the specified data; automatically registered
  // with the factory.
  MeshPtr CreateMesh(HashValue name, MeshData mesh_data);

 private:
  Registry* registry_ = nullptr;
  ResourceManager<Mesh> meshes_;
  MeshPtr empty_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::MeshFactory);

#endif  // REDUX_ENGINES_RENDER_MESH_FACTORY_H_
