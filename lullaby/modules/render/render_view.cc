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

#include "lullaby/modules/render/render_view.h"

#include "lullaby/util/math.h"

namespace lull {
namespace {

mathfu::vec4i GetEyeViewportBounds(InputManager::EyeType eye,
                                   mathfu::vec2i render_target_size) {
  if (render_target_size.x == 0) {
    LOG(DFATAL) << "Invalid render_target_size";
  }

  const int index = static_cast<int>(eye);
  const int half_width = render_target_size.x / 2;

  return mathfu::vec4i(
      mathfu::vec2i(index * half_width, 0),
      mathfu::vec2i((index + 1) * half_width, render_target_size.y));
}

// Helper for pulling data out of the input manager.  The lambda helps callers
// provide different methods of setting the viewport for a given eye.
using GetViewportFn = std::function<mathfu::recti(InputManager::EyeType)>;
void PopulateRenderViews(Registry* registry, RenderView* views, size_t num,
                         float near_clip_plane, float far_clip_plane,
                         const GetViewportFn& get_viewport_fn) {
  if (!registry) {
    LOG(DFATAL) << "PopulateRenderViews called without valid registry.";
    return;
  }
  auto* input_manager = registry->Get<InputManager>();
  const mathfu::mat4 start_from_head_transform =
      input_manager->GetDofWorldFromObjectMatrix(InputManager::kHmd);
  for (size_t i = 0; i < num; ++i) {
    const InputManager::EyeType eye = static_cast<InputManager::EyeType>(i);

    const mathfu::mat4 eye_from_head_transform =
        input_manager->GetEyeFromHead(InputManager::kHmd, eye);
    const mathfu::mat4 head_from_eye_transform =
        eye_from_head_transform.Inverse();
    const mathfu::mat4 world_from_eye_matrix =
        start_from_head_transform * head_from_eye_transform;

    const mathfu::rectf fov = input_manager->GetEyeFOV(InputManager::kHmd, eye);

    PopulateRenderView(&views[i], get_viewport_fn(eye), world_from_eye_matrix,
                       fov, near_clip_plane, far_clip_plane, eye);
  }
}

}  // namespace

const float RenderView::kDefaultNearClipPlane = 0.2f;
const float RenderView::kDefaultFarClipPlane = 1000.f;

void PopulateRenderView(RenderView* view, const mathfu::recti& viewport,
                        const mathfu::mat4& world_from_eye_matrix,
                        const mathfu::rectf& fov, float near_clip_plane,
                        float far_clip_plane, InputManager::EyeType eye) {
  const mathfu::mat4 clip_from_eye_matrix =
      CalculatePerspectiveMatrixFromView(fov, near_clip_plane, far_clip_plane);

  PopulateRenderView(view, viewport, world_from_eye_matrix,
                     clip_from_eye_matrix, fov, eye);
}

void PopulateRenderView(RenderView* view, const mathfu::recti& viewport,
                        const mathfu::mat4& world_from_eye_matrix,
                        const mathfu::mat4& clip_from_eye_matrix,
                        const mathfu::rectf& fov, InputManager::EyeType eye) {
  view->viewport = viewport.pos;
  view->dimensions = viewport.size;
  view->world_from_eye_matrix = world_from_eye_matrix;
  view->eye_from_world_matrix = view->world_from_eye_matrix.Inverse();
  view->clip_from_eye_matrix = clip_from_eye_matrix;
  view->clip_from_world_matrix =
      view->clip_from_eye_matrix * view->eye_from_world_matrix;
  view->eye = eye;
}

void PopulateRenderViews(Registry* registry, RenderView* views, size_t num,
                         float near_clip_plane, float far_clip_plane) {
  if (!registry) {
    LOG(DFATAL) << "PopulateRenderViews called without valid registry.";
    return;
  }
  auto* input_manager = registry->Get<InputManager>();
  PopulateRenderViews(registry, views, num, near_clip_plane, far_clip_plane,
                      [input_manager](InputManager::EyeType eye) {
    return input_manager->GetEyeViewport(InputManager::kHmd, eye);
  });
}

void PopulateRenderViews(Registry* registry, RenderView* views, size_t num,
                         float near_clip_plane, float far_clip_plane,
                         const mathfu::vec2i& render_target_size) {
  if (!registry) {
    LOG(DFATAL) << "PopulateRenderViews called without valid registry.";
    return;
  }
  PopulateRenderViews(registry, views, num, near_clip_plane, far_clip_plane,
                      [&render_target_size](InputManager::EyeType eye) {
                        const mathfu::vec4i viewport_bounds =
                            GetEyeViewportBounds(eye, render_target_size);
                        const mathfu::vec2i min_point =
                            mathfu::vec2i(viewport_bounds.x, viewport_bounds.y);
                        const mathfu::vec2i max_point =
                            mathfu::vec2i(viewport_bounds.z, viewport_bounds.w);
                        return mathfu::recti(min_point, max_point - min_point);
                      });
}

void PopulateRenderViewsFromInputManager(Registry* registry, RenderView* views,
                                         size_t num) {
  if (!registry) {
    LOG(DFATAL) << "PopulateRenderViews called without valid registry.";
    return;
  }

  auto* input_manager = registry->Get<InputManager>();
  const mathfu::mat4 start_from_head_transform =
      input_manager->GetDofWorldFromObjectMatrix(InputManager::kHmd);
  for (size_t i = 0; i < num; ++i) {
    RenderView& view = views[i];
    const InputManager::EyeType eye = static_cast<InputManager::EyeType>(i);

    const mathfu::mat4 eye_from_head_transform =
        input_manager->GetEyeFromHead(InputManager::kHmd, eye);
    const mathfu::mat4 head_from_eye_transform =
        eye_from_head_transform.Inverse();
    const mathfu::mat4 world_from_eye_matrix =
        start_from_head_transform * head_from_eye_transform;

    const mathfu::recti& viewport =
        input_manager->GetEyeViewport(InputManager::kHmd, eye);
    const mathfu::mat4 clip_from_eye_matrix =
        input_manager->GetScreenFromEye(InputManager::kHmd, eye);
    const mathfu::rectf fov = input_manager->GetEyeFOV(InputManager::kHmd, eye);

    PopulateRenderView(&view, viewport, world_from_eye_matrix,
                       clip_from_eye_matrix, fov, eye);
  }
}

void GenerateEyeCenteredViews(Span<RenderView> views,
                              RenderView* eye_centered_views) {
  for (size_t index = 0; index < views.size(); ++index) {
    eye_centered_views[index] = views[index];
    eye_centered_views[index].world_from_eye_matrix[12] = 0.f;
    eye_centered_views[index].world_from_eye_matrix[13] = 0.f;
    eye_centered_views[index].world_from_eye_matrix[14] = 0.f;
    eye_centered_views[index].clip_from_world_matrix =
        views[index].clip_from_eye_matrix *
        eye_centered_views[index].world_from_eye_matrix.Inverse();
  }
}

}  // namespace lull
