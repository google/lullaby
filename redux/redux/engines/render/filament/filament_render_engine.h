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

#ifndef REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_ENGINE_H_
#define REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_ENGINE_H_

#include "filament/Engine.h"
#include "filament/Renderer.h"
#include "filament/SwapChain.h"
#include "redux/engines/render/filament/filament_render_layer.h"
#include "redux/engines/render/filament/filament_render_scene.h"
#include "redux/engines/render/filament/filament_render_target.h"
#include "redux/engines/render/mesh_factory.h"
#include "redux/engines/render/render_engine.h"
#include "redux/engines/render/render_target_factory.h"
#include "redux/engines/render/shader_factory.h"
#include "redux/engines/render/texture_factory.h"

namespace redux {

class FilamentRenderEngine : public RenderEngine {
 public:
  explicit FilamentRenderEngine(Registry* registry);
  ~FilamentRenderEngine() override;

  void CreateFactories();

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

  // Accessors for the factories for various rendering asset types.
  MeshFactory* GetMeshFactory() { return mesh_factory_; }
  ShaderFactory* GetShaderFactory() { return shader_factory_; }
  TextureFactory* GetTextureFactory() { return texture_factory_; }
  RenderTargetFactory* GetRenderTargetFactory() {
    return render_target_factory_;
  }

  // Returns the image pixel data stored in the given render target.
  ImageData ReadPixels(FilamentRenderTarget* target);

  filament::Engine* GetFilamentEngine() { return fengine_; }
  filament::Renderer* GetFilamentRenderer() { return frenderer_; }

 private:
  MeshFactory* mesh_factory_ = nullptr;
  ShaderFactory* shader_factory_ = nullptr;
  TextureFactory* texture_factory_ = nullptr;
  RenderTargetFactory* render_target_factory_ = nullptr;

  filament::Engine* fengine_ = nullptr;
  filament::Renderer* frenderer_ = nullptr;
  filament::SwapChain* fswapchain_ = nullptr;

  RenderTargetPtr default_render_target_;
  absl::flat_hash_map<HashValue, RenderLayerPtr> layers_;
  absl::flat_hash_map<HashValue, RenderScenePtr> scenes_;
};

inline filament::Engine* GetFilamentEngine(Registry* registry) {
  auto engine =
      static_cast<FilamentRenderEngine*>(registry->Get<RenderEngine>());
  CHECK(engine);
  return engine->GetFilamentEngine();
}

}  // namespace redux

#endif  // REDUX_ENGINES_RENDER_FILAMENT_FILAMENT_RENDER_ENGINE_H_
