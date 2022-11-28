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

#include "redux/engines/physics/bullet/bullet_physics_engine.h"

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

static void CreatePhysicsEngine(Registry* registry) {
  auto ptr = new BulletPhysicsEngine(registry);
  registry->Register(std::unique_ptr<PhysicsEngine>(ptr));
}

static StaticRegistry Static_Register(CreatePhysicsEngine);

static void InternalTickCallback(btDynamicsWorld* world, btScalar time_step) {
  BulletPhysicsEngine* engine =
      static_cast<BulletPhysicsEngine*>(world->getWorldUserInfo());
  engine->OnSimTick();
}

BulletPhysicsEngine::BulletPhysicsEngine(Registry* registry)
    : registry_(registry) {
  bt_config_ = std::make_unique<btDefaultCollisionConfiguration>();
  bt_dispatcher_ = std::make_unique<btCollisionDispatcher>(bt_config_.get());
  bt_broadphase_ = std::make_unique<btDbvtBroadphase>();
  bt_solver_ = std::make_unique<btSequentialImpulseConstraintSolver>();
  bt_world_ = std::make_unique<btDiscreteDynamicsWorld>(
      bt_dispatcher_.get(), bt_broadphase_.get(), bt_solver_.get(),
      bt_config_.get());
  bt_world_->setGravity(ToBullet(gravity_));
  bt_world_->setInternalTickCallback(InternalTickCallback,
                                     static_cast<void*>(this));

  on_exit_collision_ = [](Entity, Entity) {};
  on_enter_collision_ = [](Entity, Entity) {};
}

void BulletPhysicsEngine::OnSimTick() {
  contacts_.clear();
  current_collisions_.clear();

  for (int i = 0; i < bt_dispatcher_->getNumManifolds(); ++i) {
    const btPersistentManifold* manifold =
        bt_dispatcher_->getManifoldByIndexInternal(i);
    ProcessContactManifold(manifold);
  }

  for (auto iter : current_collisions_) {
    const bool new_collision = !previous_collisions_.contains(iter.first);
    if (new_collision) {
      on_enter_collision_(Entity(iter.first.entities[0]),
                          Entity(iter.first.entities[1]));
    }
  }

  for (auto iter : previous_collisions_) {
    const bool expired_collision = !current_collisions_.contains(iter.first);
    if (expired_collision) {
      on_exit_collision_(Entity(iter.first.entities[0]),
                         Entity(iter.first.entities[1]));
    }
  }

  using std::swap;
  swap(current_collisions_, previous_collisions_);
}

void BulletPhysicsEngine::ProcessContactManifold(
    const btPersistentManifold* manifold) {
  const size_t num_contacts = manifold->getNumContacts();
  if (num_contacts == 0) {
    return;
  }

  auto body_a = manifold->getBody0();
  auto body_b = manifold->getBody1();
  Entity entity_a = EntityFromBulletUserIndex(body_a->getUserIndex());
  Entity entity_b = EntityFromBulletUserIndex(body_b->getUserIndex());
  CHECK(entity_a.get() && entity_b.get());

  const BulletPhysicsCollisionKey key(entity_a, entity_b);
  CHECK(key.entities[0] < key.entities[1]);
  const bool flip_normal = entity_a.get() > entity_b.get();

  const size_t start = contacts_.size();
  current_collisions_[key] = {manifold, start, num_contacts};
  for (size_t i = 0; i < num_contacts; ++i) {
    const btManifoldPoint& bt_contact = manifold->getContactPoint(i);

    ContactPoint point;
    point.world_position = FromBullet(bt_contact.getPositionWorldOnB());
    point.contact_normal = flip_normal
                               ? FromBullet(bt_contact.m_normalWorldOnB)
                               : -FromBullet(bt_contact.m_normalWorldOnB);
    contacts_.emplace_back(point);
  }
}

absl::Span<const BulletPhysicsEngine::ContactPoint>
BulletPhysicsEngine::GetActiveContacts(Entity entity_a, Entity entity_b) const {
  const BulletPhysicsCollisionKey key(entity_a, entity_b);
  auto iter = current_collisions_.find(key);
  if (iter != current_collisions_.end()) {
    return absl::Span<const BulletPhysicsEngine::ContactPoint>(
        &contacts_[iter->second.contact_index], iter->second.num_contacts);
  }
  return {};
}

void BulletPhysicsEngine::AdvanceFrame(absl::Duration timestep) {
  // During one AdvanceFrame() call, do at most a set number of 1/60 second
  // updates. Bullet will update the MotionStates of every Dynamic Entity that
  // has a transform update. These Entities will be marked for synchronization.
  const float dt = static_cast<float>(absl::ToDoubleSeconds(timestep));
  bt_world_->stepSimulation(dt, max_substeps_, timestep_);
}

void BulletPhysicsEngine::OnRegistryInitialize() {
  auto* choreographer = registry_->Get<Choreographer>();
  if (choreographer) {
    choreographer->Add<&PhysicsEngine::AdvanceFrame>(
        Choreographer::Stage::kPhysics);
  }
}

void BulletPhysicsEngine::SetOnEnterCollisionCallback(CollisionCallback cb) {
  on_enter_collision_ = std::move(cb);
}

void BulletPhysicsEngine::SetOnExitCollisionCallback(CollisionCallback cb) {
  on_exit_collision_ = std::move(cb);
}

RigidBodyPtr BulletPhysicsEngine::CreateRigidBody(
    const RigidBodyParams& params) {
  auto ptr = std::make_shared<BulletRigidBody>(params, bt_world_.get());
  return std::static_pointer_cast<RigidBody>(ptr);
}

TriggerVolumePtr BulletPhysicsEngine::CreateTriggerVolume(
    const TriggerVolumeParams& params) {
  auto ptr = std::make_shared<BulletTriggerVolume>(params, bt_world_.get());
  return std::static_pointer_cast<TriggerVolume>(ptr);
}

CollisionShapePtr BulletPhysicsEngine::CreateShape(
    CollisionDataPtr shape_data) {
  auto ptr = std::make_shared<BulletCollisionShape>(std::move(shape_data));
  return std::static_pointer_cast<CollisionShape>(ptr);
}

CollisionShapePtr BulletPhysicsEngine::CreateShape(HashValue name) {
  auto ptr = shape_data_.Find(name);
  if (ptr) {
    auto impl = std::make_shared<BulletCollisionShape>(ptr);
    return std::static_pointer_cast<CollisionShape>(impl);
  }
  return nullptr;
}

void BulletPhysicsEngine::CacheShapeData(HashValue name,
                                         CollisionDataPtr data) {
  shape_data_.Register(name, std::move(data));
}

void BulletPhysicsEngine::ReleaseShapeData(HashValue name) {
  shape_data_.Release(name);
}

}  // namespace redux
