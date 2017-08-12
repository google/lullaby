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

#include "lullaby/modules/debug/debug_camera.h"

#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/events/input_events.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/time.h"

namespace lull {

// Enable debug camera if the controller is within 30 degrees of vertical
static const float kEnableAngleCosine = cosf(kDegreesToRadians * 30.f);

// In joystick movement mode, max speed is 5 m/s per dimension
static const float kJoystickModeSpeed = 5;

static const auto kInputDevice = InputManager::kController;

DebugCamera::DebugCamera(Registry* registry) : registry_(registry) {
  auto* dispatcher = registry_->Get<Dispatcher>();

  dispatcher->Connect(this, [this](const SystemButtonLongPress& event) {
    if (ControllerIsPointingUp()) {
      if (!in_debug_mode_) {
        StartDebugMode();
      } else {
        StopDebugMode();
      }
    }
  });

  dispatcher->Connect(this, [this](const PrimaryButtonPress& event) {
    if (in_debug_mode_ && ControllerIsPointingUp()) {
      if (movement_mode_ == MovementMode::kTrackpad) {
        movement_mode_ = MovementMode::kJoystick;
      } else {
        movement_mode_ = MovementMode::kTrackpad;
      }
    }
  });

  dispatcher->Connect(this, [this](const GlobalRecenteredEvent& event) {
    if (in_debug_mode_) {
      ResetDebugPosition();
    }
  });
}

DebugCamera::~DebugCamera() {
  auto* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->DisconnectAll(this);
}

void DebugCamera::StartDebugMode() {
  if (!in_debug_mode_) {
    in_debug_mode_ = true;
    ResetDebugPosition();
    LOG(INFO) << "Enabled debug camera movement";
  }
}

void DebugCamera::StopDebugMode() {
  if (in_debug_mode_) {
    in_debug_mode_ = false;
    ResetDebugPosition();
    LOG(INFO) << "Disabled debug camera movement";
  }
}

bool DebugCamera::ControllerIsPointingUp() const {
  InputManager* input_manager = registry_->Get<InputManager>();
  const mathfu::quat orientation = input_manager->GetDofRotation(kInputDevice);
  const float controller_dot_up = (orientation * -mathfu::kAxisZ3f).y;
  return controller_dot_up > kEnableAngleCosine;
}

void DebugCamera::ResetDebugPosition() {
  debug_position_ = mathfu::kZeros3f;
}

mathfu::vec3 DebugCamera::GetTranslation() const {
  if (!in_debug_mode_) {
    return mathfu::kZeros3f;
  }
  return debug_position_;
}

void DebugCamera::AdvanceFrame(Clock::duration delta_time) {
  if (!in_debug_mode_) {
    return;
  }

  InputManager* input_manager = registry_->Get<InputManager>();
  if (!input_manager->IsConnected(kInputDevice)) {
    return;
  }
  if (!input_manager->IsValidTouch(kInputDevice)) {
    return;
  }

  mathfu::vec2 camera_pad_velocity;

  if (movement_mode_ == MovementMode::kTrackpad) {
    camera_pad_velocity = input_manager->GetTouchVelocity(kInputDevice);
  } else {
    // touch location ranges from 0 to 1, so we need to double and subtract
    // 1 to remap the range to -1 to 1.
    camera_pad_velocity =
        kJoystickModeSpeed *
        (2.f * input_manager->GetTouchLocation(kInputDevice) - 1.f);
  }

  const mathfu::quat controller_orientation =
      input_manager->GetDofRotation(kInputDevice);

  const mathfu::vec3 motion =
      controller_orientation *
      mathfu::vec3(camera_pad_velocity.x, 0, camera_pad_velocity.y);

  const float delta_time_sec = SecondsFromDuration(delta_time);

  debug_position_ += motion * delta_time_sec;
}

}  // namespace lull
