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

#ifndef REDUX_ENGINES_PHYSICS_TRIGGER_VOLUME_H_
#define REDUX_ENGINES_PHYSICS_TRIGGER_VOLUME_H_

#include <memory>

#include "redux/engines/physics/collision_shape.h"
#include "redux/modules/base/bits.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/math/transform.h"

namespace redux {

struct TriggerVolumeParams {
  // The shape of the trigger volume.
  CollisionShapePtr shape;

  // The Entity to which the trigger volume belongs. Used for collision
  // callbacks.
  Entity entity;

  // The groups to which the trigger volume belongs.
  Bits32 collision_group = Bits32::All();

  // The groups against which the trigger volume will collide.
  Bits32 collision_filter = Bits32::All();
};

// A trigger volume is a massless physics object with a shape/volume.
class TriggerVolume {
 public:
  virtual ~TriggerVolume() = default;

  TriggerVolume(const TriggerVolume&) = delete;
  TriggerVolume& operator=(const TriggerVolume&) = delete;

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

 protected:
  TriggerVolume() = default;
};

using TriggerVolumePtr = std::shared_ptr<TriggerVolume>;

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_TRIGGER_VOLUME_H_
