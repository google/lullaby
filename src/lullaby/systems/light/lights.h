/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_SYSTEMS_LIGHT_LIGHTS_H_
#define LULLABY_SYSTEMS_LIGHT_LIGHTS_H_

#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/math.h"

namespace lull {

struct AmbientLight {
  Color4ub color;
  bool operator!=(const AmbientLight& rhs) const;
};

struct DirectionalLight {
  Color4ub color;
  mathfu::quat rotation = mathfu::quat::identity;
  float exponent = 0.f;
  bool operator!=(const DirectionalLight& rhs) const;
};

struct PointLight {
  Color4ub color;
  float intensity = 0.f;
  mathfu::vec3 position = mathfu::kZeros3f;
  float exponent = 0.f;
  bool operator!=(const PointLight& rhs) const;
};

struct Lightable {
  int max_ambient_lights = 0;
  int max_directional_lights = 0;
  int max_point_lights = 0;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_LIGHT_LIGHTS_H_
