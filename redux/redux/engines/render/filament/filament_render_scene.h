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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_SCENE_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_SCENE_H_

#include "filament/Scene.h"
#include "redux/engines/render/filament/filament_utils.h"
#include "redux/engines/render/render_scene.h"
#include "redux/modules/base/registry.h"

namespace redux {

// Manages a filament Scene.
class FilamentRenderScene : public RenderScene {
 public:
  explicit FilamentRenderScene(Registry* registry);

  // Adds a renderable to the scene.
  void Add(const Renderable& renderable);

  // Removes a renderable from the scene.
  void Remove(const Renderable& renderable);

  // Adds a light to the scene.
  void Add(const Light& light);

  // Removes a light from the scene.
  void Remove(const Light& light);

  // Adds an indirect light to the scene.
  void Add(const IndirectLight& light);

  // Removes an indirect light from the scene.
  void Remove(const IndirectLight& light);

  // Returns the underlying filament scene.
  filament::Scene* GetFilamentScene() { return fscene_.get(); }

 private:
  filament::Engine* fengine_ = nullptr;
  FilamentResourcePtr<filament::Scene> fscene_ = nullptr;
};
}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_SCENE_H_
