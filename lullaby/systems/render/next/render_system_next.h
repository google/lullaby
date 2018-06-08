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

#ifndef LULLABY_SYSTEMS_RENDER_NEXT_RENDER_SYSTEM_NEXT_H_
#define LULLABY_SYSTEMS_RENDER_NEXT_RENDER_SYSTEM_NEXT_H_

#include <array>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "lullaby/events/entity_events.h"
#include "lullaby/modules/ecs/system.h"
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
#include "lullaby/systems/render/next/texture_atlas_factory.h"
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
  using InitParams = RenderSystem::InitParams;
  explicit RenderSystemNext(Registry* registry, const InitParams& init_params);
  ~RenderSystemNext() override;

  void Initialize() override;

  void SetStereoMultiviewEnabled(bool enabled);

  void PreloadFont(const char* name);
  const TexturePtr& GetWhiteTexture() const;
  const TexturePtr& GetInvalidTexture() const;
  TexturePtr LoadTexture(const std::string& filename, bool create_mips);
  TexturePtr GetTexture(HashValue texture_hash) const;
  void LoadTextureAtlas(const std::string& filename);
  MeshPtr LoadMesh(const std::string& filename);
  TexturePtr CreateTexture(const ImageData& image, bool create_mips);

  ShaderPtr LoadShader(const std::string& filename);
  ShaderPtr LoadShader(const ShaderCreateParams& params);

  void Create(Entity e, HashValue type, const Def* def) override;
  void Create(Entity e, HashValue pass);
  void PostCreateInit(Entity e, HashValue type, const Def* def) override;
  void Destroy(Entity e) override;
  void Destroy(Entity e, HashValue pass);

  void ProcessTasks();
  void WaitForAssetsToLoad();

  void BeginRendering();
  void EndRendering();
  void SubmitRenderData();

  const mathfu::vec4& GetDefaultColor(Entity entity) const;
  void SetDefaultColor(Entity entity, const mathfu::vec4& color);

  bool GetColor(Entity entity, mathfu::vec4* color) const;
  void SetColor(Entity entity, const mathfu::vec4& color);

  void SetUniform(Entity entity, Optional<HashValue> pass,
                  Optional<int> submesh_index, string_view name,
                  ShaderDataType type, Span<uint8_t> data, int count);
  bool GetUniform(Entity entity, Optional<HashValue> pass,
                  Optional<int> submesh_index, string_view name, size_t length,
                  uint8_t* data_out) const;

  void SetUniform(Entity e, const char* name, const float* data, int dimension,
                  int count);
  void SetUniform(Entity e, HashValue pass, const char* name, const float* data,
                  int dimension, int count);
  bool GetUniform(Entity e, const char* name, size_t length,
                  float* data_out) const;
  bool GetUniform(Entity e, HashValue pass, const char* name, size_t length,
                  float* data_out) const;
  void CopyUniforms(Entity entity, Entity source);
  void SetUniformChangedCallback(Entity entity, HashValue pass,
                                 RenderSystem::UniformChangedCallback callback);

  int GetNumBones(Entity entity) const;
  const uint8_t* GetBoneParents(Entity e, int* num) const;
  const std::string* GetBoneNames(Entity e, int* num) const;
  const mathfu::AffineTransform* GetDefaultBoneTransformInverses(
      Entity e, int* num) const;

  /// Sets the bone transforms for all render components associated with the
  /// entity.
  void SetBoneTransforms(Entity entity,
                         const mathfu::AffineTransform* transforms,
                         int num_transforms);
  void SetBoneTransforms(Entity entity, HashValue pass,
                         const mathfu::AffineTransform* transforms,
                         int num_transforms);

  void SetTexture(Entity e, int unit, const TexturePtr& texture);
  void SetTexture(Entity e, HashValue pass, int unit,
                  const TexturePtr& texture);
  void SetTextureExternal(Entity e, HashValue pass, int unit);

  TexturePtr CreateProcessedTexture(
      const TexturePtr& texture, bool create_mips,
      const RenderSystem::TextureProcessor& processor);

  TexturePtr CreateProcessedTexture(
      const TexturePtr& texture, bool create_mips,
      const RenderSystem::TextureProcessor& processor,
      const mathfu::vec2i& output_dimensions);

  void SetTextureId(Entity e, int unit, uint32_t texture_target,
                    uint32_t texture_id);
  void SetTextureId(Entity e, HashValue pass, int unit, uint32_t texture_target,
                    uint32_t texture_id);

  TexturePtr GetTexture(Entity entity, int unit) const;

  void SetPano(Entity entity, const std::string& filename,
               float heading_offset_deg);

  void SetText(Entity e, const std::string& text);

  bool GetQuad(Entity e, RenderQuad* quad) const;
  void SetQuad(Entity e, const RenderQuad& quad);

  void SetMesh(Entity e, const MeshData& mesh);
  void SetMesh(Entity entity, HashValue pass, const MeshData& mesh);
  void SetAndDeformMesh(Entity entity, const MeshData& mesh);
  void SetAndDeformMesh(Entity entity, HashValue pass, const MeshData& mesh);

  void SetMesh(Entity e, const std::string& file);
  void SetMesh(Entity e, const MeshPtr& mesh);
  void SetMesh(Entity e, HashValue pass, const MeshPtr& mesh);
  MeshPtr GetMesh(Entity e, HashValue pass);

  ShaderPtr GetShader(Entity entity) const;
  ShaderPtr GetShader(Entity entity, HashValue pass) const;
  void SetShader(Entity e, HashValue pass, const ShaderPtr& shader);
  void SetShader(Entity e, const ShaderPtr& shader);

  void SetMaterial(Entity e, Optional<HashValue> pass,
                   Optional<int> submesh_index, const MaterialInfo& material);

  void SetShadingModelPath(const std::string& path);

  RenderSortOrder GetSortOrder(Entity e) const;
  RenderSortOrderOffset GetSortOrderOffset(Entity e) const;
  void SetSortOrderOffset(Entity e, RenderSortOrderOffset sort_order_offset);
  void SetSortOrderOffset(Entity e, HashValue pass,
                          RenderSortOrderOffset sort_order_offset);

  void SetStencilMode(Entity e, RenderStencilMode mode, int value);
  void SetStencilMode(Entity e, HashValue pass, RenderStencilMode mode,
                      int value);

  bool IsTextureSet(Entity e, int unit) const;

  bool IsTextureLoaded(Entity e, int unit) const;
  bool IsTextureLoaded(const TexturePtr& texture) const;

  bool IsReadyToRender(Entity entity) const;
  bool IsReadyToRender(Entity entity, HashValue pass) const;

  bool IsHidden(Entity e) const;

  void SetDeformationFunction(Entity e,
                              const RenderSystem::DeformationFn& deform);

  void Hide(Entity e);
  void Show(Entity e);

  HashValue GetRenderPass(Entity entity) const;
  std::vector<HashValue> GetRenderPasses(Entity entity) const;
  void SetRenderPass(Entity e, HashValue pass);

  void SetClearParams(HashValue pass, const RenderClearParams& clear_params);

  SortMode GetSortMode(HashValue pass) const;
  void SetSortMode(HashValue pass, SortMode mode);

  void SetSortVector(HashValue pass, const mathfu::vec3& vector);

  RenderCullMode GetCullMode(HashValue pass);
  void SetCullMode(HashValue pass, RenderCullMode mode);

  void SetDefaultFrontFace(RenderFrontFace face);

  /// Sets a render state to be used when rendering a specific render pass. If
  /// a pass is rendered without a state being set, a default render state will
  /// be used.
  void SetRenderState(HashValue pass, const fplbase::RenderState& state);

  /// Sets the render target to be used when rendering a specific pass.
  void SetRenderTarget(HashValue pass, HashValue render_target_name);

  // Gets the content of the render target on the CPU.
  ImageData GetRenderTargetData(HashValue render_target_name);

  void SetDepthTest(const bool enabled);
  void SetDepthWrite(const bool enabled);

  void SetViewport(const RenderView& view);

  // Sets the model_view_projection uniform.  Doesn't take effect until the next
  // call to BindShader.
  void SetClipFromModelMatrix(const mathfu::mat4& mvp);

  void SetClipFromModelMatrixFunction(
      const RenderSystem::ClipFromModelMatrixFn& func);

  mathfu::vec4 GetClearColor() const;
  void SetClearColor(float r, float g, float b, float a);

  void BeginFrame() {}
  void EndFrame() {}

  void Render(const RenderView* views, size_t num_views);
  void Render(const RenderView* views, size_t num_views, HashValue pass);

  void SetDefaultRenderPass(HashValue pass);
  HashValue GetDefaultRenderPass() const;

  // Resets the GL state to default.  It's not necessary to call this for any
  // predefined render passes, but this can be useful for any custom ones.
  void ResetState();

  // Sets the GL blend mode to |blend_mode|.
  void SetBlendMode(fplbase::BlendMode blend_mode);

  void UpdateDynamicMesh(Entity entity, MeshData::PrimitiveType primitive_type,
                         const VertexFormat& vertex_format,
                         const size_t max_vertices, const size_t max_indices,
                         MeshData::IndexType index_type,
                         const size_t max_ranges,
                         const std::function<void(MeshData*)>& update_mesh);

  void BindShader(const ShaderPtr& shader);

  void BindTexture(int unit, const TexturePtr& texture);

  void BindUniform(const char* name, const float* data, int dimension);

  void DrawMesh(const MeshData& mesh);

  /// Returns the render state cached by the renderer.
  const fplbase::RenderState& GetCachedRenderState() const;

  /// Updates the render state cached in the renderer. This should be used if
  /// your app is sharing a GL context with another framework which affects the
  /// GL state, or if you are making GL calls on your own outside of Lullaby.
  void UpdateCachedRenderState(const fplbase::RenderState& render_state);

  /// Creates a render target that can be used in a pass for rendering, and as a
  /// texture on top of an object.
  void CreateRenderTarget(HashValue render_target_name,
                          const RenderTargetCreateParams& create_params);

  Optional<HashValue> GetGroupId(Entity entity) const;
  void SetGroupId(Entity entity, const Optional<HashValue>& group_id);
  const RenderSystem::GroupParams* GetGroupParams(HashValue group_id) const;
  void SetGroupParams(HashValue group_id,
                      const RenderSystem::GroupParams& group_params);

 protected:
  /// Structure holding cached gpu state data.
  struct GPUCache {
    std::vector<TexturePtr> bound_textures;
  };

  // Information required to sort, cull, and render a single submesh.
  struct RenderObject {
    // The mesh (vertex and index buffer) to draw.
    std::shared_ptr<Mesh> mesh;
    // The shader, uniforms, and render state to use for drawing.
    std::shared_ptr<Material> material;
    // The world-space transform of the object being drawn.
    mathfu::mat4 world_from_entity_matrix;
    // A value used to optionally sort the RenderObjects.
    union {
      uint64_t sort_order;
      double z_sort_order;
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
    RenderTarget* render_target = nullptr;

    enum LayerType { kOpaque, kBlendEnabled, kNumLayers };
    RenderLayer layers[kNumLayers];
  };

  /// The complete set of passes and associated data for drawing the frame.
  using RenderData = std::unordered_map<HashValue, RenderPassDrawContainer>;

  /// Stores a mesh to be deformed at a later time.
  struct DeferredMesh {
    Entity entity;
    HashValue pass;
    HashValue mesh_id = 0;
    MeshData mesh;
  };

  void RenderAt(const RenderObject* render_object, const RenderView* views,
                size_t num_views);

  void RenderObjects(const RenderObjectVector& objects,
                     const RenderStateT& render_state, const RenderView* views,
                     size_t num_views);

  void SetMesh(Entity entity, HashValue pass, const MeshData& mesh,
               HashValue mesh_id);
  void DeformMesh(Entity entity, HashValue pass, MeshData* mesh);

  // Triggers the generation of a quad mesh for the given entity and deforms
  // that mesh if a deformation exists on that entity.
  void SetQuadImpl(Entity e, HashValue pass, const RenderQuad& quad);

  void CreateDeferredMeshes();

  void RenderDebugStats(const RenderView* views, size_t num_views);
  void UpdateSortOrder(Entity entity);
  void OnTextureLoaded(const RenderComponent& component, HashValue pass,
                       int unit, const TexturePtr& texture);
  void OnMeshLoaded(RenderComponent* render_component, HashValue pass);
  bool IsReadyToRenderImpl(const RenderComponent& component) const;

  void SetUniformImpl(RenderComponent* component, int material_index,
                      string_view name, ShaderDataType type, Span<uint8_t> data,
                      int count);
  bool GetUniformImpl(const RenderComponent* component, int material_index,
                      string_view name, size_t length, uint8_t* data_out) const;
  void SetUniformImpl(Material* material, string_view name, ShaderDataType type,
                      Span<uint8_t> data, int count);
  bool GetUniformImpl(const Material* material, string_view name, size_t length,
                      uint8_t* data_out) const;
  void SetMaterialImpl(RenderComponent* render_component, HashValue pass_hash,
                       int submesh_index, const MaterialInfo& material_info);
  void SetMaterialImpl(RenderComponent* render_component, HashValue pass_hash,
                       const MaterialInfo& material_info);

  std::shared_ptr<Material> CreateMaterialFromMaterialInfo(
      RenderComponent* render_component, const MaterialInfo& info);
  // Create shader params for a specific render component.
  ShaderCreateParams CreateShaderParams(
      const std::string& shading_model, const RenderComponent* component,
      const std::shared_ptr<Material>& material);

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

  /// Runs the lambda for all components associated with an entity. This
  /// guarantees not to pass a nullptr to the lambda.
  void ForEachComponentOfEntity(
      Entity entity,
      const std::function<void(RenderComponent*, HashValue)>& fn);

  /// Runs the lambda for all components associated with an entity.
  void ForEachComponentOfEntity(
      Entity entity,
      const std::function<void(const RenderComponent&, HashValue)>& fn) const;

  // Finds a component in a pass, or returns nullptr if it doesn't exist.
  RenderComponent* GetComponent(Entity e, HashValue pass);
  const RenderComponent* GetComponent(Entity e, HashValue pass) const;

  // Returns a render component associated with an entity, or nullptr if no
  // component is associated with the entity.
  RenderComponent* FindRenderComponentForEntity(Entity e);
  const RenderComponent* FindRenderComponentForEntity(Entity e) const;

  /// Returns true if the SortMode is independent of the view.
  static bool IsSortModeViewIndependent(SortMode mode);

  /// Sorts a vector of RenderObject by a given (non view-dependent) sort mode.
  static void SortObjects(std::vector<RenderObject>* objects, SortMode mode);

  /// Sorts a vector of RenderObject by a given view-dependent sort mode.
  static void SortObjectsUsingView(std::vector<RenderObject>* objects,
                                   SortMode mode, const RenderView* views,
                                   size_t num_views);

  NextRenderer renderer_;

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

  /// Factories used for creating rendering-related objects.
  MeshFactoryImpl* mesh_factory_;
  ShaderFactory* shader_factory_;
  TextureFactoryImpl* texture_factory_;
  TextureAtlasFactory* texture_atlas_factory_;

  std::unordered_map<Entity, RenderSystem::DeformationFn> deformations_;
  /// Since deformations require transforms and meshes can be set before the
  /// transform system has initialized, we need to delay deformations until we
  /// can be sure that the transforms are valid.
  std::queue<DeferredMesh> deferred_meshes_;

  std::unordered_map<HashValue, std::unique_ptr<RenderTarget>> render_targets_;

  std::vector<mathfu::AffineTransform> shader_transforms_;

  mathfu::vec4 clear_color_ = mathfu::kZeros4f;

  // Stores sort order offsets and calculates sort orders.
  detail::SortOrderManager sort_order_manager_;

  bool initialized_ = false;

  ShaderPtr shader_ = nullptr;
  mathfu::mat4 model_view_projection_ = mathfu::mat4::Identity();

  fplbase::RenderState render_state_;

  std::string shading_model_path_;

  // Cached GPU state.
  GPUCache gpu_cache_;

  // A cache of texture environment flag hashes.
  std::vector<HashValue> texture_flag_hashes_;

  // The function used to calculate the clip_from_model_matrix just before
  // setting the associated uniform.
  RenderSystem::ClipFromModelMatrixFn clip_from_model_matrix_func_;

  // The winding order / GL front face to use by default.
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
