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

#include "redux/engines/platform/keyboard.h"

namespace redux {

Keyboard::Keyboard(KeyboardProfile profile, OnDestroy on_destroy)
    : VirtualDevice(std::move(on_destroy)), profile_(std::move(profile)) {
  KeyboardState init;
  state_.Initialize(init);
}

void Keyboard::Apply(absl::Duration delta_time) {
  state_.Commit();
  state_.GetMutable().keys.reset();
  state_.GetMutable().modifier = KEYMOD_NONE;
}

void Keyboard::PressKey(KeyCode code) { state_.GetMutable().keys[code] = true; }

void Keyboard::SetModifierState(KeyModifier modifier) {
  state_.GetMutable().modifier = modifier;
}

const KeyboardProfile* Keyboard::View::GetProfile() const {
  const Keyboard* device = GetDevice();
  return device ? &device->profile_ : nullptr;
}

Keyboard::TriggerFlag Keyboard::View::GetKeyState(KeyCode code) const {
  const Keyboard* device = GetDevice();
  if (device) {
    const bool curr = device->state_.GetCurrent().keys[code];
    const bool prev = device->state_.GetPrevious().keys[code];
    return DetermineTrigger(curr, prev);
  }
  return kReleased;
}

std::string Keyboard::View::GetPressedKeys() const {
  const Keyboard* device = GetDevice();
  if (device) {
    return Chord(device->state_.GetCurrent().keys,
                 device->state_.GetCurrent().modifier);
  }
  return "";
}

KeyModifier Keyboard::View::GetModifierState() const {
  const Keyboard* device = GetDevice();
  return device ? device->state_.GetCurrent().modifier : KEYMOD_NONE;
}

}  // namespace redux
