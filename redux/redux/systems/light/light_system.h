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

#ifndef REDUX_SYSTEMS_LIGHT_LIGHT_SYSTEM_H_
#define REDUX_SYSTEMS_LIGHT_LIGHT_SYSTEM_H_

#include "redux/engines/render/render_engine.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/ecs/system.h"
#include "redux/modules/math/math.h"
#include "redux/systems/light/light_def_generated.h"

namespace redux {

// Allows Entities to add lighting to a scene.
//
// Note: See RenderSystem for more information about scenes and rendering.
class LightSystem : public System {
 public:
  explicit LightSystem(Registry* registry);

  void OnRegistryInitialize();

  // Attaches a light to an Entity. If a scene is specified, then the light will
  // only be added to that specific scene. The following types of light can be
  // specified. An Entity can only have a single light attached.
  // - Directional: The light has a direction, but no position. All light rays
  //   are parallel and come from infinitely far away and from everywhere.
  // - Point: Emits light in all directions from a certain point in space.
  // - Spot: Similar to a point lights, but the light it emits is limited to a
  //   cone.
  void AddDirectionalLight(Entity entity,
                           std::optional<HashValue> scene = std::nullopt);
  void AddSpotLight(Entity entity,
                    std::optional<HashValue> scene = std::nullopt);
  void AddPointLight(Entity entity,
                     std::optional<HashValue> scene = std::nullopt);

  // Adds an environmental (indirect) light to a scene using the given textures.
  // If a scene is specified, then the light will only be added to that specific
  // scene.
  void AddEnvironmentalLight(Entity entity, const TexturePtr& reflections,
                             std::optional<HashValue> scene = std::nullopt);
  void AddEnvironmentalLight(Entity entity, const TexturePtr& reflections,
                             const TexturePtr& irradiance,
                             std::optional<HashValue> scene = std::nullopt);

  // Adds the light to the specified scene.
  void AddToScene(Entity entity, HashValue scene);

  // Removes the light from the specified scene.
  void RemoveFromScene(Entity entity, HashValue scene);

  // Returns true if the entity is a light in the given scene.
  bool IsInScene(Entity entity, HashValue scene) const;

  // Disables the light across all scenes to which it has been added.
  void Hide(Entity entity);

  // Enables the light across all scenes to which it has been added.
  void Show(Entity entity);

  // Returns true if the light is disabled.
  bool IsHidden(Entity entity) const;

  // Sets the color of the light Entity.
  void SetColor(Entity entity, const Color4f& color);

  // Sets the intensity of the light Entity. For directional lights, it
  // specifies the illuminance in lux (or lumen/m^2). For point lights and spot
  // lights, it specifies the luminous power in lumen.
  void SetIntensity(Entity entity, float intensity);

  // Sets the falloff distance of the light Entity. Only applies to spot and
  // directionall lights.
  void SetFalloffDistance(Entity entity, float distance);

  // Sets the inner/outer cone angles of a spot light.
  void SetSpotLightCone(Entity entity, float inner_angle, float outer_angle);

  // Sets the Entity's lighting data using the associated def.
  void SetFromEnvLightDef(Entity entity, const EnvLightDef& def);
  void SetFromSpotLightDef(Entity entity, const SpotLightDef& def);
  void SetFromPointLightDef(Entity entity, const PointLightDef& def);
  void SetFromDirectionalLightDef(Entity entity,
                                  const DirectionalLightDef& def);

  // Synchronizes the light with data from other Systems (e.g. transforms).
  // Note: this function is automatically bound to run before rendering if the
  // choreographer is available.
  void PrepareToRender();

 private:
  struct LightComponent {
    LightPtr light;
    IndirectLightPtr indirect_light;
  };

  void CreateLight(Entity entity, Light::Type type,
                   std::optional<HashValue> scene_name = std::nullopt);

  void OnDestroy(Entity entity) override;

  RenderEngine* engine_ = nullptr;
  absl::flat_hash_map<Entity, LightComponent> lights_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::LightSystem);

#endif  // REDUX_SYSTEMS_LIGHT_LIGHT_SYSTEM_H_
