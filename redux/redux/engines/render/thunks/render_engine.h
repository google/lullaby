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

#ifndef REDUX_ENGINES_RENDER_THUNKS_RENDER_ENGINE_H_
#define REDUX_ENGINES_RENDER_THUNKS_RENDER_ENGINE_H_

#include "redux/engines/render/render_engine.h"

namespace redux {

// Thunk functions to call the actual implementation.

void RenderEngine::OnRegistryInitialize() {
  Upcast(this)->OnRegistryInitialize();
}
RenderScenePtr RenderEngine::GetRenderScene(HashValue name) {
  return Upcast(this)->GetRenderScene(name);
}
RenderScenePtr RenderEngine::GetDefaultRenderScene() {
  return Upcast(this)->GetDefaultRenderScene();
}
RenderLayerPtr RenderEngine::GetRenderLayer(HashValue name) {
  return Upcast(this)->GetRenderLayer(name);
}
RenderLayerPtr RenderEngine::GetDefaultRenderLayer() {
  return Upcast(this)->GetDefaultRenderLayer();
}
RenderScenePtr RenderEngine::CreateRenderScene(HashValue name) {
  return Upcast(this)->CreateRenderScene(name);
}
RenderLayerPtr RenderEngine::CreateRenderLayer(HashValue name) {
  return Upcast(this)->CreateRenderLayer(name);
}
RenderablePtr RenderEngine::CreateRenderable() {
  return Upcast(this)->CreateRenderable();
}
LightPtr RenderEngine::CreateLight(Light::Type type) {
  return Upcast(this)->CreateLight(type);
}
IndirectLightPtr RenderEngine::CreateIndirectLight(
    const TexturePtr& reflection, const TexturePtr& irradiance) {
  return Upcast(this)->CreateIndirectLight(reflection, irradiance);
}
bool RenderEngine::Render() { return Upcast(this)->Render(); }
bool RenderEngine::RenderLayer(HashValue name) {
  return Upcast(this)->RenderLayer(name);
}
void RenderEngine::SyncWait() { Upcast(this)->SyncWait(); }
MeshFactory* RenderEngine::GetMeshFactory() {
  return Upcast(this)->GetMeshFactory();
}
ShaderFactory* RenderEngine::GetShaderFactory() {
  return Upcast(this)->GetShaderFactory();
}
TextureFactory* RenderEngine::GetTextureFactory() {
  return Upcast(this)->GetTextureFactory();
}
RenderTargetFactory* RenderEngine::GetRenderTargetFactory() {
  return Upcast(this)->GetRenderTargetFactory();
}

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_THUNKS_RENDER_ENGINE_H_
