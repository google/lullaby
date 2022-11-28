/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include "redux/engines/platform/mouse.h"

#include <utility>

#include "redux/modules/base/logging.h"

namespace redux {

Mouse::Mouse(MouseProfile profile, OnDestroy on_destroy)
    : VirtualDevice(std::move(on_destroy)), profile_(std::move(profile)) {
  MouseState init;
  init.buttons.resize(profile_.num_buttons);
  state_.Initialize(init);
}

void Mouse::Apply(absl::Duration delta_time) {
  timestamp_ += delta_time;
  state_.Commit();
}

void Mouse::SetButton(Button button, TriggerFlag state) {
  CHECK(state == kPressed || state == kReleased);
  const bool active = (state & kPressed);

  const int idx = GetIndexForButton(button);
  auto& button_state = state_.GetMutable().buttons[idx];
  if (button_state.active != active) {
    button_state.toggle_time = timestamp_;
  }
  button_state.active = active;
}

void Mouse::SetPosition(const vec2i& value) {
  state_.GetMutable().position = value;
}

void Mouse::SetScrollDelta(int delta) {
  CHECK(profile_.has_scroll_wheel);
  if (profile_.has_scroll_wheel) {
    state_.GetMutable().scroll_value += delta;
  }
}

const MouseProfile* Mouse::View::GetProfile() const {
  const Mouse* device = GetDevice();
  return device ? &device->profile_ : nullptr;
}

vec2i Mouse::View::GetPosition() const {
  const Mouse* mouse = GetDevice();
  return mouse ? mouse->state_.GetCurrent().position : vec2i::Zero();
}

int Mouse::View::GetScrollValue() const {
  const Mouse* mouse = GetDevice();
  return mouse ? mouse->state_.GetCurrent().scroll_value : 0;
}

Mouse::TriggerFlag Mouse::View::GetButtonState(Button button) const {
  const Mouse* mouse = GetDevice();
  if (mouse) {
    const int idx = mouse->GetIndexForButton(button);
    const auto& curr = mouse->state_.GetCurrent().buttons[idx];
    const auto& prev = mouse->state_.GetPrevious().buttons[idx];
    return DetermineTrigger(curr, prev, mouse->profile_.long_press_time_ms);
  }
  return kReleased;
}

int Mouse::GetIndexForButton(Button button) const {
  CHECK(profile_.button_map.empty());
  const int index = static_cast<int>(button);
  CHECK(index < profile_.num_buttons);
  return index;
}

}  // namespace redux
