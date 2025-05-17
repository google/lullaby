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

#ifndef REDUX_SYSTEMS_PHYSICS_PHYSICS_SYSTEM_H_
#define REDUX_SYSTEMS_PHYSICS_PHYSICS_SYSTEM_H_

#include "absl/container/flat_hash_map.h"
#include "absl/time/time.h"
#include "redux/engines/physics/collision_data.h"
#include "redux/engines/physics/collision_shape.h"
#include "redux/engines/physics/physics_engine.h"
#include "redux/engines/physics/physics_enums_generated.h"
#include "redux/engines/physics/rigid_body.h"
#include "redux/engines/physics/trigger_volume.h"
#include "redux/modules/base/bits.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/ecs/system.h"
#include "redux/systems/dispatcher/dispatcher_system.h"
#include "redux/systems/physics/rigid_body_def_generated.h"
#include "redux/systems/physics/trigger_def_generated.h"

namespace redux {

// Manages the physical properties (shape, mass, etc.) of an Entity that partake
// in the dynamics simulation.
//
// A physical entity can fall into one of three basic categories:
// - ghost bodies: these entities only have a shape which can be used for
//   collision queries (ie. raycasts, overlaps, etc.)
// - trigger volumes: these are similar to ghosts, but generate events when they
//   collide with other physical entities
// - rigid bodies: these objects have shape, mass, and other physical
//   properties; they generate both collision events and responses (ie. they
//   affect other rigid bodies).
//
// Rigid bodies are further divided into three types:
// - static: immovable entities
// - kinematic: motion is controlled by the TransformSystem
// - dynamic: motion is controlled by the PhysicsEngine
class PhysicsSystem : public System {
 public:
  explicit PhysicsSystem(Registry* registry);

  void OnRegistryInitialize();

  // Parameters used to configure a RigidBody Entity.
  struct RigidBodyConfig {
    // The method by which the RigidBody will move.
    RigidBodyMotionType type = RigidBodyMotionType::Static;

    // The mass of the rigid body. Note that this is ignored for Static bodies.
    float mass = 0.f;  // in kg

    // The coefficient of restitution (bounciness) of the rigid body.
    float restitution = 0.f;

    // The coefficient of friction when sliding of the rigid body.
    float sliding_friction = 0.f;

    // The coefficient of friction when rolling of the rigid body.
    float rolling_friction = 0.f;

    // The coefficient of friction when spinning of the rigid body.
    float spinning_friction = 0.f;
  };

  // Flags used for controlling collision responses for Entities.
  struct CollisionFlags {
    CollisionFlags()
        : collision_group(Bits32::All()), collision_filter(Bits32::All()) {}

    // The collision groups to which the Entity belongs.
    Bits32 collision_group;

    // The groups against which the Entity will report collisions.
    Bits32 collision_filter;
  };

  // Configures the Entity as a rigid body. However, the Entity will only start
  // behaving like a rigid body once it also has a shape (see below).
  void SetRigidBody(Entity entity, const RigidBodyConfig& config,
                    const CollisionFlags& flags = CollisionFlags());

  // Configures the Entity as a trigger volume. However, the Entity will only
  // start behaving like a trigger volume once it also has a shape (see below).
  void SetTriggerVolume(Entity entity,
                        const CollisionFlags& flags = CollisionFlags());

  // Sets the collision shape for the Entity.
  void SetShape(Entity entity, CollisionShapePtr shape);
  void SetShape(Entity entity, CollisionDataPtr shape_data);

  // Changes the motion type of an Entity's rigid body. The Entity must have a
  // rigid body configured using SetRigidBody.
  void SetRigidBodyMotionType(Entity entity, RigidBodyMotionType motion_type);

  // Changes the collision flags for the Entity.
  void SetCollisionFlags(Entity entity, const CollisionFlags& flags);

  // Prepares all physical Entities to be updated by the PhysicsEngine.
  void PrePhysics(absl::Duration timestep);

  // Updates all physical Entities based on the results of the PhysicsEngine
  // update.
  void PostPhysics(absl::Duration timestep);

  // Initializes physical properties from defs.
  void SetFromRigidBodyDef(Entity entity, const RigidBodyDef& def);
  void SetFromBoxTriggerDef(Entity entity, const BoxTriggerDef& def);
  void SetFromSphereTriggerDef(Entity entity, const SphereTriggerDef& def);

 private:
  struct RigidBodyComponent {
    RigidBodyParams params;
    RigidBodyPtr rigid_body;
  };
  struct TriggerVolumeComponent {
    TriggerVolumeParams params;
    TriggerVolumePtr trigger_volume;
  };

  void TryCreateRigidBody(Entity entity);
  void TryCreateTriggerVolume(Entity entity);

  void OnDestroy(Entity entity) override;
  void OnCollisionEnter(Entity entity_a, Entity entity_b);
  void OnCollisionExit(Entity entity_a, Entity entity_b);

  PhysicsEngine* engine_ = nullptr;
  DispatcherSystem* dispatcher_system_ = nullptr;
  absl::flat_hash_map<Entity, CollisionShapePtr> shapes_;
  absl::flat_hash_map<Entity, RigidBodyComponent> rigid_bodies_;
  absl::flat_hash_map<Entity, TriggerVolumeComponent> trigger_volumes_;
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::PhysicsSystem);

#endif  // REDUX_SYSTEMS_PHYSICS_PHYSICS_SYSTEM_H_
