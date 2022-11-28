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

#ifndef REDUX_ENGINES_PHYSICS_BULLET_BULLET_TRIGGER_VOLUME_H_
#define REDUX_ENGINES_PHYSICS_BULLET_BULLET_TRIGGER_VOLUME_H_

#include "BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "redux/engines/physics/trigger_volume.h"

namespace redux {

class PhysicsWorld;

class BulletTriggerVolume : public TriggerVolume {
 public:
  BulletTriggerVolume(const TriggerVolumeParams& params,
                      btDynamicsWorld* world);
  ~BulletTriggerVolume() override;

  // Enables the trigger volume to be included when performing any potential
  // collision detection.
  void Activate();

  // Disables the trigger volume from being including in any collision
  // detection.
  void Deactivate();

  // Returns true if the trigger volume is active.
  bool IsActive() const;

  // Sets the trigger volume's transform.
  void SetTransform(const Transform& transform);

  // Returns the trigger volume's position.
  vec3 GetPosition() const;

  // Returns the trigger volume's rotation.
  quat GetRotation() const;

 private:
  void UpdateFlags();

  TriggerVolumeParams params_;
  btDynamicsWorld* world_ = nullptr;
  std::unique_ptr<btMotionState> bt_motion_state_;
  std::unique_ptr<btRigidBody> bt_rigid_body_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_BULLET_BULLET_TRIGGER_VOLUME_H_
