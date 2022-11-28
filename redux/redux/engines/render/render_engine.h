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

#ifndef REDUX_ENGINES_RENDER_RENDER_ENGINE_H_
#define REDUX_ENGINES_RENDER_RENDER_ENGINE_H_

#include <memory>

#include "redux/engines/render/indirect_light.h"
#include "redux/engines/render/light.h"
#include "redux/engines/render/mesh.h"
#include "redux/engines/render/render_layer.h"
#include "redux/engines/render/render_target.h"
#include "redux/engines/render/renderable.h"
#include "redux/engines/render/shader.h"
#include "redux/engines/render/texture.h"
#include "redux/modules/base/bits.h"
#include "redux/modules/base/registry.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/graphics/color.h"
#include "redux/modules/math/bounds.h"

namespace redux {

class MeshFactory;
class ShaderFactory;
class TextureFactory;
class RenderTargetFactory;

// Creates and manages the various rendering-related objects (e.g. layers,
// scenes, lights, and renderables) and provides the main API for rendering
// them.
class RenderEngine {
 public:
  static void Create(Registry* registry);
  virtual ~RenderEngine() = default;

  void OnRegistryInitialize();

  // Creates a new RenderScene with the given name.
  RenderScenePtr CreateRenderScene(HashValue name);

  // Returns the RenderScene with the given name.
  RenderScenePtr GetRenderScene(HashValue name);

  // Returns a default RenderScene.
  RenderScenePtr GetDefaultRenderScene();

  // Creates a new RenderLayer with the given name.
  RenderLayerPtr CreateRenderLayer(HashValue name);

  // Returns the RenderLayer with the given name.
  RenderLayerPtr GetRenderLayer(HashValue name);

  // Returns a default RenderLayer.
  RenderLayerPtr GetDefaultRenderLayer();

  // Creates a new Renderable.
  RenderablePtr CreateRenderable();

  // Creates a new Light of the given type.
  LightPtr CreateLight(Light::Type type);

  // Creates a new IndirectLight.
  IndirectLightPtr CreateIndirectLight(const TexturePtr& reflection,
                                       const TexturePtr& irradiance = nullptr);

  // Renders all active RenderLayers in priority order.
  bool Render();

  // Renders the specified layer (regardless of active state).
  bool RenderLayer(HashValue name);

  // Waits until all rendering operations have completed.
  void SyncWait();

  // Accessors for the factories for various rendering asset types. These are
  // also available in the registry.
  MeshFactory* GetMeshFactory();
  ShaderFactory* GetShaderFactory();
  TextureFactory* GetTextureFactory();
  RenderTargetFactory* GetRenderTargetFactory();

 protected:
  explicit RenderEngine(Registry* registry) : registry_(registry) {}
  Registry* registry_ = nullptr;
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::RenderEngine);

#endif  // REDUX_ENGINES_RENDER_RENDER_ENGINE_H_
