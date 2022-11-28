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

#ifndef REDUX_ENGINES_PHYSICS_BULLET_BULLET_PHYSICS_ENGINE_H_
#define REDUX_ENGINES_PHYSICS_BULLET_BULLET_PHYSICS_ENGINE_H_

#include <algorithm>

#include "btBulletDynamicsCommon.h"
#include "redux/engines/physics/bullet/bullet_collision_shape.h"
#include "redux/engines/physics/bullet/bullet_rigid_body.h"
#include "redux/engines/physics/bullet/bullet_trigger_volume.h"
#include "redux/engines/physics/bullet/bullet_utils.h"
#include "redux/engines/physics/physics_engine.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/base/static_registry.h"
#include "redux/modules/ecs/entity.h"

namespace redux {

struct BulletPhysicsCollisionKey {
  BulletPhysicsCollisionKey(Entity a, Entity b) {
    static_assert(sizeof(key) == sizeof(entities));
    entities[0] = std::min(a.get(), b.get());
    entities[1] = std::max(a.get(), b.get());
  }

  template <typename H>
  friend H AbslHashValue(H h, const BulletPhysicsCollisionKey& key) {
    return H::combine(std::move(h), key.key);
  }

  bool operator==(const BulletPhysicsCollisionKey& rhs) const {
    return key == rhs.key;
  }

  union {
    uint64_t key;
    Entity::Rep entities[2];
  };
};

struct BulletPhysicsCollisionData {
  const btPersistentManifold* manifold = nullptr;
  size_t contact_index = 0;
  size_t num_contacts = 0;
};

class BulletPhysicsEngine : public PhysicsEngine {
 public:
  explicit BulletPhysicsEngine(Registry* registry);

  void OnRegistryInitialize();

  // The force of gravity that is applied to all rigid bodies in the world.
  // The default gravity is (0, -9.81, 0).
  void SetGravity(const vec3& gravity);

  // Physics works best when it is stepped with a consistent timestamp.
  // Internally, the PhysicsEngine will keep track of any time accumalation
  // between this timestep and the AdvanceFrame delta_time and may perform
  // multiple physics steps in a AdvanceFrame call to keep things in sync.
  // The default timestep is 1/60s with a max_substeps of 4.
  void SetTimestep(absl::Duration timestep, int max_substeps);

  // Advances the physics simulation by the given timestep.
  void AdvanceFrame(absl::Duration timestep);

  // Creates an active rigid body using the provided data.
  RigidBodyPtr CreateRigidBody(const RigidBodyParams& params);

  // Creates an active trigger volume using the provided data.
  TriggerVolumePtr CreateTriggerVolume(const TriggerVolumeParams& params);

  // Creates a CollisionShape using the provided data.
  CollisionShapePtr CreateShape(CollisionDataPtr shape_data);

  // Creates a CollisionShape using the data associated with |name|.
  CollisionShapePtr CreateShape(HashValue name);

  // Adds the given collision shape data to the cache using |name|.
  // CollisionShapes can then be created from this data by just refering to its
  // name.
  void CacheShapeData(HashValue name, CollisionDataPtr data);

  // Releases the cached shape data associated with |name|.
  void ReleaseShapeData(HashValue name);

  // Callback for when collisions occur between two volumes. The Entity values
  // are specified in the trigger volume/rigid body construction params.
  using CollisionCallback = std::function<void(Entity, Entity)>;

  // Sets the callback to invoke when two objects enter each others collision
  // volumes.
  void SetOnEnterCollisionCallback(CollisionCallback cb);

  // Sets the callback to invoke when two objects exit each others collision
  // volumes.
  void SetOnExitCollisionCallback(CollisionCallback cb);

  // Returns information about all the contacts between two Entities. Should be
  // used in conjunction with the above collision callbacks.
  absl::Span<const ContactPoint> GetActiveContacts(Entity entity_a,
                                                   Entity entity_b) const;

  // Internal function used by the Bullet tick/step callback.
  void OnSimTick();

 private:
  using CollisionMap = absl::flat_hash_map<BulletPhysicsCollisionKey,
                                           BulletPhysicsCollisionData>;

  void ProcessContactManifold(const btPersistentManifold* manifold);

  Registry* registry_ = nullptr;
  ResourceManager<CollisionData> shape_data_;
  CollisionCallback on_enter_collision_;
  CollisionCallback on_exit_collision_;
  std::unique_ptr<btCollisionConfiguration> bt_config_;
  std::unique_ptr<btCollisionDispatcher> bt_dispatcher_;
  std::unique_ptr<btBroadphaseInterface> bt_broadphase_;
  std::unique_ptr<btConstraintSolver> bt_solver_;
  std::unique_ptr<btDiscreteDynamicsWorld> bt_world_;
  CollisionMap current_collisions_;
  CollisionMap previous_collisions_;
  std::vector<ContactPoint> contacts_;
  vec3 gravity_ = {0, -9.81, 0};
  float timestep_ = 1 / 60.f;
  int max_substeps_ = 4;
};

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_BULLET_BULLET_PHYSICS_ENGINE_H_
