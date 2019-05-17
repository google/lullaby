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

#ifndef LULLABY_VIEWER_SRC_IMGUI_BRIDGE_H_
#define LULLABY_VIEWER_SRC_IMGUI_BRIDGE_H_

// Minimum requirement: OpenGL 3.2 / shader version 150.

#include <SDL.h>
#include <stdint.h>
#include <functional>
#include <string>
#include <vector>

namespace lull {
namespace tool {

class ImguiBridge {
 public:
  ImguiBridge() {}
  ~ImguiBridge() {}

  struct FontInfo {
    struct Entry {
      std::string path;
      float size;
      // Range of characters to include from this font.  Format is pairs of
      // characters, terminated by a 0.
      std::vector<uint16_t> ranges;
    };
    std::vector<Entry> entries;
  };

  void Initialize(SDL_Window* sdl_window, const std::vector<FontInfo>& fonts);

  void Shutdown();

  void ProcessSdlEvent(const SDL_Event* event);

  void Update(double dt, const std::function<void()>& gui_fn);

 private:
  void InitializeImgui();
  void ShutdownImgui();

  void InitializeGl();
  void ShutdownGl();

  void InitializeFontTexture(const std::vector<FontInfo>& fonts);
  void ShutdownFontTexture();

  void PrepareImgui(double dt);
  void RenderImgui();

  SDL_Window* sdl_window_ = nullptr;
  uint32_t shader_ = 0;
  uint32_t font_texture_ = 0;
  uint32_t vao_ = 0;
  uint32_t vbo_ = 0;
  uint32_t elements_ = 0;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_VIEWER_SRC_IMGUI_BRIDGE_H_
