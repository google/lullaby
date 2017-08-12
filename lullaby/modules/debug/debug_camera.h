/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_MODULES_DEBUG_DEBUG_CAMERA_H_
#define LULLABY_MODULES_DEBUG_DEBUG_CAMERA_H_

#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Hooks into controller events to allow touchpad 6DOF navigation.
// Long press of the system button while pointing up starts the
// fly mode.  Then, swiping on the touchpad will move the start
// to world translation.  To leave fly mode, long press while
// pointing up again.  A reset gesture will move the user back
// to the unmodified start position but leave fly mode on.  By
// clicking the touchpad, the two movement modes can be selected.
class DebugCamera {
 public:
  explicit DebugCamera(Registry* registry);
  ~DebugCamera();

  void StartDebugMode();

  void StopDebugMode();

  void AdvanceFrame(Clock::duration delta_time);

  mathfu::vec3 GetTranslation() const;

 private:
  enum class MovementMode {
    // The position of the touchpad determines the position of the camera
    // (like a trackpad)
    kTrackpad,
    // The position of the touchpad determines the rate of camera movement
    // (like a joystick)
    kJoystick,
  };

  bool ControllerIsPointingUp() const;

  void ResetDebugPosition();

  Registry* registry_;
  bool in_debug_mode_ = false;
  MovementMode movement_mode_ = MovementMode::kTrackpad;
  mathfu::vec3 debug_start_position_ = mathfu::kZeros3f;
  mathfu::vec3 debug_position_ = mathfu::kZeros3f;

  DebugCamera(const DebugCamera&) = delete;
  DebugCamera& operator=(const DebugCamera&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::DebugCamera);

#endif  // LULLABY_MODULES_DEBUG_DEBUG_CAMERA_H_
