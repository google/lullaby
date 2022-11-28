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

#include "redux/engines/physics/bullet/bullet_rigid_body.h"

#include "LinearMath/btDefaultMotionState.h"
#include "redux/engines/physics/bullet/bullet_collision_shape.h"
#include "redux/engines/physics/bullet/bullet_utils.h"
#include "redux/modules/base/bits.h"

namespace redux {

BulletRigidBody::BulletRigidBody(const RigidBodyParams& params,
                                 btDynamicsWorld* world)
    : params_(params), world_(world) {
  btCollisionShape* bt_shape = GetUnderlyingBtCollisionShape();

  bt_motion_state_ = std::make_unique<btDefaultMotionState>();

  btVector3 inertia(0.f, 0.f, 0.f);
  bt_shape->calculateLocalInertia(params.mass, inertia);
  btRigidBody::btRigidBodyConstructionInfo info(
      params.mass, bt_motion_state_.get(), bt_shape, inertia);
  info.m_friction = params.sliding_friction;
  info.m_restitution = params.restitution;
  bt_rigid_body_ = std::make_unique<btRigidBody>(info);
  bt_rigid_body_->setUserIndex(EntityToBulletUserIndex(params_.entity));

  UpdateFlags();
  Activate();
}

BulletRigidBody::~BulletRigidBody() { Deactivate(); }

void BulletRigidBody::UpdateFlags() {
  static constexpr int kStaticFlag =
      btCollisionObject::CollisionFlags::CF_STATIC_OBJECT;
  static constexpr int kKinematicFlag =
      btCollisionObject::CollisionFlags::CF_KINEMATIC_OBJECT;

  int flags = bt_rigid_body_->getCollisionFlags();
  if (params_.type == RigidBodyMotionType::Static) {
    flags = SetBits(flags, kStaticFlag);
  } else {
    flags = ClearBits(flags, kStaticFlag);
  }
  if (params_.type == RigidBodyMotionType::Kinematic) {
    flags = SetBits(flags, kKinematicFlag);
  } else {
    flags = ClearBits(flags, kKinematicFlag);
  }
  bt_rigid_body_->setFlags(flags);
}

void BulletRigidBody::Activate() {
  const int group = static_cast<int>(params_.collision_group.Value());
  const int mask = static_cast<int>(params_.collision_filter.Value());
  world_->addRigidBody(bt_rigid_body_.get(), group, mask);
}

void BulletRigidBody::Deactivate() {
  world_->removeRigidBody(bt_rigid_body_.get());
}

void BulletRigidBody::SetType(RigidBodyMotionType type) {
  params_.type = type;
  UpdateFlags();
}

bool BulletRigidBody::IsActive() const { return bt_rigid_body_->isInWorld(); }

void BulletRigidBody::SetTransform(const Transform& transform) {
  const auto bt_translation = ToBullet(transform.translation);
  const auto bt_rotation = ToBullet(transform.rotation);
  const auto bt_scale = ToBullet(transform.scale);
  bt_rigid_body_->setWorldTransform(btTransform(bt_rotation, bt_translation));
  GetUnderlyingBtCollisionShape()->setLocalScaling(bt_scale);
}

vec3 BulletRigidBody::GetPosition() const {
  btTransform transform;
  bt_rigid_body_->getMotionState()->getWorldTransform(transform);
  return FromBullet(transform.getOrigin());
}

quat BulletRigidBody::GetRotation() const {
  btTransform transform;
  bt_rigid_body_->getMotionState()->getWorldTransform(transform);
  return FromBullet(transform.getRotation());
}

void BulletRigidBody::SetMass(float mass_in_kg) {
  btVector3 inertia(0.f, 0.f, 0.f);
  GetUnderlyingBtCollisionShape()->calculateLocalInertia(mass_in_kg, inertia);
  bt_rigid_body_->setMassProps(mass_in_kg, inertia);
  // setMassProps() can change collision flags, so reset them to be sure.
  UpdateFlags();
}

float BulletRigidBody::GetMass() const { return bt_rigid_body_->getMass(); }

void BulletRigidBody::SetRestitution(float restitution) {
  bt_rigid_body_->setRestitution(restitution);
}

float BulletRigidBody::GetRestitution() const {
  return bt_rigid_body_->getRestitution();
}

void BulletRigidBody::SetSlidingFriction(float friction) {
  bt_rigid_body_->setFriction(friction);
}

float BulletRigidBody::GetSlidingFriction() const {
  return bt_rigid_body_->getFriction();
}

void BulletRigidBody::SetRollingFriction(float friction) {
  bt_rigid_body_->setRollingFriction(friction);
}

float BulletRigidBody::GetRollingFriction() const {
  return bt_rigid_body_->getRollingFriction();
}

void BulletRigidBody::SetSpinningFriction(float friction) {
  bt_rigid_body_->setSpinningFriction(friction);
}

float BulletRigidBody::GetSpinningFriction() const {
  return bt_rigid_body_->getSpinningFriction();
}

void BulletRigidBody::SetLinearDamping(float damping) {
  const float angular_damping = bt_rigid_body_->getAngularDamping();
  bt_rigid_body_->setDamping(damping, angular_damping);
}

float BulletRigidBody::GetLinearDamping() const {
  return bt_rigid_body_->getLinearDamping();
}

void BulletRigidBody::SetAngularDamping(float damping) {
  const float linear_damping = bt_rigid_body_->getLinearDamping();
  bt_rigid_body_->setDamping(linear_damping, damping);
}

float BulletRigidBody::GetAngularDamping() const {
  return bt_rigid_body_->getAngularDamping();
}

void BulletRigidBody::SetLinearVelocity(const vec3& velocity) {
  bt_rigid_body_->setLinearVelocity(ToBullet(velocity));
}

vec3 BulletRigidBody::GetLinearVelocity() const {
  return FromBullet(bt_rigid_body_->getLinearVelocity());
}

void BulletRigidBody::SetAngularVelocity(const vec3& velocity) {
  bt_rigid_body_->setAngularVelocity(ToBullet(velocity));
}

vec3 BulletRigidBody::GetAngularVelocity() const {
  return FromBullet(bt_rigid_body_->getAngularVelocity());
}

void BulletRigidBody::ApplyForce(const vec3& force, const vec3& position) {
  bt_rigid_body_->applyForce(ToBullet(force), ToBullet(position));
}

void BulletRigidBody::ApplyImpulse(const vec3& impulse, const vec3& position) {
  bt_rigid_body_->applyImpulse(ToBullet(impulse), ToBullet(position));
}

void BulletRigidBody::ApplyTorque(const vec3& torque) {
  bt_rigid_body_->applyTorque(ToBullet(torque));
}

btCollisionShape* BulletRigidBody::GetUnderlyingBtCollisionShape() {
  return Upcast(params_.shape.get())->GetUnderlyingBtCollisionShape();
}

}  // namespace redux
