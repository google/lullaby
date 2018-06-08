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

#include "lullaby/modules/input_processor/input_processor.h"

#include <limits>  // std::numeric_limits

#include "lullaby/events/input_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/bits.h"

namespace lull {
namespace {

// TODO(b/69162952) These values need to be configurable on a system wide level.
// The angle in radians between the InputFocus's collision_ray and a ray from
// the collision_ray's origin to the current cursor_position.
constexpr float kRayDragSlop = 2.0f * kDegreesToRadians;
constexpr float kRayCancelSlop = 35.0f * kDegreesToRadians;
constexpr const char* kAnyPrefix = "Any";

template <typename PressEvent, typename ReleaseEvent, typename ClickEvent,
          typename LongPressEvent, typename LongClickEvent>
void ProcessEventsForButton(InputManager* input, Dispatcher* dispatcher,
                            InputManager::DeviceType device,
                            InputManager::ButtonId button) {
  if (!input->HasButton(device, button)) {
    return;
  }
  const InputManager::ButtonState state = input->GetButtonState(device, button);
  if (CheckBit(state, InputManager::kJustReleased)) {
    dispatcher->Send(ReleaseEvent());
    if (!CheckBit(state, InputManager::kLongPressed)) {
      // Only send click if the press-release time is short.
      dispatcher->Send(ClickEvent());
    } else {
      dispatcher->Send(LongClickEvent());
    }
  } else if (CheckBit(state, InputManager::kJustLongPressed)) {
    dispatcher->Send(LongPressEvent());
  }
  if (CheckBit(state, InputManager::kJustPressed)) {
    dispatcher->Send(PressEvent());
  }
}
}  // namespace

void ProcessEventsForDevice(Registry* registry,
                            InputManager::DeviceType device) {
  auto* input = registry->Get<lull::InputManager>();
  auto* dispatcher = registry->Get<lull::Dispatcher>();
  if (input->IsConnected(device)) {
    ProcessEventsForButton<PrimaryButtonPress, PrimaryButtonRelease,
                           PrimaryButtonClick, PrimaryButtonLongPress,
                           PrimaryButtonLongClick>(
        input, dispatcher, device, InputManager::kPrimaryButton);

    ProcessEventsForButton<SecondaryButtonPress, SecondaryButtonRelease,
                           SecondaryButtonClick, SecondaryButtonLongPress,
                           SecondaryButtonLongClick>(
        input, dispatcher, device, InputManager::kSecondaryButton);

    ProcessEventsForButton<SystemButtonPress, SystemButtonRelease,
                           SystemButtonClick, SystemButtonLongPress,
                           SystemButtonLongClick>(
        input, dispatcher, device, InputManager::kRecenterButton);
  }
}

InputProcessor::InputProcessor(Registry* registry, LegacyMode legacy_mode)
    : registry_(registry), legacy_mode_(legacy_mode) {
  if (legacy_mode_ != kNoEvents) {
    SetupDeviceEvents(kAnyPrefix, &any_device_events_);
    SetupButtonEvents(kAnyPrefix, &any_button_events_);
    if (legacy_mode != kNoLegacy) {
      SetupLegacyEvents();
    }
  }
}

const InputFocus* InputProcessor::GetInputFocus(
    InputManager::DeviceType device) const {
  if (device == InputManager::kMaxNumDeviceTypes) {
    return nullptr;
  }
  auto iter = input_foci_.find(device);
  if (iter == input_foci_.end()) {
    return nullptr;
  }
  return &iter->second.current;
}

const InputFocus* InputProcessor::GetPreviousFocus(
    InputManager::DeviceType device) const {
  if (device == InputManager::kMaxNumDeviceTypes) {
    return nullptr;
  }
  auto iter = input_foci_.find(device);
  if (iter == input_foci_.end()) {
    return nullptr;
  }
  return &iter->second.previous;
}

void InputProcessor::UpdateDevice(const Clock::duration& delta_time,
                                  const InputFocus& input_focus) {
  SwapBuffers(input_focus);
  if (legacy_mode_ == kNoEvents) {
    return;
  }

  // Send events based on input_focus changes.
  UpdateFocus(input_focus.device);
  if (legacy_mode_ == kLegacyEventsAndLogic) {
    UpdateButtonsLegacy(delta_time, input_focus.device);
  } else {
    UpdateButtons(delta_time, input_focus.device);
  }
}

void InputProcessor::SwapBuffers(const InputFocus& input_focus) {
  FocusPair& focus = input_foci_[input_focus.device];
  focus.previous = focus.current;
  focus.current = input_focus;
}

void InputProcessor::UpdateFocus(InputManager::DeviceType device) {
  FocusPair& focus = input_foci_[device];
  // Target is changing, send focus events.
  const Entity current =
      focus.current.interactive ? focus.current.target : kNullEntity;
  const Entity previous =
      focus.previous.interactive ? focus.previous.target : kNullEntity;
  if (current != previous) {
    if (previous != kNullEntity) {
      SendDeviceEvent(device, kFocusStop, previous, nullptr);
    }
    if (current != kNullEntity) {
      SendDeviceEvent(device, kFocusStart, current, nullptr);
    }
  }

  // TODO(b/67940602) send more non-button specific events
}

void InputProcessor::UpdateButtons(const Clock::duration& delta_time,
                                   InputManager::DeviceType device) {
  auto input_manager = registry_->Get<InputManager>();
  const InputFocus& focus = input_foci_[device].current;
  // Send events based on Button state (click, release, etc).
  const size_t num_buttons = input_manager->GetNumButtons(device);
  for (size_t i = 0; i < num_buttons; i++) {
    const auto button_id = static_cast<InputManager::ButtonId>(i);
    const InputManager::ButtonState button =
        input_manager->GetButtonState(device, button_id);
    auto& button_state = button_states_[std::make_pair(device, button_id)];
    if (CheckBit(button, InputManager::kJustPressed)) {
      button_state.state = kInsideSlop;
      HandlePress(device, button_id, &button_state);
    } else if (CheckBit(button, InputManager::kPressed)) {
      const Entity current = focus.interactive ? focus.target : kNullEntity;
      button_state.ms_since_press +=
          std::chrono::duration_cast<std::chrono::milliseconds>(delta_time)
              .count();

      if (button_state.focused_entity != current) {
        if (button_state.state != kCanceled) {
          // Cancel the press on the previous target
          HandleCancel(device, button_id, &button_state);
        }

        // Set the button to be targeting the new focus.
        SetButtonTarget(device, &button_state);
        // Set the state to kPressedBeforeFocus to prevent Click, LongPress,
        // DragStart, or DragStop from being sent.
        button_state.state = kPressedBeforeFocus;
      }

      ButtonStates new_state = kCanceled;
      if (button_state.state != kCanceled) {
        float slop_angle = CalculateRaySlop(button_state, focus);

        if (button_state.state == kPressedBeforeFocus) {
          if (slop_angle <= kRayCancelSlop) {
            new_state = kPressedBeforeFocus;
          }
        } else {
          if (slop_angle <= kRayDragSlop) {
            new_state = kInsideSlop;
          } else if (slop_angle <= kRayCancelSlop) {
            if (focus.draggable) {
              new_state = kDragging;
            } else {
              new_state = kInsideSlop;
            }
          }
        }
      }

      if (new_state == kCanceled && button_state.state != kCanceled) {
        // Just left Cancel threshold for first time.
        HandleCancel(device, button_id, &button_state);
        button_state.state = kCanceled;
      }

      if (new_state == kDragging && button_state.state == kInsideSlop) {
        // Just left Drag threshold for first time.
        button_state.state = kDragging;
        HandleDragStart(device, button_id, &button_state);
      }

      if (CheckBit(button, InputManager::kJustLongPressed) &&
          button_state.state == kInsideSlop) {
        SendButtonEvent(device, button_id, kLongPress,
                        button_state.focused_entity, nullptr);
      }
    }
    if (CheckBit(button, InputManager::kJustReleased)) {
      // Send Release
      HandleRelease(device, button_id, button, &button_state);
      button_state.state = kReleased;
    } else if (!CheckBit(button, InputManager::kPressed) &&
               button_state.state != kReleased) {
      // button_state thinks it's pressed, but the button isn't actually
      // pressed.  This can happen if the app paused and resumed, so just
      // cancel it.
      HandleCancel(device, button_id, &button_state);
      button_state.state = kReleased;
    }
  }
}

void InputProcessor::UpdateButtonsLegacy(const Clock::duration& delta_time,
                                         InputManager::DeviceType device) {
  auto input_manager = registry_->Get<InputManager>();

  // Send events based on Button state (click, release, etc).
  const size_t num_buttons = input_manager->GetNumButtons(device);
  for (size_t i = 0; i < num_buttons; i++) {
    const auto button_id = static_cast<InputManager::ButtonId>(i);
    auto& button_state = button_states_[std::make_pair(device, button_id)];
    const InputManager::ButtonState button =
        input_manager->GetButtonState(device, button_id);
    if (CheckBit(button, InputManager::kJustPressed)) {
      HandlePress(device, button_id, &button_state);
    } else if (CheckBit(button, InputManager::kPressed)) {
      button_state.ms_since_press +=
          std::chrono::duration_cast<std::chrono::milliseconds>(delta_time)
              .count();
      if (CheckBit(button, InputManager::kJustLongPressed)) {
        HandleLongPressLegacy(device, button_id, &button_state);
      }
    } else if (CheckBit(button, InputManager::kJustReleased)) {
      HandleReleaseLegacy(device, button_id, button, &button_state);
    }
  }
}

void InputProcessor::HandlePress(InputManager::DeviceType device,
                                 InputManager::ButtonId button_id,
                                 ButtonState* button_state) {
  FocusPair& focus = input_foci_[device];
  button_state->pressed_entity =
      focus.current.interactive ? focus.current.target : kNullEntity;
  button_state->ms_since_press = 0;
  SetButtonTarget(device, button_state);
  VariantMap values;
  values.emplace(kLocationHash, button_state->pressed_location);
  SendButtonEvent(device, button_id, kPress, button_state->focused_entity,
                  &values);
}

void InputProcessor::HandleDragStart(InputManager::DeviceType device,
                                     InputManager::ButtonId button_id,
                                     ButtonState* /*button_state*/) {
  FocusPair& focus = input_foci_[device];
  const Entity current =
      focus.current.interactive ? focus.current.target : kNullEntity;

  mathfu::vec3 drag_start_location = mathfu::kZeros3f;
  if (current != kNullEntity) {
    auto transform_system = registry_->Get<TransformSystem>();
    const mathfu::mat4* world_mat =
        transform_system->GetWorldFromEntityMatrix(current);
    if (world_mat != nullptr) {
      drag_start_location =
          world_mat->Inverse() * focus.current.cursor_position;
    }
  }
  VariantMap values;
  values.emplace(kLocationHash, drag_start_location);
  SendButtonEvent(device, button_id, kDragStart, current, &values);
}

void InputProcessor::HandleRelease(InputManager::DeviceType device,
                                   InputManager::ButtonId button_id,
                                   const InputManager::ButtonState& button,
                                   ButtonState* button_state) {
  FocusPair& focus = input_foci_[device];
  const Entity current =
      focus.current.interactive ? focus.current.target : kNullEntity;

  SendButtonEvent(device, button_id, kRelease, current, nullptr);
  if (current != button_state->pressed_entity) {
    SendButtonEvent(device, button_id, kRelease, button_state->pressed_entity,
                    nullptr);
  }
  if (button_state->state == kDragging) {
    SendButtonEvent(device, button_id, kDragStop, current, nullptr);
  } else if (button_state->state == kInsideSlop &&
             button_state->focused_entity == current &&
             !CheckBit(button, InputManager::kLongPressed)) {
    VariantMap values;
    values.emplace(kDurationHash, button_state->ms_since_press);
    SendButtonEvent(device, button_id, kClick, current, &values);
  }

  ResetButton(button_state);
}

void InputProcessor::HandleCancel(InputManager::DeviceType device,
                                  InputManager::ButtonId button_id,
                                  ButtonState* button_state) {
  SendButtonEvent(device, button_id, kCancel, button_state->focused_entity,
                  nullptr);
  if (button_state->state == kDragging) {
    SendButtonEvent(device, button_id, kDragStop, button_state->focused_entity,
                    nullptr);
  }
}

void InputProcessor::HandleReleaseLegacy(
    InputManager::DeviceType device, InputManager::ButtonId button_id,
    const InputManager::ButtonState& button, ButtonState* button_state) {
  FocusPair& focus = input_foci_[device];
  const Entity current =
      focus.current.interactive ? focus.current.target : kNullEntity;

  // ReticleSystem sends release to both pressed and release entities, so need
  // to do that if emulating the old logic.
  if (current != button_state->focused_entity &&
      button_state->focused_entity != kNullEntity &&
      legacy_mode_ == kLegacyEventsAndLogic && device == GetPrimaryDevice() &&
      button_id == InputManager::kPrimaryButton) {
    auto dispatcher_system = registry_->Get<DispatcherSystem>();
    if (dispatcher_system) {
      dispatcher_system->Send(
          button_state->focused_entity,
          ClickReleasedEvent(button_state->focused_entity, current));
    }
  }

  VariantMap values;
  values.emplace(kPressedEntityHash, button_state->focused_entity);
  SendButtonEvent(device, button_id, kRelease, current, &values);

  // TODO(b/67940602) need to only send click if within touch slop / cancel
  // threshold.
  if (button_state->focused_entity == current &&
      !CheckBit(button, InputManager::kLongPressed)) {
    VariantMap click_values;
    click_values.emplace(kDurationHash, button_state->ms_since_press);
    SendButtonEvent(device, button_id, kClick, button_state->focused_entity,
                    &click_values);
  }
  ResetButton(button_state);
}

void InputProcessor::HandleLongPressLegacy(InputManager::DeviceType device,
                                           InputManager::ButtonId button_id,
                                           ButtonState* button_state) {
  FocusPair& focus = input_foci_[device];
  const Entity current =
      focus.current.interactive ? focus.current.target : kNullEntity;
  if (button_state->focused_entity == current) {
    SendButtonEvent(device, button_id, kLongPress, current, nullptr);
  }
}

void InputProcessor::SetButtonTarget(InputManager::DeviceType device,
                                     ButtonState* button_state) {
  FocusPair& focus = input_foci_[device];
  button_state->focused_entity =
      focus.current.interactive ? focus.current.target : kNullEntity;

  button_state->pressed_location = mathfu::kZeros3f;
  if (button_state->state != kReleased &&
      button_state->focused_entity != kNullEntity) {
    auto transform_system = registry_->Get<TransformSystem>();
    const mathfu::mat4* world_mat = transform_system->GetWorldFromEntityMatrix(
        button_state->focused_entity);
    if (world_mat != nullptr) {
      button_state->pressed_location =
          world_mat->Inverse() * focus.current.cursor_position;
    } else {
      LOG(DFATAL) << "no world matrix on focused_entity";
    }
  }
}

void InputProcessor::ResetButton(ButtonState* button_state) {
  button_state->pressed_entity = kNullEntity;
  button_state->focused_entity = kNullEntity;
  button_state->pressed_location = mathfu::kZeros3f;
  button_state->ms_since_press = 0;
}

float InputProcessor::CalculateRaySlop(const ButtonState& button_state,
                                       const InputFocus& focus) const {
  // convert the local space collision location to world space:
  const auto* transform_system = registry_->Get<TransformSystem>();
  if (button_state.focused_entity == kNullEntity) {
    return 0;
  }
  const mathfu::mat4* world_mat =
      transform_system->GetWorldFromEntityMatrix(button_state.focused_entity);
  if (world_mat == nullptr) {
    return std::numeric_limits<float>::max();
  }
  const mathfu::vec3 pressed_location_in_world_space =
      (*world_mat) * button_state.pressed_location;

  const mathfu::vec3 source_to_original =
      pressed_location_in_world_space - focus.collision_ray.origin;
  const mathfu::vec3 source_to_current =
      focus.no_hit_cursor_position - focus.collision_ray.origin;
  return mathfu::vec3::Angle(source_to_original, source_to_current);
}

void InputProcessor::SetPrimaryDevice(InputManager::DeviceType device) {
  primary_device_ = device;
}

InputManager::DeviceType InputProcessor::GetPrimaryDevice() const {
  return primary_device_;
}

void InputProcessor::SetPrefix(InputManager::DeviceType device,
                               string_view prefix) {
  DeviceEvents& events = device_events_[device];
  SetupDeviceEvents(prefix, &events);
}

void InputProcessor::SetPrefix(InputManager::DeviceType device,
                               InputManager::ButtonId button,
                               string_view prefix) {
  ButtonEvents& events = button_events_[std::make_pair(device, button)];
  SetupButtonEvents(prefix, &events);
}

void InputProcessor::ClearPrefix(InputManager::DeviceType device) {
  device_events_.erase(device);
}

void InputProcessor::ClearPrefix(InputManager::DeviceType device,
                                 InputManager::ButtonId button) {
  button_events_.erase(std::make_pair(device, button));
}

void InputProcessor::SendDeviceEvent(InputManager::DeviceType device,
                                     DeviceEventType event_type, Entity target,
                                     const VariantMap* values) {
  auto iter = device_events_.find(device);
  if (iter != device_events_.end()) {
    // Send events with a specific prefix.
    SendEvent(iter->second, event_type, target, device,
              InputManager::kInvalidButton, values);
  }

  // Send generic events.
  SendEvent(any_device_events_, event_type, target, device,
            InputManager::kInvalidButton, values);

  if (legacy_mode_ != kNoLegacy && device == GetPrimaryDevice() &&
      legacy_device_events_.events[event_type] != 0) {
    SendEvent(legacy_device_events_, event_type, target, device,
              InputManager::kInvalidButton, values);
  }
}

void InputProcessor::SendButtonEvent(InputManager::DeviceType device,
                                     InputManager::ButtonId button,
                                     ButtonEventType event_type, Entity target,
                                     const VariantMap* values) {
  auto iter = button_events_.find(std::make_pair(device, button));
  if (iter != button_events_.end()) {
    // Send events with a specific prefix.
    SendEvent(iter->second, event_type, target, device, button, values);
  }

  // Send generic events.
  SendEvent(any_button_events_, event_type, target, device, button, values);

  if (legacy_mode_ != kNoLegacy && device == GetPrimaryDevice() &&
      button == InputManager::kPrimaryButton &&
      legacy_button_events_.events[event_type] != 0) {
    if (event_type == kLongPress) {
      // TODO(b/68854711) remove this special case when old global events are
      // supported here.
      // Need to only send this locally, due to it already being sent by the old
      // input_processor logic.
      auto dispatcher_system = registry_->Get<DispatcherSystem>();
      if (dispatcher_system && target != kNullEntity) {
        dispatcher_system->Send(target, PrimaryButtonLongPress());
      }
    } else {
      SendEvent(legacy_button_events_, event_type, target, device, button,
                values);
    }
  }
}

template <typename EventSet, typename EventType>
void InputProcessor::SendEvent(const EventSet& event_set, EventType event_type,
                               Entity target, InputManager::DeviceType device,
                               InputManager::ButtonId button,
                               const VariantMap* values) {
#if LULLABY_TRACK_EVENT_NAMES
  EventWrapper event(event_set.events[event_type], event_set.names[event_type]);
#else
  EventWrapper event(event_set.events[event_type]);
#endif
  if (values != nullptr) {
    event.SetValues(*values);
  }
  event.SetValue(kEntityHash, target);
  event.SetValue(kTargetHash, target);
  event.SetValue<InputManager::DeviceType>(kDeviceHash, device);
  if (button != InputManager::kInvalidButton) {
    event.SetValue<InputManager::ButtonId>(kButtonHash, button);
  }

  auto dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Send(event);

  if (target != kNullEntity) {
    auto dispatcher_system = registry_->Get<DispatcherSystem>();
    if (dispatcher_system) {
      dispatcher_system->Send(target, event);
    }
  }
}

// A macro to expand an event list into a switch statement that returns the name
// as a string..
#define LULLABY_GENERATE_SWITCH_CASE(Enum, Name) \
  case Enum:                                     \
    return #Name;

const char* InputProcessor::GetDeviceEventName(DeviceEventType type) {
  switch (type) {
    LULLABY_DEVICE_EVENT_LIST(LULLABY_GENERATE_SWITCH_CASE)
    default:
      DCHECK(false) << "Invalid event type " << type;
      return "InvalidEvent";
  }
}
const char* InputProcessor::GetButtonEventName(ButtonEventType type) {
  switch (type) {
    LULLABY_BUTTON_EVENT_LIST(LULLABY_GENERATE_SWITCH_CASE)
    default:
      DCHECK(false) << "Invalid event type " << type;
      return "InvalidEvent";
  }
}
#undef LULLABY_GENERATE_SWITCH_CASE

void InputProcessor::SetupDeviceEvents(const string_view prefix,
                                       DeviceEvents* events) {
  HashValue prefix_hash = Hash(prefix);
  for (size_t index = 0; index < kNumDeviceEventTypes; index++) {
    const DeviceEventType type = static_cast<DeviceEventType>(index);
    events->events[index] = Hash(prefix_hash, GetDeviceEventName(type));
#if LULLABY_TRACK_EVENT_NAMES
    events->names[index] = prefix.to_string() + GetDeviceEventName(type);
#endif
  }
}

void InputProcessor::SetupButtonEvents(const string_view prefix,
                                       ButtonEvents* events) {
  HashValue prefix_hash = Hash(prefix);
  for (size_t index = 0; index < kNumButtonEventTypes; index++) {
    const ButtonEventType type = static_cast<ButtonEventType>(index);
    events->events[index] = Hash(prefix_hash, GetButtonEventName(type));
#if LULLABY_TRACK_EVENT_NAMES
    events->names[index] = prefix.to_string() + GetButtonEventName(type);
#endif
  }
}

void InputProcessor::SetupLegacyEvents() {
  legacy_button_events_.events[kPress] = GetTypeId<ClickEvent>();
  legacy_button_events_.events[kRelease] = GetTypeId<ClickReleasedEvent>();
  legacy_button_events_.events[kClick] =
      GetTypeId<ClickPressedAndReleasedEvent>();
  legacy_button_events_.events[kLongPress] =
      GetTypeId<PrimaryButtonLongPress>();
  legacy_device_events_.events[kFocusStart] = GetTypeId<StartHoverEvent>();
  legacy_device_events_.events[kFocusStop] = GetTypeId<StopHoverEvent>();

#if LULLABY_TRACK_EVENT_NAMES
  legacy_button_events_.names[kPress] = GetTypeName<ClickEvent>();
  legacy_button_events_.names[kRelease] = GetTypeName<ClickReleasedEvent>();
  legacy_button_events_.names[kClick] =
      GetTypeName<ClickPressedAndReleasedEvent>();
  legacy_button_events_.names[kLongPress] =
      GetTypeName<PrimaryButtonLongPress>();
  legacy_device_events_.names[kFocusStart] = GetTypeName<StartHoverEvent>();
  legacy_device_events_.names[kFocusStop] = GetTypeName<StopHoverEvent>();
#endif
}
}  // namespace lull
