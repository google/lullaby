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

#ifndef REDUX_ENGINES_RENDER_THUNKS_RENDER_LAYER_H_
#define REDUX_ENGINES_RENDER_THUNKS_RENDER_LAYER_H_

#include "redux/engines/render/render_layer.h"

namespace redux {

// Thunk functions to call the actual implementation.
void RenderLayer::Enable() { Upcast(this)->Enable(); }
void RenderLayer::Disable() { Upcast(this)->Disable(); }
bool RenderLayer::IsEnabled() const { return Upcast(this)->IsEnabled(); }
void RenderLayer::SetPriority(int priority) {
  Upcast(this)->SetPriority(priority);
}
int RenderLayer::GetPriority() const { return Upcast(this)->GetPriority(); }
void RenderLayer::SetScene(const RenderScenePtr& scene) {
  Upcast(this)->SetScene(scene);
}
void RenderLayer::SetRenderTarget(RenderTargetPtr target) {
  Upcast(this)->SetRenderTarget(target);
}
void RenderLayer::SetClipPlaneDistances(float near, float far) {
  Upcast(this)->SetClipPlaneDistances(near, far);
}
void RenderLayer::SetViewport(const Bounds2f& viewport) {
  Upcast(this)->SetViewport(viewport);
}
Bounds2i RenderLayer::GetAbsoluteViewport() const {
  return Upcast(this)->GetAbsoluteViewport();
}
void RenderLayer::SetViewMatrix(const mat4& view_matrix) {
  Upcast(this)->SetViewMatrix(view_matrix);
}
void RenderLayer::SetProjectionMatrix(const mat4& projection_matrix) {
  Upcast(this)->SetProjectionMatrix(projection_matrix);
}
void RenderLayer::SetCameraExposure(float aperture, float shutter_speed,
                                    float iso_sensitivity) {
  Upcast(this)->SetCameraExposure(aperture, shutter_speed, iso_sensitivity);
}
void RenderLayer::SetCameraFocalDistance(float focus_distance) {
  Upcast(this)->SetCameraFocalDistance(focus_distance);
}
void RenderLayer::EnableAntiAliasing(
    const MultiSampleAntiAliasingOptions& opts) {
  Upcast(this)->EnableAntiAliasing(opts);
}
void RenderLayer::DisableAntiAliasing() { Upcast(this)->DisableAntiAliasing(); }
void RenderLayer::EnableDepthOfField(const DepthOfFieldOptions& opts) {
  Upcast(this)->EnableDepthOfField(opts);
}
void RenderLayer::DisableDepthOfField() { Upcast(this)->DisableDepthOfField(); }
void RenderLayer::EnableVignette(const VignetteOptions& opts) {
  Upcast(this)->EnableVignette(opts);
}
void RenderLayer::DisableVignette() { Upcast(this)->DisableVignette(); }
void RenderLayer::EnableBloom(const BloomOptions& opts) {
  Upcast(this)->EnableBloom(opts);
}
void RenderLayer::DisableBloom() { Upcast(this)->DisableBloom(); }
void RenderLayer::EnableFog(const FogOptions& opts) {
  Upcast(this)->EnableFog(opts);
}
void RenderLayer::DisableFog() { Upcast(this)->DisableFog(); }
void RenderLayer::EnableAmbientOcclusion(const AmbientOcclusionOptions& opts) {
  Upcast(this)->EnableAmbientOcclusion(opts);
}
void RenderLayer::DisableAmbientOcclusion() {
  Upcast(this)->DisableAmbientOcclusion();
}
void RenderLayer::EnableScreenSpaceConeTracing(
    const ScreenSpaceConeTracingOptions& opts) {
  Upcast(this)->EnableScreenSpaceConeTracing(opts);
}
void RenderLayer::DisableScreenSpaceConeTracing() {
  Upcast(this)->DisableScreenSpaceConeTracing();
}
void RenderLayer::EnableScreenSpaceReflections(
    const ScreenSpaceReflectionsOptions& opts) {
  Upcast(this)->EnableScreenSpaceReflections(opts);
}
void RenderLayer::DisableScreenSpaceReflections() {
  Upcast(this)->DisableScreenSpaceReflections();
}
void RenderLayer::EnablePostProcessing() {
  Upcast(this)->EnablePostProcessing();
}
void RenderLayer::DisablePostProcessing() {
  Upcast(this)->DisablePostProcessing();
}

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_THUNKS_RENDER_LAYER_H_
