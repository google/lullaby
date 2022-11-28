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

#ifndef REDUX_ENGINES_PLATFORM_SDL2_SDL2_DISPLAY_H_
#define REDUX_ENGINES_PLATFORM_SDL2_SDL2_DISPLAY_H_

#include "redux/engines/platform/device_manager.h"
#include "redux/engines/platform/display.h"
#include "redux/engines/platform/sdl2/sdl2_event_handler.h"

namespace redux {

class Sdl2Display : public Sdl2EventHandler {
 public:
  // Creates an SDL2 window with given title and size.
  static std::unique_ptr<Sdl2Display> Create(DeviceManager* dm,
                                             std::string_view title,
                                             const vec2i& size);

  // Creates a "headless" SDL2 display of the given size.
  static std::unique_ptr<Sdl2Display> CreateHeadless(DeviceManager* dm,
                                                     const vec2i& size);

  ~Sdl2Display() override;

  void Commit() override;

 private:
  void OnWindowCreated(DeviceManager* dm, SDL_Window* window,
                       const vec2i& size);

  SDL_Window* window_ = nullptr;
  void* native_window_ = nullptr;
  std::unique_ptr<Display> display_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_SDL2_SDL2_DISPLAY_H_
