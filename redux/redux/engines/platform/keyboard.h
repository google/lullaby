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

#ifndef REDUX_ENGINES_PLATFORM_KEYBOARD_H_
#define REDUX_ENGINES_PLATFORM_KEYBOARD_H_

#include "redux/engines/platform/buffered_state.h"
#include "redux/engines/platform/device_profiles.h"
#include "redux/engines/platform/keycodes.h"
#include "redux/engines/platform/virtual_device.h"

namespace redux {

class Keyboard : public VirtualDevice {
 public:
  using Profile = KeyboardProfile;

  // Records a single keycode as being pressed. Key presses are valid only for a
  // single frame and will be cleared on the next frame. We assume client code
  // will handle key repeats on their own.
  void PressKey(KeyCode code);

  // Records the active modifier keys that are set. The entire state must be
  // set here (i.e. the modifiers should be bit-wise or'ed together).
  void SetModifierState(KeyModifier modifier);

  // The state of the keyboard that will be exposed by the device manager.
  struct View : VirtualView<Keyboard> {
    const Profile* GetProfile() const;
    TriggerFlag GetKeyState(KeyCode code) const;
    std::string GetPressedKeys() const;
    KeyModifier GetModifierState() const;
  };

 private:
  friend class DeviceManager;
  Keyboard(KeyboardProfile profile, OnDestroy on_destroy);

  void Apply(absl::Duration delta_time) override;

  struct KeyboardState {
    KeycodeBitset keys;
    KeyModifier modifier;
  };

  KeyboardProfile profile_;
  detail::BufferedState<KeyboardState> state_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_KEYBOARD_H_
