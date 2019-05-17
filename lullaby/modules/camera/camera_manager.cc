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

#include "lullaby/modules/camera/camera_manager.h"

#include <algorithm>

namespace lull {

void CameraManager::RegisterCamera(CameraPtr camera,
                                   RenderTargetId render_target) {
  std::vector<CameraPtr>& cam_vec = cameras_[render_target];
  if (std::find(cam_vec.begin(), cam_vec.end(), camera) != cam_vec.end()) {
    DCHECK(false) << "Camera is already registered for render_target: "
                  << render_target;
  } else {
    cam_vec.push_back(camera);
  }
}

void CameraManager::RegisterScreenCamera(CameraPtr camera) {
  RegisterCamera(camera, screen_render_target_);
}

void CameraManager::UnregisterCamera(CameraPtr camera,
                                     RenderTargetId render_target) {
  auto map_iter = cameras_.find(render_target);
  if (map_iter != cameras_.end()) {
    std::vector<CameraPtr>& cam_vec = map_iter->second;
    auto vec_iter = std::find(cam_vec.begin(), cam_vec.end(), camera);
    if (vec_iter != cam_vec.end()) {
      cam_vec.erase(vec_iter);
    }
    if (cam_vec.empty()) {
      cameras_.erase(render_target);
    }
  }
}

void CameraManager::UnregisterScreenCamera(CameraPtr camera) {
  UnregisterCamera(camera, screen_render_target_);
}

const CameraList* CameraManager::GetCameras(
    RenderTargetId render_target) const {
  auto map_iter = cameras_.find(render_target);
  if (map_iter != cameras_.end()) {
    return &map_iter->second;
  }
  return nullptr;
}

const CameraList* CameraManager::GetScreenCameras() const {
  return GetCameras(screen_render_target_);
}

void CameraManager::SetScreenRenderTarget(RenderTargetId render_target) {
  screen_render_target_ = render_target;
}

Camera* CameraManager::GetCameraByTargetPixel(
    RenderTargetId render_target, const mathfu::vec2& target_pixel) const {
  auto map_iter = cameras_.find(render_target);
  if (map_iter == cameras_.end()) {
    return nullptr;
  }
  for (const CameraPtr& camera : map_iter->second) {
    const mathfu::recti viewport = camera->Viewport();
    // TODO this check may need to apply device orientation?
    if (target_pixel.x >= viewport.pos.x && target_pixel.y >= viewport.pos.y &&
        target_pixel.x <= viewport.pos.x + viewport.size.x &&
        target_pixel.y <= viewport.pos.y + viewport.size.y) {
      return camera.get();
    }
  }
  return nullptr;
}

Optional<Ray> CameraManager::WorldRayFromTargetPixel(
    RenderTargetId render_target, const mathfu::vec2& target_pixel) const {
  const Camera* camera = GetCameraByTargetPixel(render_target, target_pixel);
  if (camera == nullptr) {
    LOG(ERROR) << "No camera for target pixel";
    return Optional<Ray>();
  }
  return camera->WorldRayFromPixel(target_pixel);
}

size_t CameraManager::GetNumCamerasForTarget(
    RenderTargetId render_target) const {
  auto map_iter = cameras_.find(render_target);
  if (map_iter == cameras_.end()) {
    return 0;
  } else {
    return map_iter->second.size();
  }
}

void CameraManager::PopulateRenderViewsForTarget(RenderTargetId render_target,
                                                 RenderView* views,
                                                 size_t num_views) const {
  if (num_views == 0) {
    return;
  }
  auto map_iter = cameras_.find(render_target);
  if (map_iter == cameras_.end()) {
    DCHECK(false) << "No Cameras registered for RenderTarget: "
                  << render_target;
    return;
  }
  size_t index = 0;
  for (const CameraPtr& camera : map_iter->second) {
    if (index >= num_views) {
      DCHECK(false) << "Not enough views. Render Target: " << render_target
                    << " num_views: " << num_views;
      return;
    }
    camera->PopulateRenderView(&views[index]);
    ++index;
  }
  DCHECK(index == num_views)
      << "Too many views. Render Target: " << render_target
      << " num_views: " << num_views;
}

Camera* CameraManager::GetCameraByScreenPixel(
    const mathfu::vec2& screen_pixel) const {
  return GetCameraByTargetPixel(screen_render_target_, screen_pixel);
}

Optional<Ray> CameraManager::WorldRayFromScreenPixel(
    const mathfu::vec2& screen_pixel) const {
  return WorldRayFromTargetPixel(screen_render_target_, screen_pixel);
}

Optional<Ray> CameraManager::WorldRayFromScreenUV(
    const mathfu::vec2& screen_uv) const {
  Optional<mathfu::vec2> screen_pixel = PixelFromScreenUV(screen_uv);
  if (!screen_pixel) {
    return Optional<Ray>();
  }
  return WorldRayFromTargetPixel(screen_render_target_, screen_pixel.value());
}

size_t CameraManager::GetNumCamerasForScreen() const {
  return GetNumCamerasForTarget(screen_render_target_);
}

void CameraManager::PopulateRenderViewsForScreen(RenderView* views,
                                                 size_t num_views) const {
  return PopulateRenderViewsForTarget(screen_render_target_, views, num_views);
}

Optional<mathfu::vec2> CameraManager::PixelFromScreenUV(
    mathfu::vec2 screen_uv) const {
  const CameraList* cameras_ptr = GetScreenCameras();
  if (cameras_ptr) {
    const CameraList& cameras = *cameras_ptr;
    const mathfu::recti& first_viewport = cameras[0]->Viewport();
    mathfu::vec2i min_vec = first_viewport.pos;
    mathfu::vec2i max_vec = first_viewport.pos + first_viewport.size;
    // If there are more than 1 screen cameras, merge them into a single rect.
    for (size_t i = 1; i < cameras.size(); ++i) {
      const mathfu::recti& viewport = cameras[i]->Viewport();
      min_vec.x = std::min(viewport.pos.x, min_vec.x);
      min_vec.y = std::min(viewport.pos.y, min_vec.y);
      max_vec.x = std::max(viewport.pos.x + viewport.size.x, max_vec.x);
      max_vec.y = std::max(viewport.pos.y + viewport.size.y, max_vec.y);
    }
    // Have to do this piecewise because we're combining int and float vectors.
    return mathfu::vec2(min_vec.x + max_vec.x * screen_uv.x,
                        min_vec.y + max_vec.y * screen_uv.y);
  }
  return Optional<mathfu::vec2>();
}

}  // namespace lull
