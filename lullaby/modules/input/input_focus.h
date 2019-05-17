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

#ifndef LULLABY_UTIL_INPUT_FOCUS_H_
#define LULLABY_UTIL_INPUT_FOCUS_H_

#include "lullaby/util/entity.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/math.h"

namespace lull {

/// A struct representing what entity a device is currently focused on.
struct InputFocus {
  /// The entity being focused on.
  Entity target = kNullEntity;
  /// The world space location of the input source.
  mathfu::vec3 origin = mathfu::kZeros3f;
  /// The world space Ray that collided with the target.
  Ray collision_ray = Ray(mathfu::kZeros3f, mathfu::kZeros3f);
  /// The world space position where the cursor should be rendered. Usually
  /// but not always the point where the collision_ray hit the target.
  mathfu::vec3 cursor_position = mathfu::kZeros3f;
  /// The world space position where the cursor would be, disregarding any
  /// collisions or other focus modifiers.
  mathfu::vec3 no_hit_cursor_position = mathfu::kZeros3f;
  /// True if the selected entity is interactive.
  bool interactive = false;
  /// True if the selected entity is draggable.
  bool draggable = false;
  /// The device this focus is associated with.
  InputManager::DeviceType device = InputManager::kMaxNumDeviceTypes;
};
}  // namespace lull

#endif  // LULLABY_UTIL_INPUT_FOCUS_H_
