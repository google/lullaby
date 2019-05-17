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

#include "lullaby/viewer/src/window.h"

#include <SDL.h>
#include "dear_imgui/imgui.h"
#include "fplbase/glplatform.h"
#include "fplbase/utilities.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace tool {

void Window::Update() {
  const double current_time = SDL_GetTicks() / 1000.0;
  double delta_time = 1.0 / 60.0;
  if (time_ > 0.0) {
    delta_time = current_time - time_;
  }
  time_ = current_time;

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ProcessSdlEvent(&event);
  }

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  imgui_bridge_.Update(delta_time, [=]() {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(sdl_window_, &width, &height);

    SDL_GL_MakeCurrent(sdl_window_, sdl_context_);

    AdvanceFrame(delta_time, width, height);
  });

  SDL_GL_SwapWindow(sdl_window_);
}

void Window::Initialize(const InitParams& params) {
  InitializeSdl(params.label, params.width, params.height);

  std::vector<lull::tool::ImguiBridge::FontInfo> fonts(1);
  lull::tool::ImguiBridge::FontInfo& font = fonts[0];
  font.entries.push_back({"", 13.0f, {}});
  font.entries.push_back(
      {"fontawesome-webfont.ttf", 13.0f, {0xf000, 0xf3ff, 0}});
  imgui_bridge_.Initialize(sdl_window_, fonts);

  OnInitialize();
}

void Window::Shutdown() {
  OnShutdown();
  imgui_bridge_.Shutdown();
  ShutdownSdl();
}

void Window::InitializeSdl(const std::string& name, size_t width,
                           size_t height) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    LOG(ERROR) << "Could not initialize SDL.";
    return;
  }

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  sdl_window_ = SDL_CreateWindow(
      name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width,
      height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if (sdl_window_ == nullptr) {
    LOG(ERROR) << "Could not create window.";
    return;
  }

  sdl_context_ = SDL_GL_CreateContext(sdl_window_);
  SDL_GL_SetSwapInterval(1);

  fplbase::ChangeToUpstreamDir("./", "assets");
}

void Window::ShutdownSdl() {
  SDL_GL_DeleteContext(sdl_context_);
  SDL_DestroyWindow(sdl_window_);
  SDL_Quit();
}

void Window::ProcessSdlEvent(const SDL_Event* event) {
  imgui_bridge_.ProcessSdlEvent(event);
  switch (event->type) {
    case SDL_QUIT: {
      quit_ = true;
      break;
    }
  }
}

void Window::Exit(const std::string& message, int exit_code) {
  if (exit_code != 0) {
    LOG(ERROR) << message;
    exit_code_ = exit_code;
  }
  quit_ = true;
}

}  // namespace tool
}  // namespace lull
