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

#ifndef REDUX_ENGINES_PLATFORM_DISPLAY_H_
#define REDUX_ENGINES_PLATFORM_DISPLAY_H_

#include "redux/engines/platform/buffered_state.h"
#include "redux/engines/platform/device_profiles.h"
#include "redux/engines/platform/virtual_device.h"

namespace redux {

// Represents the visual device on which graphical rendering will be performed.
//
// For desktops, this represents the window.
class Display : public VirtualDevice {
 public:
  using Profile = DisplayProfile;

  // Records the dimensions of the display.
  void SetSize(const vec2i& size);

  // The state of the display that will be exposed by the device manager.
  struct View : VirtualView<Display> {
    const Profile* GetProfile() const;
    vec2i GetSize() const;
  };

 private:
  friend class DeviceManager;
  Display(DisplayProfile profile, OnDestroy on_destroy);

  void Apply(absl::Duration delta_time) override;

  struct DisplayState {
    vec2i size = vec2i::Zero();
  };

  DisplayProfile profile_;
  detail::BufferedState<DisplayState> state_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_DISPLAY_H_
