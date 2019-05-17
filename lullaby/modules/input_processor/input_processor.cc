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

#include "lullaby/modules/input_processor/input_processor.h"

#include <limits>  // std::numeric_limits

#include "lullaby/events/input_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/bits.h"

namespace lull {
namespace {

// TODO These values need to be configurable on a system wide level.
// The angle in radians between the InputFocus's collision_ray and a ray from
// the collision_ray's origin to the current cursor_position.
constexpr float kRayDragSlop = 2.0f * kDegreesToRadians;
constexpr float kRayCancelSlop = 35.0f * kDegreesToRadians;
constexpr float kTouchCancelSlop = .1f;
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
    SetupTouchEvents(kAnyPrefix, &any_touch_events_);
    if (legacy_mode != kNoLegacy) {
      SetupLegacyEvents();
    }
  }
}

InputProcessor* InputProcessor::Create(Registry* registry,
                                       LegacyMode legacy_mode) {
  return registry->Create<InputProcessor>(registry, legacy_mode);
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

GesturePtr InputProcessor::GetTouchOwner(InputManager::DeviceType device,
                                         InputManager::TouchpadId touchpad,
                                         InputManager::TouchId id) {
  Touchpad& touchpad_state = touchpad_states_[std::make_pair(device, touchpad)];
  auto iter = touchpad_state.touches.find(id);
  if (iter == touchpad_state.touches.end()) {
    return nullptr;
  } else {
    return iter->second.owner;
  }
}

void InputProcessor::UpdateDevice(const Clock::duration& delta_time,
                                  const InputFocus& input_focus) {
  SwapBuffers(input_focus);
  if (legacy_mode_ == kNoEvents) {
    return;
  }

  // If the input processor is overridden, then we run the topmost instead
  // the current one.
  if (!override_input_processors_.empty()) {
    override_input_processors_.back()->UpdateDevice(delta_time, input_focus);
    return;
  }

  const InputManager::DeviceType device = input_focus.device;
  const InputManager::TouchpadId touchpad = InputManager::kPrimaryTouchpadId;
  // Send events based on input_focus changes.
  UpdateFocus(device);
  if (legacy_mode_ == kLegacyEventsAndLogic) {
    UpdateButtonsLegacy(delta_time, device);
  } else {
    UpdateButtons(delta_time, device);
  }

  auto input_manager = registry_->Get<InputManager>();
  if (input_manager->HasTouchpad(device, touchpad)) {
    UpdateTouches(delta_time, device, touchpad);
    UpdateTouchGestures(delta_time, device, touchpad);
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

  // TODO need to only send click if within touch slop / cancel
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

void InputProcessor::UpdateTouches(const Clock::duration& delta_time,
                                   InputManager::DeviceType device,
                                   InputManager::TouchpadId touchpad) {
  const auto input_manager = registry_->Get<InputManager>();
  std::vector<InputManager::TouchId> touch_ids =
      input_manager->GetTouches(device, touchpad);

  Touchpad& touchpad_state = touchpad_states_[std::make_pair(device, touchpad)];

  // Find any new touches.
  for (auto id : touch_ids) {
    auto iter = touchpad_state.touches.find(id);
    if (iter == touchpad_state.touches.end()) {
      // Found a new touch.
      auto insert_pair = touchpad_state.touches.emplace(id, Touch());
      Touch& touch = insert_pair.first->second;
      HandleTouchPress(device, touchpad, id, &touch);
      touch.state = kInsideSlop;
    }
  }
  int64_t delta_time_int =
      std::chrono::duration_cast<std::chrono::milliseconds>(delta_time).count();

  // Update existing touches.
  for (auto& iter : touchpad_state.touches) {
    InputManager::TouchId id = iter.first;
    InputManager::TouchState touch_state =
        input_manager->GetTouchState(device, touchpad, id);
    Touch& touch = iter.second;
    // If not new and not ended
    if (CheckBit(touch_state, InputManager::kPressed) &&
        !CheckBit(touch_state, InputManager::kJustPressed)) {
      UpdateTouch(delta_time_int, device, touchpad, id, touch_state);
    } else if (CheckBit(touch_state, InputManager::kReleased)) {
      if (CheckBit(touch_state, InputManager::kJustReleased)) {
        HandleTouchRelease(device, touchpad, id, &touch, touch_state);
        touch.state = kReleased;
      } else {
        // Found a touch that input_manager doesn't know about. This often
        // happens if the app paused and resumed, so just cancel it.
        touch.state = kCanceled;
        HandleTouchCancel(device, touchpad, id, &touch);
        if (touch.owner) {
          touch.owner->Cancel();
        }
      }
    }
  }
  // Remove old Touches.
  // Iterate in a way that allows us to remove touches that have ended.
  for (auto iter = touchpad_state.touches.begin();
       iter != touchpad_state.touches.end();) {
    ButtonStates state = iter->second.state;
    if (state == kReleased || state == kCanceled) {
      iter = touchpad_state.touches.erase(iter);
    } else {
      ++iter;
    }
  }
}

void InputProcessor::UpdateTouch(const int64_t delta_time,
                                 InputManager::DeviceType device,
                                 InputManager::TouchpadId touchpad,
                                 InputManager::TouchId id,
                                 InputManager::TouchState touch_state) {
  const auto input_manager = registry_->Get<InputManager>();
  const InputFocus& focus = input_foci_[device].current;
  Touchpad& touchpad_state = touchpad_states_[std::make_pair(device, touchpad)];
  Touch& touch = touchpad_state.touches[id];
  const Entity current = focus.interactive ? focus.target : kNullEntity;
  touch.ms_since_press += delta_time;

  if (touch.focused_entity != current) {
    if (touch.state != kCanceled) {
      HandleTouchCancel(device, touchpad, id, &touch);
    }

    SetButtonTarget(device, &touch);
    touch.state = kPressedBeforeFocus;
  }
  // If the app isn't using gestures, use the legacy gesture code:
  // TODO convert the below to be gestures set by
  // StandardInputPipeline.
  if (touchpad_state.recognizers.empty()) {
    ButtonStates new_state = kCanceled;
    if (touch.state != kCanceled) {
      float slop_angle = CalculateRaySlop(touch, focus);

      if (touch.state == kPressedBeforeFocus) {
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

    // Canceled by ray slop
    if (new_state == kCanceled && touch.state != kCanceled) {
      HandleTouchCancel(device, touchpad, id, &touch);
      touch.state = kCanceled;
    }

    // Check for moving outside touch slop
    if (new_state != kCanceled && touch.state != kReleased &&
        touch.state != kTouchMoved) {
      mathfu::vec2 touch_position =
          input_manager->GetTouchLocation(device, touchpad, id);
      mathfu::vec2 start_position =
          input_manager->GetTouchGestureOrigin(device, touchpad, id);
      float touch_move_distance = (touch_position - start_position).Length();
      if (touch_move_distance > kTouchCancelSlop) {
        HandleTouchSwipeStart(device, touchpad, id, &touch);
        touch.state = kTouchMoved;
      }
    }

    if (new_state == kDragging && touch.state == kInsideSlop) {
      touch.state = kDragging;
      HandleTouchDragStart(device, touchpad, id, &touch);
    }
  }

  if (CheckBit(touch_state, InputManager::kJustLongPressed) &&
      touch.state == kInsideSlop) {
    SendTouchEvent(device, touchpad, id, kTouchLongPress, touch.focused_entity,
                   nullptr);
  }
}

void InputProcessor::UpdateTouchGestures(const Clock::duration& delta_time,
                                         InputManager::DeviceType device,
                                         InputManager::TouchpadId touchpad) {
  const auto input_manager = registry_->Get<InputManager>();
  Touchpad& touchpad_state = touchpad_states_[std::make_pair(device, touchpad)];
  if (touchpad_state.recognizers.empty()) {
    return;
  }
  const Optional<mathfu::vec2> touchpad_size =
      input_manager->GetTouchpadSize(device, touchpad);
  if (!touchpad_size) {
    LOG(DFATAL) << "Warning: Touch Gestures require a touchpad size.";
    return;
  }

  // for each Recognizer
  //   for each tuple of pressed touches
  //     TryCreate()
  Gesture::TouchIdVector touches;
  for (auto recognizer : touchpad_state.recognizers) {
    recognizer->SetTouchpadSize(touchpad_size.value());
    const size_t num_touches = recognizer->GetNumTouches();
    touches.resize(num_touches);
    if (num_touches == 1) {
      for (auto& pair : touchpad_state.touches) {
        touches[0] = pair.first;
        GesturePtr gesture = recognizer->TryStart(device, touchpad, touches);
        if (gesture != nullptr) {
          HandleGestureStart(device, touchpad, touches, gesture, recognizer);
        }
      }
    } else if (num_touches == 2) {
      // Note: these loops will call TryStart will all unique pairs of
      // touches.
      for (auto first_iter = touchpad_state.touches.begin();
           first_iter != touchpad_state.touches.end(); ++first_iter) {
        touches[0] = first_iter->first;
        auto second_iter = first_iter;
        ++second_iter;
        for (; second_iter != touchpad_state.touches.end(); ++second_iter) {
          touches[1] = second_iter->first;
          GesturePtr gesture = recognizer->TryStart(device, touchpad, touches);
          if (gesture != nullptr) {
            HandleGestureStart(device, touchpad, touches, gesture, recognizer);
          }
        }
      }
    } else {
      DCHECK(false) << "Currently only support 1 or 2 finger gestures";
    }
  }

  // Update gestures and delete finished gestures.
  for (auto iter = touchpad_state.gestures.begin();
       iter != touchpad_state.gestures.end();) {
    GesturePtr& gesture = *iter;
    Gesture::State gesture_state = gesture->AdvanceFrame(delta_time);
    if (gesture_state == Gesture::kCanceled) {
      HandleGestureEnd(device, touchpad, gesture, kGestureCancel);
      iter = touchpad_state.gestures.erase(iter);
    } else if (gesture_state == Gesture::kEnding) {
      HandleGestureEnd(device, touchpad, gesture, kGestureStop);
      iter = touchpad_state.gestures.erase(iter);
    } else {
      // Gesture is running.
      ++iter;
    }
  }
}

void InputProcessor::CancelAllGestures(const Clock::duration& delta_time,
                                       InputManager::DeviceType device,
                                       InputManager::TouchpadId touchpad) {
  auto pair = std::make_pair(device, touchpad);
  auto& touchpad_state = touchpad_states_[pair];
  // Cancel, then update, then end all gestures.
  for (auto& gesture : touchpad_state.gestures) {
    gesture->Cancel();
    Gesture::State gesture_state = gesture->AdvanceFrame(delta_time);
    // A canceled gesture should return a canceled state.
    DCHECK(gesture_state == Gesture::kCanceled);
    HandleGestureEnd(device, touchpad, gesture, kGestureCancel);
  }
  touchpad_state.gestures.clear();
}

void InputProcessor::HandleGestureStart(InputManager::DeviceType device,
                                        InputManager::TouchpadId touchpad,
                                        Gesture::TouchIdSpan ids,
                                        GesturePtr gesture,
                                        GestureRecognizerPtr recognizer) {
  Touchpad& touchpad_state = touchpad_states_[std::make_pair(device, touchpad)];
  touchpad_state.gestures.emplace_back(gesture);

  gesture->Setup(registry_, recognizer->GetHash(), device, touchpad, ids,
                 recognizer->GetTouchpadSize());
  for (InputManager::TouchId id : ids) {
    auto iter = touchpad_state.touches.find(id);
    if (iter != touchpad_state.touches.end()) {
      iter->second.owner = gesture;
      iter->second.state = kGesturing;
    }
  }

  FocusPair& focus = input_foci_[device];
  Entity target =
      focus.current.interactive ? focus.current.target : kNullEntity;
  VariantMap values = gesture->GetEventValues();
  SendGestureEvent(device, touchpad, gesture, kGestureStart, target, &values);
}

void InputProcessor::HandleGestureEnd(InputManager::DeviceType device,
                                      InputManager::TouchpadId touchpad,
                                      GesturePtr gesture,
                                      GestureEventType event_type) {
  auto* input_manager = registry_->Get<InputManager>();
  FocusPair& focus = input_foci_[device];
  Entity target =
      focus.current.interactive ? focus.current.target : kNullEntity;
  VariantMap values = gesture->GetEventValues();
  SendGestureEvent(device, touchpad, gesture, event_type, target, &values);

  Touchpad& touchpad_state = touchpad_states_[std::make_pair(device, touchpad)];
  for (auto& iter : touchpad_state.touches) {
    if (iter.second.owner == gesture) {
      iter.second.owner = nullptr;
      input_manager->ResetTouchGestureOrigin(device, touchpad, iter.first);
    }
  }
}

void InputProcessor::HandleTouchPress(InputManager::DeviceType device,
                                      InputManager::TouchpadId touchpad,
                                      InputManager::TouchId id, Touch* touch) {
  FocusPair& focus = input_foci_[device];
  const mathfu::vec2 touch_start_position =
      registry_->Get<InputManager>()->GetTouchLocation(device, touchpad, id);
  touch->pressed_entity =
      focus.current.interactive ? focus.current.target : kNullEntity;
  touch->ms_since_press = 0;
  SetButtonTarget(device, touch);
  VariantMap values;
  values.emplace(kLocationHash, touch->pressed_location);
  values.emplace(kTouchLocationHash, touch_start_position);
  SendTouchEvent(device, touchpad, id, kTouchPress, touch->focused_entity,
                 &values);
}

void InputProcessor::HandleTouchRelease(
    InputManager::DeviceType device, InputManager::TouchpadId touchpad,
    InputManager::TouchId id, Touch* touch,
    const InputManager::TouchState& touch_state) {
  FocusPair& focus = input_foci_[device];
  const Entity current =
      focus.current.interactive ? focus.current.target : kNullEntity;
  SendTouchEvent(device, touchpad, id, kTouchRelease, current, nullptr);
  if (current != touch->pressed_entity) {
    SendTouchEvent(device, touchpad, id, kTouchRelease, touch->pressed_entity,
                   nullptr);
  }
  if (touch->state == kDragging) {
    SendTouchEvent(device, touchpad, id, kTouchDragStop, current, nullptr);
  }
  if (touch->state != kGesturing && touch->state == kTouchMoved) {
    SendTouchEvent(device, touchpad, id, kSwipeStop, current, nullptr);
  }

  if (touch->state == kInsideSlop &&
      !CheckBit(touch_state, InputManager::kLongPressed)) {
    VariantMap values;
    values.emplace(kDurationHash, touch->ms_since_press);
    SendTouchEvent(device, touchpad, id, kTouchClick, current, &values);
  }
  ResetTouch(touch);
}

void InputProcessor::HandleTouchCancel(InputManager::DeviceType device,
                                       InputManager::TouchpadId touchpad,
                                       InputManager::TouchId id, Touch* touch) {
  SendTouchEvent(device, touchpad, id, kTouchCancel, touch->focused_entity,
                 nullptr);
  if (touch->state == kDragging) {
    SendTouchEvent(device, touchpad, id, kTouchDragStop, touch->focused_entity,
                   nullptr);
  }

  if (touch->state != kGesturing && touch->state == kTouchMoved) {
    SendTouchEvent(device, touchpad, id, kSwipeStop, touch->focused_entity,
                   nullptr);
  }
}

void InputProcessor::HandleTouchDragStart(InputManager::DeviceType device,
                                          InputManager::TouchpadId touchpad,
                                          InputManager::TouchId id,
                                          Touch* touch) {
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
  SendTouchEvent(device, touchpad, id, kTouchDragStart, current, &values);
}

void InputProcessor::HandleTouchSwipeStart(InputManager::DeviceType device,
                                           InputManager::TouchpadId touchpad,
                                           InputManager::TouchId id,
                                           Touch* touch) {
  FocusPair& focus = input_foci_[device];
  const Entity current =
      focus.current.interactive ? focus.current.target : kNullEntity;
  mathfu::vec2 touch_move_start_location = mathfu::kZeros2f;
  VariantMap values;
  values.emplace(kLocationHash, touch_move_start_location);
  SendTouchEvent(device, touchpad, id, kSwipeStart, current, &values);
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

void InputProcessor::ResetTouch(Touch* touch) {
  ResetButton(touch);
  touch->state = kReleased;
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

void InputProcessor::SetTouchPrefix(InputManager::DeviceType device,
                                    InputManager::TouchpadId touchpad,
                                    string_view prefix) {
  auto pair = std::make_pair(device, touchpad);
  touchpad_prefixes_[pair] = prefix.to_string();
  TouchEvents& events = touch_events_[device];
  SetupTouchEvents(prefix, &events);
  SetupGestureEvents(prefix, &touchpad_states_[pair]);
}

void InputProcessor::SetTouchGestureRecognizers(
    InputManager::DeviceType device, InputManager::TouchpadId touchpad,
    GestureRecognizerList recognizers) {
  auto pair = std::make_pair(device, touchpad);
  if (!touchpad_states_[pair].gestures.empty()) {
    CancelAllGestures(Clock::duration::zero(), device, touchpad);
  }
  touchpad_states_[pair].recognizers = recognizers;
  SetupGestureEvents(touchpad_prefixes_[pair], &touchpad_states_[pair]);
}

void InputProcessor::ClearPrefix(InputManager::DeviceType device) {
  device_events_.erase(device);
}

void InputProcessor::ClearPrefix(InputManager::DeviceType device,
                                 InputManager::ButtonId button) {
  button_events_.erase(std::make_pair(device, button));
}

void InputProcessor::ClearTouchPrefix(InputManager::DeviceType device,
                                      InputManager::TouchpadId touchpad) {
  touchpad_prefixes_.erase(std::make_pair(device, touchpad));
  touch_events_.erase(device);
}

void InputProcessor::SendDeviceEvent(InputManager::DeviceType device,
                                     DeviceEventType event_type, Entity target,
                                     const VariantMap* values) {
  auto iter = device_events_.find(device);
  if (iter != device_events_.end()) {
    // Send events with a specific prefix.
    SendEvent(iter->second, event_type, target, device, values);
  }

  // Send generic events.
  SendEvent(any_device_events_, event_type, target, device, values);

  if (legacy_mode_ != kNoLegacy && device == GetPrimaryDevice() &&
      legacy_device_events_.events[event_type] != 0) {
    SendEvent(legacy_device_events_, event_type, target, device, values);
  }
}

void InputProcessor::SendGestureEvent(InputManager::DeviceType device,
                                      InputManager::TouchpadId touchpad,
                                      const GesturePtr gesture,
                                      GestureEventType event_type,
                                      Entity target, VariantMap* values) {
  VariantMap event_values;
  if (values == nullptr) {
    values = &event_values;
  }
  values->emplace(kTouchpadIdHash, touchpad);

  Gesture::TouchIdVector ids = gesture->GetTouches();
  DCHECK(ids.size() <= kMaxTouchesPerGesture);
  for (int i = 0; i < ids.size() && i < kMaxTouchesPerGesture; ++i) {
    values->emplace(kTouchIdHashes[i], ids[i]);
  }

  Touchpad& touchpad_state = touchpad_states_[std::make_pair(device, touchpad)];
  auto iter = touchpad_state.events.find(gesture->GetHash());
  if (iter != touchpad_state.events.end()) {
    SendEvent(iter->second, event_type, target, device, values);
  }
  iter = touchpad_state.any_events.find(gesture->GetHash());
  if (iter != touchpad_state.any_events.end()) {
    SendEvent(iter->second, event_type, target, device, values);
  }
}

void InputProcessor::SendTouchEvent(InputManager::DeviceType device,
                                    InputManager::TouchpadId touchpad,
                                    InputManager::TouchId id,
                                    TouchEventType event_type, Entity target,
                                    VariantMap* values) {
  VariantMap touch_event_values;
  if (values == nullptr) {
    values = &touch_event_values;
  }
  values->emplace(kTouchpadIdHash, touchpad);
  values->emplace(kTouchIdHash, id);
  auto iter = touch_events_.find(device);
  if (iter != touch_events_.end()) {
    SendEvent(iter->second, event_type, target, device, values);
  }
  SendEvent(any_touch_events_, event_type, target, device, values);
}

void InputProcessor::SendButtonEvent(InputManager::DeviceType device,
                                     InputManager::ButtonId button,
                                     ButtonEventType event_type, Entity target,
                                     VariantMap* values) {
  VariantMap event_values;
  if (values == nullptr) {
    values = &event_values;
  }
  values->emplace(kButtonHash, button);

  auto iter = button_events_.find(std::make_pair(device, button));
  if (iter != button_events_.end()) {
    // Send events with a specific prefix.
    SendEvent(iter->second, event_type, target, device, values);
  }

  // Send generic events.
  SendEvent(any_button_events_, event_type, target, device, values);

  if (legacy_mode_ != kNoLegacy && device == GetPrimaryDevice() &&
      button == InputManager::kPrimaryButton &&
      legacy_button_events_.events[event_type] != 0) {
    if (event_type == kLongPress) {
      // TODO remove this special case when old global events are
      // supported here.
      // Need to only send this locally, due to it already being sent by the old
      // input_processor logic.
      auto dispatcher_system = registry_->Get<DispatcherSystem>();
      if (dispatcher_system && target != kNullEntity) {
        dispatcher_system->Send(target, PrimaryButtonLongPress());
      }
    } else {
      SendEvent(legacy_button_events_, event_type, target, device, values);
    }
  }
}

template <typename EventSet, typename EventType>
void InputProcessor::SendEvent(const EventSet& event_set, EventType event_type,
                               Entity target, InputManager::DeviceType device,
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

const char* InputProcessor::GetTouchEventName(TouchEventType type) {
  switch (type) {
    LULLABY_TOUCH_EVENT_LIST(LULLABY_GENERATE_SWITCH_CASE)
    default:
      DCHECK(false) << "Invalid event type " << type;
      return "InvalidEvent";
  }
}

const char* InputProcessor::GetGestureEventName(GestureEventType type) {
  switch (type) {
    LULLABY_GESTURE_EVENT_LIST(LULLABY_GENERATE_SWITCH_CASE)
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
    events->names[index] = prefix + GetDeviceEventName(type);
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
    events->names[index] = prefix + GetButtonEventName(type);
#endif
  }
}

void InputProcessor::SetupTouchEvents(const string_view prefix,
                                      TouchEvents* events) {
  HashValue prefix_hash = Hash(prefix);
  for (size_t index = 0; index < kNumTouchEventTypes; index++) {
    const TouchEventType type = static_cast<TouchEventType>(index);
    events->events[index] = Hash(prefix_hash, GetTouchEventName(type));
#if LULLABY_TRACK_EVENT_NAMES
    events->names[index] = prefix + GetTouchEventName(type);
#endif
  }
}

void InputProcessor::SetupGestureEvents(const string_view prefix,
                                        Touchpad* touchpad) {
  HashValue prefix_hash = Hash(prefix);
  touchpad->events.clear();
  touchpad->any_events.clear();
  for (auto& recognizer : touchpad->recognizers) {
    string_view gesture_name = recognizer->GetName();
    HashValue gesture_hash = recognizer->GetHash();
    for (size_t index = 0; index < kNumGestureEventTypes; index++) {
      const GestureEventType type = static_cast<GestureEventType>(index);
      const char* event_name = GetGestureEventName(type);
      touchpad->events[gesture_hash].events[index] =
          Hash(prefix_hash, gesture_name + event_name);
      touchpad->any_events[gesture_hash].events[index] =
          Hash(kAnyPrefix + gesture_name + event_name);
#if LULLABY_TRACK_EVENT_NAMES
      touchpad->events[gesture_hash].names[index] =
          prefix + gesture_name + event_name;
      touchpad->any_events[gesture_hash].names[index] =
          kAnyPrefix + gesture_name + event_name;
#endif
    }
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

void InputProcessor::AddOverrideProcessor(
    const std::shared_ptr<InputProcessor>& processor) {
  if (processor == nullptr) {
    LOG(DFATAL) << "Invalid input to AddOverrideProcessor: nullptr";
    return;
  }
  override_input_processors_.push_back(processor);
}

void InputProcessor::RemoveOverrideProcessor(
    const std::shared_ptr<InputProcessor>& processor) {
  override_input_processors_.erase(
      std::remove(override_input_processors_.begin(),
                  override_input_processors_.end(), processor),
      override_input_processors_.end());
}
}  // namespace lull
