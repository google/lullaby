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
void RenderLayer::EnableAntiAliasing() { Upcast(this)->EnableAntiAliasing(); }
void RenderLayer::DisableAntiAliasing() { Upcast(this)->DisableAntiAliasing(); }
void RenderLayer::DisablePostProcessing() {
  Upcast(this)->DisablePostProcessing();
}
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

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_THUNKS_RENDER_LAYER_H_
