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

#include "redux/engines/physics/bullet/bullet_trigger_volume.h"

#include <memory>

#include "LinearMath/btDefaultMotionState.h"
#include "redux/engines/physics/bullet/bullet_collision_shape.h"
#include "redux/engines/physics/bullet/bullet_utils.h"

namespace redux {

BulletTriggerVolume::BulletTriggerVolume(const TriggerVolumeParams& params,
                                         btDynamicsWorld* world)
    : params_(params), world_(world) {
  const btScalar mass = 0.f;
  btCollisionShape* bt_shape =
      Upcast(params_.shape.get())->GetUnderlyingBtCollisionShape();
  bt_motion_state_ = std::make_unique<btDefaultMotionState>();
  bt_rigid_body_ =
      std::make_unique<btRigidBody>(mass, bt_motion_state_.get(), bt_shape);
  bt_rigid_body_->setUserIndex(EntityToBulletUserIndex(params_.entity));
  UpdateFlags();
  Activate();
}

BulletTriggerVolume::~BulletTriggerVolume() { Deactivate(); }

void BulletTriggerVolume::Activate() {
  const int group = static_cast<int>(params_.collision_group.Value());
  const int mask = static_cast<int>(params_.collision_filter.Value());
  world_->addRigidBody(bt_rigid_body_.get(), group, mask);
}

void BulletTriggerVolume::Deactivate() {
  world_->removeRigidBody(bt_rigid_body_.get());
}

bool BulletTriggerVolume::IsActive() const {
  return bt_rigid_body_->isInWorld();
}

void BulletTriggerVolume::SetTransform(const Transform& transform) {
  btCollisionShape* bt_shape =
      Upcast(params_.shape.get())->GetUnderlyingBtCollisionShape();

  const auto bt_translation = ToBullet(transform.translation);
  const auto bt_rotation = ToBullet(transform.rotation);
  const auto bt_scale = ToBullet(transform.scale);
  bt_rigid_body_->setWorldTransform(btTransform(bt_rotation, bt_translation));
  bt_shape->setLocalScaling(bt_scale);
}

vec3 BulletTriggerVolume::GetPosition() const {
  btTransform transform;
  bt_rigid_body_->getMotionState()->getWorldTransform(transform);
  return FromBullet(transform.getOrigin());
}

quat BulletTriggerVolume::GetRotation() const {
  btTransform transform;
  bt_rigid_body_->getMotionState()->getWorldTransform(transform);
  return FromBullet(transform.getRotation());
}

void BulletTriggerVolume::UpdateFlags() {
  static constexpr int kStaticFlag =
      btCollisionObject::CollisionFlags::CF_STATIC_OBJECT;
  static constexpr int kKinematicFlag =
      btCollisionObject::CollisionFlags::CF_KINEMATIC_OBJECT;
  static constexpr int kNoResponseFlag =
      btCollisionObject::CollisionFlags::CF_NO_CONTACT_RESPONSE;
  static constexpr int kNoGravityFlag =
      btRigidBodyFlags::BT_DISABLE_WORLD_GRAVITY;

  int collision_flags = bt_rigid_body_->getCollisionFlags();
  collision_flags = SetBits(collision_flags, kNoResponseFlag);
  collision_flags = ClearBits(collision_flags, kStaticFlag);
  collision_flags = ClearBits(collision_flags, kKinematicFlag);
  bt_rigid_body_->setCollisionFlags(collision_flags);

  int rigid_body_flags = bt_rigid_body_->getFlags();
  rigid_body_flags = SetBits(rigid_body_flags, kNoGravityFlag);
  bt_rigid_body_->setFlags(rigid_body_flags);
}

}  // namespace redux
