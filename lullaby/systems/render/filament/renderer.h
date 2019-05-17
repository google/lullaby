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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_FILAMENT_WRAPPER_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_FILAMENT_WRAPPER_H_

#include "filament/Camera.h"
#include "filament/Engine.h"
#include "filament/LightManager.h"
#include "filament/RenderableManager.h"
#include "filament/Renderer.h"
#include "filament/Scene.h"
#include "filament/SwapChain.h"
#include "filament/TransformManager.h"
#include "filament/View.h"
#include "filamat/MaterialBuilder.h"
#include "lullaby/modules/render/render_view.h"
#include "lullaby/util/span.h"

namespace lull {

class SceneView;

// Renderer keeps the boiler plate Filament code all in one place.
//
// IMPORTANT: This needs to only be accessed from the same thread per instance,
// since it holds some thread_local context.
class Renderer {
 public:
  Renderer();
  ~Renderer();
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  filament::Engine* GetEngine();

  // Pass the platform specific native window to Filament so that it can create
  // its swap chain.
  void SetNativeWindow(void* native_window);

  void SetClearColor(const mathfu::vec4 color);
  mathfu::vec4 GetClearColor() const;

  // Render the specified views using Filament.
  void Render(SceneView* sceneview, Span<RenderView> views);

 private:
  filament::Renderer* renderer_ = nullptr;
  filament::SwapChain* swap_chain_ = nullptr;
  mathfu::vec4 clear_color_ = {0, 0, 0, 1};
  const void* thread_local_ptr_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_FILAMENT_WRAPPER_H_
