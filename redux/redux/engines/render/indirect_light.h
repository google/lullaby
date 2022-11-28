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

#ifndef REDUX_ENGINES_RENDER_INDIRECT_LIGHT_H_
#define REDUX_ENGINES_RENDER_INDIRECT_LIGHT_H_

#include <memory>

#include "redux/engines/render/texture.h"
#include "redux/modules/graphics/color.h"
#include "redux/modules/graphics/texture_usage.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/var/var.h"

namespace redux {

// Simulates environmental light using cube maps.
class IndirectLight {
 public:
  virtual ~IndirectLight() = default;

  IndirectLight(const IndirectLight&) = delete;
  IndirectLight& operator=(const IndirectLight&) = delete;

  // Hides/disables the light from the scene.
  void Disable();

  // Shows/enables the light from the scene.
  void Enable();

  // Returns true if the light is enabled in the scene.
  bool IsEnabled() const;

  // Sets the transform of the light.
  void SetTransform(const mat4& transform);

 protected:
  explicit IndirectLight() = default;
};

using IndirectLightPtr = std::shared_ptr<IndirectLight>;

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_INDIRECT_LIGHT_H_
