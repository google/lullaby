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

#include "redux/engines/render/mesh_factory.h"

#include <utility>

#include "redux/engines/render/filament/filament_mesh.h"
#include "redux/engines/render/filament/filament_render_engine.h"

namespace redux {

MeshFactory::MeshFactory(Registry* registry) : registry_(registry) {}

MeshPtr MeshFactory::CreateMesh(MeshData mesh_data) {
  auto ptr = std::make_shared<MeshData>(std::move(mesh_data));
  auto impl = std::make_shared<FilamentMesh>(registry_, ptr);
  return std::static_pointer_cast<Mesh>(impl);
}

MeshPtr MeshFactory::CreateMesh(HashValue name, MeshData mesh_data) {
  MeshPtr mesh = CreateMesh(std::move(mesh_data));
  CacheMesh(name, mesh);
  return mesh;
}

void MeshFactory::CacheMesh(HashValue name, const MeshPtr& mesh) {
  meshes_.Register(name, mesh);
}

MeshPtr MeshFactory::GetMesh(HashValue name) const {
  return meshes_.Find(name);
}

void MeshFactory::ReleaseMesh(HashValue name) { meshes_.Release(name); }

MeshPtr MeshFactory::EmptyMesh() {
  if (!empty_) {
    auto mesh = std::make_shared<FilamentMesh>(registry_, nullptr);
    empty_ = std::static_pointer_cast<Mesh>(mesh);
  }
  return empty_;
}

}  // namespace redux
