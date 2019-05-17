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

#ifndef LULLABY_VIEWER_SRC_WIDGETS_PREVIEW_WINDOW_H_
#define LULLABY_VIEWER_SRC_WIDGETS_PREVIEW_WINDOW_H_

#include "lullaby/util/registry.h"
#include "lullaby/util/math.h"

namespace lull {
namespace tool {

class PreviewWindow {
 public:
  PreviewWindow(Registry* registry, size_t width, size_t height);
  ~PreviewWindow();

  void AdvanceFrame();

 private:
  void CheckInput();
  void Render();

  Registry* registry_ = nullptr;
  mathfu::vec3 translation_ = {0, 0, 2};  // Position the camera a little back.
  mathfu::vec3 rotation_ = {0, 0, 0};
  uint32_t framebuffer_ = 0;
  uint32_t depthbuffer_ = 0;
  uint32_t texture_ = 0;
  size_t width_ = 0;
  size_t height_ = 0;
};

}  // namespace tool
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::tool::PreviewWindow);

#endif  // LULLABY_VIEWER_WIDGETS_PREVIEW_WINDOW_H_
