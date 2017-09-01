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

#ifndef LULLABY_SYSTEMS_PHYSICS_PHYSICS_SYSTEM_H_
#define LULLABY_SYSTEMS_PHYSICS_PHYSICS_SYSTEM_H_

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "btBulletDynamicsCommon.h"
#include "lullaby/generated/rigid_body_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/physics/bullet_utils.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"

namespace lull {

/// The PhysicsSystem provides rigid body physics simulation to Entities using
/// the Bullet physics engine. It will update the transforms of simulated
/// Entities in the TransformSystem and dispatch events to Entities when they
/// enter or exit collision.
class PhysicsSystem : public System {
 public:
  /// Configuration params for the physics simulation.
  struct InitParams {
    mathfu::vec3 gravity = mathfu::vec3(0.f, -9.81f, 0.f);
    float timestep = 1.f / 60.f;
    int max_substeps = 4;
  };

  explicit PhysicsSystem(Registry* registry);
  PhysicsSystem(Registry* registry, const InitParams& params);
  ~PhysicsSystem() override;

  void Initialize() override;
  void Create(Entity entity, HashValue type, const Def* def) override;
  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;

  /// Update the physics simulation by |delta_time| seconds.
  void AdvanceFrame(Clock::duration delta_time);

  /// Check if two Entities are in contact.
  bool AreInContact(Entity one, Entity two) const;

  /// Set linear and angular velocities. Only has effect on Dynamic rigid
  /// bodies.
  void SetLinearVelocity(Entity entity, const mathfu::vec3& velocity);
  void SetAngularVelocity(Entity entity, const mathfu::vec3& velocity);

  /// Set gravity for the entire world.
  void SetGravity(const mathfu::vec3& gravity);

  /// Enable, disable, or check the enabled state of |entity|'s physics body.
  void EnablePhysics(Entity entity);
  void DisablePhysics(Entity entity);
  bool IsPhysicsEnabled(Entity entity) const;

 private:
  // The object responsible for synchronizing transform state between Bullet's
  // simulation objects and Lullaby's TransformSystem.
  class MotionState : public btMotionState {
   public:
    MotionState(const btTransform& transform, PhysicsSystem* physics_system,
                Entity entity)
        : input_transform_(transform),
          physics_system_(physics_system),
          entity_(entity) {}

    // Called at construction time for all bodies, as well as once per
    // simulation update for kinematic bodies.
    void getWorldTransform(btTransform& world_transform) const override;

    // Called every time the simulation updates this body's transform. This
    // function will no longer be called when the body is not active.
    void setWorldTransform(const btTransform& interpolated_transform) override;

    // Manually set the transform. This function will only have effect on
    // kinematic bodies.
    void SetKinematicTransform(const btTransform& world_transform);

   private:
    btTransform input_transform_;
    PhysicsSystem* physics_system_;
    Entity entity_;
  };

  struct RigidBody : Component {
    explicit RigidBody(Entity entity) : Component(entity) {}

    std::unique_ptr<btRigidBody> bt_body;
    std::unique_ptr<MotionState> bt_motion_state;
    // The shape that actually represents this rigid body. The owning unique_ptr
    // of this shape will be the last member of bt_shapes.
    btCollisionShape* bt_primary_shape = nullptr;
    // The scale applied to bt_primary_shape prior to any Entity-related
    // scaling. Required for applying scale changes while updating simulation
    // transforms.
    mathfu::vec3 primary_shape_scale = mathfu::kOnes3f;
    // Ownership of the shape(s) that this RigidBody uses.
    std::vector<std::unique_ptr<btCollisionShape>> bt_shapes;

    mathfu::vec3 center_of_mass_translation = mathfu::kZeros3f;
    RigidBodyType type = RigidBodyType::RigidBodyType_Dynamic;
    ColliderType collider_type = ColliderType::ColliderType_Standard;
    // This flag persists when the Entity becomes disabled, and is used to
    // determine whether or not the Entity should be re-added to the simulation
    // when re-enabled.
    bool enabled = false;
  };

  void InitRigidBody(Entity entity, const RigidBodyDef* data);
  void InitCollisionShape(RigidBody* body, const RigidBodyDef* data);

  // Setup Bullet flags. Should be called whenever the body changes rigid body
  // types or collider types. Will be called by SetupBtIntertialProperties() as
  // well.
  void SetupBtFlags(RigidBody* body) const;

  // Setup mass and local inertia. Should be called whenever a Dynamic body
  // changes mass or shape.
  void SetupBtIntertialProperties(RigidBody* body) const;

  // Setup the collision shape to match the AABB of the Entity. Assumes that the
  // shape has already been constructed and only needs to be re-scaled.
  void SetupAabbCollisionShape(RigidBody* body);

  // The post-tick internal callback allows contact events to be dispatched.
  static void InternalTickCallback(btDynamicsWorld* world, btScalar time_step);
  void PostSimulationTick();

  // For Lullaby -> simulation transform syncs.
  void UpdateSimulationTransform(
      Entity entity, const mathfu::mat4& world_from_entity_mat);

  // For simulation -> Lullaby transform syncs.
  void MarkForUpdate(Entity entity);
  void UpdateLullabyTransform(Entity entity);

  // Delegate |one| and |two| to the outputs |primary| and |secondary| for
  // accessing the ContactMap.
  static void PickPrimaryAndSecondaryEntities(
      Entity one, Entity two, Entity* primary, Entity* secondary);

  // Determine if this rigid body is using the motion state to push transforms
  // to Bullet.
  static bool UsesKinematicMotionState(const RigidBody* body);

  void OnEntityDisabled(Entity entity);
  void OnEntityEnabled(Entity entity);
  void OnParentChanged(Entity entity, Entity new_parent);
  void OnAabbChanged(Entity entity);

  ComponentPool<RigidBody> rigid_bodies_;
  TransformSystem* transform_system_;
  TransformSystem::TransformFlags transform_flag_;
  // The list of Entities that changed during the most recent set of simulation
  // updates.
  std::vector<Entity> updated_entities_;

  // Maps each Entity to the set of its current contacts (as of the last
  // simulation update). For each pair of Entities A and B, the contact will be
  // stored in the smaller of their two values. So this means a contact between
  // Entities 3 and 7, for example, will be stored in current_contacts_[3].
  using ContactMap =  std::unordered_map<Entity, std::unordered_set<Entity>>;
  ContactMap current_contacts_;

  float timestep_;
  int max_substeps_;

  std::unique_ptr<btCollisionConfiguration> bt_config_;
  std::unique_ptr<btCollisionDispatcher> bt_dispatcher_;
  std::unique_ptr<btBroadphaseInterface> bt_broadphase_;
  std::unique_ptr<btConstraintSolver> bt_solver_;
  std::unique_ptr<btDiscreteDynamicsWorld> bt_world_;

  PhysicsSystem(const PhysicsSystem&) = delete;
  PhysicsSystem& operator=(const PhysicsSystem&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::PhysicsSystem);

#endif  // LULLABY_SYSTEMS_PHYSICS_PHYSICS_SYSTEM_H_
