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

#include "redux/systems/physics/physics_system.h"

#include <memory>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/time/time.h"
#include "redux/engines/physics/collision_data.h"
#include "redux/engines/physics/collision_shape.h"
#include "redux/engines/physics/physics_engine.h"
#include "redux/engines/physics/physics_enums_generated.h"
#include "redux/modules/base/choreographer.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/ecs/system.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/transform.h"
#include "redux/modules/math/vector.h"
#include "redux/systems/physics/events.h"
#include "redux/systems/dispatcher/dispatcher_system.h"
#include "redux/systems/physics/rigid_body_def_generated.h"
#include "redux/systems/physics/trigger_def_generated.h"
#include "redux/systems/transform/transform_system.h"

namespace redux {

PhysicsSystem::PhysicsSystem(Registry* registry) : System(registry) {
  RegisterDependency<PhysicsEngine>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDef(&PhysicsSystem::SetFromRigidBodyDef);
  RegisterDef(&PhysicsSystem::SetFromBoxTriggerDef);
  RegisterDef(&PhysicsSystem::SetFromSphereTriggerDef);
}

void PhysicsSystem::OnRegistryInitialize() {
  engine_ = registry_->Get<PhysicsEngine>();
  dispatcher_system_ = registry_->Get<DispatcherSystem>();

  if (dispatcher_system_) {
    engine_->SetOnEnterCollisionCallback(
        [=, this](Entity entity_a, Entity entity_b) {
          OnCollisionEnter(entity_a, entity_b);
        });
    engine_->SetOnExitCollisionCallback(
        [=, this](Entity entity_a, Entity entity_b) {
          OnCollisionExit(entity_a, entity_b);
        });
  }

  auto choreo = registry_->Get<Choreographer>();
  if (choreo) {
    choreo->Add<&PhysicsSystem::PrePhysics>(Choreographer::Stage::kPhysics)
        .Before<&PhysicsEngine::AdvanceFrame>();
    choreo->Add<&PhysicsSystem::PostPhysics>(Choreographer::Stage::kPhysics)
        .After<&PhysicsEngine::AdvanceFrame>();
  }
}

void PhysicsSystem::SetShape(Entity entity, CollisionDataPtr shape_data) {
  if (entity == kNullEntity) {
    return;
  }
  CollisionShapePtr shape = engine_->CreateShape(std::move(shape_data));
  SetShape(entity, std::move(shape));
}

void PhysicsSystem::SetShape(Entity entity, CollisionShapePtr shape) {
  if (entity == kNullEntity) {
    return;
  }
  shapes_[entity] = shape;
  if (rigid_bodies_.contains(entity)) {
    TryCreateRigidBody(entity);
  }
  if (trigger_volumes_.contains(entity)) {
    TryCreateTriggerVolume(entity);
  }
}

void PhysicsSystem::SetRigidBody(Entity entity, const RigidBodyConfig& config,
                                 const CollisionFlags& flags) {
  if (entity == kNullEntity) {
    return;
  }

  RigidBodyComponent& c = rigid_bodies_[entity];
  c.params.type = config.type;
  c.params.mass = config.mass;
  c.params.restitution = config.restitution;
  c.params.sliding_friction = config.sliding_friction;
  c.params.rolling_friction = config.rolling_friction;
  c.params.spinning_friction = config.spinning_friction;
  c.params.collision_filter = flags.collision_filter;
  c.params.collision_group = flags.collision_group;
  TryCreateRigidBody(entity);
}

void PhysicsSystem::SetTriggerVolume(Entity entity,
                                     const CollisionFlags& flags) {
  if (entity == kNullEntity) {
    return;
  }

  TriggerVolumeComponent& c = trigger_volumes_[entity];
  c.params.collision_filter = flags.collision_filter;
  c.params.collision_group = flags.collision_group;
  TryCreateTriggerVolume(entity);
}

void PhysicsSystem::OnDestroy(Entity entity) {
  trigger_volumes_.erase(entity);
  rigid_bodies_.erase(entity);
  shapes_.erase(entity);
}

void PhysicsSystem::SetRigidBodyMotionType(Entity entity,
                                           RigidBodyMotionType motion_type) {
  auto iter = rigid_bodies_.find(entity);
  if (iter == rigid_bodies_.end()) {
    LOG(FATAL) << "Entity is not a rigid body.";
  }

  iter->second.params.type = motion_type;
  if (iter->second.rigid_body) {
    iter->second.rigid_body->SetMotionType(motion_type);
  }
}

void PhysicsSystem::SetCollisionFlags(Entity entity,
                                      const CollisionFlags& flags) {
  if (auto iter = rigid_bodies_.find(entity); iter != rigid_bodies_.end()) {
    iter->second.params.collision_group = flags.collision_group;
    iter->second.params.collision_filter = flags.collision_filter;
    if (iter->second.rigid_body) {
      iter->second.rigid_body->SetCollisionState(flags.collision_group,
                                                 flags.collision_filter);
    }
    return;
  }

}

void PhysicsSystem::PrePhysics(absl::Duration timestep) {
  auto* transform_system = registry_->Get<TransformSystem>();

  for (auto& iter : trigger_volumes_) {
    if (iter.second.trigger_volume == nullptr) {
      continue;
    }
    if (!iter.second.trigger_volume->IsActive()) {
      continue;
    }

    const Transform transform = transform_system->GetTransform(iter.first);
    iter.second.trigger_volume->SetTransform(transform);
  }

  for (auto& iter : rigid_bodies_) {
    if (iter.second.params.type != RigidBodyMotionType::Kinematic) {
      continue;
    }
    if (iter.second.rigid_body == nullptr) {
      continue;
    }
    if (!iter.second.rigid_body->IsActive()) {
      continue;
    }

    const Transform transform = transform_system->GetTransform(iter.first);
    iter.second.rigid_body->SetTransform(transform);
  }
}

void PhysicsSystem::PostPhysics(absl::Duration timestep) {
  auto* transform_system = registry_->Get<TransformSystem>();

  for (auto& iter : rigid_bodies_) {
    if (iter.second.params.type != RigidBodyMotionType::Dynamic) {
      continue;
    }
    if (iter.second.rigid_body == nullptr) {
      continue;
    }
    if (!iter.second.rigid_body->IsActive()) {
      continue;
    }

    Transform transform = transform_system->GetTransform(iter.first);
    transform.translation = iter.second.rigid_body->GetPosition();
    transform.rotation = iter.second.rigid_body->GetRotation();
    transform_system->SetTransform(iter.first, transform);
  }
}

void PhysicsSystem::OnCollisionEnter(Entity entity_a, Entity entity_b) {
  if (dispatcher_system_->GetConnectionCount<CollisionEnterEvent>(entity_a)) {
    dispatcher_system_->SendToEntity(entity_a,
                                     CollisionEnterEvent{entity_a, entity_b});
  }
  if (dispatcher_system_->GetConnectionCount<CollisionEnterEvent>(entity_b)) {
    dispatcher_system_->SendToEntity(entity_b,
                                     CollisionEnterEvent{entity_b, entity_a});
  }
}

void PhysicsSystem::OnCollisionExit(Entity entity_a, Entity entity_b) {
  if (dispatcher_system_->GetConnectionCount<CollisionExitEvent>(entity_a)) {
    dispatcher_system_->SendToEntity(entity_a,
                                     CollisionExitEvent{entity_a, entity_b});
  }
  if (dispatcher_system_->GetConnectionCount<CollisionExitEvent>(entity_b)) {
    dispatcher_system_->SendToEntity(entity_b,
                                     CollisionExitEvent{entity_b, entity_a});
  }
}

void PhysicsSystem::TryCreateRigidBody(Entity entity) {
  auto iter = rigid_bodies_.find(entity);
  if (iter == rigid_bodies_.end()) {
    return;
  }

  iter->second.rigid_body = nullptr;
  if (auto shape = shapes_.find(entity); shape != shapes_.end()) {
    iter->second.params.entity = entity;
    iter->second.params.shape = shape->second;
    iter->second.rigid_body = engine_->CreateRigidBody(iter->second.params);

    auto* transform_system = registry_->Get<TransformSystem>();
    const Transform transform = transform_system->GetTransform(entity);
    iter->second.rigid_body->SetTransform(transform);
  }
}

void PhysicsSystem::TryCreateTriggerVolume(Entity entity) {
  auto iter = trigger_volumes_.find(entity);
  if (iter == trigger_volumes_.end()) {
    return;
  }

  iter->second.trigger_volume = nullptr;
  if (auto shape = shapes_.find(entity); shape != shapes_.end()) {
    iter->second.params.entity = entity;
    iter->second.params.shape = shape->second;
    iter->second.trigger_volume =
        engine_->CreateTriggerVolume(iter->second.params);

    auto* transform_system = registry_->Get<TransformSystem>();
    const Transform transform = transform_system->GetTransform(entity);
    iter->second.trigger_volume->SetTransform(transform);
  }
}

void PhysicsSystem::SetFromRigidBodyDef(Entity entity,
                                        const RigidBodyDef& def) {
  RigidBodyConfig config;
  config.type = def.motion_type;
  config.mass = def.mass;
  config.restitution = def.restitution;
  config.sliding_friction = def.friction;
  SetRigidBody(entity, config, CollisionFlags());
}

void PhysicsSystem::SetFromBoxTriggerDef(Entity entity,
                                         const BoxTriggerDef& def) {
  CollisionDataPtr shape = std::make_shared<CollisionData>();
  shape->AddBox(vec3::Zero(), quat::Identity(), def.half_extents);
  SetShape(entity, shape);
  SetTriggerVolume(entity, CollisionFlags());
}

void PhysicsSystem::SetFromSphereTriggerDef(Entity entity,
                                            const SphereTriggerDef& def) {
  CollisionDataPtr shape = std::make_shared<CollisionData>();
  shape->AddSphere(vec3::Zero(), def.radius);
  SetShape(entity, shape);
  SetTriggerVolume(entity, CollisionFlags());
}

}  // namespace redux
