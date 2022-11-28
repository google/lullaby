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

#ifndef REDUX_ENGINES_PLATFORM_SDL2_SDL2_MAINLOOP_H_
#define REDUX_ENGINES_PLATFORM_SDL2_SDL2_MAINLOOP_H_

#include <memory>
#include <string_view>
#include <vector>

#include "SDL.h"
#include "redux/engines/platform/mainloop.h"
#include "redux/engines/platform/sdl2/sdl2_event_handler.h"

namespace redux {

class Sdl2Mainloop : public Mainloop {
 public:
  Sdl2Mainloop();
  ~Sdl2Mainloop() override;

  void CreateDisplay(std::string_view title, vec2i size) override;
  void CreateKeyboard() override;
  void CreateMouse() override;
  void CreateSpeaker() override;

 private:
  absl::StatusCode PollEvents() override;

  std::vector<std::unique_ptr<Sdl2EventHandler>> handlers_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_SDL2_SDL2_MAINLOOP_H_
