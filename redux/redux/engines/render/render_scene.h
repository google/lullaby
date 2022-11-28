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

#ifndef REDUX_ENGINES_RENDER_RENDER_SCENE_H_
#define REDUX_ENGINES_RENDER_RENDER_SCENE_H_

#include <optional>

#include "redux/engines/render/indirect_light.h"
#include "redux/engines/render/light.h"
#include "redux/engines/render/render_target.h"
#include "redux/engines/render/renderable.h"
#include "redux/modules/graphics/color.h"
#include "redux/modules/math/bounds.h"

namespace redux {

// A collection of lights and renderables that will be rendered together.
class RenderScene {
 public:
  virtual ~RenderScene() = default;

  RenderScene(const RenderScene&) = delete;
  RenderScene& operator=(const RenderScene&) = delete;

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

 protected:
  explicit RenderScene() = default;
};

using RenderScenePtr = std::shared_ptr<RenderScene>;

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_RENDER_SCENE_H_
