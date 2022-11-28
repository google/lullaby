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

#include "redux/systems/shape/shape_system.h"

#include <utility>

#include "redux/modules/base/data_builder.h"
#include "redux/modules/graphics/vertex.h"
#include "redux/systems/physics/physics_system.h"
#include "redux/systems/render/render_system.h"
#include "redux/systems/shape/shape_builder.h"

namespace redux {
namespace {

using VertexPosition3f =
    VertexElement<VertexType::Vec3f, VertexUsage::Position>;
using VertexNormal3f = VertexElement<VertexType::Vec3f, VertexUsage::Normal>;
using VertexOrientation4f =
    VertexElement<VertexType::Vec4f, VertexUsage::Orientation>;
using VertexUv2f = VertexElement<VertexType::Vec2f, VertexUsage::TexCoord0>;
using ShapeVertex = Vertex<VertexPosition3f, VertexOrientation4f, VertexUv2f>;
using ShapeIndex = uint16_t;

}  // namespace

ShapeSystem::ShapeSystem(Registry* registry) : System(registry) {
  RegisterDef(&ShapeSystem::AddBoxShapeDef);
  RegisterDef(&ShapeSystem::AddSphereShapeDef);
}

void ShapeSystem::AddBoxShapeDef(Entity entity, const BoxShapeDef& def) {
  const Box bounds(-def.half_extents, def.half_extents);
  BuildCollisionShape(entity, &def);
  BuildMeshShape(entity, &def, bounds);
}

void ShapeSystem::AddSphereShapeDef(Entity entity, const SphereShapeDef& def) {
  const Box bounds(vec3(-def.radius), vec3(def.radius));
  BuildCollisionShape(entity, &def);
  BuildMeshShape(entity, &def, bounds);
}

template <typename T>
void ShapeSystem::BuildCollisionShape(Entity entity, const T* def) {
  auto physics_system = registry_->Get<PhysicsSystem>();
  if (physics_system == nullptr) {
    return;
  }

  CollisionDataPtr collision_data = std::make_shared<CollisionData>();
  if constexpr (std::is_same_v<T, SphereShapeDef>) {
    collision_data->AddSphere(vec3::Zero(), def->radius);
  } else if constexpr (std::is_same_v<T, BoxShapeDef>) {
    collision_data->AddBox(vec3::Zero(), quat::Identity(), def->half_extents);
  } else {
    CHECK(false);
  }
  physics_system->SetShape(entity, std::move(collision_data));
}

template <typename T>
void ShapeSystem::BuildMeshShape(Entity entity, const T* def,
                                 const Box& bounds) {
  auto render_systems = registry_->Get<RenderSystem>();
  if (render_systems == nullptr) {
    return;
  }

  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  builder.Build(*def);

  MeshData::PartData part;
  part.primitive_type = MeshPrimitiveType::Triangles;
  part.start = 0;
  part.end = builder.Indices().size();
  part.box = bounds;
  DataBuilder parts(sizeof(MeshData::PartData));
  parts.Append(part);

  MeshData mesh_data;
  const VertexFormat vertex_format = ShapeVertex().GetVertexFormat();
  mesh_data.SetVertexData(vertex_format, builder.ReleaseVertices(), bounds);
  mesh_data.SetIndexData(MeshIndexType::U16, builder.ReleaseIndices());
  mesh_data.SetParts(parts.Release());

  render_systems->SetMesh(entity, std::move(mesh_data));
}
}  // namespace redux
