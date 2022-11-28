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
#include "redux/engines/physics/physics_engine.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/ecs/system.h"
#include "redux/modules/math/math.h"
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

  // Sets the rigid body properties of an Entity. Only when an Entity has both
  // RigidBodyParams and a Shape (see SetShape below), will it start behaving
  // like a rigid body.
  void SetRigidBodyParams(Entity entity, RigidBodyParams params);

  // Sets the trigger volume properties of an Entity. Only when an Entity has
  // both TriggerVolumeParams and a Shape (see SetShape below), will it start
  // behaving like a trigger volume.
  void SetTriggerVolumeParams(Entity entity, TriggerVolumeParams params);

  // Sets the collision shape for the Entity.
  void SetShape(Entity entity, CollisionShapePtr shape);
  void SetShape(Entity entity, CollisionDataPtr shape_data);

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
