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

#include "lullaby/systems/shape/shape_system.h"

#include "lullaby/modules/render/mesh_util.h"
#include "lullaby/systems/render/render_system.h"

namespace lull {

ShapeSystem::ShapeSystem(Registry* registry) : System(registry) {
  RegisterDef<SphereDefT>(this);
  RegisterDef<RectMeshDefT>(this);
  RegisterDependency<RenderSystem>(this);
}

void ShapeSystem::PostCreateComponent(Entity entity,
                                      const Blueprint& blueprint) {
  if (blueprint.Is<SphereDefT>()) {
    SphereDefT sphere;
    blueprint.Read(&sphere);
    CreateSphere(entity, sphere);
  } else if (blueprint.Is<RectMeshDefT>()) {
    RectMeshDefT quad;
    blueprint.Read(&quad);
    CreateRect(entity, quad);
  } else {
    LOG(DFATAL) << "Unsupported shape.";
  }
}

void ShapeSystem::CreateRect(Entity entity, const RectMeshDefT& rect) {
  MeshData mesh = CreateQuadMesh<VertexPT>(
      rect.size_x, rect.size_y, rect.verts_x, rect.verts_y, rect.corner_radius,
      rect.corner_verts);
  CreateShape(entity, rect.pass, std::move(mesh));
}

void ShapeSystem::CreateSphere(Entity entity, const SphereDefT& sphere) {
  MeshData mesh = CreateLatLonSphere(sphere.radius, sphere.num_parallels,
                                     sphere.num_meridians);
  CreateShape(entity, sphere.pass, std::move(mesh));
}

void ShapeSystem::CreateShape(Entity entity, HashValue pass,
                              MeshData mesh_data) {
  auto* render_system = registry_->Get<RenderSystem>();
  if (pass == 0) {
    pass = render_system->GetDefaultRenderPass();
  }
  render_system->SetMesh({entity, pass}, mesh_data);
}

}  // namespace lull
