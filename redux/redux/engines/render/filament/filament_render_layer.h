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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_LAYER_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_LAYER_H_

#include "absl/container/btree_set.h"
#include "filament/Camera.h"
#include "filament/Engine.h"
#include "filament/Renderer.h"
#include "filament/Scene.h"
#include "filament/View.h"
#include "filament/Viewport.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/light.h"
#include "redux/engines/render/render_engine.h"
#include "redux/engines/render/renderable.h"
#include "redux/modules/base/registry.h"

namespace redux {

// Manages a filament Camera and View.
class FilamentRenderLayer : public RenderLayer {
 public:
  FilamentRenderLayer(Registry* registry, RenderTargetPtr target);

  // Adds the layer to the list of layers to be rendered, effectively enabling
  // it.
  void Enable();

  // Removes the layer from the list of layers to be rendered, effectively
  // disabling it.
  void Disable();

  // Returns true if the layer will be rendered.
  bool IsEnabled() const;

  // Sets the priority at which the layer will be rendered. Higher priority
  // layers will be rendered first. Two layers with the same priority will be
  // rendered in arbitrary order.
  void SetPriority(int priority);

  // Returns the render priority of the layer.
  int GetPriority() const;

  // Associates a scene (which contains lights and renderables) with this layer.
  // A layer can only have a single scene at a time.
  void SetScene(const RenderScenePtr& scene);

  // Sets the render target on which to perform the drawing/rendering.
  void SetRenderTarget(RenderTargetPtr target);

  // Sets the clip plane distances for rendering.
  void SetClipPlaneDistances(float near, float far);

  // Sets the viewport (i.e. area) on the render target in which the rendering
  // will be performed.
  void SetViewport(const Bounds2f& viewport);

  // Returns the viewport (i.e. area) on the render target in which the
  // rendering will be performed.
  Bounds2i GetAbsoluteViewport() const;

  // Sets the view matrix that will be used for rendering. This is effectively
  // the transform of the camera from which the scene will be rendered.
  void SetViewMatrix(const mat4& view_matrix);

  // Sets the projection matrix that will be used for rendering. This is
  // effectively the lens of the camera form which the scene will be rendered.
  void SetProjectionMatrix(const mat4& projection_matrix);

  // Enables anti-aliasing when rendering the layer.
  void EnableAntiAliasing();

  // Disables anti-aliasing when rendering the layer.
  void DisableAntiAliasing();

  // Disables post-processing (like tone mapping) when rendering the layer.
  void DisablePostProcessing();

  // Returns the underlying filament view.
  filament::View* GetFilamentView() { return fview_.get(); }

  // Returns the underlying filament camera.
  filament::Camera* GetFilamentCamera() { return fcamera_.get(); }

  // Returns the underlying render target for this layer.
  const RenderTargetPtr& GetRenderTarget() const {
    return render_target_;
  }

 private:
  FilamentResourcePtr<filament::Camera> fcamera_ = nullptr;
  FilamentResourcePtr<filament::View> fview_ = nullptr;
  RenderTargetPtr render_target_;
  Bounds2f viewport_;
  float near_plane_ = 0.1f;
  float far_plane_ = 1000.0f;
  int priority_ = 0;
  bool enabled_ = true;
};
}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_LAYER_H_
