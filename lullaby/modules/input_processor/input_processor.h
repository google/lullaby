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

#ifndef LULLABY_UTIL_INPUT_PROCESSOR_H_
#define LULLABY_UTIL_INPUT_PROCESSOR_H_

#include <memory>
#include <set>
#include <vector>

#include "lullaby/events/input_events.h"
#include "lullaby/modules/input/input_focus.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input_processor/gesture.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/enum_hash.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/variant.h"

namespace lull {

/// InputProcessor generates input events, and serves as a standard storage
/// location for what each input device is focused on.  By allowing custom
/// prefixes, apps can set up specific devices and device/button pairs to map
/// to general functionality.
/// See standard_input_pipeline.cc for the standard usage.
/// See hello_cursor.cc for an example of setting up prefixes.
///
/// To setup InputProcessor, an app should first set up any prefixes it desires.
/// In general, the 'main' input device should have an empty string as a prefix,
/// and the primary button on that input device should also be set to an empty
/// string.  This means that entities can listen for "FocusStartEvent" or
/// "ClickEvent" as their default.
/// Whenever InputProcessor is going to send out an event, it also sends out a
/// copy of that event with the "Any" prefix, i.e. "AnyClickEvent".  Apps should
/// use code to check the |device| and |button| parameters when using "Any*"
/// events.
///
/// To update InputProcessor, an app should decide what entity their devices are
/// pointed at, setup an InputFocus based on that info, and call UpdateDevice().
class InputProcessor {
 public:
  /// Enum to configure the legacy support behavior of input_processor
  enum LegacyMode {
    /// Use the original reticle_system event names and event logic.  Also send
    /// the new events, when they match with an old event.
    /// WARNING: this mode will not send cancel or drag events.
    kLegacyEventsAndLogic,
    /// Use the new input_processor logic, but send both old and new events.
    kLegacyEvents,
    /// Don't use the old logic or event names.
    kNoLegacy,
    /// Send no events - just store InputFocus data
    kNoEvents
  };

  /// If |enable_legacy_events| is set to true, the old lull:: input events
  /// will be sent out in addition to the prefix+suffix events.
  explicit InputProcessor(Registry* registry,
                          LegacyMode legacy_mode = kNoLegacy);

  /// Create and register a new InputProcessor in the Registry.
  static InputProcessor* Create(Registry* registry,
                                LegacyMode legacy_mode = kNoLegacy);

  /// Gets the entity that the |device| is focused on, with some metadata.
  const InputFocus* GetInputFocus(InputManager::DeviceType device) const;

  /// Gets the entity that the |device| was focused on last frame, with some
  /// metadata.
  const InputFocus* GetPreviousFocus(InputManager::DeviceType device) const;

  /// Set which device is the main selection device.  This is not used directly
  /// by input processor, but may be used by other systems.
  void SetPrimaryDevice(InputManager::DeviceType device);

  /// Returns the main device that should be used for interaction with UI.
  /// Defaults to kController.
  InputManager::DeviceType GetPrimaryDevice() const;

  /// If set for a device, focus events for that device will be prefixed by
  /// |prefix|.  i.e. "FocusStart" could become "MainFocusStart".  All devices
  /// will also send an event named "AnyFocusStart".
  /// If |prefix| is an empty string, events for that device will be sent with
  /// no prefix.
  void SetPrefix(InputManager::DeviceType device, string_view prefix);

  /// If set for a device and a button, button events for that pair will be
  /// prefixed by |prefix|.  i.e. "ClickEvent" could become "SystemClickEvent".
  /// All buttons will also send an event named "AnyClickEvent".
  /// If |prefix| is an empty string, events for that pair will be sent with no
  /// prefix.
  void SetPrefix(InputManager::DeviceType device, InputManager::ButtonId button,
                 string_view prefix);

  /// If set for a device, touch events for that device will be
  /// |prefix|.  i.e. "ClickEvent" could become "SystemClickEvent". All touches
  /// will also send an event named "AnyClickEvent".
  /// If |prefix| is an empty string, events for that device will be sent with
  /// no prefix.
  void SetTouchPrefix(InputManager::DeviceType device,
                      InputManager::TouchpadId touchpad, string_view prefix);

  /// Set a list of gesture recognizers to process touches.  If set, the Swipe
  /// and Drag events will not be sent by the touch.  Instead, the recognizers
  /// will be fed touches, and any detected gestures will generate events.
  /// The recognizers are called in the order they are present in a list, so
  /// lower index recognizers will get first chance at claiming touches.
  void SetTouchGestureRecognizers(InputManager::DeviceType device,
                                  InputManager::TouchpadId touchpad,
                                  GestureRecognizerList recognizers);

  /// Removes the prefix for |device|, meaning focus events for that device will
  /// only be sent with the "Any" prefix.
  void ClearPrefix(InputManager::DeviceType device);

  /// Removes the prefix for the |device| |button| pair, meaning button events
  /// for that pair will only be sent with the "Any" prefix.
  void ClearPrefix(InputManager::DeviceType device,
                   InputManager::ButtonId button);

  /// Removes the touch prefix for |device|, meaning touch events for that
  /// device will only be sent with the "Any" prefix.
  void ClearTouchPrefix(InputManager::DeviceType device,
                        InputManager::TouchpadId touchpad);

  /// Update the focus state and send events for |device|.  This should be
  /// called once per frame per device, with input_focus containing information
  /// about what entity the device is currently focused on.
  void UpdateDevice(const Clock::duration& delta_time,
                    const InputFocus& input_focus);

  /// If a touch is currently driving a gesture, this will return that gesture.
  GesturePtr GetTouchOwner(InputManager::DeviceType device,
                           InputManager::TouchpadId touchpad,
                           InputManager::TouchId id);

  /// Pauses the input processing of the current processor in favor of
  /// |processor|. Use this to implement alternate input modes. Multiple
  /// overriding processors are stored in the order they were added, with only
  /// the most recent receiving updates.
  void AddOverrideProcessor(const std::shared_ptr<InputProcessor>& processor);

  /// Removes |processor| from the list of override InputProcessors.
  void RemoveOverrideProcessor(
      const std::shared_ptr<InputProcessor>& processor);

 private:
// A macro to expand an event list into the contents of an enum.
#define LULLABY_GENERATE_ENUM(Enum, Name) Enum,
  // clang-format off
  /// Events that are sent on a per-device basis:
  enum DeviceEventType {
    LULLABY_DEVICE_EVENT_LIST(LULLABY_GENERATE_ENUM)
    kNumDeviceEventTypes
  };

  // Events that are sent on a per-button and device basis:
  enum ButtonEventType {
    LULLABY_BUTTON_EVENT_LIST(LULLABY_GENERATE_ENUM)
    kNumButtonEventTypes
  };

  enum TouchEventType {
    LULLABY_TOUCH_EVENT_LIST(LULLABY_GENERATE_ENUM)
    kNumTouchEventTypes
  };

  enum GestureEventType {
    LULLABY_GESTURE_EVENT_LIST(LULLABY_GENERATE_ENUM)
    kNumGestureEventTypes
  };
  // clang-format on
#undef LULLABY_GENERATE_ENUM

  using DeviceButtonPair =
      std::pair<InputManager::DeviceType, InputManager::ButtonId>;
  // Hash function to allow DeviceButtonPair to be used as a key in a map.
  struct DeviceButtonPairHash {
    std::size_t operator()(const DeviceButtonPair& pair) const {
      uint64_t combine = pair.first;
      combine = combine << 32;
      combine += pair.second;
      return std::hash<uint64_t>{}(combine);
    }
  };
  using DeviceTouchpadPair =
      std::pair<InputManager::DeviceType, InputManager::TouchpadId>;
  // Hash function to allow DeviceTouchpadPair to be used as a key in a map.
  struct DeviceTouchpadPairHash {
    std::size_t operator()(const DeviceTouchpadPair& pair) const {
      uint64_t combine = pair.first;
      combine = combine << 32;
      combine += pair.second;
      return std::hash<uint64_t>{}(combine);
    }
  };

  template <int N>
  struct EventHashes {
    HashValue events[N];
    // #if LULLABY_TRACK_EVENT_NAMES
    std::string names[N];
    // #endif
  };
  using DeviceEvents = EventHashes<kNumDeviceEventTypes>;
  using ButtonEvents = EventHashes<kNumButtonEventTypes>;
  using TouchEvents = EventHashes<kNumTouchEventTypes>;
  using GestureEvents = EventHashes<kNumGestureEventTypes>;

  struct FocusPair {
    InputFocus current;
    InputFocus previous;
  };

  /// Enum to capture the current state of a particular button press.
  enum ButtonStates {
    /// Collision ray is still inside the ray slop.
    kInsideSlop,
    // TODO (b/126901687) remove kDragging and replace with kGesturing
    /// Collision ray is between the ray slop and the cancel threshold.
    kDragging,
    // TODO (b/126901687) remove this and replace with kGesturing
    /// Touch exclusive state, occurs when the touch (and not the ray) moves.
    kTouchMoved,
    /// A gesture is happening or has happened.
    kGesturing,
    /// Focus changed after press happened.  Only ReleaseEvent can be sent from
    /// this state..
    kPressedBeforeFocus,
    /// Collision ray exceeded cancel threshold.
    kCanceled,
    /// Button was released.
    kReleased,
  };

  /// The current state of a button
  struct ButtonState {
    // The entity that the device was focused on when the button was pressed.
    Entity pressed_entity = kNullEntity;
    // The entity that the device was focused on the last time this button was
    // updated.
    Entity focused_entity = kNullEntity;
    // The current logical state of the button.
    ButtonStates state = kReleased;
    // Location in local space of the first frame that this button was pressed
    // and focused on focused_entity.
    mathfu::vec3 pressed_location = mathfu::kZeros3f;
    // Time since the button was pressed.
    int64_t ms_since_press = 0;
  };

  /// The current state of a touch. Mostly the same as a button, but with
  /// gesture support.
  struct Touch : ButtonState {
    /// The gesture that currently 'owns' this touch.
    GesturePtr owner = nullptr;
  };

  struct Touchpad {
    std::unordered_map<InputManager::TouchId, Touch> touches;
    std::vector<GesturePtr> gestures;
    GestureRecognizerList recognizers;
    // List of events for each recognizer.
    std::unordered_map<HashValue, GestureEvents> events;
    std::unordered_map<HashValue, GestureEvents> any_events;
  };

  /// Update the stored InputFocus.  Must be called before UpdateFocus or
  /// UpdateButtons.
  void SwapBuffers(const InputFocus& input_focus);

  /// Update the current focus for |device| and send focus events.
  void UpdateFocus(InputManager::DeviceType device);

  /// Sends events associated with |device|.  This should be called once per
  /// frame, after setting the focus for |device|.
  void UpdateButtons(const Clock::duration& delta_time,
                     InputManager::DeviceType device);

  /// As UpdateButtons, but uses the logic from ReticleSystem.
  void UpdateButtonsLegacy(const Clock::duration& delta_time,
                           InputManager::DeviceType device);

  /// Send touch touchpad events associated with |device|.
  void UpdateTouches(const Clock::duration& delta_time,
                     InputManager::DeviceType device,
                     InputManager::TouchpadId touchpad);

  void UpdateTouch(const int64_t delta_time, InputManager::DeviceType device,
                   InputManager::TouchpadId touchpad, InputManager::TouchId id,
                   InputManager::TouchState touch_state);

  void UpdateTouchGestures(const Clock::duration& delta_time,
                           InputManager::DeviceType device,
                           InputManager::TouchpadId touchpad);
  void CancelAllGestures(const Clock::duration& delta_time,
                         InputManager::DeviceType device,
                         InputManager::TouchpadId touchpad);

  void HandleGestureStart(InputManager::DeviceType device,
                          InputManager::TouchpadId touchpad,
                          Gesture::TouchIdSpan ids, GesturePtr gesture,
                          GestureRecognizerPtr recognizer);
  void HandleGestureEnd(InputManager::DeviceType device,
                        InputManager::TouchpadId touchpad, GesturePtr gesture,
                        GestureEventType event_type);

  void HandlePress(InputManager::DeviceType device,
                   InputManager::ButtonId button_id, ButtonState* button_state);
  void HandleDragStart(InputManager::DeviceType device,
                       InputManager::ButtonId button_id,
                       ButtonState* button_state);
  void HandleRelease(InputManager::DeviceType device,
                     InputManager::ButtonId button_id,
                     const InputManager::ButtonState& button,
                     ButtonState* button_state);
  void HandleCancel(InputManager::DeviceType device,
                    InputManager::ButtonId button_id,
                    ButtonState* button_state);

  void HandleReleaseLegacy(InputManager::DeviceType device,
                           InputManager::ButtonId button_id,
                           const InputManager::ButtonState& button,
                           ButtonState* button_state);
  void HandleLongPressLegacy(InputManager::DeviceType device,
                             InputManager::ButtonId button_id,
                             ButtonState* button_state);

  void HandleTouchPress(InputManager::DeviceType device,
                        InputManager::TouchpadId touchpad,
                        InputManager::TouchId id, Touch* touch);
  void HandleTouchDragStart(InputManager::DeviceType device,
                            InputManager::TouchpadId touchpad,
                            InputManager::TouchId id, Touch* touch);
  void HandleTouchRelease(InputManager::DeviceType device,
                          InputManager::TouchpadId touchpad,
                          InputManager::TouchId id, Touch* touch,
                          const InputManager::TouchState& touch_state);
  void HandleTouchCancel(InputManager::DeviceType device,
                         InputManager::TouchpadId touchpad,
                         InputManager::TouchId id, Touch* touch);
  void HandleTouchSwipeStart(InputManager::DeviceType device,
                             InputManager::TouchpadId touchpad,
                             InputManager::TouchId id, Touch* touch);

  void SetButtonTarget(InputManager::DeviceType device,
                       ButtonState* button_state);

  void ResetButton(ButtonState* button_state);
  void ResetTouch(Touch* touch);

  void SendDeviceEvent(InputManager::DeviceType device,
                       DeviceEventType event_type, Entity target,
                       const VariantMap* values);

  void SendButtonEvent(InputManager::DeviceType device,
                       InputManager::ButtonId button,
                       ButtonEventType event_type, Entity target,
                       VariantMap* values);
  void SendTouchEvent(InputManager::DeviceType device,
                      InputManager::TouchpadId touchpad,
                      InputManager::TouchId id, TouchEventType event_type,
                      Entity target, VariantMap* values);
  void SendGestureEvent(InputManager::DeviceType device,
                        InputManager::TouchpadId touchpad,
                        const GesturePtr gesture, GestureEventType event_type,
                        Entity target, VariantMap* values);

  template <typename EventSet, typename EventType>
  void SendEvent(const EventSet& event_set, EventType event_type, Entity target,
                 InputManager::DeviceType device, const VariantMap* values);

  void SetupDeviceEvents(const string_view prefix,
                         InputProcessor::DeviceEvents* events);
  void SetupButtonEvents(const string_view prefix,
                         InputProcessor::ButtonEvents* events);
  void SetupTouchEvents(const string_view prefix,
                        InputProcessor::TouchEvents* events);
  void SetupGestureEvents(const string_view prefix, Touchpad* touchpad);
  void SetupLegacyEvents();

  /// Calculate the angle in radians [0, PI] between the InputFocus's
  /// collision_ray and a ray from the collision_ray's origin to the current
  /// cursor_position. returns 0 if there is no focused entity, and float max if
  /// the focused entity does not have a transform component.
  float CalculateRaySlop(const ButtonState& button_state,
                         const InputFocus& focus) const;

  static const char* GetDeviceEventName(DeviceEventType type);
  static const char* GetButtonEventName(ButtonEventType type);
  static const char* GetTouchEventName(TouchEventType type);
  static const char* GetGestureEventName(GestureEventType type);
  std::vector<std::shared_ptr<InputProcessor>> override_input_processors_;
  Registry* registry_;

  std::unordered_map<InputManager::DeviceType, FocusPair, EnumHash> input_foci_;
  std::unordered_map<DeviceButtonPair, ButtonState, DeviceButtonPairHash>
      button_states_;
  std::unordered_map<DeviceTouchpadPair, Touchpad, DeviceButtonPairHash>
      touchpad_states_;

  // Maps from device (& button) to a hash of the prefix, if set.
  std::unordered_map<InputManager::DeviceType, DeviceEvents, EnumHash>
      device_events_;
  std::unordered_map<DeviceButtonPair, ButtonEvents, DeviceButtonPairHash>
      button_events_;
  std::unordered_map<InputManager::DeviceType, TouchEvents, EnumHash>
      touch_events_;
  std::unordered_map<DeviceTouchpadPair, std::string, DeviceTouchpadPairHash>
      touchpad_prefixes_;

  // Names and hashes for events with the "Any" prefix.
  DeviceEvents any_device_events_;
  ButtonEvents any_button_events_;
  TouchEvents any_touch_events_;

  DeviceEvents legacy_device_events_;
  ButtonEvents legacy_button_events_;

  InputManager::DeviceType primary_device_ = InputManager::kMaxNumDeviceTypes;

  LegacyMode legacy_mode_ = kNoLegacy;
};

// DEPRECATED. Queries the InputManager's button states, and sends out global
// events based on those states.  Replaced by the InputProcessor's UpdateDevice
// function.
void ProcessEventsForDevice(Registry* registry,
                            InputManager::DeviceType device);

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::InputProcessor);

#endif  // LULLABY_UTIL_INPUT_PROCESSOR_H_
