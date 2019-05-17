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

#ifndef LULLABY_SYSTEMS_PHYSICS_PHYSICS_SHAPE_SYSTEM_H_
#define LULLABY_SYSTEMS_PHYSICS_PHYSICS_SHAPE_SYSTEM_H_

#include <unordered_map>

#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "btBulletCollisionCommon.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/systems/collision/collision_provider.h"
#include "lullaby/systems/model_asset/model_asset_system.h"

namespace lull {

// Contains the results of a raycast.
struct RaycastHit {
  mathfu::vec3 pos;
  Entity entity;
};

// System for storing and raycasting shapes.
class PhysicsShapeSystem : public System, public CollisionProvider {
 public:
  explicit PhysicsShapeSystem(Registry* registry);
  void Initialize() override;
  void Create(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;
  void AdvanceFrame();

  // Returns whether the ray between from and to hits an entity. If true, also
  // fills out the result struct, with information about the first hit.
  bool Raycast(const mathfu::vec3& from, const mathfu::vec3& to,
               RaycastHit* result) const;

  // From the CollisionProvider interface.
  void CreateMeshShape(Entity entity, HashValue mesh_id,
                       const MeshData& mesh_data) override;

 private:
  struct MeshCollider : Component {
    explicit MeshCollider(Entity entity) : Component(entity) {}

    bool enabled = false;
    btStridingMeshInterface* mesh_interface = nullptr;
    std::unique_ptr<btDefaultMotionState> motion_state;
    std::unique_ptr<btGImpactMeshShape> collision_shape;
    std::unique_ptr<btCollisionObject> collision_object;
  };

  struct ShapeData {
    ShapeData(std::vector<btScalar> vert, std::vector<int> ind)
        : vertices(std::move(vert)), indices(std::move(ind)) {}
    std::vector<btScalar> vertices;
    std::vector<int> indices;
    std::unique_ptr<btStridingMeshInterface> mesh_interface;
  };

  void SetMesh(MeshCollider* collider, btStridingMeshInterface* mesh_interface);
  void UpdateCollider(MeshCollider* collider);
  void UpdateCollider(MeshCollider* collider, bool enable);
  bool FinishLoadingEntity(Entity entity);
  static std::vector<btScalar> GetMeshVertices(const MeshData& mesh_data);
  static std::vector<int> GetMeshIndices(const MeshData& mesh_data);
  template <typename T>
  static std::vector<int> GetMeshIndicesImpl(size_t num_indices,
                                             const T* indices);

  ComponentPool<MeshCollider> mesh_colliders_;
  std::unordered_map<HashValue, std::unique_ptr<ShapeData>> shape_cache_;
  std::vector<Entity> pending_entities_;
  std::unique_ptr<btCollisionConfiguration> config_;
  std::unique_ptr<btCollisionDispatcher> dispatcher_;
  std::unique_ptr<btBroadphaseInterface> broadphase_;
  std::unique_ptr<btCollisionWorld> world_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::PhysicsShapeSystem);

#endif  // LULLABY_SYSTEMS_PHYSICS_PHYSICS_SHAPE_SYSTEM_H_
