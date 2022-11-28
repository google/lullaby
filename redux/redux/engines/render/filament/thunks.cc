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

#include "redux/engines/render/filament/filament_indirect_light.h"
#include "redux/engines/render/filament/filament_light.h"
#include "redux/engines/render/filament/filament_mesh.h"
#include "redux/engines/render/filament/filament_render_engine.h"
#include "redux/engines/render/filament/filament_render_layer.h"
#include "redux/engines/render/filament/filament_render_scene.h"
#include "redux/engines/render/filament/filament_render_target.h"
#include "redux/engines/render/filament/filament_renderable.h"
#include "redux/engines/render/filament/filament_shader.h"
#include "redux/engines/render/filament/filament_texture.h"

namespace redux {

inline FilamentIndirectLight* Upcast(IndirectLight* ptr) {
  return static_cast<FilamentIndirectLight*>(ptr);
}
inline const FilamentIndirectLight* Upcast(const IndirectLight* ptr) {
  return static_cast<const FilamentIndirectLight*>(ptr);
}
inline FilamentLight* Upcast(Light* ptr) {
  return static_cast<FilamentLight*>(ptr);
}
inline const FilamentLight* Upcast(const Light* ptr) {
  return static_cast<const FilamentLight*>(ptr);
}
inline FilamentMesh* Upcast(Mesh* ptr) {
  return static_cast<FilamentMesh*>(ptr);
}
inline const FilamentMesh* Upcast(const Mesh* ptr) {
  return static_cast<const FilamentMesh*>(ptr);
}
inline FilamentRenderEngine* Upcast(RenderEngine* ptr) {
  return static_cast<FilamentRenderEngine*>(ptr);
}
inline const FilamentRenderEngine* Upcast(const RenderEngine* ptr) {
  return static_cast<const FilamentRenderEngine*>(ptr);
}
inline FilamentRenderLayer* Upcast(RenderLayer* ptr) {
  return static_cast<FilamentRenderLayer*>(ptr);
}
inline const FilamentRenderLayer* Upcast(const RenderLayer* ptr) {
  return static_cast<const FilamentRenderLayer*>(ptr);
}
inline FilamentRenderScene* Upcast(RenderScene* ptr) {
  return static_cast<FilamentRenderScene*>(ptr);
}
inline const FilamentRenderScene* Upcast(const RenderScene* ptr) {
  return static_cast<const FilamentRenderScene*>(ptr);
}
inline FilamentRenderTarget* Upcast(RenderTarget* ptr) {
  return static_cast<FilamentRenderTarget*>(ptr);
}
inline const FilamentRenderTarget* Upcast(const RenderTarget* ptr) {
  return static_cast<const FilamentRenderTarget*>(ptr);
}
inline FilamentRenderable* Upcast(Renderable* ptr) {
  return static_cast<FilamentRenderable*>(ptr);
}
inline const FilamentRenderable* Upcast(const Renderable* ptr) {
  return static_cast<const FilamentRenderable*>(ptr);
}
inline FilamentTexture* Upcast(Texture* ptr) {
  return static_cast<FilamentTexture*>(ptr);
}
inline const FilamentTexture* Upcast(const Texture* ptr) {
  return static_cast<const FilamentTexture*>(ptr);
}

}  // namespace redux

#include "redux/engines/render/thunks/indirect_light.h"
#include "redux/engines/render/thunks/light.h"
#include "redux/engines/render/thunks/mesh.h"
#include "redux/engines/render/thunks/render_engine.h"
#include "redux/engines/render/thunks/render_layer.h"
#include "redux/engines/render/thunks/render_scene.h"
#include "redux/engines/render/thunks/render_target.h"
#include "redux/engines/render/thunks/renderable.h"
#include "redux/engines/render/thunks/texture.h"
