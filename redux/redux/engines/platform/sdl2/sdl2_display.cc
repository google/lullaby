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

#include "redux/engines/platform/sdl2/sdl2_display.h"

#include "SDL_syswm.h"
#include "redux/engines/platform/device_manager.h"

#if defined(__APPLE__)
extern void* GetNativeWindowOsx(void* window);
#endif

namespace redux {

std::unique_ptr<Sdl2Display> Sdl2Display::Create(DeviceManager* dm,
                                                 std::string_view title,
                                                 const vec2i& size) {
  auto window = SDL_CreateWindow(title.data(), SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED, size.x, size.y, 0);

  auto display = std::make_unique<Sdl2Display>();
  display->OnWindowCreated(dm, window, size);
  return display;
}

std::unique_ptr<Sdl2Display> Sdl2Display::CreateHeadless(DeviceManager* dm,
                                                         const vec2i& size) {
  auto window =
      SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       size.x, size.y, SDL_WINDOW_HIDDEN);

  auto display = std::make_unique<Sdl2Display>();
  display->OnWindowCreated(dm, window, size);
  return display;
}

Sdl2Display::~Sdl2Display() {
  display_.reset();
  SDL_DestroyWindow(window_);
}

void Sdl2Display::OnWindowCreated(DeviceManager* dm, SDL_Window* window,
                                  const vec2i& size) {
  CHECK(window != nullptr);
  window_ = window;

  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  SDL_GetWindowWMInfo(window_, &wmi);

#if defined(__linux__)
  native_window_ = reinterpret_cast<void*>(wmi.info.x11.window);
#elif defined(MSC_VER)
  native_window_ = reinterpret_cast<void*>(wmi.info.win.window);
#elif defined(__APPLE__)
  // Looks like a bug in SDL? https://hg.libsdl.org/SDL/rev/ab7529cb9558
  SDL_SetWindowSize(window_, size.x / 2, size.y / 2);
  native_window_ =
      GetNativeWindowOsx(reinterpret_cast<void*>(wmi.info.cocoa.window));
#endif

  DisplayProfile profile;
  profile.native_window = native_window_;
  profile.display_size = size;
  display_ = dm->Connect(profile);
}

void Sdl2Display::Commit() {
  int w = 0;
  int h = 0;
  SDL_GetWindowSize(window_, &w, &h);
  display_->SetSize(vec2i(w, h));

#if defined(__APPLE__)
  SDL_GL_GetDrawableSize(window_, &w, &h);
  display_->SetSize(vec2i(w, h));
#endif
}
}  // namespace redux
