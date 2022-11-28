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

#include "redux/engines/platform/sdl2/sdl2_mainloop.h"

#include <memory>

#include "redux/engines/platform/sdl2/sdl2_display.h"
#include "redux/engines/platform/sdl2/sdl2_keyboard.h"
#include "redux/engines/platform/sdl2/sdl2_mouse.h"
#include "redux/engines/platform/sdl2/sdl2_speaker.h"

namespace redux {

std::unique_ptr<Mainloop> Mainloop::Create() {
  auto ptr = new Sdl2Mainloop();
  return std::unique_ptr<Mainloop>(ptr);
}

Sdl2Mainloop::Sdl2Mainloop() {
  const auto init = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
  CHECK_EQ(init, 0);
}

Sdl2Mainloop::~Sdl2Mainloop() { SDL_Quit(); }

void Sdl2Mainloop::CreateDisplay(std::string_view title, vec2i size) {
  auto device_manager = registry_.Get<DeviceManager>();
  handlers_.emplace_back(Sdl2Display::Create(device_manager, title, size));
}

void Sdl2Mainloop::CreateKeyboard() {
  auto device_manager = registry_.Get<DeviceManager>();
  handlers_.emplace_back(new Sdl2Keyboard(device_manager));
}

void Sdl2Mainloop::CreateMouse() {
  auto device_manager = registry_.Get<DeviceManager>();
  handlers_.emplace_back(new Sdl2Mouse(device_manager));
}

void Sdl2Mainloop::CreateSpeaker() {
  auto device_manager = registry_.Get<DeviceManager>();
  handlers_.emplace_back(new Sdl2Speaker(device_manager));
}

absl::StatusCode Sdl2Mainloop::PollEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      return absl::StatusCode::kCancelled;
    } else if (event.type == SDL_APP_WILLENTERBACKGROUND) {
      return absl::StatusCode::kCancelled;
    }

    for (auto& iter : handlers_) {
      iter->HandleEvent(event);
    }
  }
  for (auto& iter : handlers_) {
    iter->Commit();
  }
  return absl::StatusCode::kOk;
}

}  // namespace redux
