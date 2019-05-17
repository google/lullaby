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

#include "lullaby/modules/input/input_manager.h"

#include <algorithm>

#include "lullaby/util/bits.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/time.h"
#include "mathfu/constants.h"
#include "mathfu/utilities.h"

#include "mathfu/io.h"

namespace lull {
namespace {
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
const InputManager::ButtonId InputManager::kInvalidButton =
    std::numeric_limits<InputManager::ButtonId>::max();

const char* const InputManager::kKeyBackspace = "\x08";
const char* const InputManager::kKeyReturn = "\x0d";
const Clock::time_point InputManager::kInvalidSampleTime =
    Clock::time_point::min();
const mathfu::vec2 InputManager::kInvalidTouchLocation(-1, -1);

const InputManager::TouchpadId InputManager::kPrimaryTouchpadId = 0;
const InputManager::TouchId InputManager::kPrimaryTouchId =
    std::numeric_limits<InputManager::TouchId>::max();

const uint8_t InputManager::kInvalidBatteryCharge = 255;

InputManager::DeviceParams::DeviceParams()
    : has_position_dof(false),
      is_position_fake(false),
      has_rotation_dof(false),
      has_touchpad(false),
      has_touch_gesture(false),
      has_scroll(false),
      has_battery(false),
      num_joysticks(0),
      num_buttons(0),
      num_eyes(0),
      long_press_time(kDefaultLongPressTime) {}

InputManager::InputManager() {}

void InputManager::AdvanceFrame(Clock::duration delta_time) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (Device& device : devices_) {
    // TODO: Update connected state in a thread-safe manner.
    device.Advance(delta_time);
  }
}

void InputManager::ConnectDevice(DeviceType device,
                                 const DeviceProfile& profile) {
  if (device == kMaxNumDeviceTypes) {
    LOG(DFATAL) << "Invalid device type: " << GetDeviceName(device);
    return;
  }

  // TODO: Update connected state in a thread-safe manner.
  std::unique_lock<std::mutex> lock(mutex_);
  devices_[device].Connect(profile);
}

void InputManager::ConnectDevice(DeviceType device,
                                 const DeviceParams& params) {
  if (device == kMaxNumDeviceTypes) {
    LOG(DFATAL) << "Invalid device type: " << GetDeviceName(device);
    return;
  }

  // Translate from deprecated InputManager::DeviceParams to new
  // DeviceProfile.
  DeviceProfile profile;
  profile.rotation_dof = params.has_rotation_dof
                             ? DeviceProfile::kRealDof
                             : DeviceProfile::kUnavailableDof;
  if (params.has_position_dof) {
    profile.position_dof = params.is_position_fake ? DeviceProfile::kFakeDof
                                                   : DeviceProfile::kRealDof;
  } else {
    profile.position_dof = DeviceProfile::kUnavailableDof;
  }

  if (params.has_touchpad) {
    profile.touchpads.resize(1);
    profile.touchpads[0].has_gestures = params.has_touch_gesture;
  }
  if (params.has_scroll) {
    profile.scroll_wheels.resize(1);
  }
  if (params.has_battery) {
    profile.battery.emplace();
  }
  profile.joysticks.resize(params.num_joysticks);
  profile.buttons.resize(params.num_buttons);
  profile.eyes.resize(params.num_eyes);
  profile.long_press_time = params.long_press_time;

  ConnectDevice(device, profile);
}

void InputManager::DisconnectDevice(DeviceType device) {
  if (device == kMaxNumDeviceTypes) {
    LOG(DFATAL) << "Invalid device type: " << GetDeviceName(device);
    return;
  }

  // TODO: Update connected state in a thread-safe manner.
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
  UpdateTouch(device, kPrimaryTouchpadId, 0, value, valid);
}

void InputManager::UpdateTouch(DeviceType device, TouchpadId touchpad_id,
                               TouchId touch_id, const mathfu::vec2& value,
                               bool valid) {
  const Touch kDefaultTouch;

  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* device_state = GetDeviceStateForWriteLocked(device);
  if (device_state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  DataBuffer* buffer = GetDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return;
  }

  if (touchpad_id >= device_state->touchpads.size()) {
    LOG(DFATAL) << "Invalid touchpad id for device: " << GetDeviceName(device);
    return;
  }

  TouchpadState& new_state = device_state->touchpads[touchpad_id];
  const TouchpadState& old_state = buffer->GetCurrent().touchpads[touchpad_id];

  if (valid) {
    auto new_pair = new_state.touches.find(touch_id);
    if (new_pair == new_state.touches.end()) {
      // New touch
      new_state.current_touches.push_back(touch_id);
      new_state.touches.emplace(touch_id, Touch{});
      new_state.touches[touch_id].gesture_origin = ClampVec2(value, 0.0f, 1.0f);
    } else if (!new_pair->second.valid) {
      // Resuming a touch that was released.
      new_state.current_touches.push_back(touch_id);
    }
    if (new_state.current_touches.size() == 1) {
      // Only active touch, set as primary.
      new_state.primary_touch = touch_id;
    }
  }

  const auto old_pair = old_state.touches.find(touch_id);
  const Touch& prev =
      old_pair != old_state.touches.end() ? old_pair->second : kDefaultTouch;

  if (valid) {
    Touch& touch = new_state.touches[touch_id];
    touch.position = ClampVec2(value, 0.0f, 1.0f);
    touch.time = Clock::now();
    touch.valid = true;

    if (prev.valid) {
      const float kCutoffHz = 10.0f;
      const float kRc = static_cast<float>(1.0 / (2.0 * M_PI * kCutoffHz));

      const float delta_sec = SecondsFromDuration(touch.time - prev.time);
      const mathfu::vec2 instantaneous_velocity =
          (touch.position - prev.position) / delta_sec;

      touch.velocity = mathfu::Lerp(prev.velocity, instantaneous_velocity,
                                    delta_sec / (kRc + delta_sec));
    } else {
      touch.velocity = mathfu::kZeros2f;
      touch.press_time = device_state->time_stamp;
    }
  } else if (prev.valid) {
    // If we just ended the touch, keep the velocity around for 1 more
    // frame so we can actually use it.
    Touch& touch = new_state.touches[touch_id];
    touch.position = kInvalidTouchLocation;
    touch.time = kInvalidSampleTime;
    touch.valid = false;
    touch.velocity = prev.velocity;

    auto& cur_touches = new_state.current_touches;
    cur_touches.erase(
        std::remove(cur_touches.begin(), cur_touches.end(), touch_id),
        cur_touches.end());
    if (new_state.primary_touch == touch_id && !cur_touches.empty()) {
      new_state.primary_touch = cur_touches[0];
    }
  }
}

void InputManager::ResetTouchGestureOrigin(DeviceType device,
                                           TouchpadId touchpad_id,
                                           TouchId touch_id) {
  const Touch kDefaultTouch;
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* device_state = GetDeviceStateForWriteLocked(device);
  if (device_state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  DataBuffer* buffer = GetDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return;
  }

  if (touchpad_id >= device_state->touchpads.size()) {
    LOG(DFATAL) << "Invalid touchpad id for device: " << GetDeviceName(device);
    return;
  }

  TouchpadState& new_state = device_state->touchpads[touchpad_id];

  const TouchpadState& old_state = buffer->GetCurrent().touchpads[touchpad_id];
  const auto old_pair = old_state.touches.find(touch_id);
  const Touch& prev =
      old_pair != old_state.touches.end() ? old_pair->second : kDefaultTouch;
  if (!prev.valid) {
    return;
  }

  auto new_pair = new_state.touches.find(touch_id);
  if (new_pair == new_state.touches.end()) {
    return;
  }

  new_pair->second.gesture_origin = prev.position;
}

void InputManager::UpdateGesture(DeviceType device, TouchpadId touchpad_id,
                                 GestureType type, GestureDirection direction,
                                 const mathfu::vec2& displacement,
                                 const mathfu::vec2& velocity) {
  std::unique_lock<std::mutex> lock(mutex_);
  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || touchpad_id >= profile->touchpads.size() ||
      !profile->touchpads[touchpad_id].has_gestures) {
    LOG(DFATAL) << "Touch gestures not enabled for device: "
                << GetDeviceName(device);
    return;
  }

  DeviceState* state = GetDeviceStateForWriteLocked(device);
  if (state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  if (touchpad_id >= state->touchpads.size()) {
    LOG(DFATAL) << "Invalid touchpad id for device: " << GetDeviceName(device);
    return;
  }

  TouchpadState& touchpad = state->touchpads[touchpad_id];

  TouchGesture& gesture = touchpad.gesture;
  gesture.type = type;
  gesture.direction = direction;
  gesture.displacement = displacement;
  gesture.velocity = velocity;
  if (type == GestureType::kScrollStart) {
    gesture.initial_displacement_axis =
        std::abs(displacement[0]) > std::abs(displacement[1])
            ? mathfu::kAxisX2f
            : mathfu::kAxisY2f;
  } else if (type == GestureType::kScrollEnd || type == GestureType::kFling) {
    gesture.initial_displacement_axis = mathfu::kZeros2f;
  }
}

void InputManager::UpdateTouchpadSize(DeviceType device, TouchpadId touchpad_id,
                               const mathfu::vec2& size_cm) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* device_state = GetDeviceStateForWriteLocked(device);
  if (device_state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  DataBuffer* buffer = GetDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return;
  }

  if (touchpad_id >= device_state->touchpads.size()) {
    LOG(DFATAL) << "Invalid touchpad id for device: " << GetDeviceName(device);
    return;
  }

  TouchpadState& new_state = device_state->touchpads[touchpad_id];

  new_state.size_cm = size_cm;
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
                             const mathfu::mat4& screen_from_eye_matrix,
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

  if (eye < state->screen_from_eye_matrix.size()) {
    state->screen_from_eye_matrix[eye] = screen_from_eye_matrix;
  } else {
    LOG(DFATAL) << "Invalid screen from eye matrix [" << eye
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

void InputManager::UpdateBattery(DeviceType device, BatteryState state,
                                 uint8_t charge) {
  std::unique_lock<std::mutex> lock(mutex_);
  DeviceState* device_state = GetDeviceStateForWriteLocked(device);
  if (device_state == nullptr) {
    LOG(DFATAL) << "No state for device: " << GetDeviceName(device);
    return;
  }

  if (device_state->battery_charge.size() == 1 &&
      device_state->battery_state.size() == 1) {
    device_state->battery_charge[0] = charge;
    device_state->battery_state[0] = state;
  } else {
    LOG(DFATAL) << "Battery not enabled for device: " << GetDeviceName(device);
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
  const DeviceProfile* profile = GetDeviceProfile(device);
  return profile ? profile->position_dof != DeviceProfile::kUnavailableDof
                 : false;
}

bool InputManager::HasRotationDof(DeviceType device) const {
  const DeviceProfile* profile = GetDeviceProfile(device);
  return profile ? profile->rotation_dof != DeviceProfile::kUnavailableDof
                 : false;
}

bool InputManager::HasTouchpad(DeviceType device,
                               TouchpadId touchpad_id) const {
  const DeviceProfile* profile = GetDeviceProfile(device);
  return profile ? touchpad_id < profile->touchpads.size() : false;
}

bool InputManager::HasJoystick(DeviceType device, JoystickType joystick) const {
  const DeviceProfile* profile = GetDeviceProfile(device);
  return profile ? profile->joysticks.size() > static_cast<size_t>(joystick)
                 : false;
}

bool InputManager::HasScroll(DeviceType device) const {
  const DeviceProfile* profile = GetDeviceProfile(device);
  return profile ? !profile->scroll_wheels.empty() : false;
}

bool InputManager::HasButton(DeviceType device, ButtonId button) const {
  const DeviceProfile* profile = GetDeviceProfile(device);
  return profile ? profile->buttons.size() > static_cast<size_t>(button)
                 : false;
}

size_t InputManager::GetNumButtons(DeviceType device) const {
  const DeviceProfile* profile = GetDeviceProfile(device);
  return profile ? profile->buttons.size() : 0;
}

bool InputManager::HasEye(DeviceType device, EyeType eye) const {
  const DeviceProfile* profile = GetDeviceProfile(device);
  return profile ? profile->eyes.size() > static_cast<size_t>(eye) : false;
}

size_t InputManager::GetNumEyes(DeviceType device) const {
  const DeviceProfile* profile = GetDeviceProfile(device);
  return profile ? profile->eyes.size() : 0;
}

bool InputManager::HasBattery(DeviceType device) const {
  const DeviceProfile* profile = GetDeviceProfile(device);
  return profile ? (bool)profile->battery : false;
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

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->buttons.size() <= static_cast<size_t>(button)) {
    LOG(DFATAL) << "Invalid button [" << button
                << "] for device: " << GetDeviceName(device);
    return kInvalidButtonState;
  }

  const DeviceState& curr_buffer = buffer->GetCurrent();
  const DeviceState& prev_buffer = buffer->GetPrevious();

  const bool curr_press = curr_buffer.buttons[button];
  const bool prev_press = prev_buffer.buttons[button];
  const bool repeat = buffer->GetCurrent().repeat[button];

  return GetButtonState(curr_press, prev_press, repeat,
                        profile->long_press_time, curr_buffer.time_stamp,
                        prev_buffer.time_stamp,
                        curr_buffer.button_press_times[button],
                        prev_buffer.button_press_times[button]);
}

Clock::duration InputManager::GetButtonPressedDuration(DeviceType device,
                                                       ButtonId button) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return Clock::duration::zero();
  }

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->buttons.size() <= static_cast<size_t>(button)) {
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

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->joysticks.size() <= static_cast<size_t>(joystick)) {
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

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->joysticks.size() <= static_cast<size_t>(joystick)) {
    LOG(DFATAL) << "Invalid joystick [" << joystick
                << "] for device: " << GetDeviceName(device);
    return mathfu::kZeros2f;
  }

  const mathfu::vec2& curr = buffer->GetCurrent().joystick[joystick];
  const mathfu::vec2& prev = buffer->GetPrevious().joystick[joystick];
  return curr - prev;
}

mathfu::vec2 InputManager::GetTouchLocation(DeviceType device,
                                            TouchpadId touchpad_id,
                                            TouchId touch_id) const {
  const Touch* touch = GetTouchPtr(device, touchpad_id, touch_id);
  return touch == nullptr ? kInvalidTouchLocation : touch->position;
}

mathfu::vec2 InputManager::GetPreviousTouchLocation(DeviceType device,
                                                    TouchpadId touchpad_id,
                                                    TouchId touch_id) const {
  const Touch* touch = GetPreviousTouchPtr(device, touchpad_id, touch_id);

  if (!touch || !touch->valid) {
    return kInvalidTouchLocation;
  }

  return touch->position;
}

mathfu::vec2 InputManager::GetTouchGestureOrigin(DeviceType device,
                                                 TouchpadId touchpad_id,
                                                 TouchId touch_id) const {
  const Touch* touch = GetTouchPtr(device, touchpad_id, touch_id);
  return touch == nullptr ? kInvalidTouchLocation : touch->gesture_origin;
}

std::vector<InputManager::TouchId> InputManager::GetTouches(
    DeviceType device, TouchpadId touchpad_id) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  std::vector<TouchId> result;
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return result;
  }

  const DeviceState& state = buffer->GetCurrent();
  if (touchpad_id >= state.touchpads.size()) {
    LOG(DFATAL) << "Invalid touchpad id for device: " << GetDeviceName(device);
    return result;
  }

  return state.touchpads[touchpad_id].current_touches;
}

bool InputManager::IsValidTouch(DeviceType device, TouchpadId touchpad_id,
                                TouchId touch_id) const {
  return CheckBit(GetTouchState(device, touchpad_id, touch_id), kPressed);
}

bool InputManager::IsTouchGestureAvailable(DeviceType device,
                                           TouchpadId touchpad_id) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(INFO) << "Invalid buffer for device: " << GetDeviceName(device);
    return false;
  }

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || touchpad_id >= profile->touchpads.size() ||
      !profile->touchpads[touchpad_id].has_gestures) {
    LOG(INFO) << "Gesture not setup for device: " << GetDeviceName(device);
    return false;
  }

  return true;
}

InputManager::TouchState InputManager::GetTouchState(DeviceType device,
                                                     TouchpadId touchpad_id,
                                                     TouchId touch_id) const {
  const Touch* curr_touch = GetTouchPtr(device, touchpad_id, touch_id);
  const Touch* prev_touch = GetPreviousTouchPtr(device, touchpad_id, touch_id);
  const DeviceProfile* profile = GetDeviceProfile(device);

  if (curr_touch == nullptr && prev_touch == nullptr) {
    return kReleased;
  }

  bool curr_press = curr_touch ? curr_touch->valid : false;
  bool prev_press = prev_touch ? prev_touch->valid : false;
  Clock::time_point curr_press_time =
      curr_touch ? curr_touch->press_time : Clock::time_point();
  Clock::time_point prev_press_time =
      prev_touch ? prev_touch->press_time : Clock::time_point();
  const bool repeat = false;

  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  const DeviceState& curr_buffer = buffer->GetCurrent();
  const DeviceState& prev_buffer = buffer->GetPrevious();
  const Clock::duration long_press_time =
      profile ? profile->long_press_time : kDefaultLongPressTime;
  return GetButtonState(curr_press, prev_press, repeat, long_press_time,
                        curr_buffer.time_stamp, prev_buffer.time_stamp,
                        curr_press_time, prev_press_time);
}

mathfu::vec2 InputManager::GetTouchDelta(DeviceType device,
                                         TouchpadId touchpad_id,
                                         TouchId touch_id) const {
  const Touch* curr_touch = GetTouchPtr(device, touchpad_id, touch_id);
  const Touch* prev_touch = GetPreviousTouchPtr(device, touchpad_id, touch_id);

  if (touch_id == kPrimaryTouchId) {
    const TouchGesture* gesture = GetTouchGesturePtr(device, touchpad_id);
    if (gesture) {
      return gesture->displacement;
    }
  }

  if (curr_touch == nullptr || prev_touch == nullptr) {
    return mathfu::kZeros2f;
  }

  if (!curr_touch->valid) {
    return mathfu::kZeros2f;
  }

  const mathfu::vec2& curr = curr_touch->position;
  const mathfu::vec2& prev = prev_touch->position;
  return curr - prev;
}

mathfu::vec2 InputManager::GetLockedTouchDelta(DeviceType device,
                                               TouchpadId touchpad_id,
                                               TouchId touch_id) const {
  return GetTouchDelta(device, touchpad_id, touch_id) *
         GetInitialDisplacementAxis(device, touchpad_id);
}

mathfu::vec2 InputManager::GetTouchVelocity(DeviceType device,
                                            TouchpadId touchpad_id,
                                            TouchId touch_id) const {
  const Touch* curr_touch = GetTouchPtr(device, touchpad_id, touch_id);

  if (touch_id == kPrimaryTouchId) {
    const TouchGesture* gesture = GetTouchGesturePtr(device, touchpad_id);
    if (gesture) {
      return gesture->velocity;
    }
  }

  if (curr_touch == nullptr) {
    return mathfu::kZeros2f;
  }

  return curr_touch->velocity;
}

InputManager::GestureType InputManager::GetTouchGestureType(
    DeviceType device, TouchpadId touchpad_id) const {
  const TouchGesture* gesture = GetTouchGesturePtr(device, touchpad_id);
  if (gesture) {
    return gesture->type;
  }

  LOG(DFATAL) << "Gesture not setup for device: " << GetDeviceName(device);
  return GestureType::kNone;
}

InputManager::GestureDirection InputManager::GetTouchGestureDirection(
    DeviceType device, TouchpadId touchpad_id) const {
  const TouchGesture* gesture = GetTouchGesturePtr(device, touchpad_id);
  if (gesture) {
    if (gesture->type == GestureType::kFling) {
      return gesture->direction;
    } else {
      return GestureDirection::kNone;
    }
  }

  const Touch* curr_touch = GetTouchPtr(device, touchpad_id, kPrimaryTouchId);
  const Touch* prev_touch =
      GetPreviousTouchPtr(device, touchpad_id, kPrimaryTouchId);
  if (curr_touch == nullptr || prev_touch == nullptr) {
    return GestureDirection::kNone;
  }

  if (curr_touch->valid || !prev_touch->valid) {
    return GestureDirection::kNone;
  }

  const float kMinVelocitySqr = .4f * .4f;  // From UX.
  const mathfu::vec2& velocity = curr_touch->velocity;
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

mathfu::vec2 InputManager::GetInitialDisplacementAxis(
    DeviceType device, TouchpadId touchpad_id) const {
  if (!IsTouchGestureAvailable(device, touchpad_id)) {
    LOG(DFATAL) << "Gesture not setup for device: " << GetDeviceName(device);
    return mathfu::kZeros2f;
  }

  const TouchGesture* gesture = GetTouchGesturePtr(device, touchpad_id);
  return gesture->initial_displacement_axis;
}

Optional<mathfu::vec2> InputManager::GetTouchpadSize(
    DeviceType device, TouchpadId touchpad_id) const {

  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return NullOpt;
  }

  const DeviceState& state = buffer->GetCurrent();
  if (touchpad_id >= state.touchpads.size()) {
    LOG(DFATAL) << "Invalid touchpad id for device: " << GetDeviceName(device);
    return NullOpt;
  }

  const TouchpadState& touchpad = state.touchpads[touchpad_id];
  if (touchpad.size_cm.x < 0) {
    LOG(DFATAL) << "Touchpad Size has not been set: " << GetDeviceName(device);
    // Size Not Set
    return NullOpt;
  }

  return touchpad.size_cm;
}

mathfu::vec3 InputManager::GetDofPosition(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::kZeros3f;
  }

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->position_dof == DeviceProfile::kUnavailableDof) {
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

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->position_dof == DeviceProfile::kUnavailableDof) {
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

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->rotation_dof == DeviceProfile::kUnavailableDof) {
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

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->rotation_dof == DeviceProfile::kUnavailableDof) {
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

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || (profile->rotation_dof == DeviceProfile::kUnavailableDof &&
                   profile->position_dof == DeviceProfile::kUnavailableDof)) {
    LOG(DFATAL) << "WorldFromObjectMatrix not setup for device: "
                << GetDeviceName(device);
    return mathfu::mat4::Identity();
  }

  const mathfu::quat& rot =
      profile->rotation_dof != DeviceProfile::kUnavailableDof
          ? buffer->GetCurrent().rotation[0]
          : mathfu::quat::identity;
  const mathfu::vec3& pos =
      profile->position_dof != DeviceProfile::kUnavailableDof
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

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->scroll_wheels.empty()) {
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

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->eyes.size() <= static_cast<size_t>(eye)) {
    LOG(DFATAL) << "Invalid eye [" << eye
                << "] for device: " << GetDeviceName(device);
    return mathfu::mat4::Identity();
  }

  return buffer->GetCurrent().eye_from_head_matrix[eye];
}

mathfu::mat4 InputManager::GetScreenFromEye(DeviceType device,
                                            EyeType eye) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::mat4::Identity();
  }

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->eyes.size() <= static_cast<size_t>(eye)) {
    LOG(DFATAL) << "Invalid eye [" << eye
                << "] for device: " << GetDeviceName(device);
    return mathfu::mat4::Identity();
  }

  return buffer->GetCurrent().screen_from_eye_matrix[eye];
}

mathfu::rectf InputManager::GetEyeFOV(DeviceType device, EyeType eye) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return mathfu::rectf();
  }

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->eyes.size() <= static_cast<size_t>(eye)) {
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

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || profile->eyes.size() <= static_cast<size_t>(eye)) {
    LOG(DFATAL) << "Invalid eye [" << eye
                << "] for device: " << GetDeviceName(device);
    return mathfu::recti();
  }

  return buffer->GetCurrent().eye_viewport[eye];
}

uint8_t InputManager::GetBatteryCharge(DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return kInvalidBatteryCharge;
  }

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || !profile->battery) {
    LOG(DFATAL) << "Battery not supported for device: "
                << GetDeviceName(device);
    return kInvalidBatteryCharge;
  }

  return buffer->GetCurrent().battery_charge[0];
}

InputManager::BatteryState InputManager::GetBatteryState(
    DeviceType device) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return BatteryState::kUnknown;
  }

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile || !profile->battery) {
    LOG(DFATAL) << "Battery not supported for device: "
                << GetDeviceName(device);
    return BatteryState::kUnknown;
  }

  return buffer->GetCurrent().battery_state[0];
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
    DeviceType device, TouchpadId touchpad_id) const {
  const DataBuffer* buffer = GetDataBuffer(device);
  if (buffer == nullptr) {
    return nullptr;
  }

  const DeviceState& state = buffer->GetCurrent();
  if (touchpad_id >= state.touchpads.size()) {
    LOG(DFATAL) << "Invalid touchpad id for device: " << GetDeviceName(device);
    return nullptr;
  }

  const DeviceProfile* profile = GetDeviceProfile(device);
  if (!profile->touchpads[touchpad_id].has_gestures) {
    return nullptr;
  }

  return &state.touchpads[touchpad_id].gesture;
}

const InputManager::Touch* InputManager::GetTouchPtr(DeviceType device,
                                                     TouchpadId touchpad_id,
                                                     TouchId touch_id) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return nullptr;
  }

  const DeviceState& state = buffer->GetCurrent();
  if (touchpad_id >= state.touchpads.size()) {
    LOG(DFATAL) << "Invalid touchpad id for device: " << GetDeviceName(device);
    return nullptr;
  }

  const TouchpadState& touchpad = state.touchpads[touchpad_id];

  if (touch_id == kPrimaryTouchId) {
    if (touchpad.primary_touch == kPrimaryTouchId) {
      // No current touch.
      return nullptr;
    }
    touch_id = touchpad.primary_touch;
  }

  const auto iter = touchpad.touches.find(touch_id);
  if (iter == touchpad.touches.end()) {
    // Touch has been released or never existed.  Reasonable to happen at run
    // time for legacy apps, so no log.
    return nullptr;
  }

  return &iter->second;
}

const InputManager::Touch* InputManager::GetPreviousTouchPtr(
    DeviceType device, TouchpadId touchpad_id, TouchId touch_id) const {
  const DataBuffer* buffer = GetConnectedDataBuffer(device);
  if (buffer == nullptr) {
    LOG(DFATAL) << "Invalid buffer for device: " << GetDeviceName(device);
    return nullptr;
  }

  const DeviceState& state = buffer->GetPrevious();
  if (touchpad_id >= state.touchpads.size()) {
    LOG(DFATAL) << "Invalid touchpad id for device: " << GetDeviceName(device);
    return nullptr;
  }

  const TouchpadState& touchpad = state.touchpads[touchpad_id];
  if (touch_id == kPrimaryTouchId) {
    if (touchpad.primary_touch == kPrimaryTouchId) {
      // No current touch.
      return nullptr;
    }
    touch_id = touchpad.primary_touch;
  }

  const auto iter = touchpad.touches.find(touch_id);
  if (iter == touchpad.touches.end()) {
    // Touch has been released or never existed.  Reasonable to happen at run
    // time for legacy apps, so no log.
    return nullptr;
  }

  return &iter->second;
}

const DeviceProfile* InputManager::GetDeviceProfile(DeviceType device) const {
  return device != kMaxNumDeviceTypes ? &devices_[device].GetDeviceProfile()
                                      : nullptr;
}

Variant InputManager::GetDeviceInfo(DeviceType device, HashValue key) const {
  if (device != kMaxNumDeviceTypes) {
    const VariantMap& info = devices_[device].GetDeviceInfo();
    auto iter = info.find(key);
    if (iter != info.end()) {
      return iter->second;
    }
  }
  return Variant();
}

void InputManager::SetDeviceInfo(DeviceType device, HashValue key,
                                 const Variant& value) {
  if (device != kMaxNumDeviceTypes) {
    devices_[device].GetDeviceInfo().emplace(key, value);
  }
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
    Clock::time_point curr_time_stamp, Clock::time_point prev_time_stamp,
    Clock::time_point curr_press_time, Clock::time_point prev_press_time) {
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
      if (prev) {
        const Clock::duration prev_press_duration =
            prev_time_stamp - prev_press_time;
        const bool prev_long_press = prev_press_duration >= long_press_time;
        if (!prev_long_press) {
          state |= kJustLongPressed;
        }
      } else {
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

void InputManager::Device::Connect(const DeviceProfile& profile) {
  DCHECK(!connected_) << "Device is already connected.";
  connected_ = true;
  profile_ = profile;

  DeviceState state;
  // state.keys;
  state.scroll.resize(profile.scroll_wheels.size(), 0);
  state.buttons.resize(profile.buttons.size(), false);
  state.repeat.resize(profile.buttons.size(), false);
  state.button_press_times.resize(profile.buttons.size(), Clock::time_point());
  state.joystick.resize(profile.joysticks.size(), mathfu::kZeros2f);
  state.touchpads.resize(profile.touchpads.size(), TouchpadState());
  bool has_pos = profile.position_dof != DeviceProfile::kUnavailableDof;
  state.position.resize(has_pos ? 1 : 0, mathfu::kZeros3f);
  bool has_rot = profile.rotation_dof != DeviceProfile::kUnavailableDof;
  state.rotation.resize(has_rot ? 1 : 0, mathfu::quat::identity);
  state.eye_from_head_matrix.resize(profile.eyes.size(),
                                    mathfu::mat4::Identity());
  state.screen_from_eye_matrix.resize(profile.eyes.size(),
                                      mathfu::mat4::Identity());
  state.eye_viewport.resize(profile.eyes.size());
  state.eye_fov.resize(profile.eyes.size());
  state.battery_state.resize(profile.battery ? 1 : 0, BatteryState::kUnknown);
  state.battery_charge.resize(profile.battery ? 1 : 0, kInvalidBatteryCharge);
  buffer_.reset(new DataBuffer(state));
  info_.clear();
}

void InputManager::Device::Disconnect() {
  DCHECK(connected_) << "Device is not connected.";
  profile_ = DeviceProfile();
  connected_ = false;
  buffer_.reset();
  info_.clear();
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
  // The state that will be readable this frame.
  DeviceState& readable_state = buffer_[curr_index_];
  readable_state.time_stamp += delta_time;

  RemoveInactiveTouches();

  curr_index_ = (curr_index_ + kBufferSize - 1) % kBufferSize;
  // The state that will be writable this frame.
  DeviceState& writable_state = buffer_[curr_index_];
  writable_state = readable_state;
  writable_state.keys.clear();
}

void InputManager::DataBuffer::RemoveInactiveTouches() {
  // The state that was readable last frame.
  DeviceState& previous_readable_state =
      buffer_[(curr_index_ + 1) % kBufferSize];
  // The state that will be readable this frame.
  DeviceState& new_readable_state = buffer_[curr_index_];
  // Need to remove any touches that have been inactive for more than one frame.
  for (size_t i = 0; i < new_readable_state.touchpads.size(); i++) {
    TouchpadState& touchpad = new_readable_state.touchpads[i];
    std::unordered_map<TouchId, Touch>& curr_touches = touchpad.touches;
    for (auto& prev_pair : previous_readable_state.touchpads[i].touches) {
      auto curr_pair = curr_touches.find(prev_pair.first);
      if (curr_pair != curr_touches.end() && !prev_pair.second.valid &&
          !curr_pair->second.valid) {
        curr_touches.erase(prev_pair.first);
        if (touchpad.primary_touch == prev_pair.first) {
          if (touchpad.current_touches.empty()) {
            touchpad.primary_touch = kPrimaryTouchId;
          } else {
            touchpad.primary_touch = touchpad.current_touches[0];
          }
        }
      }
    }
  }
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
