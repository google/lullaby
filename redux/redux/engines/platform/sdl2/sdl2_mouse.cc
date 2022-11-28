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

#include "redux/engines/platform/sdl2/sdl2_mouse.h"

#include "SDL_events.h"
#include "redux/engines/platform/device_manager.h"

namespace redux {

Sdl2Mouse::Sdl2Mouse(DeviceManager* dm) {
  MouseProfile profile;
  profile.num_buttons = 3;
  profile.has_scroll_wheel = true;
  mouse_ = dm->Connect(profile);
}

Sdl2Mouse::~Sdl2Mouse() { mouse_.reset(); }

static std::optional<MouseProfile::Button> ButtonType(SDL_Event event) {
  if (event.button.button == SDL_BUTTON_LEFT) {
    return MouseProfile::kLeftButton;
  } else if (event.button.button == SDL_BUTTON_RIGHT) {
    return MouseProfile::kRightButton;
  } else if (event.button.button == SDL_BUTTON_MIDDLE) {
    return MouseProfile::kMiddleButton;
  } else if (event.button.button == SDL_BUTTON_X1) {
    return MouseProfile::kForwardButton;
  } else if (event.button.button == SDL_BUTTON_X2) {
    return MouseProfile::kBackButton;
  } else {
    return std::nullopt;
  }
}

void Sdl2Mouse::HandleEvent(SDL_Event event) {
  if (event.type == SDL_MOUSEBUTTONDOWN) {
    auto button = ButtonType(event);
    if (button) {
      mouse_->SetButton(button.value(), Mouse::kPressed);
    }
  } else if (event.type == SDL_MOUSEBUTTONUP) {
    auto button = ButtonType(event);
    if (button) {
      mouse_->SetButton(button.value(), Mouse::kReleased);
    }
  } else if (event.type == SDL_MOUSEWHEEL) {
    mouse_->SetScrollDelta(event.wheel.x);
  }
}

void Sdl2Mouse::Commit() {
  int x = 0;
  int y = 0;
  SDL_GetMouseState(&x, &y);
  mouse_->SetPosition(vec2i(x, y));
}
}  // namespace redux
