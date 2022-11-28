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

#ifndef REDUX_ENGINES_PHYSICS_PHYSICS_ENGINE_H_
#define REDUX_ENGINES_PHYSICS_PHYSICS_ENGINE_H_

#include <functional>

#include "absl/time/time.h"
#include "redux/engines/physics/collision_data.h"
#include "redux/engines/physics/collision_shape.h"
#include "redux/engines/physics/rigid_body.h"
#include "redux/engines/physics/trigger_volume.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/resource_manager.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Holds and updates all physics objects.
//
// A physics object is essentially any object that occupies a volume in 3D
// space. There are two main types of physics objects: trigger volumes and
// rigid bodies.
//
// Trigger volumes are massless and exist solely to notify users when they are
// colliding/intersecting with other physics objects. (See trigger_volume.h
// for more details.)
//
// Rigid bodies, on the other hand, have mass which allows them to partake in
// dynamics, "the study of forces and their effects on motion." Simply put,
// rigid bodies can move on their own and bounce off each other. (See
// rigid_body.h for more details.)
//
// The PhysicsEngine is responsible for managing these objects, detecting
// collisions between these objects and, in the case of rigid bodies, updating
// their transforms.
class PhysicsEngine {
 public:
  virtual ~PhysicsEngine() = default;

  PhysicsEngine(const PhysicsEngine&) = delete;
  PhysicsEngine& operator=(const PhysicsEngine&) = delete;

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

  struct ContactPoint {
    vec3 world_position = vec3::Zero();
    vec3 contact_normal = vec3::Zero();
  };

  // Returns information about all the contacts between two Entities. Should be
  // used in conjunction with the above collision callbacks.
  absl::Span<const ContactPoint> GetActiveContacts(Entity entity_a,
                                                   Entity entity_b) const;

 protected:
  PhysicsEngine() = default;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::PhysicsEngine);

#endif  // REDUX_ENGINES_PHYSICS_PHYSICS_ENGINE_H_
