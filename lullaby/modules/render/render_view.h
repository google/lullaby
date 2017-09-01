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

#ifndef LULLABY_MODULES_RENDER_RENDER_VIEW_H_
#define LULLABY_MODULES_RENDER_RENDER_VIEW_H_

#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/span.h"

namespace lull {

// Structure representing a viewport used for rendering.
struct RenderView {
  static const float kDefaultNearClipPlane;
  static const float kDefaultFarClipPlane;

  /// The offset of the viewport.
  mathfu::vec2i viewport;
  /// The size of the viewport in pixels.
  mathfu::vec2i dimensions;
  /// The camera's world position. The inverse of this is the view matrix.
  mathfu::mat4 world_from_eye_matrix;
  /// The projection matrix.
  mathfu::mat4 clip_from_eye_matrix;
  /// The combined view projection matrix.
  mathfu::mat4 clip_from_world_matrix;
  /// The eye this view renders to. 0 = left, 1 = right. For monoscopic
  /// rendering leave this at 0.
  InputManager::EyeType eye = 1;
};

// Populates the RenderView arrays using information from the InputManager.
void PopulateRenderViews(Registry* registry, RenderView* views, size_t num,
                         float near_clip_plane, float far_clip_plane);

// Similar to above, but allows for explicit render target size and clip planes
// to be used.
void PopulateRenderViews(Registry* registry, RenderView* views, size_t num,
                         float near_clip_plane, float far_clip_plane,
                         const mathfu::vec2i& render_target_size);

// Similar to above, but allows for a default near/far clip planes to be used.
inline void PopulateRenderViews(Registry* registry, RenderView* views,
                                size_t num) {
  PopulateRenderViews(registry, views, num, RenderView::kDefaultNearClipPlane,
                      RenderView::kDefaultFarClipPlane);
}

// Similar to above, but allows for explicit render target size to be used.
inline void PopulateRenderViews(Registry* registry, RenderView* views,
                                size_t num,
                                const mathfu::vec2i& render_target_size) {
  PopulateRenderViews(registry, views, num, RenderView::kDefaultNearClipPlane,
                      RenderView::kDefaultFarClipPlane, render_target_size);
}

/// Copies |views| to |eye_centered_views| but modifies the transform matrices
/// so that eye_from_world translation is zeroed.  This is useful for rendering
/// objects like spherical panoramas which should stay centered around the eye
/// regardless of eye translation but should still rotate due to changes in
/// eye orientation.
void GenerateEyeCenteredViews(Span<RenderView> views,
                              RenderView* eye_centered_views);

}  // namespace lull

#endif  // LULLABY_MODULES_RENDER_RENDER_VIEW_H_
