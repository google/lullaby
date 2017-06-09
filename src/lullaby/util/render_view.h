/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_UTIL_RENDER_VIEW_H_
#define LULLABY_UTIL_RENDER_VIEW_H_

#include "lullaby/base/input_manager.h"
#include "lullaby/base/registry.h"

namespace lull {

// Structure representing a viewport used for rendering.
struct RenderView {
  mathfu::vec2i viewport;
  mathfu::vec2i dimensions;
  mathfu::mat4 world_from_eye_matrix;
  mathfu::mat4 clip_from_eye_matrix;
  mathfu::mat4 clip_from_world_matrix;
  InputManager::EyeType eye = 1;
};

// Populates the RenderView arrays using the HMD data.
void PopulateRenderViews(Registry* registry, RenderView* views, size_t num,
                         float near_clip_plane, float far_clip_plane,
                         const mathfu::vec2i& render_target_size);

inline void PopulateRenderViews(Registry* registry, RenderView* views,
                                size_t num,
                                const mathfu::vec2i& render_target_size) {
  const float near_clip_plane = 0.2f;
  const float far_clip_plane = 1000.f;
  PopulateRenderViews(registry, views, num, near_clip_plane, far_clip_plane,
                      render_target_size);
}

}  // namespace lull

#endif  // LULLABY_UTIL_RENDER_VIEW_H_
