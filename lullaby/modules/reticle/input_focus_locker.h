/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_MODULES_RETICLE_INPUT_FOCUS_LOCKER_H_
#define LULLABY_MODULES_RETICLE_INPUT_FOCUS_LOCKER_H_

#include <unordered_map>

#include "lullaby/util/entity.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input/input_focus.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"

namespace lull {
/// This class forces a device to stay focused on a specific entity.  It also
/// stores an offset in that entity's local space, so that a grabber or cursor
/// can be kept still relative to the entity it is locked to.
class InputFocusLocker {
 public:
  explicit InputFocusLocker(Registry* registry) : registry_(registry) {}

  /// Lock the device to focus on a specific entity.  For the duration of the
  /// lock, the device's cursor entity will maintain a constant local space
  /// offset from the target entity's world location. Pass kNullEntity to unlock
  /// the device.
  void LockOn(InputManager::DeviceType device, Entity entity,
              const mathfu::vec3& offset);

  /// Returns the Entity |device| is currently locked to, or kNullEntity if
  /// it isn't locked to anything.
  Entity GetCurrentLock(InputManager::DeviceType device) const;

  /// Reset the lock state of the device.
  void Unlock(InputManager::DeviceType device);

  /// Applies the current Lock to the InputFocus.  This should be called early
  /// in the InputFocus update step, and if it returns true other collision
  /// checks can be skipped.
  bool UpdateInputFocus(InputFocus* focus);

 private:
  struct Lock {
    // The entity the device is locked to.
    Entity entity = kNullEntity;

    // The world space offset from the locked_entity's position.
    mathfu::vec3 offset = mathfu::kZeros3f;
  };

  Lock locks_[InputManager::kMaxNumDeviceTypes];

  Registry* registry_;
};
}  // namespace lull
LULLABY_SETUP_TYPEID(lull::InputFocusLocker);

#endif  // LULLABY_MODULES_RETICLE_INPUT_FOCUS_LOCKER_H_
