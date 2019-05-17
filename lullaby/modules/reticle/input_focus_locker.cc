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

#include "lullaby/modules/reticle/input_focus_locker.h"

#include "lullaby/systems/transform/transform_system.h"

namespace lull {
void InputFocusLocker::LockOn(InputManager::DeviceType device, Entity entity,
                              const mathfu::vec3& offset) {
  if (device == InputManager::kMaxNumDeviceTypes) {
    LOG(DFATAL) << "Invalid device";
    return;
  }
  locks_[device].entity = entity;
  locks_[device].offset = offset;
}

Entity InputFocusLocker::GetCurrentLock(InputManager::DeviceType device) const {
  if (device == InputManager::kMaxNumDeviceTypes) {
    LOG(DFATAL) << "Invalid device";
    return kNullEntity;
  }
  return locks_[device].entity;
}

void InputFocusLocker::Unlock(InputManager::DeviceType device) {
  LockOn(device, kNullEntity, mathfu::kZeros3f);
}

bool InputFocusLocker::UpdateInputFocus(InputFocus* focus) {
  if (focus->device == InputManager::kMaxNumDeviceTypes) {
    LOG(DFATAL) << "Invalid device";
    return false;
  }
  if (locks_[focus->device].entity != kNullEntity) {
    auto* transform_system = registry_->Get<TransformSystem>();
    const mathfu::mat4* target_mat = transform_system->GetWorldFromEntityMatrix(
        locks_[focus->device].entity);
    if (target_mat) {
      focus->target = locks_[focus->device].entity;
      focus->cursor_position = *target_mat * locks_[focus->device].offset;
      return true;
    } else {
      // No transform for the locked entity, so reset the lock.
      Unlock(focus->device);
    }
  }
  return false;
}
}  // namespace lull
