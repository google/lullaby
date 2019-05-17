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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_RENDER_SYSTEM_NEXT_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_RENDER_SYSTEM_NEXT_H_

#include <array>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "lullaby/generated/flatbuffers/shader_def_generated.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/render/animated_texture_processor.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/render/detail/sort_order.h"
#include "lullaby/systems/render/next/material.h"
#include "lullaby/systems/render/next/mesh.h"
#include "lullaby/systems/render/next/mesh_factory.h"
#include "lullaby/systems/render/next/next_renderer.h"
#include "lullaby/systems/render/next/render_component.h"
#include "lullaby/systems/render/next/render_target.h"
#include "lullaby/systems/render/next/shader_factory.h"
#include "lullaby/systems/render/next/texture_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/async_processor.h"
#include "lullaby/util/buffered_data.h"
#include "lullaby/util/string_view.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/generated/render_pass_def_generated.h"
#include "lullaby/generated/render_target_def_generated.h"

namespace lull {

// The main implementation of RenderSystem.  For documentation of the public
// functions, refer to the RenderSystem class declaration.
class RenderSystemNext : public System {
 public:
  using Drawable = RenderSystem::Drawable;
  using InitParams = RenderSystem::InitParams;

  explicit RenderSystemNext(Registry* registry, const InitParams& init_params);
  ~RenderSystemNext() override;

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
                                 RenderSystem::UniformChangedCallback callback);

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
  // Information required to sort, cull, and render a single submesh.
  struct RenderObject {
    RenderObject() {}
    // The mesh (vertex and index buffer) to draw.
    std::shared_ptr<Mesh> mesh;
    // The shader, uniforms, and render state to use for drawing.
    std::shared_ptr<Material> material;
    // The world-space transform of the object being drawn.
    mathfu::mat4 world_from_entity_matrix;
    // The position in world space.
    mathfu::vec3 world_position;
    // A value used to optionally sort the RenderObjects.
    union {
      RenderSortOrder sort_order;
      double depth_sort_order;
    };
    // A negative submesh index indicates a request to draw the entire mesh.
    int submesh_index = -1;
  };

  /// Structure holding the render components and definition of a pass.
  struct RenderPassObject {
    RenderPassObject();

    RenderStateT render_state;
    RenderClearParams clear_params;
    SortMode sort_mode = SortMode_None;
    RenderCullMode cull_mode = RenderCullMode::kNone;
    HashValue render_target = 0;

    ComponentPool<RenderComponent> components;
  };

  // Container for render objects.
  using RenderObjectVector = std::vector<RenderObject>;

  /// A group of render objects and associated parameters for rendering.
  struct RenderLayer {
    RenderStateT render_state;
    SortMode sort_mode = SortMode_None;
    RenderCullMode cull_mode = RenderCullMode::kNone;
    RenderObjectVector render_objects;
  };

  /// A container for holding the data to render entities in a render pass.
  struct RenderPassDrawContainer {
    RenderClearParams clear_params;
    std::shared_ptr<RenderTarget> render_target;

    enum LayerType { kOpaque, kBlendEnabled, kNumLayers };
    RenderLayer layers[kNumLayers];
  };

  /// The complete set of passes and associated data for drawing the frame.
  using RenderData = std::unordered_map<HashValue, RenderPassDrawContainer>;

  void RenderAt(const RenderObject* render_object,
                const RenderStateT& render_state, const RenderView* views,
                size_t num_views);

  void RenderObjects(const RenderObjectVector& objects,
                     const RenderStateT& render_state, const RenderView* views,
                     size_t num_views);

  void SetMeshImpl(Entity entity, HashValue pass, const MeshPtr& mesh);

  void RenderDebugStats(const RenderView* views, size_t num_views);
  void UpdateSortOrder(Entity entity);
  void OnTextureLoaded(RenderComponent* component, HashValue pass,
                       TextureUsageInfo usage, const TexturePtr& texture);
  void OnMeshLoaded(RenderComponent* render_component, HashValue pass);
  bool IsReadyToRenderImpl(const RenderComponent& component) const;

  void SetUniformImpl(RenderComponent* component, int material_index,
                      string_view name, ShaderDataType type, Span<uint8_t> data,
                      int count);

  bool GetUniformImpl(const RenderComponent* component, int material_index,
                      string_view name, size_t length, uint8_t* data_out) const;
  void SetMaterialImpl(RenderComponent* render_component, HashValue pass_hash,
                       int submesh_index, const MaterialInfo& material_info);
  void SetMaterialImpl(RenderComponent* render_component, HashValue pass_hash,
                       const MaterialInfo& material_info);

  std::shared_ptr<Material> CreateMaterialFromMaterialInfo(
      RenderComponent* render_component, int submesh_index,
      const MaterialInfo& info);

  // Create shader params for a specific render component.
  ShaderCreateParams CreateShaderParams(
      const std::string& shading_model, const RenderComponent* component,
      int submesh_index, const std::shared_ptr<Material>& material) const;
  ShaderPtr LoadShader(const ShaderCreateParams& params);

  // Ensures the internally cached render state matches the actual GL state.
  void ValidateRenderState();

  // High-level functions that control the render state using FPLBase states.
  void SetBlendMode(fplbase::BlendMode blend_mode, float amount);
  void SetStencilMode(fplbase::StencilMode mode, int ref, uint32_t mask);
  void SetDepthFunction(fplbase::DepthFunction depth_function);
  void SetCulling(fplbase::CullingMode mode);
  void SetFrontFace(fplbase::CullState::FrontFace front_face);

  // Functions that actually update the underlying GL state to match the
  // provided states.
  void SetViewport(const mathfu::recti& viewport);
  void SetAlphaTestState(const fplbase::AlphaTestState& alpha_test_state);
  void SetBlendState(const fplbase::BlendState& blend_state);
  void SetCullState(const fplbase::CullState& cull_state);
  void SetDepthState(const fplbase::DepthState& depth_state);
  void SetPointState(const fplbase::PointState& point_state);
  void SetScissorState(const fplbase::ScissorState& scissor_state);
  void SetStencilState(const fplbase::StencilState& stencil_state);

  /// Initializes the default render passes. This includes the following passes:
  /// RenderPass_Pano, RenderPass_Opaque, RenderPass_Main, RenderPass_OverDraw
  /// and RenderPass_OverDrawGlow.
  void InitDefaultRenderPassObjects();

  // Returns a render pass. This creates the pass if needed.
  RenderPassObject* GetRenderPassObject(HashValue pass);
  // Returns a render pass or nullptr if the pass doesn't exist.
  RenderPassObject* FindRenderPassObject(HashValue pass);
  const RenderPassObject* FindRenderPassObject(HashValue pass) const;

  /// Finds the component that best matches the criteria for the given drawable.
  RenderComponent* GetComponent(const Drawable& drawable);
  const RenderComponent* GetComponent(const Drawable& drawable) const;
  RenderComponent* GetComponent(Entity entity, HashValue pass);
  const RenderComponent* GetComponent(Entity entity, HashValue pass) const;

  /// Invokes the callback for each RenderComponent as defined by the drawable.
  using OnComponentFn =
      std::function<void(const RenderComponent& component, HashValue)>;
  using OnMutableComponentFn =
      std::function<void(RenderComponent* component, HashValue)>;
  void ForEachComponent(const Drawable& drawable,
                        const OnComponentFn& fn) const;
  void ForEachComponent(const Drawable& drawable,
                        const OnMutableComponentFn& fn);

  /// Finds the material that best matches the criteria for the given drawable.
  Material* GetMaterial(const Drawable& drawable);
  const Material* GetMaterial(const Drawable& drawable) const;

  /// Invokes the callback for each Material as defined by the drawable.
  using OnMaterialFn =
      std::function<void(const Material& material)>;
  using OnMutableMaterialFn =
      std::function<void(Material* material)>;
  void ForEachMaterial(const Drawable& drawable, const OnMaterialFn& fn) const;
  void ForEachMaterial(const Drawable& drawable, const OnMutableMaterialFn& fn,
                       bool rebuild_shader = true);

  /// Returns true if the SortMode is independent of the view.
  static bool IsSortModeViewIndependent(SortMode mode);

  /// Sorts a vector of RenderObject by a given (non view-dependent) sort mode.
  static void SortObjects(std::vector<RenderObject>* objects, SortMode mode);

  /// Sorts a vector of RenderObject by a given view-dependent sort mode.
  static void SortObjectsUsingView(std::vector<RenderObject>* objects,
                                   SortMode mode, const RenderView* views,
                                   size_t num_views);

  // Rebuild a shader for given material and render component.
  void RebuildShader(RenderComponent* component, int submesh_index,
                     std::shared_ptr<Material> material);

  NextRenderer renderer_;
  fplbase::RenderState render_state_;
  detail::SortOrderManager sort_order_manager_;

  /// Factories used for creating rendering-related objects.
  MeshFactoryImpl* mesh_factory_;
  ShaderFactory* shader_factory_;
  TextureFactoryImpl* texture_factory_;
  AnimatedTextureProcessor* animated_texture_processor_;

  /// The set of data the renderer will use to draw things onto the screen. The
  /// RenderData is created by SubmitRenderData, and this pointer is set at
  /// BeginFrame and released at EndFrame.
  RenderData* active_render_data_ = nullptr;

  /// Buffer of RenderData objects so one thread can write data while another
  /// can use data for rendering.
  BufferedData<RenderData> render_data_buffer_;

  /// Definitions of Render Passes.
  HashValue default_pass_ = ConstHash("Main");
  std::unordered_map<HashValue, RenderPassObject> render_passes_;
  std::unordered_map<HashValue, std::shared_ptr<RenderTarget>> render_targets_;

  // Cached state for immediate mode rendering.
  ShaderPtr bound_shader_;
  std::vector<TexturePtr> bound_textures_;

  std::string shading_model_path_;
  RenderFrontFace default_front_face_ = RenderFrontFace::kCounterClockwise;

  RenderSystemNext(const RenderSystemNext&) = delete;
  RenderSystemNext& operator=(const RenderSystemNext&) = delete;
};

#ifdef LULLABY_RENDER_BACKEND_NEXT
class RenderSystemImpl : public RenderSystemNext {
 public:
  using RenderSystemNext::RenderSystemNext;
};
#endif  // LULLABY_RENDER_BACKEND_NEXT

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RenderSystemNext);

#endif  // LULLABY_SYSTEMS_RENDER_NEXT_RENDER_SYSTEM_NEXT_H_
