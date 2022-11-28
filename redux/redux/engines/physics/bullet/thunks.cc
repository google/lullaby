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
#include "redux/engines/physics/bullet/bullet_rigid_body.h"
#include "redux/engines/physics/bullet/bullet_trigger_volume.h"

namespace redux {

inline BulletPhysicsEngine* Upcast(PhysicsEngine* ptr) {
  return static_cast<BulletPhysicsEngine*>(ptr);
}
inline const BulletPhysicsEngine* Upcast(const PhysicsEngine* ptr) {
  return static_cast<const BulletPhysicsEngine*>(ptr);
}
inline BulletTriggerVolume* Upcast(TriggerVolume* ptr) {
  return static_cast<BulletTriggerVolume*>(ptr);
}
inline const BulletTriggerVolume* Upcast(const TriggerVolume* ptr) {
  return static_cast<const BulletTriggerVolume*>(ptr);
}
inline BulletRigidBody* Upcast(RigidBody* ptr) {
  return static_cast<BulletRigidBody*>(ptr);
}
inline const BulletRigidBody* Upcast(const RigidBody* ptr) {
  return static_cast<const BulletRigidBody*>(ptr);
}

}  // namespace redux

#include "redux/engines/physics/thunks/physics_engine.h"
#include "redux/engines/physics/thunks/rigid_body.h"
#include "redux/engines/physics/thunks/trigger_volume.h"
