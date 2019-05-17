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

#ifndef LULLABY_MODULES_CAMERA_CAMERA_MANAGER_H_
#define LULLABY_MODULES_CAMERA_CAMERA_MANAGER_H_

#include <vector>

#include "lullaby/modules/camera/camera.h"

namespace lull {

/// A class to track multiple cameras, and ease converting between screen space
/// and world space.
class CameraManager {
 public:
  using RenderTargetId = HashValue;

  /// By default, cameras are assumed to render to 0, and CameraManager assumes
  /// that render_target == 0 means it is being rendered to the screen.
  static const RenderTargetId kDefaultScreenRenderTarget = 0;
  /// A RenderTargetId for cameras that are not actually used for rendering.
  static const RenderTargetId kNoRenderTarget = ConstHash("NoTarget");

  /// Register a |camera| to a |render_target|.
  void RegisterCamera(CameraPtr camera, RenderTargetId render_target);

  /// Register a |camera| to the default render_target.
  void RegisterScreenCamera(CameraPtr camera);

  /// Remove a |camera| from a |render_target|.
  void UnregisterCamera(CameraPtr camera, RenderTargetId render_target);

  /// Remove a |camera| from the default render_target.
  void UnregisterScreenCamera(CameraPtr camera);

  /// Returns all cameras that are rendering to a given |render_target|.
  const CameraList* GetCameras(RenderTargetId render_target) const;

  /// Returns all cameras that are rendering to the screen.
  const CameraList* GetScreenCameras() const;

  /// If using a custom |render_target| id for your main render target, set that
  /// here so that the *ScreenPixel() functions work correctly.
  void SetScreenRenderTarget(RenderTargetId render_target);

  /// Returns the camera with a matching |render_target| with a viewport
  /// that contains |target_pixel|.  If multiple camera viewports contain the
  /// pixel and are set to the same render target, this will return the earliest
  /// registered camera.
  Camera* GetCameraByTargetPixel(RenderTargetId render_target,
                                 const mathfu::vec2& target_pixel) const;

  /// Converts |target_pixel| in a |render_target| to a world space ray.  Camera
  /// is chosen by the same logic as GetCameraByTargetPixel().
  Optional<Ray> WorldRayFromTargetPixel(RenderTargetId render_target,
                                        const mathfu::vec2& target_pixel) const;

  /// Returns the number of cameras that render to the given |render_target|.
  size_t GetNumCamerasForTarget(RenderTargetId render_target) const;

  /// Fill in |views| using the cameras associated with that |render_target|.
  /// Order will depend on the order that cameras were registered with
  /// CameraManager.
  void PopulateRenderViewsForTarget(RenderTargetId render_target,
                                    RenderView* views, size_t num_views) const;

  // Convenience functions that just use the screen render target.

  /// As GetCameraByTargetPixel(), but uses the screen_render_target.
  Camera* GetCameraByScreenPixel(const mathfu::vec2& screen_pixel) const;

  /// As WorldRayFromTargetPixel(), but uses the screen_render_target.
  Optional<Ray> WorldRayFromScreenPixel(const mathfu::vec2& screen_pixel) const;

  /// Converts a |screen_uv| to a world space ray.  Useful for handling
  /// InputManager touches.
  Optional<Ray> WorldRayFromScreenUV(const mathfu::vec2& screen_uv) const;

  /// As GetNumCamerasForTarget() but uses the screen_render_target.
  size_t GetNumCamerasForScreen() const;

  /// As PopulateRenderViewsForTarget() but uses the screen_render_target.
  void PopulateRenderViewsForScreen(RenderView* views, size_t num_views) const;

  /// Translate from a |screen_uv| (i.e. InputManager touch position) to a
  /// screen pixel for use with other CameraManager functions. |screen_uv|
  /// values should be in the range (0, 1), output values should be in the range
  /// (viewport min, viewport max).
  Optional<mathfu::vec2> PixelFromScreenUV(mathfu::vec2 screen_uv) const;

 private:
  std::unordered_map<RenderTargetId, CameraList> cameras_;
  RenderTargetId screen_render_target_ = kDefaultScreenRenderTarget;
};
}  // namespace lull
LULLABY_SETUP_TYPEID(lull::CameraManager);

#endif  // LULLABY_MODULES_CAMERA_CAMERA_MANAGER_H_
