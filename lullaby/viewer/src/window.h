/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_VIEWER_SRC_WINDOW_H_
#define LULLABY_VIEWER_SRC_WINDOW_H_

#include <SDL.h>
#include <stdint.h>
#include <string>

#include "lullaby/viewer/src/imgui_bridge.h"

namespace lull {
namespace tool {

class Window {
 public:
  Window() {}
  virtual ~Window() {}

  struct InitParams {
    size_t width;
    size_t height;
    std::string label;
  };

  void Initialize(const InitParams& params);
  void Update();
  void Shutdown();

  bool ShouldQuit() const { return quit_; }
  int GetExitCode() const { return exit_code_; }

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

 protected:
  virtual void OnInitialize() {}
  virtual void AdvanceFrame(double dt, int width, int height) = 0;
  virtual void OnShutdown() {}

  void Exit(const std::string& message, int exit_code);

 private:
  void Render(void* ptr);

  void InitializeSdl(const std::string& name, size_t width, size_t height);
  void ShutdownSdl();

  void ProcessSdlEvent(const SDL_Event* event);

  void* sdl_context_;
  SDL_Window* sdl_window_ = nullptr;

  ImguiBridge imgui_bridge_;
  double time_ = 0.f;
  bool quit_ = false;
  int exit_code_ = 0;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_VIEWER_SRC_WINDOW_H_
