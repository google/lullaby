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

#ifndef REDUX_ENGINES_PHYSICS_RIGID_BODY_H_
#define REDUX_ENGINES_PHYSICS_RIGID_BODY_H_

#include <memory>

#include "redux/engines/physics/collision_shape.h"
#include "redux/engines/physics/enums.h"
#include "redux/modules/base/bits.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/math/transform.h"

namespace redux {

struct RigidBodyParams {
  RigidBodyMotionType type = RigidBodyMotionType::Static;

  float mass = 0.f;  // in kg
  float restitution = 0.f;
  float sliding_friction = 0.f;
  float rolling_friction = 0.f;
  float spinning_friction = 0.f;

  // The shape of the volume.
  CollisionShapePtr shape;

  // The Entity to which the rigid body belongs. Used for collision callbacks.
  Entity entity;

  // The groups to which the rigid body belongs.
  Bits32 collision_group = Bits32::All();

  // The groups against which the rigid body will collide.
  Bits32 collision_filter = Bits32::All();
};

// A rigid body is a physics object that has mass and shape/volume.
class RigidBody {
 public:
  virtual ~RigidBody() = default;

  RigidBody(const RigidBody&) = delete;
  RigidBody& operator=(const RigidBody&) = delete;

  // Enables the rigid body to be included in any dynamics calculations and
  // collision detection.
  void Activate();

  // Disables the rigid body from being included in any dynamics calculations or
  // collision detection.
  void Deactivate();

  // Changes the type of the rigid body.
  void SetType(RigidBodyMotionType type);

  // Returns true if the rigid body is active.
  bool IsActive() const;

  // Applies a force onto the rigid body at the given position.
  void ApplyForce(const vec3& force, const vec3& position);

  // Applies a force onto the rigid body at the given position.
  void ApplyImpulse(const vec3& impulse, const vec3& position);

  // Applies toque onto the rigid body.
  void ApplyTorque(const vec3& torque);

  // Sets the rigid body's transform.
  void SetTransform(const Transform& transform);

  // Returns the rigid body's position.
  vec3 GetPosition() const;

  // Returns the rigid body's rotation.
  quat GetRotation() const;

  // Sets the rigid body's linear velocity.
  void SetLinearVelocity(const vec3& velocity);

  // Returns the rigid body's linear velocity.
  vec3 GetLinearVelocity() const;

  // Sets the rigid body's angular velocity (in radians/second per axis).
  void SetAngularVelocity(const vec3& velocity);

  // Returns the rigid body's angular velocity.
  vec3 GetAngularVelocity() const;

  // Sets the rigid body's mass (in kg).
  void SetMass(float mass_in_kg);

  // Returns the rigid body's mass.
  float GetMass() const;

  // Sets the rigid body's co-efficient of restitution (i.e. bounciness), with a
  // range of 0.0 (i.e. will not bounce whatsoever) to 1.0 (i.e. does not lose
  // any energy from bouncing).
  void SetRestitution(float restitution);

  // Returns the rigid body's co-efficient of restitution (i.e. bounciness).
  float GetRestitution() const;

  // Sets the rigid body's co-efficient of sliding friction, with a range of 0.0
  // (i.e. will not stop) to 1.0 (i.e. will not slide).
  void SetSlidingFriction(float friction);

  // Returns the rigid body's co-efficient of sliding friction.
  float GetSlidingFriction() const;

  // Sets the rigid body's co-efficient of rolling friction, with a range of 0.0
  // (i.e. will not stop) to 1.0 (i.e. will not slide).
  void SetRollingFriction(float friction);

  // Returns the rigid body's co-efficient of rolling friction.
  float GetRollingFriction() const;

  // Sets the rigid body's co-efficient of spinning friction, with a range of
  // 0.0 (i.e. will not stop) to 1.0 (i.e. will not spin).
  void SetSpinningFriction(float friction);

  // Returns the rigid body's co-efficient of spinning friction.
  float GetSpinningFriction() const;

  // Sets the rigid body's linear damping, with a range of 0.0 (i.e. do not
  // apply damping) to 1.0 (i.e. set the linear velocity to zero).
  void SetLinearDamping(float damping);

  // Returns the rigid body's linear damping value.
  float GetLinearDamping() const;

  // Sets the rigid body's angular damping, with a range of 0.0 (i.e. do not
  // apply damping) to 1.0 (i.e. set the angular velocity to zero).
  void SetAngularDamping(float damping);

  // Returns the rigid body's angular damping value.
  float GetAngularDamping() const;

 protected:
  RigidBody() = default;
};

using RigidBodyPtr = std::shared_ptr<RigidBody>;

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_RIGID_BODY_H_
