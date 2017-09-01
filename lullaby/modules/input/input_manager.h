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

#ifndef LULLABY_MODULES_INPUT_INPUT_MANAGER_H_
#define LULLABY_MODULES_INPUT_INPUT_MANAGER_H_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "lullaby/util/clock.h"
#include "lullaby/util/typeid.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// The InputManager is responsible for marshalling input events into a single,
// cohesive interface.  Input events can be generated from arbitrary sources
// (ex. event loops, callbacks, polling threads, etc.)
//
// The InputManager keeps a small buffer of state for each connected input
// device.  The "front" of the buffer is used for recording the incoming state
// for the devices from the input events.  The rest of the buffer is read-only
// and can be used to query the state of a device.  The readable portion of
// the buffer stores a two-frame history (current and previous) to allow for
// limited support of queries like "just pressed" and "touch delta".
//
// The AdvanceFrame function is used to update the buffer such that the "front"
// state becomes the "current" state and a new "front" state is made available
// for write operations.  The InputManager allows multiple threads to generate
// input events by using a mutex.  State information is also safe to read from
// multiple threads as they are read-only operations.  However, it is assumed
// that no query operations will be performed during the AdvanceFrame call.
class InputManager {
 public:
  InputManager();

  // List of potential input devices.
  enum DeviceType {
    kHmd,
    kMouse,
    kKeyboard,
    kController,
    kController2,
    kHand,
    kMaxNumDeviceTypes
  };

  // Type for representing the state of a button (or key or touch).  All states
  // are not necessarily mutually exclusive, so bitwise checks should be used
  // instead of direct comparisons.
  using ButtonState = uint8_t;
  using TouchState = ButtonState;
  static const ButtonState kReleased;
  static const ButtonState kPressed;
  static const ButtonState kLongPressed;
  static const ButtonState kJustReleased;
  static const ButtonState kJustPressed;
  static const ButtonState kJustLongPressed;
  static const ButtonState kRepeat;

  // Identifier for each button on a device.  The number of buttons supported by
  // a device can be queried by calling GetNumButtons.
  using ButtonId = uint32_t;

  // Common mouse button mappings.
  static const ButtonId kLeftMouse;
  static const ButtonId kRightMouse;
  static const ButtonId kMiddleMouse;
  static const ButtonId kBackMouse;
  static const ButtonId kForwardMouse;

  // Common controller button mappings.  For controllers that support additional
  // buttons, clients can simply define their own identifiers or use numeric
  // values directly.
  static const ButtonId kPrimaryButton;
  static const ButtonId kSecondaryButton;
  static const ButtonId kRecenterButton;

  // Common joystick mappings.  For controllers that support additional
  // joysticks, clients can simply define their own identifiers or use numeric
  // values directly.
  enum JoystickType {
    kLeftJoystick = 0,
    kRightJoystick = 1,
    kDirectionalPad = 2,
  };

  // Common keyboard key mappings.
  static const char* const kKeyBackspace;
  static const char* const kKeyReturn;

  // Invalid touch location used to represent touchpad is not active.
  static const mathfu::vec2 kInvalidTouchLocation;

  // Type representing the eye (left or right).
  using EyeType = uint32_t;

  // Types of supported explicit gestures.
  enum class GestureType {
    kNone,
    kScrollStart,
    kScrollUpdate,
    kScrollEnd,
    kFling,
  };

  // Fling gesture directions.
  enum class GestureDirection {
    kNone,
    kLeft,
    kRight,
    kUp,
    kDown,
  };

  static const char* GetDeviceName(DeviceType device);

  // Gets the current state of a keyboard's |key|.
  ButtonState GetKeyState(DeviceType device, const std::string& key) const;

  // Gets the keys which were pressed.
  std::vector<std::string> GetPressedKeys(DeviceType device) const;

  // Gets the current state of a device's |button|.
  ButtonState GetButtonState(DeviceType device, ButtonId button) const;

  // Gets the amount of time the device's |button| has been held down.  Will
  // be reset 1 frame after the |button| has been released.
  Clock::duration GetButtonPressedDuration(DeviceType device,
                                                ButtonId button) const;

  // Gets the current 2D position of the |joystick| on the |device|.  The range
  // of values for each element is [-1.f, 1.f].
  mathfu::vec2 GetJoystickValue(DeviceType device, JoystickType joystick) const;

  // Gets the change in 2D position of the |joystick| on the |device|.  The
  // range of values for each element is [-2.f, 2.f].
  mathfu::vec2 GetJoystickDelta(DeviceType device, JoystickType joystick) const;

  // Returns true if |device|'s touchpad is active.
  bool IsValidTouch(DeviceType device) const;

  // Gets the current state of the |device|'s touchpad.
  TouchState GetTouchState(DeviceType device) const;

  // Gets the current touch position of the |device|'s touchpad.  The range of
  // values for each element is [0.f, 1.f].  A value of kInvalidTouchLocation
  // indicates that the touchpad is not currently being touched.
  mathfu::vec2 GetTouchLocation(DeviceType device) const;

  // Gets the change in 2D position of the |device|'s touchpad.  The range of
  // values for each element is [-1.f, 1.f].
  mathfu::vec2 GetTouchDelta(DeviceType device) const;

  // Gets the filtered touch velocity of |device| or (0,0) if touch isn't valid.
  mathfu::vec2 GetTouchVelocity(DeviceType device) const;

  // Gets the just-completed gesture of |device|'s touchpad, if any.
  GestureDirection GetTouchGestureDirection(DeviceType device) const;

  // Gets the current position of a |device| with a positional sensor.
  mathfu::vec3 GetDofPosition(DeviceType device) const;

  // Gets the change in position of a |device| with a positional sensor.
  mathfu::vec3 GetDofDelta(DeviceType device) const;

  // Gets the current rotation of a |device| with a rotational sensor.
  mathfu::quat GetDofRotation(DeviceType device) const;

  // Gets the change in rotation (roll, pitch, yaw) of a |device| with a
  // rotational sensor.
  mathfu::quat GetDofAngularDelta(DeviceType device) const;

  // Gets a matrix composed of the Position and Rotation (if the |device| has
  // those degrees of freedom).
  mathfu::mat4 GetDofWorldFromObjectMatrix(DeviceType device) const;

  // Gets the delta value for a |device| with a scroll wheel.
  int GetScrollDelta(DeviceType device) const;

  // Gets the eye_from_head_matrix for the specified |eye| on the |device|.
  mathfu::mat4 GetEyeFromHead(DeviceType device, EyeType eye) const;

  // Gets the field of view for the specified |eye| on the |device|.
  mathfu::rectf GetEyeFOV(DeviceType device, EyeType eye) const;

  // Gets the viewport for the specified |eye| on the |device|.
  mathfu::recti GetEyeViewport(DeviceType device, EyeType eye) const;

  // Queries for the capabilities of a |device|.
  bool HasPositionDof(DeviceType device) const;
  bool HasRotationDof(DeviceType device) const;
  bool HasTouchpad(DeviceType device) const;
  bool HasJoystick(DeviceType device, JoystickType joystick) const;
  bool HasScroll(DeviceType device) const;
  bool HasButton(DeviceType device, ButtonId button) const;
  size_t GetNumButtons(DeviceType device) const;
  bool HasEye(DeviceType device, EyeType eye) const;
  size_t GetNumEyes(DeviceType device) const;

  // Updates the internal buffers such that the write-state is now the first
  // read-only state and a new write-state is available.
  // Important:  No queries should be made concurrently while calling this
  // function.
  void AdvanceFrame(Clock::duration delta_time);

  // Structure that provides information about a device being connected.
  struct DeviceParams {
    DeviceParams();

    bool has_position_dof;
    bool has_rotation_dof;
    bool has_touchpad;
    bool has_touch_gesture;
    bool has_scroll;
    size_t num_joysticks;
    size_t num_buttons;
    size_t num_eyes;
    Clock::duration long_press_time;
  };

  // Enables the |device| with the given |params|.
  void ConnectDevice(DeviceType device, const DeviceParams& params);

  // Disables the |device|.
  void DisconnectDevice(DeviceType device);

  // Checks if the |device| is currently connected.
  bool IsConnected(DeviceType device) const;

  // Updates key state for the |device|.  The |repeat| flag can be used to
  // indicate whether the key has been held long enough for the repeat rate to
  // trigger another event.
  void UpdateKey(DeviceType device, const std::string& key, bool repeat);

  // Updates which alphanumeric keys are pressed on the |device|.
  void KeyPressed(DeviceType device, const std::string& key);

  // Updates button state for the |device|.  The |pressed| flag is used to
  // specify if the button is pressed or released.  The |repeat| flag can be
  // used to indicate whether the button has been held long enough for the
  // repeat rate to trigger another event.
  void UpdateButton(DeviceType device, ButtonId id, bool pressed, bool repeat);

  // Updates |joystick| value for the |device|.  The value should be normalized
  // such that individual components are in the range [-1.f, 1.f].
  void UpdateJoystick(DeviceType device, JoystickType joystick,
                      const mathfu::vec2& value);

  // Updates touchpad state for the |device|.  The value should be normalized
  // such that the individual components are in the range [0.f, 1.f].  The
  // |valid| flag indicates whether the touchpad is actually being touched or
  // not.
  void UpdateTouch(DeviceType device, const mathfu::vec2& value, bool valid);

  // Update gesture for |device|.
  void UpdateGesture(DeviceType device, GestureType type,
                     GestureDirection direction,
                     const mathfu::vec2& displacement,
                     const mathfu::vec2& velocity);

  // Updates the scroll value for the |device|.
  void UpdateScroll(DeviceType device, int delta);

  // Updates position of the |device|.
  void UpdatePosition(DeviceType device, const mathfu::vec3& value);

  // Updates rotation of the |device|.
  void UpdateRotation(DeviceType device, const mathfu::quat& value);

  // Updates the "eye from head", "field of view", and "viewport" settings for
  // the |device| and |eye|.
  void UpdateEye(DeviceType device, EyeType eye,
                 const mathfu::mat4& eye_from_head_matrix,
                 const mathfu::rectf& eye_fov,
                 const mathfu::recti& eye_viewport = {0, 0, 0, 0});

  // Gets the DeviceParams for a |device|.
  DeviceParams GetDeviceParamsCopy(DeviceType device) const;

 private:
  static const Clock::time_point kInvalidSampleTime;

  struct TouchpadState {
    mathfu::vec2 position = kInvalidTouchLocation;
    mathfu::vec2 velocity = mathfu::kZeros2f;
    Clock::time_point time = kInvalidSampleTime;
    bool valid = false;
  };

  // Struct storing type, direction, velocity and displacement of the gesture.
  struct TouchGesture {
    GestureType type = GestureType::kNone;
    GestureDirection direction = GestureDirection::kNone;
    mathfu::vec2 velocity = mathfu::kZeros2f;
    mathfu::vec2 displacement = mathfu::kZeros2f;
  };

  // Structure to hold the "input" state of a device.  std::vector's are used
  // as not all devices support all potential states.
  struct DeviceState {
    DeviceState() {}

    std::vector<std::string> keys;
    std::vector<int> scroll;
    std::vector<bool> buttons;
    std::vector<Clock::time_point> button_press_times;
    std::vector<bool> repeat;
    std::vector<mathfu::vec2> joystick;
    std::vector<TouchpadState> touch;
    std::vector<Clock::time_point> touch_press_times;
    std::vector<mathfu::vec3> position;
    std::vector<mathfu::quat> rotation;
    std::vector<mathfu::mat4> eye_from_head_matrix;
    std::vector<mathfu::recti> eye_viewport;
    std::vector<mathfu::rectf> eye_fov;
    std::vector<TouchGesture> touch_gesture;
    Clock::time_point time_stamp;
  };

  // Buffer for holding DeviceState.
  class DataBuffer {
   public:
    // Constructor that initializes all internal states in the buffer to the
    // provided |reference_state|.
    explicit DataBuffer(const DeviceState& reference_state);

    // Update the write-state to now be the first (ie. current) read-only state
    // and prepare a new write-state.
    void Advance(Clock::duration delta_time);

    // Get reference to writable state.
    DeviceState& GetMutable();

    // Get read-only reference to most recent state.
    const DeviceState& GetCurrent() const;

    // Get read-only reference to previous state.
    const DeviceState& GetPrevious() const;

   private:
    static const int kBufferSize = 3;
    DeviceState buffer_[kBufferSize];
    int curr_index_;
  };

  // Class representing a single input device.
  class Device {
   public:
    Device();

    void Connect(const DeviceParams& params);

    void Disconnect();

    void Advance(Clock::duration delta_time);

    bool IsConnected() const { return connected_; }

    DataBuffer* GetDataBuffer() { return buffer_.get(); }

    const DataBuffer* GetDataBuffer() const { return buffer_.get(); }

    const DeviceParams& GetDeviceParams() const { return params_; }

   private:
    bool connected_;
    DeviceParams params_;
    std::unique_ptr<DataBuffer> buffer_;
  };

  static const ButtonState kInvalidButtonState = 0;
  static ButtonState GetButtonState(bool curr, bool prev, bool repeat,
                                    Clock::duration long_press_time,
                                    Clock::time_point curr_time_stamp,
                                    Clock::time_point prev_time_stamp,
                                    Clock::time_point curr_press_time,
                                    Clock::time_point prev_press_time);

  DataBuffer* GetDataBuffer(DeviceType device);
  const DataBuffer* GetDataBuffer(DeviceType device) const;
  const DataBuffer* GetConnectedDataBuffer(DeviceType device) const;
  const DeviceParams* GetDeviceParams(DeviceType device) const;
  DeviceState* GetDeviceStateForWriteLocked(DeviceType device);
  const TouchGesture* GetTouchGesturePtr(DeviceType device) const;

  std::mutex mutex_;
  Device devices_[kMaxNumDeviceTypes];
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::InputManager);

#endif  // LULLABY_MODULES_INPUT_INPUT_MANAGER_H_
