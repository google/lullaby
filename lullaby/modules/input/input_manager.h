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

#ifndef LULLABY_MODULES_INPUT_INPUT_MANAGER_H_
#define LULLABY_MODULES_INPUT_INPUT_MANAGER_H_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "lullaby/modules/input/device_profile.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/typeid.h"
#include "lullaby/util/variant.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// The InputManager is responsible for marshalling input events into a single,
// cohesive interface.  Input events can be generated from arbitrary sources
// (ex. event loops, callbacks, polling threads, etc.)
//
// The InputManager keeps a small buffer of state for each connected input
// device, containing three frames: front, current, and previous.  |front| is
// used for recording the incoming state for the device, i.e. from input events.
// |current| and |previous| are read-only and can be used to query the state of
// the device.  This two-frame history allows for limited support of queries
// like "just pressed" and "touch delta".
//
// The AdvanceFrame function is used to update the buffer such that the |front|
// state becomes the |current| state and a new |front| state is made available
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
  
  //     //java/com/google/lullaby/\
  //         InputManager.java
  // )

  // Type for representing the state of a button (or key or touchpad).  States
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
  // The ButtonId is the index of the button in the DeviceProfile.
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

  // Reserved mapping for an unset or invalid button (MAX if ButtonId type).
  static const ButtonId kInvalidButton;

  // Common controller joystick mappings.  For controllers that support
  // additional joysticks, clients can simply define their own identifiers or
  // use numeric values directly.
  enum JoystickType {
    kLeftJoystick = 0,
    kRightJoystick = 1,
    kDirectionalPad = 2,
  };

  // Common keyboard key mappings.
  static const char* const kKeyBackspace;
  static const char* const kKeyReturn;

  // Reserved mapping for an invalid touch location, used to represent that the
  // touchpad is not active.
  static const mathfu::vec2 kInvalidTouchLocation;

  // The index of a touchpad on a given device.  Will be kDefaultTouchId unless
  // the device has more than one touchpad.
  using TouchpadId = uint32_t;

  // The id of the first / primary touchpad.
  static const TouchpadId kPrimaryTouchpadId;

  // When dealing with multitouch, use this to uniquely identify each touch.
  // When a touch begins it will be assigned an id, and no other touch will
  // use the same id until all touches have ended on that touchpad.
  using TouchId = uint32_t;

  // Used to support legacy calls to touch functions.  Will select the oldest
  // active touch, or the most recently active if the last touch was just
  // released.  (MAX_VALUE of TouchId)
  static const TouchId kPrimaryTouchId;

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

  // Battery states:
  enum class BatteryState {
    kError,
    kUnknown,
    kCharging,
    kDischarging,
    kNotCharging,
    kFull,
  };

  // Number returned by GetBatteryCharge when the charge is unknown or not
  // supported
  static const uint8_t kInvalidBatteryCharge;

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

  // Returns a vector of touch id's for currently active touches, sorted by age
  // with the oldest touch at index 0.  Use this for tracking touches on
  // multitouch devices.  As soon as a touch is released it will disappear from
  // this list, but the TouchState and GetPreviousTouchLocation will remain
  // for one frame.
  std::vector<TouchId> GetTouches(DeviceType device,
                                  TouchpadId touchpad_id) const;

  // Returns true if |device|'s touchpad is active.
  bool IsValidTouch(DeviceType device,
                    TouchpadId touchpad_id = kPrimaryTouchpadId,
                    TouchId touch_id = kPrimaryTouchId) const;

  // Returns whether or not TouchGesture is queryable for |device|.
  bool IsTouchGestureAvailable(
      DeviceType device, TouchpadId touchpad_id = kPrimaryTouchpadId) const;

  // Gets the current state of the |device|'s touchpad.
  TouchState GetTouchState(DeviceType device,
                           TouchpadId touchpad_id = kPrimaryTouchpadId,
                           TouchId touch_id = kPrimaryTouchId) const;

  // Gets the current touch position of the |device|'s touchpad.  The range of
  // values for each element is [0.f, 1.f].  A value of kInvalidTouchLocation
  // indicates that the touchpad is not currently being touched.
  mathfu::vec2 GetTouchLocation(DeviceType device,
                                TouchpadId touchpad_id = kPrimaryTouchpadId,
                                TouchId touch_id = kPrimaryTouchId) const;

  // Gets the second to last sampled touch position of the |device|'s touchpad.
  // The range of values for each element is [0.f, 1.f].  A value of
  // kInvalidTouchLocation indicates that the touchpad was not touched earlier.
  mathfu::vec2 GetPreviousTouchLocation(
      DeviceType device, TouchpadId touchpad_id = kPrimaryTouchpadId,
      TouchId touch_id = kPrimaryTouchId) const;

  // A touch location that should be used for starting gestures.  Compare this
  // with the current location to compare to total drag thresholds. This will
  // initially be the press position, and will be reset whenever a gesture using
  // the touch finishes.  The range of values for each element is [0.f, 1.f].  A
  // value of kInvalidTouchLocation indicates that the touchpad is not currently
  // being touched.
  mathfu::vec2 GetTouchGestureOrigin(
      DeviceType device, TouchpadId touchpad_id = kPrimaryTouchpadId,
      TouchId touch_id = kPrimaryTouchId) const;

  // Gets the change in 2D position of the |device|'s touchpad.  The range of
  // values for each element is [-1.f, 1.f].
  mathfu::vec2 GetTouchDelta(DeviceType device,
                             TouchpadId touchpad_id = kPrimaryTouchpadId,
                             TouchId touch_id = kPrimaryTouchId) const;

  // Gets the change in 2D position of the |device|'s touchpad, locked to the
  // axis of its initial displacement. The range of values for each element is
  // [-1.f, 1.f]. Always returns [0.f, 0.f] for devices that don't support
  // TouchGesture.
  mathfu::vec2 GetLockedTouchDelta(InputManager::DeviceType device,
                                   TouchpadId touchpad_id = kPrimaryTouchpadId,
                                   TouchId touch_id = kPrimaryTouchId) const;

  // Gets the filtered touch velocity of |device| or (0,0) if touch isn't valid.
  mathfu::vec2 GetTouchVelocity(DeviceType device,
                                TouchpadId touchpad_id = kPrimaryTouchpadId,
                                TouchId touch_id = kPrimaryTouchId) const;

  // Gets the direction of a just-completed fling on |device|'s touchpad.
  // Despite the generalized name it returns GestureDirection::kNone for all
  // non-fling events.
  GestureDirection GetTouchGestureDirection(
      DeviceType device, TouchpadId touchpad_id = kPrimaryTouchpadId) const;

  // Gets the just-completed gesture type of |device|'s touchpad, if any.
  // Only valid for the current frame.
  GestureType GetTouchGestureType(
      DeviceType device, TouchpadId touchpad_id = kPrimaryTouchpadId) const;

  // Gets the initial displacement axis across |device|'s touchpad, if any.
  // Returns [0.f, 0.f] if the user hasn't been scrolling (IE if they've flung
  // or haven't performed a touch gesture recently at all), mathfu::kAxisY/X2f
  // otherwise.
  mathfu::vec2 GetInitialDisplacementAxis(
      DeviceType device, TouchpadId touchpad_id = kPrimaryTouchpadId) const;

  // The physical size of the touchpad in centimeters.  Use this when checking
  // touch movements against thresholds for gesture detection.
  // Returns NullOpt if no touchpad size has been specified.
  Optional<mathfu::vec2> GetTouchpadSize(
      DeviceType device, TouchpadId touchpad_id = kPrimaryTouchpadId) const;

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

  // Gets the screen_from_eye_matrix for the specified |eye| on the |device|.
  mathfu::mat4 GetScreenFromEye(DeviceType device, EyeType eye) const;

  // Gets the field of view for the specified |eye| on the |device|.
  mathfu::rectf GetEyeFOV(DeviceType device, EyeType eye) const;

  // Gets the viewport for the specified |eye| on the |device|.
  mathfu::recti GetEyeViewport(DeviceType device, EyeType eye) const;

  // Returns the current charge level of the device, from 0 to 100.
  // Returns 255 if the battery state is unknown or the device doesn't report
  // a battery level.
  uint8_t GetBatteryCharge(DeviceType device) const;

  // Returns the current state of the battery.
  // Returns kUnknown if the device doesn't report a battery state.
  BatteryState GetBatteryState(DeviceType device) const;

  // Queries for the capabilities of a |device|.
  bool HasPositionDof(DeviceType device) const;
  bool HasRotationDof(DeviceType device) const;
  bool HasTouchpad(DeviceType device,
                   TouchpadId touchpad_id = kPrimaryTouchpadId) const;
  bool HasJoystick(DeviceType device, JoystickType joystick) const;
  bool HasScroll(DeviceType device) const;
  bool HasButton(DeviceType device, ButtonId button) const;
  size_t GetNumButtons(DeviceType device) const;
  bool HasEye(DeviceType device, EyeType eye) const;
  size_t GetNumEyes(DeviceType device) const;
  bool HasBattery(DeviceType device) const;

  // Updates the internal buffers such that the write-state is now the first
  // read-only state and a new write-state is available.
  // Important:  No queries should be made concurrently while calling this
  // function.
  void AdvanceFrame(Clock::duration delta_time);

  // DEPRECATED: use lull::DeviceProfile from device_profile.h instead.
  struct DeviceParams {
    DeviceParams();

    bool has_position_dof;
    // Set |is_position_fake| in addition to |has_position_dof| if your
    // device has artificial movement (e.g. via the elbow model) instead of
    // actual position DoF info.
    bool is_position_fake;
    bool has_rotation_dof;
    bool has_touchpad;
    bool has_touch_gesture;
    bool has_scroll;
    bool has_battery;
    size_t num_joysticks;
    size_t num_buttons;
    size_t num_eyes;
    Clock::duration long_press_time;
  };

  // Enables the |device| with the given |profile|.
  void ConnectDevice(DeviceType device, const DeviceProfile& profile);
  // Support for deprecated path:
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
  // not. If dealing with a non-multitouch device, use '0' for |touch_id|.
  // If dealing with multitouch, each touch should have an id that is unique
  // until all touches are ended.
  void UpdateTouch(DeviceType device, TouchpadId touchpad_id, TouchId touch_id,
                   const mathfu::vec2& value, bool valid);
  void UpdateTouch(DeviceType device, const mathfu::vec2& value, bool valid);

  // Resets the gesture origin for the |touch_id|.  This should be called when
  // a multi touch gesture is released.
  void ResetTouchGestureOrigin(DeviceType device, TouchpadId touchpad_id,
                               TouchId touch_id);

  // Update gesture for |device|.
  void UpdateGesture(DeviceType device, TouchpadId touchpad_id,
                     GestureType type, GestureDirection direction,
                     const mathfu::vec2& displacement,
                     const mathfu::vec2& velocity);

  // Set the size of the touchpad in centimeters.
  void UpdateTouchpadSize(DeviceType device, TouchpadId touchpad_id, const mathfu::vec2& size_cm);

  // Updates the scroll value for the |device|.
  void UpdateScroll(DeviceType device, int delta);

  // Updates position of the |device|.
  void UpdatePosition(DeviceType device, const mathfu::vec3& value);

  // Updates rotation of the |device|.
  void UpdateRotation(DeviceType device, const mathfu::quat& value);

  // Updates the "eye from head", "screen from eye", "field of view", and
  // "viewport" settings for the |device| and |eye|.
  void UpdateEye(DeviceType device, EyeType eye,
                 const mathfu::mat4& eye_from_head_matrix,
                 const mathfu::mat4& screen_from_eye_matrix,
                 const mathfu::rectf& eye_fov,
                 const mathfu::recti& eye_viewport = {0, 0, 0, 0});

  // Updates the battery charge and state for the |device|.  |charge| should be
  // a percentage (0-100).
  void UpdateBattery(DeviceType device, BatteryState state, uint8_t charge);

  // Gets the DeviceProfile for a |device|.
  const DeviceProfile* GetDeviceProfile(DeviceType device) const;

  // Gets an arbitrary piece of data for the |device| that was previously set.
  // This should be used for unchanging data about the connected device.
  Variant GetDeviceInfo(DeviceType device, HashValue key) const;

  // Stores an arbitrary piece of data for the |device|.
  // This should be used for unchanging data about the connected device.
  void SetDeviceInfo(DeviceType device, HashValue key, const Variant& value);

 private:
  static const Clock::time_point kInvalidSampleTime;
  struct Touch {
    mathfu::vec2 position = kInvalidTouchLocation;
    mathfu::vec2 gesture_origin = kInvalidTouchLocation;
    mathfu::vec2 velocity = mathfu::kZeros2f;
    Clock::time_point time = kInvalidSampleTime;
    Clock::time_point press_time = kInvalidSampleTime;
    bool valid = false;
  };

  // Struct storing type, direction, velocity and displacement of the gesture.
  struct TouchGesture {
    GestureType type = GestureType::kNone;
    GestureDirection direction = GestureDirection::kNone;
    mathfu::vec2 velocity = mathfu::kZeros2f;
    mathfu::vec2 displacement = mathfu::kZeros2f;
    mathfu::vec2 initial_displacement_axis = mathfu::kZeros2f;
  };

  struct TouchpadState {
    TouchId primary_touch = kPrimaryTouchId;
    std::vector<TouchId> current_touches;
    std::unordered_map<TouchId, Touch> touches;
    TouchGesture gesture;
    mathfu::vec2 size_cm = mathfu::vec2(-1.0f, -1.0f);
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
    std::vector<TouchpadState> touchpads;
    std::vector<mathfu::vec3> position;
    std::vector<mathfu::quat> rotation;
    std::vector<mathfu::mat4> eye_from_head_matrix;
    std::vector<mathfu::mat4> screen_from_eye_matrix;
    std::vector<mathfu::recti> eye_viewport;
    std::vector<mathfu::rectf> eye_fov;
    std::vector<uint8_t> battery_charge;
    std::vector<BatteryState> battery_state;
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

    // Remove touches that were released 2 frames ago.
    void RemoveInactiveTouches();

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

    void Connect(const DeviceProfile& profile);

    void Disconnect();

    void Advance(Clock::duration delta_time);

    bool IsConnected() const { return connected_; }

    DataBuffer* GetDataBuffer() { return buffer_.get(); }

    const DataBuffer* GetDataBuffer() const { return buffer_.get(); }

    const DeviceProfile& GetDeviceProfile() const { return profile_; }

    VariantMap& GetDeviceInfo() { return info_; }
    const VariantMap& GetDeviceInfo() const { return info_; }

   private:
    bool connected_;
    DeviceProfile profile_;
    std::unique_ptr<DataBuffer> buffer_;
    VariantMap info_;
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
  DeviceState* GetDeviceStateForWriteLocked(DeviceType device);
  const TouchGesture* GetTouchGesturePtr(DeviceType device,
                                         TouchpadId touchpad_id) const;
  const Touch* GetTouchPtr(DeviceType device, TouchpadId touchpad_id,
                           TouchId touch_id) const;
  const Touch* GetPreviousTouchPtr(DeviceType device, TouchpadId touchpad_id,
                                   TouchId touch_id) const;

  std::mutex mutex_;
  Device devices_[kMaxNumDeviceTypes];
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::InputManager);
LULLABY_SETUP_TYPEID(lull::InputManager::DeviceType);

#endif  // LULLABY_MODULES_INPUT_INPUT_MANAGER_H_
