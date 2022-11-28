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

#ifndef REDUX_ENGINES_PHYSICS_THUNKS_RIGID_BODY_H_
#define REDUX_ENGINES_PHYSICS_THUNKS_RIGID_BODY_H_

#include "redux/engines/physics/rigid_body.h"

namespace redux {

// Thunk functions to call the actual implementation.
void RigidBody::Activate() { Upcast(this)->Activate(); }
void RigidBody::Deactivate() { Upcast(this)->Deactivate(); }
void RigidBody::SetType(RigidBodyMotionType type) {
  Upcast(this)->SetType(type);
}
bool RigidBody::IsActive() const { return Upcast(this)->IsActive(); }
void RigidBody::SetTransform(const Transform& transform) {
  Upcast(this)->SetTransform(transform);
}
vec3 RigidBody::GetPosition() const { return Upcast(this)->GetPosition(); }
quat RigidBody::GetRotation() const { return Upcast(this)->GetRotation(); }
void RigidBody::SetMass(float mass_in_kg) { Upcast(this)->SetMass(mass_in_kg); }
float RigidBody::GetMass() const { return Upcast(this)->GetMass(); }
void RigidBody::SetRestitution(float restitution) {
  Upcast(this)->SetRestitution(restitution);
}
float RigidBody::GetRestitution() const {
  return Upcast(this)->GetRestitution();
}
void RigidBody::SetSlidingFriction(float friction) {
  Upcast(this)->SetSlidingFriction(friction);
}
float RigidBody::GetSlidingFriction() const {
  return Upcast(this)->GetSlidingFriction();
}
void RigidBody::SetRollingFriction(float friction) {
  Upcast(this)->SetRollingFriction(friction);
}
float RigidBody::GetRollingFriction() const {
  return Upcast(this)->GetRollingFriction();
}
void RigidBody::SetSpinningFriction(float friction) {
  Upcast(this)->SetSpinningFriction(friction);
}
float RigidBody::GetSpinningFriction() const {
  return Upcast(this)->GetSpinningFriction();
}
void RigidBody::SetLinearDamping(float damping) {
  Upcast(this)->SetLinearDamping(damping);
}
float RigidBody::GetLinearDamping() const {
  return Upcast(this)->GetLinearDamping();
}
void RigidBody::SetAngularDamping(float damping) {
  Upcast(this)->SetAngularDamping(damping);
}
float RigidBody::GetAngularDamping() const {
  return Upcast(this)->GetAngularDamping();
}
void RigidBody::SetLinearVelocity(const vec3& velocity) {
  Upcast(this)->SetLinearVelocity(velocity);
}
vec3 RigidBody::GetLinearVelocity() const {
  return Upcast(this)->GetLinearVelocity();
}
void RigidBody::SetAngularVelocity(const vec3& velocity) {
  Upcast(this)->SetAngularVelocity(velocity);
}
vec3 RigidBody::GetAngularVelocity() const {
  return Upcast(this)->GetAngularVelocity();
}
void RigidBody::ApplyForce(const vec3& force, const vec3& position) {
  Upcast(this)->ApplyForce(force, position);
}
void RigidBody::ApplyImpulse(const vec3& impulse, const vec3& position) {
  Upcast(this)->ApplyImpulse(impulse, position);
}
void RigidBody::ApplyTorque(const vec3& torque) {
  Upcast(this)->ApplyTorque(torque);
}

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_THUNKS_RIGID_BODY_H_
