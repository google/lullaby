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

#ifndef REDUX_ENGINES_RENDER_LIGHT_H_
#define REDUX_ENGINES_RENDER_LIGHT_H_

#include <memory>

#include "redux/engines/render/texture.h"
#include "redux/modules/graphics/color.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/var/var.h"

namespace redux {

// Simulates light rays within a scene. Without lights, a physically-rendered
// scene will be black.
//
// A light can belong to multiple scenes.
class Light {
 public:
  enum Type {
    // Simulates light rays that are travelling in parallel from the same
    // direction uniformly across a scene.
    Directional,

    // Simulates light rays eminating from a single point in the scene.
    Point,

    // Simulates light rays as those coming from a spot light (i.e. a cone
    // shape) in the scene.
    Spot,
  };

  virtual ~Light() = default;

  Light(const Light&) = delete;
  Light& operator=(const Light&) = delete;

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
  // falloff attenuation and the outer angle defines the light's influence.
  // `inner` should be between 0 and pi/2, and `outer` should be beteen `inner`
  // and pi/2.
  void SetSpotLightConeAngles(float inner, float outer);

  // Sets the distance at which the lights stops being effective.
  // For point lights, the intensity diminishes with the inverse square of the
  // distance to the light.
  void SetFalloffDistance(float distance);

 protected:
  explicit Light() = default;
};

using LightPtr = std::shared_ptr<Light>;

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_LIGHT_H_
