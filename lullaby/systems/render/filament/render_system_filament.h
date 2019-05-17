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

#ifndef LULLABY_SYSTEMS_RENDER_FILAMENT_RENDER_SYSTEM_FILAMENT_H_
#define LULLABY_SYSTEMS_RENDER_FILAMENT_RENDER_SYSTEM_FILAMENT_H_

#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "lullaby/events/entity_events.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/render/detail/sort_order.h"
#include "lullaby/systems/render/filament/mesh.h"
#include "lullaby/systems/render/filament/mesh_factory.h"
#include "lullaby/systems/render/filament/renderable.h"
#include "lullaby/systems/render/filament/renderer.h"
#include "lullaby/systems/render/filament/sceneview.h"
#include "lullaby/systems/render/filament/shader_factory.h"
#include "lullaby/systems/render/filament/texture_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/async_processor.h"
#include "lullaby/util/buffered_data.h"
#include "lullaby/util/string_view.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/generated/render_pass_def_generated.h"
#include "lullaby/generated/render_target_def_generated.h"

namespace lull {

// Filament-based implementation of RenderSystem.
//
// For documentation of the public functions, refer to the RenderSystem class
// declaration.
class RenderSystemFilament : public System {
 public:
  using Drawable = RenderSystem::Drawable;
  using InitParams = RenderSystem::InitParams;
  using UniformChangedCallback = RenderSystem::UniformChangedCallback;

  explicit RenderSystemFilament(Registry* registry,
                                const InitParams& init_params);
  ~RenderSystemFilament() override;

  void Initialize() override;

  // Functions for configuring the render system.
  void SetShadingModelPath(const std::string& path);
  void SetDefaultFrontFace(RenderFrontFace face);
  void SetStereoMultiviewEnabled(bool enabled);

  // System-level functions for controlling rendering.
  void BeginRendering();
  void EndRendering();
  void SubmitRenderData();
  void Render(const RenderView* views, size_t num_views);
  void Render(const RenderView* views, size_t num_views, HashValue pass);
  void SetClearColor(float r, float g, float b, float a);
  mathfu::vec4 GetClearColor() const;

  // Render target functions.
  void CreateRenderTarget(HashValue render_target_name,
                          const RenderTargetCreateParams& create_params);
  ImageData GetRenderTargetData(HashValue render_target_name);

  // Render pass configuration functions.
  void SetDefaultRenderPass(HashValue pass);
  HashValue GetDefaultRenderPass() const;
  void SetClearParams(HashValue pass, const RenderClearParams& clear_params);
  void SetRenderState(HashValue pass, const fplbase::RenderState& state);
  void SetRenderTarget(HashValue pass, HashValue render_target_name);
  void SetSortMode(HashValue pass, SortMode mode);
  void SetSortVector(HashValue pass, const mathfu::vec3& vector);
  void SetCullMode(HashValue pass, RenderCullMode mode);

  // Entity creation/destruction functions.
  void Create(Entity entity, HashValue type, const Def* def) override;
  void Create(Entity entity, HashValue pass);
  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;
  void Destroy(Entity entity, HashValue pass);

  // Basic Entity functions.
  bool IsReadyToRender(const Drawable& drawable) const;
  std::vector<HashValue> GetRenderPasses(Entity entity) const;

  // Entity visibility functions.
  bool IsHidden(const Drawable& drawable) const;
  void Hide(const Drawable& drawable);
  void Show(const Drawable& drawable);

  // Mesh functions.
  void SetMesh(const Drawable& drawable, const MeshData& mesh);
  void SetMesh(const Drawable& drawable, const MeshPtr& mesh);
  MeshPtr GetMesh(const Drawable& drawable);

  // Material functions.
  void SetMaterial(const Drawable& drawable, const MaterialInfo& info);
  void RequestShaderFeature(const Drawable& drawable, HashValue feature);
  void ClearShaderFeature(const Drawable& drawable, HashValue feature);
  bool IsShaderFeatureRequested(const Drawable& drawable,
                                HashValue feature) const;

  // Uniform functions.
  void SetUniform(const Drawable& drawable, string_view name,
                  ShaderDataType type, Span<uint8_t> data, int count = 1);
  bool GetUniform(const Drawable& drawable, string_view name, size_t length,
                  uint8_t* data_out) const;

  void CopyUniforms(Entity entity, Entity source);
  void SetUniformChangedCallback(Entity entity, HashValue pass,
                                 UniformChangedCallback callback);

  const mathfu::vec4& GetDefaultColor(Entity entity) const;
  void SetDefaultColor(Entity entity, const mathfu::vec4& color);
  bool GetColor(Entity entity, mathfu::vec4* color) const;
  void SetColor(Entity entity, const mathfu::vec4& color);

  // Texture functions.
  void SetTexture(const Drawable& drawable, TextureUsageInfo usage,
                  const TexturePtr& texture);
  TexturePtr GetTexture(const Drawable& drawable, TextureUsageInfo usage) const;

  void SetTextureExternal(const Drawable& drawable, TextureUsageInfo usage);
  void SetTextureId(const Drawable& drawable, TextureUsageInfo usage,
                    uint32_t texture_target, uint32_t texture_id);

  // Shader functions.
  void SetShader(const Drawable& drawable, const ShaderPtr& shader);
  ShaderPtr GetShader(const Drawable& drawable) const;
  ShaderPtr LoadShader(const std::string& filename);

  // Stencil functions.
  void SetStencilMode(Entity entity, RenderStencilMode mode, int value);
  void SetStencilMode(Entity entity, HashValue pass, RenderStencilMode mode,
                      int value);

  // Sort order functions.
  void SetSortOrderOffset(Entity entity,
                          RenderSortOrderOffset sort_order_offset);
  void SetSortOrderOffset(Entity entity, HashValue pass,
                          RenderSortOrderOffset sort_order_offset);
  RenderSortOrder GetSortOrder(Entity entity) const;
  RenderSortOrderOffset GetSortOrderOffset(Entity entity) const;

  // Render state management functions.
  void SetDepthTest(bool enabled);
  void SetDepthWrite(bool enabled);
  void SetViewport(const RenderView& view);
  void SetBlendMode(fplbase::BlendMode blend_mode);
  void UpdateCachedRenderState(const fplbase::RenderState& render_state);
  const fplbase::RenderState& GetCachedRenderState() const;

  // Immediate mode drawing functions.
  void BindShader(const ShaderPtr& shader);
  void BindTexture(int unit, const TexturePtr& texture);
  void BindUniform(const char* name, const float* data, int dimension);
  void DrawMesh(const MeshData& mesh, Optional<mathfu::mat4> clip_from_model);

  // Debug functions.
  std::string GetShaderString(Entity entity, HashValue pass, int submesh_index,
                              ShaderStageType stage) const;
  ShaderPtr CompileShaderString(const std::string& vertex_string,
                                const std::string& fragment_string);

 protected:
  struct RenderComponent : Component {
    explicit RenderComponent(Entity entity) : Component(entity) {}

    MeshPtr mesh;
    std::vector<RenderablePtr> renderables;
    UniformChangedCallback uniform_changed_callback;
  };

  struct RenderPassObject {
    RenderPassObject() : components(32) {}

    std::unique_ptr<SceneView> sceneview;
    ComponentPool<RenderComponent> components;
  };

  void SetNativeWindow(void* native_window);
  void ResizeRenderables(RenderComponent* component, size_t count);

  void SetMeshImpl(const Drawable& drawable, const MeshPtr& mesh);
  void SetShaderImpl(RenderComponent* component, ShaderPtr shader);
  void SetMaterialImpl(Renderable* renderable, const MaterialInfo& info);
  void SetTextureImpl(Renderable* renderable,
                      const TextureUsageInfo& usage_info,
                      const TexturePtr& texture);

  void OnMeshLoaded(const Drawable& drawable);
  void OnTextureLoaded(const Drawable& drawable);

  void RebuildShader(Renderable* renderable);
  ShaderPtr BuildShader(const std::string& shading_model,
                        const Renderable* renderable);

  void Render(RenderPassObject* render_pass, Span<RenderView> views);

  // Returns a reference to the RenderPassObject specified by the pass, creating
  // one if necessary.
  RenderPassObject& GetRenderPassObject(HashValue pass);

  // Returns the RenderPassObject associated with the pass, or nullptr if no
  // such association exists.
  RenderPassObject* FindRenderPassObject(HashValue pass);
  const RenderPassObject* FindRenderPassObject(HashValue pass) const;

  // Returns the RenderComponent associated with the entity and pass, or nullptr
  // if no such association exsists.
  RenderComponent* GetRenderComponent(const Drawable& drawable);
  const RenderComponent* GetRenderComponent(const Drawable& drawable) const;

  using OnRenderableFn = std::function<void(const Renderable&)>;
  using OnMutableRenderableFn = std::function<void(Renderable*)>;

  void ForEachRenderable(const Drawable& drawable, OnMutableRenderableFn fn);
  void ForEachRenderable(const Drawable& drawable, OnRenderableFn fn) const;

  // The Renderer instance which wraps the filament Engine and other filament
  // functionality.
  std::unique_ptr<Renderer> renderer_;

  std::unordered_map<HashValue, RenderPassObject> render_passes_;

  MeshFactoryImpl* mesh_factory_ = nullptr;
  TextureFactoryImpl* texture_factory_ = nullptr;
  std::unique_ptr<ShaderFactory> shader_factory_ = nullptr;

  RenderSystemFilament(const RenderSystemFilament&) = delete;
  RenderSystemFilament& operator=(const RenderSystemFilament&) = delete;
};

#ifdef LULLABY_RENDER_BACKEND_FILAMENT
class RenderSystemImpl : public RenderSystemFilament {
 public:
  using RenderSystemFilament::RenderSystemFilament;
};
#endif  // LULLABY_RENDER_BACKEND_FILAMENT

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RenderSystemFilament);

#endif  // LULLABY_SYSTEMS_RENDER_FILAMENT_RENDER_SYSTEM_FILAMENT_H_
