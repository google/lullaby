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

#include "lullaby/systems/physics/physics_shape_system.h"

#include "lullaby/events/render_events.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/systems/model_asset/model_asset_system.h"
#include "lullaby/systems/physics/bullet_utils.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/registry.h"
#include "lullaby/generated/physics_shape_def_generated.h"
#include "lullaby/generated/physics_shapes_generated.h"

namespace lull {
namespace {
constexpr HashValue kPhysicsShapeDef = ConstHash("PhysicsShapeDef");
constexpr int kVertPerTri = 3;
constexpr int kFloatPerVert = 3;
}  // namespace

PhysicsShapeSystem::PhysicsShapeSystem(Registry* registry)
    : System(registry),
      mesh_colliders_(4),
      config_(MakeUnique<btDefaultCollisionConfiguration>()),
      dispatcher_(MakeUnique<btCollisionDispatcher>(config_.get())),
      broadphase_(MakeUnique<btDbvtBroadphase>()),
      world_(MakeUnique<btCollisionWorld>(dispatcher_.get(), broadphase_.get(),
                                          config_.get())) {
  RegisterDef<PhysicsShapeDefT>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<ModelAssetSystem>(this);
}

void PhysicsShapeSystem::Initialize() {
  auto* collision_system = registry_->Get<CollisionSystem>();
  if (collision_system) {
    collision_system->RegisterCollisionProvider(this);
  }
}

void PhysicsShapeSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type != kPhysicsShapeDef) {
    LOG(DFATAL) << "Unsupported ComponentDef type: " << type;
    return;
  }
  const PhysicsShapeDef* data = ConvertDef<PhysicsShapeDef>(def);
  const auto* shape_parts = data->shapes();
  if (shape_parts == nullptr || shape_parts->size() < 1) {
    return;
  }
  if (shape_parts->size() > 1) {
    // TODO: Support multiple shape parts.
    LOG(DFATAL) << "Multiple shape parts not yet supported in PhysicsShapeDef";
    return;
  }
  const PhysicsShapePart* part = (*shape_parts)[0];
  const auto shape_type = part->shape_type();
  if (shape_type != PhysicsShapePrimitive_PhysicsMeshShape) {
    // TODO: Support different shape types.
    LOG(DFATAL) << "Unsupported shape type in PhysicsShapeDef";
    return;
  }
  mesh_colliders_.Emplace(entity);
  pending_entities_.push_back(entity);
}

void PhysicsShapeSystem::CreateMeshShape(Entity entity, HashValue mesh_id,
                                         const MeshData& mesh_data) {
  if (!mesh_colliders_.Contains(entity)) {
    return;
  }
  if (shape_cache_.count(mesh_id) > 0) {
    return;
  }

  std::unique_ptr<ShapeData> shape = MakeUnique<ShapeData>(
      GetMeshVertices(mesh_data), GetMeshIndices(mesh_data));

  shape->mesh_interface = MakeUnique<btTriangleIndexVertexArray>(
      shape->indices.size() / kVertPerTri, shape->indices.data(),
      kVertPerTri * sizeof(decltype(shape->indices)::value_type),
      shape->vertices.size(), shape->vertices.data(),
      kFloatPerVert * sizeof(decltype(shape->vertices)::value_type));

  shape_cache_[mesh_id] = std::move(shape);
}

void PhysicsShapeSystem::SetMesh(MeshCollider* collider,
                                 btStridingMeshInterface* mesh_interface) {
  std::unique_ptr<btGImpactMeshShape> collision_shape =
      MakeUnique<btGImpactMeshShape>(mesh_interface);
  collision_shape->postUpdate();
  collision_shape->updateBound();

  std::unique_ptr<btDefaultMotionState> motion_state =
      MakeUnique<btDefaultMotionState>();
  std::unique_ptr<btCollisionObject> collision_object = MakeUnique<btRigidBody>(
      btScalar(0), motion_state.get(), collision_shape.get());
  collision_object->setUserIndex(collider->GetEntity().AsUint32());

  collider->motion_state = std::move(motion_state);
  collider->collision_shape = std::move(collision_shape);
  collider->collision_object = std::move(collision_object);

  UpdateCollider(collider);
}

std::vector<btScalar> PhysicsShapeSystem::GetMeshVertices(
    const MeshData& mesh_data) {
  std::vector<btScalar> vert;
  vert.reserve(mesh_data.GetNumVertices() * kFloatPerVert);
  ForEachVertexPosition(mesh_data.GetVertexBytes(), mesh_data.GetNumVertices(),
                        mesh_data.GetVertexFormat(),
                        [&vert](const mathfu::vec3& v) {
                          vert.push_back(v.x);
                          vert.push_back(v.y);
                          vert.push_back(v.z);
                        });
  return vert;
}

std::vector<int> PhysicsShapeSystem::GetMeshIndices(const MeshData& mesh_data) {
  switch (mesh_data.GetIndexType()) {
    case MeshData::kIndexU16:
      return GetMeshIndicesImpl(mesh_data.GetNumIndices(),
                                mesh_data.GetIndexData<uint16_t>());
    case MeshData::kIndexU32:
      return GetMeshIndicesImpl(mesh_data.GetNumIndices(),
                                mesh_data.GetIndexData<uint32_t>());
    default:
      LOG(DFATAL) << "Unrecognized IndexType " << mesh_data.GetIndexType();
      return std::vector<int>();
  }
}

template <typename T>
std::vector<int> PhysicsShapeSystem::GetMeshIndicesImpl(size_t num_indices,
                                                        const T* indices) {
  std::vector<int> idx;
  idx.reserve(num_indices);
  for (size_t i = 0; i < num_indices; ++i) {
    idx.push_back(indices[i]);
  }
  return idx;
}

void PhysicsShapeSystem::Destroy(Entity entity) {
  MeshCollider* collider = mesh_colliders_.Get(entity);
  if (!collider) {
    return;
  }

  btCollisionObject* collision_object = collider->collision_object.get();
  world_->removeCollisionObject(collision_object);
  mesh_colliders_.Destroy(entity);
}

bool PhysicsShapeSystem::FinishLoadingEntity(Entity entity) {
  MeshCollider* collider = mesh_colliders_.Get(entity);
  if (!collider) {
    // The entity's collider was deleted before the asset was loaded. Return
    // true so that the entity is removed from pending_entities_.
    return true;
  }

  auto* model_asset_system = registry_->Get<ModelAssetSystem>();
  const ModelAsset* asset = model_asset_system->GetModelAsset(entity);
  if (!asset) {
    // The entity's asset isn't loaded yet, so keep it in pending_entities_.
    return false;
  }

  const auto itr = shape_cache_.find(asset->GetId());
  if (itr == shape_cache_.end()) {
    // The asset hasn't been converted to a shape yet, so keep the entity in
    // pending_entities_.
    return false;
  }

  // The asset is loaded and converted to a shape, so finish loading the entity.
  SetMesh(collider, itr->second->mesh_interface.get());
  return true;
}

void PhysicsShapeSystem::AdvanceFrame() {
  for (int i = 0; i < pending_entities_.size();) {
    if (FinishLoadingEntity(pending_entities_[i])) {
      pending_entities_[i] = pending_entities_.back();
      pending_entities_.pop_back();
    } else {
      ++i;
    }
  }
  for (MeshCollider& collider : mesh_colliders_) {
    UpdateCollider(&collider);
  }
  world_->updateAabbs();
}

void PhysicsShapeSystem::UpdateCollider(MeshCollider* collider) {
  auto* transform_system = registry_->Get<TransformSystem>();
  bool entity_enabled = transform_system->IsEnabled(collider->GetEntity());
  UpdateCollider(collider, entity_enabled);
}

void PhysicsShapeSystem::UpdateCollider(MeshCollider* collider, bool enable) {
  if (!collider->collision_object) {
    // This means the entity has been created, but the mesh isn't loaded yet.
    return;
  }
  auto* transform_system = registry_->Get<TransformSystem>();
  if (enable) {
    if (!collider->enabled) {
      world_->addCollisionObject(collider->collision_object.get());
      collider->enabled = true;
    }
    const Sqt sqt = CalculateSqtFromMatrix(
        *transform_system->GetWorldFromEntityMatrix(collider->GetEntity()));
    const btTransform bt_tf(BtQuatFromMathfu(sqt.rotation),
                            BtVectorFromMathfu(sqt.translation));
    collider->collision_object->setWorldTransform(bt_tf);
    btVector3 bt_scale = BtVectorFromMathfu(sqt.scale);
    collider->collision_shape->setLocalScaling(bt_scale);
    collider->collision_shape->postUpdate();
    collider->collision_shape->updateBound();
  } else if (collider->enabled) {
    world_->removeCollisionObject(collider->collision_object.get());
    collider->enabled = false;
  }
}

bool PhysicsShapeSystem::Raycast(const mathfu::vec3& from,
                                 const mathfu::vec3& to,
                                 RaycastHit* result) const {
  btVector3 bt_from = BtVectorFromMathfu(from);
  btVector3 bt_to = BtVectorFromMathfu(to);
  btCollisionWorld::ClosestRayResultCallback ray_callback(bt_from, bt_to);
  world_->rayTest(bt_from, bt_to, ray_callback);

  if (!ray_callback.hasHit()) {
    return false;
  }

  result->pos = MathfuVectorFromBt(ray_callback.m_hitPointWorld);
  result->entity = ray_callback.m_collisionObject->getUserIndex();

  return true;
}

}  // namespace lull
