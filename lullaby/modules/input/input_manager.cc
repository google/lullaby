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

#include "lullaby/modules/input/input_manager.h"

#include <algorithm>

#include "lullaby/util/bits.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/time.h"
#include "mathfu/constants.h"
#include "mathfu/utilities.h"

namespace lull {
namespace {
const Clock::duration kDefaultLongPressTime =
    std::chrono::milliseconds(500);

// Clamps the components of the vector between min and max and logs an error if
// they fall outside that range.
mathfu::vec2 ClampVec2(const mathfu::vec2& value, float min, float max) {
  if (value.x < min || value.x > max || value.y < min || value.y > max) {
    LOG(ERROR) << "Input outside valid range [" << min << ", " << max << "]";
    return mathfu::vec2(mathfu::Clamp(value.x, min, max),
                        mathfu::Clamp(value.y, min, max));
  }
  return value;
}
}  // namespace

const InputManager::ButtonState InputManager::kReleased = 0x01 << 0;
const InputManager::ButtonState InputManager::kPressed = 0x01 << 1;
const InputManager::ButtonState InputManager::kLongPressed = 0x01 << 2;
const InputManager::ButtonState InputManager::kJustReleased = 0x01 << 3;
const InputManager::ButtonState InputManager::kJustPressed = 0x01 << 4;
const InputManager::ButtonState InputManager::kJustLongPressed = 0x01 << 5;
const InputManager::ButtonState InputManager::kRepeat = 0x01 << 6;

const InputManager::ButtonId InputManager::kLeftMouse = 0;
const InputManager::ButtonId InputManager::kRightMouse = 1;
const InputManager::ButtonId InputManager::kMiddleMouse = 2;
const InputManager::ButtonId InputManager::kBackMouse = 3;
const InputManager::ButtonId InputManager::kForwardMouse = 4;

const InputManager::ButtonId InputManager::kPrimaryButton = 0;
const InputManager::ButtonId InputManager::kSecondaryButton = 1;
const InputManager::ButtonId InputManager::kRecenterButton = 2;

const char* const InputManager::kKeyBackspace = "\x08";
const char* const InputManager::kKeyReturn = "\x0d";
const Clock::time_point InputManager::kInvalidSampleTime =
    Clock::time_point::min();
const mathfu::vec2 InputManager::kInvalidTouchLocation(-1, -1);

InputManager::DeviceParams::DeviceParams()
    : has_position_dof(false),
      has_rotation_dof(false),
      has_touchpad(false),
      has_touch_gesture(false),
      has_scroll(false),
      num_joysticks(0),
      num_buttons(0),
      num_eyes(0),
      long_press_time(kDefaultLongPressTime) {}

InputManager::InputManager() {}

void InputManager::AdvanceFrame(Clock::duration delta_time) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (Device& device : devices_) {
    // TODO(b/26692955): Update connected state in a thread-safe manner.
    device.Advance(delta_time);
  }
}

void InputManager::ConnectDevice(DeviceType device,
                                 const DeviceParams& params) {
  if (device == kMaxNumDeviceTypes) {
    LOG(DFATAL) << "Invalid device type: " << GetDeviceName(device);
    return;
  }

  // TODO(b/26692955): Update connected state in a thread-safe manner.
  std::unique_lock<std::mutex> lock(mutex_);
  devices_[device].Connect(params);
}

void InputManager::DisconnectDevice(DeviceType device) {
  if (device == kMaxNumDeviceTypes) {
    LOG(DFATAL) << "Invalid device type: " << GetDeviceName(device);
    return;
  }

  // TODO(b/26692955): Update connected state in a thread-safe manner.
  std::unique_lock<std::mutex> lock(mutex_);
  devices_[device].Disconnect();
}

void InputManager::UpdateKey(DeviceType device, const std::string& key,
                             bool repeat) {
  LOG(DFATAL) << "Keyboard support not yet implemented.";
}

void InputManager::KeyPressed(DeviceType device, const std::string& key) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* state = GetDeviceStateForWriteLocked(device);
  if (state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  state->keys.push_back(key);
}

void InputManager::UpdateButton(DeviceType device, ButtonId id, bool pressed,
                                bool repeat) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* state = GetDeviceStateForWriteLocked(device);
  if (state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  if (id < state->buttons.size()) {
    state->buttons[id] = pressed;

    const DataBuffer* buffer = GetDataBuffer(device);
    if (buffer == nullptr) {
      LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
      return;
    }

    // Update the press time if the button was just pressed.
    if (pressed && !buffer->GetCurrent().buttons[id]) {
      state->button_press_times[id] = state->time_stamp;
    }
  } else {
    LOG(DFATAL) << "Invalid button [" << id
                << "] for device: " << GetDeviceName(device);
  }

  if (id < state->repeat.size()) {
    state->repeat[id] = repeat;
  } else {
    LOG(DFATAL) << "Invalid repeat button [" << id
                << "] for device: " << GetDeviceName(device);
  }
}

void InputManager::UpdateJoystick(DeviceType device, JoystickType joystick,
                                  const mathfu::vec2& value) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* state = GetDeviceStateForWriteLocked(device);
  if (state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  if (joystick < state->joystick.size()) {
    state->joystick[joystick] = ClampVec2(value, -1.0f, 1.0f);
  } else {
    LOG(DFATAL) << "Invalid joystick [" << joystick
                << "] for device: " << GetDeviceName(device);
  }
}

void InputManager::UpdateTouch(DeviceType device, const mathfu::vec2& value,
                               bool valid) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* state = GetDeviceStateForWriteLocked(device);
  if (state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  DataBuffer* buffer = GetDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return;
  }

  if (state->touch.size() != 1) {
    LOG(DFATAL) << "Touch not enabled for device: " << GetDeviceName(device);
    return;
  }

  const TouchpadState& prev = buffer->GetCurrent().touch[0];
  TouchpadState& touch = state->touch[0];
  if (valid) {
    touch.position = ClampVec2(value, 0.0f, 1.0f);
    touch.time = Clock::now();
    touch.valid = true;

    if (prev.valid) {
      const float kCutoffHz = 10.0f;
      const float kRc = static_cast<float>(1.0 / (2.0 * M_PI * kCutoffHz));

      const TouchpadState& prev = buffer->GetCurrent().touch[0];
      const float delta_sec = SecondsFromDuration(touch.time - prev.time);
      const mathfu::vec2 instantaneous_velocity =
          (touch.position - prev.position) / delta_sec;

      touch.velocity = mathfu::Lerp(prev.velocity, instantaneous_velocity,
                                    delta_sec / (kRc + delta_sec));
    } else {
      touch.velocity = mathfu::kZeros2f;
      state->touch_press_times[0] = state->time_stamp;
    }
  } else {
    touch.position = kInvalidTouchLocation;
    touch.time = kInvalidSampleTime;
    touch.valid = false;

    if (prev.valid) {
      // If we just ended the touch, keep the velocity around for 1 more
      // frame so we can actually use it.
      touch.velocity = prev.velocity;
    } else {
      touch.velocity = mathfu::kZeros2f;
    }
  }
}

void InputManager::UpdateGesture(DeviceType device, GestureType type,
                                 GestureDirection direction,
                                 const mathfu::vec2& displacement,
                                 const mathfu::vec2& velocity) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* state = GetDeviceStateForWriteLocked(device);
  if (state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  if (state->touch_gesture.size() == 1) {
    TouchGesture& touch_gesture = state->touch_gesture[0];
    touch_gesture.type = type;
    touch_gesture.direction = direction;
    touch_gesture.displacement = displacement;
    touch_gesture.velocity = velocity;
  } else {
    LOG(DFATAL) << "Touch gestures not enabled for device: "
                << GetDeviceName(device);
  }
}

void InputManager::UpdateScroll(DeviceType device, int delta) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* state = GetDeviceStateForWriteLocked(device);
  if (state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  if (state->scroll.size() == 1) {
    state->scroll[0] = delta;
  } else {
    LOG(DFATAL) << "Touch scroll not enabled for device: "
                << GetDeviceName(device);
  }
}

void InputManager::UpdatePosition(DeviceType device,
                                  const mathfu::vec3& value) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* state = GetDeviceStateForWriteLocked(device);
  if (state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  if (state->position.size() == 1) {
    state->position[0] = value;
  } else {
    LOG(DFATAL) << "Position DOF not enabled for device: "
                << GetDeviceName(device);
  }
}

void InputManager::UpdateRotation(DeviceType device,
                                  const mathfu::quat& value) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* state = GetDeviceStateForWriteLocked(device);
  if (state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  if (state->rotation.size() == 1) {
    state->rotation[0] = value;
  } else {
    LOG(DFATAL) << "Rotation DOF not enabled for device: "
                << GetDeviceName(device);
  }
}

void InputManager::UpdateEye(DeviceType device, EyeType eye,
                             const mathfu::mat4& eye_from_head_matrix,
                             const mathfu::rectf& eye_fov,
                             const mathfu::recti& eye_viewport) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* state = GetDeviceStateForWriteLocked(device);
  if (state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  if (eye < state->eye_from_head_matrix.size()) {
    state->eye_from_head_matrix[eye] = eye_from_head_matrix;
  } else {
    LOG(DFATAL) << "Invalid eye matrix [" << eye
                << "] for device: " << GetDeviceName(device);
  }

  if (eye < state->eye_fov.size()) {
    state->eye_fov[eye] = eye_fov;
  } else {
    LOG(DFATAL) << "Invalid eye fov [" << eye
                << "] for device: " << GetDeviceName(device);
  }

  if (eye < state->eye_viewport.size()) {
    state->eye_viewport[eye] = eye_viewport;
  } else {
    LOG(DFATAL) << "Invalid eye viewport [" << eye
                << "] for device: " << GetDeviceName(device);
  }
}

bool InputManager::IsConnected(DeviceType device) const {
  if (device == kMaxNumDeviceTypes) {
    LOG(DFATAL) << "Invalid device type: " << GetDeviceName(device);
    return false;
  }
  return devices_[device].IsConnected();
}

bool InputManager::HasPositionDof(DeviceType device) const {
  const DeviceParams* params = GetDeviceParams(device);
  return params ? params->has_position_dof : false;
}

bool InputManager::HasRotationDof(DeviceType device) const {
  const DeviceParams* params = GetDeviceParams(device);
  return params ? params->has_rotation_dof : false;
}

bool InputManager::HasTouchpad(DeviceType device) const {
  const DeviceParams* params = GetDeviceParams(device);
  return params ? params->has_touchpad : false;
}

bool InputManager::HasJoystick(DeviceType device, JoystickType joystick) const {
  const DeviceParams* params = GetDeviceParams(device);
  return params ? params->num_joysticks > static_cast<size_t>(joystick) : false;
}

bool InputManager::HasScroll(DeviceType device) const {
  const DeviceParams* params = GetDeviceParams(device);
  return params ? params->has_scroll : false;
}

bool InputManager::HasButton(DeviceType device, ButtonId button) const {
  const DeviceParams* params = GetDeviceParams(device);
  return params ? params->num_buttons > static_cast<size_t>(button) : false;
}

size_t InputManager::GetNumButtons(DeviceType device) const {
  const DeviceParams* params = GetDeviceParams(device);
  return params ? params->num_buttons : 0;
}

bool InputManager::HasEye(DeviceType device, EyeType eye) const {
  const DeviceParams* params = GetDeviceParams(device);
  return params ? params->num_eyes > static_cast<size_t>(eye) : false;
}

size_t InputManager::GetNumEyes(DeviceType device) const {
  const DeviceParams* params = GetDeviceParams(device);
  return params ? params->num_eyes : 0;
}

const char* InputManager::GetDeviceName(DeviceType device) {
  switch (device) {
    case kHmd:
      return "HMD";
    case kMouse:
      return "Mouse";
    case kKeyboard:
      return "Keyboard";
    case kController:
      return "Controller";
    case kController2:
      return "Controller2";
    case kHand:
      return "Hand";
    case kMaxNumDeviceTypes:
      break;
  }
  LOG(DFATAL) << "Unknown device.";
  return "";
}

std::vector<std::string> InputManager::GetPressedKeys(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return std::vector<std::string>();
  }
  return buffer->GetCurrent().keys;
}

InputManager::ButtonState InputManager::GetKeyState(
    DeviceType device, const std::string& key) const {
  LOG(DFATAL) << "GetKeyState() not yet implemented.";
  return 0;
}

InputManager::ButtonState InputManager::GetButtonState(DeviceType device,
                                                       ButtonId button) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return kInvalidButtonState;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || params->num_buttons <= static_cast<size_t>(button)) {
    LOG(DFATAL) << "Invalid button [" << button
                << "] for device: " << GetDeviceName(device);
    return kInvalidButtonState;
  }

  const DeviceState& curr_buffer = buffer->GetCurrent();
  const DeviceState& prev_buffer = buffer->GetPrevious();

  const bool curr_press = curr_buffer.buttons[button];
  const bool prev_press = prev_buffer.buttons[button];
  const bool repeat = buffer->GetCurrent().repeat[button];

  return GetButtonState(curr_press, prev_press, repeat, params->long_press_time,
                        curr_buffer.time_stamp, prev_buffer.time_stamp,
                        curr_buffer.button_press_times[button],
                        prev_buffer.button_press_times[button]);
}

Clock::duration InputManager::GetButtonPressedDuration(
    DeviceType device, ButtonId button) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return Clock::duration::zero();
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || params->num_buttons <= static_cast<size_t>(button)) {
    LOG(DFATAL) << "Invalid button [" << button
                << "] for device: " << GetDeviceName(device);
    return Clock::duration::zero();
  }

  const DeviceState& state = buffer->GetCurrent();
  const bool pressed = state.buttons[button];
  if (pressed) {
    // Only return a press time if the button has been pressed for at least one
    // frame, or was just released.
    return state.time_stamp - state.button_press_times[button];
  } else {
    const DeviceState& prev_state = buffer->GetPrevious();
    const bool prev_pressed = prev_state.buttons[button];
    if (prev_pressed) {
      return prev_state.time_stamp - prev_state.button_press_times[button];
    } else {
      return Clock::duration::zero();
    }
  }
}

mathfu::vec2 InputManager::GetJoystickValue(DeviceType device,
                                            JoystickType joystick) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::kZeros2f;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || params->num_joysticks <= static_cast<size_t>(joystick)) {
    LOG(DFATAL) << "Invalid joystick [" << joystick
                << "] for device: " << GetDeviceName(device);
    return mathfu::kZeros2f;
  }

  return buffer->GetCurrent().joystick[joystick];
}

mathfu::vec2 InputManager::GetJoystickDelta(DeviceType device,
                                            JoystickType joystick) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::kZeros2f;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || params->num_joysticks <= static_cast<size_t>(joystick)) {
    LOG(DFATAL) << "Invalid joystick [" << joystick
                << "] for device: " << GetDeviceName(device);
    return mathfu::kZeros2f;
  }

  const mathfu::vec2& curr = buffer->GetCurrent().joystick[joystick];
  const mathfu::vec2& prev = buffer->GetPrevious().joystick[joystick];
  return curr - prev;
}

mathfu::vec2 InputManager::GetTouchLocation(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return kInvalidTouchLocation;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || !params->has_touchpad) {
    LOG(DFATAL) << "Touchpad not setup for device: " << GetDeviceName(device);
    return kInvalidTouchLocation;
  }

  return buffer->GetCurrent().touch[0].position;
}

bool InputManager::IsValidTouch(DeviceType device) const {
  return CheckBit(GetTouchState(device), kPressed);
}

InputManager::TouchState InputManager::GetTouchState(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return false;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || !params->has_touchpad) {
    LOG(DFATAL) << "Touchpad not setup for device: " << GetDeviceName(device);
    return false;
  }

  const DeviceState& curr_buffer = buffer->GetCurrent();
  const DeviceState& prev_buffer = buffer->GetPrevious();

  const bool curr_press = curr_buffer.touch[0].valid;
  const bool prev_press = prev_buffer.touch[0].valid;
  const bool repeat = false;

  return GetButtonState(curr_press, prev_press, repeat, params->long_press_time,
                        curr_buffer.time_stamp, prev_buffer.time_stamp,
                        curr_buffer.touch_press_times[0],
                        prev_buffer.touch_press_times[0]);
}

mathfu::vec2 InputManager::GetTouchDelta(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::kZeros2f;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || !params->has_touchpad) {
    LOG(DFATAL) << "Touchpad not setup for device: " << GetDeviceName(device);
    return mathfu::kZeros2f;
  }

  if (!IsValidTouch(device)) {
    return mathfu::kZeros2f;
  }

  if (params->has_touch_gesture) {
    const TouchGesture* touch_gesture_ptr = GetTouchGesturePtr(device);
    if (!touch_gesture_ptr) {
      LOG(DFATAL) << "Gesture not setup for device: " << GetDeviceName(device);
      return mathfu::kZeros2f;
    }
    return touch_gesture_ptr->displacement;
  }

  if (!buffer->GetPrevious().touch[0].valid) {
    return mathfu::kZeros2f;
  }

  const mathfu::vec2& curr = buffer->GetCurrent().touch[0].position;
  const mathfu::vec2& prev = buffer->GetPrevious().touch[0].position;
  return curr - prev;
}

mathfu::vec2 InputManager::GetTouchVelocity(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::kZeros2f;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || !params->has_touchpad) {
    LOG(DFATAL) << "Touchpad not setup for device: " << GetDeviceName(device);
    return mathfu::kZeros2f;
  }

  if (params->has_touch_gesture) {
    const TouchGesture* touch_gesture_ptr = GetTouchGesturePtr(device);
    if (!touch_gesture_ptr) {
      LOG(DFATAL) << "Gesture not setup for device: " << GetDeviceName(device);
      return mathfu::kZeros2f;
    }
    return touch_gesture_ptr->velocity;
  }

  return buffer->GetCurrent().touch[0].velocity;
}

InputManager::GestureDirection InputManager::GetTouchGestureDirection(
    DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return GestureDirection::kNone;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || !params->has_touchpad) {
    LOG(DFATAL) << "Touchpad not setup for device: " << GetDeviceName(device);
    return GestureDirection::kNone;
  }

  if (params->has_touch_gesture) {
    const TouchGesture* touch_gesture_ptr = GetTouchGesturePtr(device);
    if (!touch_gesture_ptr) {
      LOG(DFATAL) << "Gesture not setup for device: " << GetDeviceName(device);
      return GestureDirection::kNone;
    } else if (touch_gesture_ptr->type == GestureType::kFling) {
      return touch_gesture_ptr->direction;
    } else {
      return GestureDirection::kNone;
    }
  }

  const DeviceState& curr = buffer->GetCurrent();
  const DeviceState& prev = buffer->GetPrevious();
  if (curr.touch[0].valid || !prev.touch[0].valid) {
    return GestureDirection::kNone;
  }

  const float kMinVelocitySqr = .4f * .4f;  // From UX.
  const mathfu::vec2& velocity = curr.touch[0].velocity;
  if (velocity.LengthSquared() < kMinVelocitySqr) {
    return GestureDirection::kNone;
  }

  // Angle is measured clockwise from top of the touchpad, where (0,0) is upper
  // left and (1,1) is lower right:
  float angle = std::atan2(velocity.x, -velocity.y);
  if (angle < 0.0f) {
    angle += 2.0f * static_cast<float>(M_PI);
  }

  if (angle < 45.0f * kDegreesToRadians || angle > 315.0f * kDegreesToRadians) {
    return GestureDirection::kUp;
  }
  if (angle < 135.0f * kDegreesToRadians) {
    return GestureDirection::kRight;
  }
  if (angle < 225.0f * kDegreesToRadians) {
    return GestureDirection::kDown;
  }
  return GestureDirection::kLeft;
}

mathfu::vec3 InputManager::GetDofPosition(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::kZeros3f;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || !params->has_position_dof) {
    LOG(DFATAL) << "Position DOF not setup for device: "
                << GetDeviceName(device);
    return mathfu::kZeros3f;
  }

  return buffer->GetCurrent().position[0];
}

mathfu::vec3 InputManager::GetDofDelta(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::kZeros3f;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || !params->has_position_dof) {
    LOG(DFATAL) << "Position DOF not setup for device: "
                << GetDeviceName(device);
    return mathfu::kZeros3f;
  }

  const mathfu::vec3& curr = buffer->GetCurrent().position[0];
  const mathfu::vec3& prev = buffer->GetPrevious().position[0];
  return curr - prev;
}

mathfu::quat InputManager::GetDofRotation(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::quat();
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || !params->has_rotation_dof) {
    LOG(DFATAL) << "Rotation DOF not setup for device: "
                << GetDeviceName(device);
    return mathfu::quat();
  }

  return buffer->GetCurrent().rotation[0];
}

mathfu::quat InputManager::GetDofAngularDelta(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::quat();
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || !params->has_rotation_dof) {
    LOG(DFATAL) << "Rotation DOF not setup for device: "
                << GetDeviceName(device);
    return mathfu::quat();
  }

  const mathfu::quat& curr = buffer->GetCurrent().rotation[0];
  const mathfu::quat& prev = buffer->GetPrevious().rotation[0];
  return prev.Inverse() * curr;
}

mathfu::mat4 InputManager::GetDofWorldFromObjectMatrix(
    DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::mat4::Identity();
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || (!params->has_rotation_dof && !params->has_position_dof)) {
    LOG(DFATAL) << "WorldFromObjectMatrix not setup for device: "
                << GetDeviceName(device);
    return mathfu::mat4::Identity();
  }

  const mathfu::quat& rot = params->has_rotation_dof
                                ? buffer->GetCurrent().rotation[0]
                                : mathfu::quat::identity;
  const mathfu::vec3& pos = params->has_position_dof
                                ? buffer->GetCurrent().position[0]
                                : mathfu::kZeros3f;

  return CalculateTransformMatrix(pos, rot, mathfu::kOnes3f);
}

int InputManager::GetScrollDelta(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return 0;
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || !params->has_scroll) {
    LOG(DFATAL) << "Scrolling not setup for device: " << GetDeviceName(device);
    return 0;
  }

  return buffer->GetCurrent().scroll[0];
}

mathfu::mat4 InputManager::GetEyeFromHead(DeviceType device,
                                          EyeType eye) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::mat4::Identity();
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || params->num_eyes <= static_cast<size_t>(eye)) {
    LOG(DFATAL) << "Invalid eye [" << eye
                << "] for device: " << GetDeviceName(device);
    return mathfu::mat4::Identity();
  }

  return buffer->GetCurrent().eye_from_head_matrix[eye];
}

mathfu::rectf InputManager::GetEyeFOV(DeviceType device, EyeType eye) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::rectf();
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || params->num_eyes <= static_cast<size_t>(eye)) {
    LOG(DFATAL) << "Invalid eye [" << eye
                << "] for device: " << GetDeviceName(device);
    return mathfu::rectf();
  }

  return buffer->GetCurrent().eye_fov[eye];
}

mathfu::recti InputManager::GetEyeViewport(DeviceType device,
                                           EyeType eye) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::recti();
  }

  const DeviceParams* params = GetDeviceParams(device);
  if (!params || params->num_eyes <= static_cast<size_t>(eye)) {
    LOG(DFATAL) << "Invalid eye [" << eye
                << "] for device: " << GetDeviceName(device);
    return mathfu::recti();
  }

  return buffer->GetCurrent().eye_viewport[eye];
}

InputManager::DataBuffer* InputManager::GetDataBuffer(DeviceType device) {
  return device != kMaxNumDeviceTypes ? devices_[device].GetDataBuffer()
                                      : nullptr;
}

const InputManager::DataBuffer* InputManager::GetDataBuffer(
    DeviceType device) const {
  return device != kMaxNumDeviceTypes ? devices_[device].GetDataBuffer()
                                      : nullptr;
}

const InputManager::DataBuffer* InputManager::GetConnectedDataBuffer(
    DeviceType device) const {
  if (device == kMaxNumDeviceTypes) {
    return nullptr;
  } else if (!IsConnected(device)) {
    return nullptr;
  }
  return devices_[device].GetDataBuffer();
}

const InputManager::TouchGesture* InputManager::GetTouchGesturePtr(
    DeviceType device) const {
  const DataBuffer* buffer = GetDataBuffer(device);
  if (buffer == nullptr) {
    return nullptr;
  }

  if (buffer->GetCurrent().touch_gesture.empty()) {
    return nullptr;
  }

  return &(buffer->GetCurrent().touch_gesture[0]);
}

const InputManager::DeviceParams* InputManager::GetDeviceParams(
    DeviceType device) const {
  return device != kMaxNumDeviceTypes ? &devices_[device].GetDeviceParams()
                                      : nullptr;
}

InputManager::DeviceParams InputManager::GetDeviceParamsCopy(
    DeviceType device) const {
  const DeviceParams* params = GetDeviceParams(device);
  return params ? *params : DeviceParams();
}

InputManager::DeviceState* InputManager::GetDeviceStateForWriteLocked(
    DeviceType device) {
  if (device == kMaxNumDeviceTypes) {
    return nullptr;
  }

  DataBuffer* buffer = devices_[device].GetDataBuffer();
  if (buffer == nullptr) {
    return nullptr;
  }
  return &buffer->GetMutable();
}

InputManager::ButtonState InputManager::GetButtonState(
    bool curr, bool prev, bool repeat, Clock::duration long_press_time,
    Clock::time_point curr_time_stamp,
    Clock::time_point prev_time_stamp,
    Clock::time_point curr_press_time,
    Clock::time_point prev_press_time) {
  ButtonState state = 0;
  if (curr) {
    state |= kPressed;
    if (!prev) {
      state |= kJustPressed;
    }
    if (repeat) {
      state |= kRepeat;
    }

    // Check for long press:
    const Clock::duration curr_press_duration =
        curr_time_stamp - curr_press_time;
    const bool curr_long_press = curr_press_duration >= long_press_time;
    if (curr_long_press) {
      state |= kLongPressed;
      const Clock::duration prev_press_duration =
          prev_time_stamp - prev_press_time;
      const bool prev_long_press = prev_press_duration >= long_press_time;
      if (!prev_long_press) {
        state |= kJustLongPressed;
      }
    }
  } else {
    state |= kReleased;
    if (prev) {
      state |= kJustReleased;
      // Check if the released press was held for more than long_press_time.
      // If released on the first frame that would be a long_press, assume it
      // was released before the time limit passed and don't set kLongPressed.
      const Clock::duration prev_press_duration =
          prev_time_stamp - prev_press_time;
      const bool prev_long_press = prev_press_duration >= long_press_time;
      if (prev_long_press) {
        state |= kLongPressed;
      }
    }
  }

  return state;
}

InputManager::Device::Device() : connected_(false) {}

void InputManager::Device::Connect(const DeviceParams& params) {
  DCHECK(!connected_) << "Device is already connected.";
  connected_ = true;
  params_ = params;

  DeviceState state;
  // state.keys;
  state.scroll.resize(params.has_scroll ? 1 : 0, 0);
  state.buttons.resize(params.num_buttons, false);
  state.repeat.resize(params.num_buttons, false);
  state.button_press_times.resize(params.num_buttons,
                                  Clock::time_point());
  state.joystick.resize(params.num_joysticks, mathfu::kZeros2f);
  state.touch.resize(params.has_touchpad ? 1 : 0, TouchpadState());
  state.touch_press_times.resize(params.has_touchpad ? 1 : 0,
                                 Clock::time_point());
  state.touch_gesture.resize(params.has_touch_gesture ? 1 : 0, TouchGesture());
  state.position.resize(params.has_position_dof ? 1 : 0, mathfu::kZeros3f);
  state.rotation.resize(params.has_rotation_dof ? 1 : 0,
                        mathfu::quat::identity);
  state.eye_from_head_matrix.resize(params.num_eyes, mathfu::mat4::Identity());
  state.eye_viewport.resize(params.num_eyes);
  state.eye_fov.resize(params.num_eyes);
  buffer_.reset(new DataBuffer(state));
}

void InputManager::Device::Disconnect() {
  DCHECK(connected_) << "Device is not connected.";
  params_ = DeviceParams();
  connected_ = false;
  buffer_.reset();
}

void InputManager::Device::Advance(Clock::duration delta_time) {
  if (buffer_) {
    buffer_->Advance(delta_time);
  }
}

InputManager::DataBuffer::DataBuffer(
    const InputManager::DeviceState& reference_state) {
  curr_index_ = 0;
  for (DeviceState& state : buffer_) {
    state = reference_state;
  }
}

void InputManager::DataBuffer::Advance(Clock::duration delta_time) {
  DeviceState& prev = buffer_[curr_index_];
  prev.time_stamp += delta_time;
  curr_index_ = (curr_index_ + kBufferSize - 1) % kBufferSize;
  DeviceState& state = buffer_[curr_index_];
  state = prev;
  state.keys.clear();
}

InputManager::DeviceState& InputManager::DataBuffer::GetMutable() {
  return buffer_[curr_index_];
}

const InputManager::DeviceState& InputManager::DataBuffer::GetCurrent() const {
  const int index = (curr_index_ + 1) % kBufferSize;
  return buffer_[index];
}

const InputManager::DeviceState& InputManager::DataBuffer::GetPrevious() const {
  const int index = (curr_index_ + 2) % kBufferSize;
  return buffer_[index];
}

}  // namespace lull
