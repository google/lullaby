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

#ifndef REDUX_ENGINES_PLATFORM_MOUSE_H_
#define REDUX_ENGINES_PLATFORM_MOUSE_H_

#include "redux/engines/platform/buffered_state.h"
#include "redux/engines/platform/device_profiles.h"
#include "redux/engines/platform/virtual_device.h"

namespace redux {

class Mouse : public VirtualDevice {
 public:
  using Profile = MouseProfile;
  using Button = MouseProfile::Button;

  // Records the active buttons of the mouse.
  void SetButton(Button button, TriggerFlag state);

  // Records the position of the mouse.
  void SetPosition(const vec2i& value);

  // Records the amount the scroll wheel has been moved.
  void SetScrollDelta(int delta);

  // The state of the mouse that will be exposed by the device manager.
  struct View : VirtualView<Mouse> {
    const Profile* GetProfile() const;
    vec2i GetPosition() const;
    int GetScrollValue() const;
    TriggerFlag GetButtonState(Button button) const;
  };

 private:
  friend class DeviceManager;
  Mouse(MouseProfile profile, OnDestroy on_destroy);

  void Apply(absl::Duration delta_time) override;

  int GetIndexForButton(Button button) const;

  struct MouseState {
    vec2i position = vec2i::Zero();
    int scroll_value = 0;
    std::vector<BooleanState> buttons;
  };

  MouseProfile profile_;
  detail::BufferedState<MouseState> state_;
  absl::Duration timestamp_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_MOUSE_H_
