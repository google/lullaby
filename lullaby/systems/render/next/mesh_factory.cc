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

#include "lullaby/systems/render/next/mesh_factory.h"

namespace lull {

MeshFactory::MeshFactory(Registry* registry, fplbase::Renderer* renderer,
                         std::shared_ptr<fplbase::AssetManager> asset_manager)
    : registry_(registry),
      fpl_renderer_(renderer),
      fpl_asset_manager_(std::move(asset_manager)) {}

MeshPtr MeshFactory::LoadMesh(const std::string& filename) {
  const HashValue key = Hash(filename.c_str());
  MeshPtr mesh = meshes_.Create(
      key, [&]() { return MeshPtr(new Mesh(LoadFplMesh(filename))); });
  meshes_.Release(key);
  return mesh;
}

MeshPtr MeshFactory::CreateMesh(const MeshData& mesh) {
  if (mesh.GetNumVertices() == 0) {
    return MeshPtr();
  }
  return MeshPtr(new Mesh(mesh));
}

MeshPtr MeshFactory::CreateMesh(HashValue key, const MeshData& mesh) {
  DCHECK(key != 0) << "Invalid key for render factory mesh.";
  if (mesh.GetNumVertices() == 0) {
    return MeshPtr();
  }
  return meshes_.Create(key, [&]() { return MeshPtr(new Mesh(mesh)); });
}

Mesh::MeshImplPtr MeshFactory::LoadFplMesh(const std::string& name) {
  constexpr bool async = true;
  fplbase::Mesh* mesh = fpl_asset_manager_->LoadMesh(name.c_str(), async);
  if (!mesh) {
    LOG(ERROR) << "Could not load mesh: " << name;
    return nullptr;
  }

  std::weak_ptr<fplbase::AssetManager> asset_manager_weak = fpl_asset_manager_;
  return Mesh::MeshImplPtr(
      mesh, [asset_manager_weak, name](const fplbase::Mesh*) {
        if (auto asset_manager = asset_manager_weak.lock()) {
          asset_manager->UnloadMesh(name.c_str());
        }
      });
}

MeshPtr MeshFactory::GetCachedMesh(HashValue key) const {
  return meshes_.Find(key);
}

void MeshFactory::CacheMesh(HashValue key, const MeshPtr& mesh) {
  meshes_.Create(key, [mesh] { return mesh; });
}

void MeshFactory::ReleaseMeshFromCache(HashValue key) { meshes_.Release(key); }

}  // namespace lull
