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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_LIGHT_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_LIGHT_H_

#include "filament/Engine.h"
#include "filament/LightManager.h"
#include "utils/Entity.h"
#include "redux/engines/render/filament/filament_render_scene.h"
#include "redux/engines/render/light.h"
#include "redux/modules/base/registry.h"

namespace redux {

// Manages filament light entities.
class FilamentLight : public Light {
 public:
  FilamentLight(Registry* registry, Type type);
  ~FilamentLight() override;

  // Hides/disables the light from the scene.
  void Disable();

  // Shows/enables the light from the scene.
  void Enable();

  // Returns true if the light is enabled in the scene.
  bool IsEnabled() const;

  // Sets the transform of the light.
  void SetTransform(const mat4& transform);

  // Sets the color of the light.
  void SetColor(const Color4f& color);

  // Sets the intensity of the light. For directional lights, it specifies the
  // illuminance in lux. For point and spot lights, it specifies the luminous
  // power in lumen.
  void SetIntensity(float intensity);

  // Sets the spot light cone angles. The inner angle defines the light's
  // falloff attenuation and the outer angle ddefines the light's influence.
  // `inner` should be between 0 and pi/2, and `outer` should be beteen `inner`
  // and pi/2.
  void SetSpotLightConeAngles(float inner, float outer);

  // Sets the distance at which the lights stops being effective.
  // For point lights, the intensity diminishes with the inverse square of the
  // distance to the light.
  void SetFalloffDistance(float distance);

  // Adds the light to a filament scene.
  void AddToScene(FilamentRenderScene* scene) const;

  // Removes the light from a filament scene.
  void RemoveFromScene(FilamentRenderScene* scene) const;

 private:
  void CreateLightEntity(filament::LightManager::Type type);

  Type type_ = Directional;
  filament::Engine* fengine_ = nullptr;
  utils::Entity fentity_;
  mutable absl::flat_hash_set<filament::Scene*> scenes_;
  bool visible_ = true;
};

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_LIGHT_H_
