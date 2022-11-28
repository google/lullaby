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

#ifndef REDUX_ENGINES_PHYSICS_THUNKS_PHYSICS_ENGINE_H_
#define REDUX_ENGINES_PHYSICS_THUNKS_PHYSICS_ENGINE_H_

#include <utility>

#include "redux/engines/physics/physics_engine.h"

namespace redux {

// Thunk functions to call the actual implementation.
void PhysicsEngine::OnRegistryInitialize() {
  Upcast(this)->OnRegistryInitialize();
}
void PhysicsEngine::SetOnEnterCollisionCallback(CollisionCallback cb) {
  Upcast(this)->SetOnEnterCollisionCallback(std::move(cb));
}
void PhysicsEngine::SetOnExitCollisionCallback(CollisionCallback cb) {
  Upcast(this)->SetOnExitCollisionCallback(std::move(cb));
}
absl::Span<const PhysicsEngine::ContactPoint> PhysicsEngine::GetActiveContacts(
    Entity entity_a, Entity entity_b) const {
  return Upcast(this)->GetActiveContacts(entity_a, entity_b);
}
void PhysicsEngine::AdvanceFrame(absl::Duration timestep) {
  Upcast(this)->AdvanceFrame(timestep);
}
RigidBodyPtr PhysicsEngine::CreateRigidBody(const RigidBodyParams& params) {
  return Upcast(this)->CreateRigidBody(params);
}
TriggerVolumePtr PhysicsEngine::CreateTriggerVolume(
    const TriggerVolumeParams& params) {
  return Upcast(this)->CreateTriggerVolume(params);
}
CollisionShapePtr PhysicsEngine::CreateShape(CollisionDataPtr shape_data) {
  return Upcast(this)->CreateShape(std::move(shape_data));
}
CollisionShapePtr PhysicsEngine::CreateShape(HashValue name) {
  return Upcast(this)->CreateShape(name);
}
void PhysicsEngine::CacheShapeData(HashValue name, CollisionDataPtr data) {
  Upcast(this)->CacheShapeData(name, std::move(data));
}
void PhysicsEngine::ReleaseShapeData(HashValue name) {
  Upcast(this)->ReleaseShapeData(name);
}

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_THUNKS_PHYSICS_ENGINE_H_
