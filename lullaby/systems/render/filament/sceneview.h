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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_SCENEVIEW_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_SCENEVIEW_H_

#include "lullaby/generated/light_def_generated.h"
#include "lullaby/systems/render/filament/renderer.h"
#include "lullaby/systems/render/filament/texture.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

// Manages a single filament scene and associated views, cameras, and lights.
class SceneView {
 public:
  SceneView(Registry* registry, filament::Engine* engine);
  ~SceneView();

  // Ensures the internal data structures are ready for rendering including
  // updating the filament light data based on the TransformSystem.
  void Prepare(const mathfu::vec4& clear_color, Span<RenderView> views);

  // Associates a directional light with the specified entity.
  void CreateLight(Entity entity, const DirectionalLightDef& light);

  // Associates an environmental light (ie. IBL) with the specified entity.
  void CreateLight(Entity entity, const EnvironmentLightDef& light);

  // Associates a point light with the specified entity.
  void CreateLight(Entity entity, const PointLightDef& light);

  // Destroys any lights associated with the specified entity.
  void DestroyLight(Entity entity);

  // Returns the internally managed filament View.
  filament::View* GetView(size_t index);

  // Returns the internally managed filament Scene.
  filament::Scene* GetScene();

 private:
  void InitializeViews(const mathfu::vec4& clear_color, size_t count);
  void CreateLight(Entity entity, filament::LightManager::Builder& builder);

  Registry* registry_ = nullptr;
  filament::Engine* engine_ = nullptr;
  filament::Scene* scene_ = nullptr;
  std::vector<filament::View*> views_;
  std::vector<filament::Camera*> cameras_;
  std::unordered_map<Entity, utils::Entity> lights_;
  filament::IndirectLight* indirect_light_ = nullptr;
  TexturePtr ibl_reflection_;
  TexturePtr ibl_irradiance_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_SCENEVIEW_H_
