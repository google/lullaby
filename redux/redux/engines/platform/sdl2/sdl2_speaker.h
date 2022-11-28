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

#ifndef REDUX_ENGINES_PLATFORM_SDL2_SDL2_SPEAKER_H_
#define REDUX_ENGINES_PLATFORM_SDL2_SDL2_SPEAKER_H_

#include "redux/engines/platform/device_manager.h"
#include "redux/engines/platform/sdl2/sdl2_event_handler.h"
#include "redux/engines/platform/speaker.h"

namespace redux {

class Sdl2Speaker : public Sdl2EventHandler {
 public:
  explicit Sdl2Speaker(DeviceManager* dm);

  ~Sdl2Speaker() override;

  void Commit() override;

 private:
  DeviceManager* device_manager_;
  std::unique_ptr<Speaker> speaker_;
  SDL_AudioDeviceID sdl_device_id_;
  SDL_AudioSpec sdl_audio_spec_;
};

}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_SDL2_SDL2_SPEAKER_H_
