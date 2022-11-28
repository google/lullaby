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

#include "redux/engines/render/filament/filament_render_layer.h"

#include "utils/EntityManager.h"
#include "redux/engines/render/filament/filament_light.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_render_target.h"
#include "redux/engines/render/filament/filament_renderable.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/modules/math/matrix.h"

namespace redux {

FilamentRenderLayer::FilamentRenderLayer(Registry* registry,
                                         RenderTargetPtr target)
    : render_target_(target) {
  auto fengine = redux::GetFilamentEngine(registry);
  fview_ = MakeFilamentResource(fengine->createView(), fengine);

  static const float kCameraAperture = 16.f;
  static const float kCameraShutterSpeed = 1.0f / 125.0f;
  static const float kCameraSensitivity = 100.0f;
  auto camera_entity = utils::EntityManager::get().create();
  fcamera_ =
      MakeFilamentResource(fengine->createCamera(camera_entity), fengine);
  fcamera_->setExposure(kCameraAperture, kCameraShutterSpeed,
                        kCameraSensitivity);
  fview_->setCamera(fcamera_.get());

  SetViewport({vec2::Zero(), vec2::One()});
}

void FilamentRenderLayer::Enable() { enabled_ = true; }

void FilamentRenderLayer::Disable() { enabled_ = false; }

bool FilamentRenderLayer::IsEnabled() const { return enabled_; }

void FilamentRenderLayer::SetPriority(int priority) { priority_ = priority; }

int FilamentRenderLayer::GetPriority() const { return priority_; }

void FilamentRenderLayer::EnableAntiAliasing() {
  fview_->setAntiAliasing(filament::View::AntiAliasing::FXAA);
}

void FilamentRenderLayer::DisableAntiAliasing() {
  fview_->setAntiAliasing(filament::View::AntiAliasing::NONE);
  fview_->setPostProcessingEnabled(false);
}

void FilamentRenderLayer::DisablePostProcessing() {
  fview_->setPostProcessingEnabled(false);
}

void FilamentRenderLayer::SetClipPlaneDistances(float near, float far) {
  near_plane_ = near;
  far_plane_ = far;
}

void FilamentRenderLayer::SetViewport(const Bounds2f& viewport) {
  viewport_ = viewport;

  const vec2i target_size = render_target_->GetDimensions();
  filament::Viewport vp;
  vp.left = static_cast<int32_t>(viewport.min.x * target_size.x);
  vp.bottom = static_cast<int32_t>(viewport.min.y * target_size.y);
  vp.width = static_cast<uint32_t>(viewport.Size().x * target_size.x);
  vp.height = static_cast<uint32_t>(viewport.Size().y * target_size.y);
  fview_->setViewport(vp);
}

Bounds2i FilamentRenderLayer::GetAbsoluteViewport() const {
  const filament::Viewport& vp = fview_->getViewport();
  Bounds2i res;
  res.min = vec2i{vp.left, vp.bottom};
  res.max = vec2i{static_cast<int>(vp.width), static_cast<int>(vp.height)};
  return res;
}

void FilamentRenderLayer::SetRenderTarget(RenderTargetPtr target) {
  auto impl = static_cast<FilamentRenderTarget*>(target.get());
  fview_->setRenderTarget(impl ? impl->GetFilamentRenderTarget() : nullptr);
  render_target_ = target;
  SetViewport(viewport_);
}

void FilamentRenderLayer::SetScene(const RenderScenePtr& scene) {
  auto impl = static_cast<FilamentRenderScene*>(scene.get());
  fview_->setScene(impl ? impl->GetFilamentScene() : nullptr);
}

void FilamentRenderLayer::SetViewMatrix(const mat4& view_matrix) {
  fcamera_->setModelMatrix(ToFilament(view_matrix));
}

void FilamentRenderLayer::SetProjectionMatrix(const mat4& projection_matrix) {
  fcamera_->setCustomProjection(
      filament::math::mat4(ToFilament(projection_matrix)), near_plane_,
      far_plane_);
}

}  // namespace redux
