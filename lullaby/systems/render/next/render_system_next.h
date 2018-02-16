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
#include <string>
#include <unordered_map>
#include <vector>

#include "fplbase/renderer.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/render/detail/sort_order.h"
#include "lullaby/systems/render/next/mesh.h"
#include "lullaby/systems/render/next/mesh_factory.h"
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

// The FPL implementation of RenderSystem.  For documentation of the public
// functions, refer to the RenderSystem class declaration.
class RenderSystemNext : public System {
 public:
  using InitParams = RenderSystem::InitParams;
  explicit RenderSystemNext(Registry* registry, const InitParams& init_params);
  ~RenderSystemNext() override;

  void Initialize() override;

  void SetStereoMultiviewEnabled(bool enabled);

  void PreloadFont(const char* name);
  FontPtr LoadFonts(const std::vector<std::string>& names);
  const TexturePtr& GetWhiteTexture() const;
  const TexturePtr& GetInvalidTexture() const;
  TexturePtr LoadTexture(const std::string& filename, bool create_mips);
  TexturePtr GetTexture(HashValue texture_hash) const;
  void LoadTextureAtlas(const std::string& filename);
  MeshPtr LoadMesh(const std::string& filename);
  TexturePtr CreateTexture(const ImageData& image, bool create_mips);

  ShaderPtr LoadShader(const std::string& filename);

  void Create(Entity e, HashValue type, const Def* def) override;
  void Create(Entity e, HashValue pass, HashValue pass_enum);
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

  void SetUniform(Entity e, const char* name, const float* data, int dimension,
                  int count);
  void SetUniform(Entity e, HashValue pass, const char* name, const float* data,
                  int dimension, int count);
  bool GetUniform(Entity e, const char* name, size_t length,
                  float* data_out) const;
  bool GetUniform(Entity e, HashValue pass, const char* name, size_t length,
                  float* data_out) const;
  void CopyUniforms(Entity entity, Entity source);

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
  void SetQuad(Entity e, HashValue pass, const RenderQuad& quad);

  void SetMesh(Entity e, const MeshData& mesh);
  void SetMesh(Entity entity, HashValue pass, const MeshData& mesh);
  void SetAndDeformMesh(Entity entity, const MeshData& mesh);
  void SetAndDeformMesh(Entity entity, HashValue pass, const MeshData& mesh);

  void SetMesh(Entity e, const std::string& file);
  void SetMesh(Entity e, HashValue pass, const MeshPtr& mesh);
  MeshPtr GetMesh(Entity e, HashValue pass);

  ShaderPtr GetShader(Entity entity) const;
  ShaderPtr GetShader(Entity entity, HashValue pass) const;
  void SetShader(Entity e, HashValue pass, const ShaderPtr& shader);
  void SetShader(Entity e, const ShaderPtr& shader);

  void SetMaterial(Entity e, int submesh_index, const MaterialInfo& material);

  void SetFont(Entity entity, const FontPtr& font);
  void SetTextSize(Entity entity, int size);

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

  bool IsHidden(Entity e) const;

  void SetDeformationFunction(Entity e,
                              const RenderSystem::DeformationFn& deform);

  void Hide(Entity e);
  void Show(Entity e);

  HashValue GetRenderPass(Entity entity) const;
  void SetRenderPass(Entity e, HashValue pass);

  void SetClearParams(HashValue pass, const RenderClearParams& clear_params);

  SortMode GetSortMode(HashValue pass) const;
  void SetSortMode(HashValue pass, SortMode mode);

  RenderCullMode GetCullMode(HashValue pass);
  void SetCullMode(HashValue pass, RenderCullMode mode);

  void SetDefaultFrontFace(RenderFrontFace face);

  /// Sets a render state to be used when rendering a specific render pass. If
  /// a pass is rendered without a state being set, a default render state will
  /// be used.
  void SetRenderState(HashValue pass, const fplbase::RenderState& state);

  /// Retrieves the render state associated with a specific render pass. If no
  /// state is explicitly set, |nullptr| will be returned. The returned pointer
  /// should not be stored beyond the scope as the memory address may change at
  /// run-time.
  const fplbase::RenderState* GetRenderState(HashValue pass) const;

  /// Sets the render target to be used when rendering a specific pass.
  void SetRenderTarget(HashValue pass, HashValue render_target_name);

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

  // Resets the GL state to default.  It's not necessary to call this for any
  // predefined render passes, but this can be useful for any custom ones.
  void ResetState();

  // Sets the GL blend mode to |blend_mode|.
  void SetBlendMode(fplbase::BlendMode blend_mode);

  void UpdateDynamicMesh(Entity entity, MeshData::PrimitiveType primitive_type,
                         const VertexFormat& vertex_format,
                         const size_t max_vertices, const size_t max_indices,
                         const std::function<void(MeshData*)>& update_mesh);

  void BindShader(const ShaderPtr& shader);

  void BindTexture(int unit, const TexturePtr& texture);

  void BindUniform(const char* name, const float* data, int dimension);

  void DrawMesh(const MeshData& mesh);

  /// Returns the render state cached by the FPL renderer.
  const fplbase::RenderState& GetCachedRenderState() const;

  /// Updates the render state cached in the renderer. This should be used if
  /// your app is sharing a GL context with another framework which affects the
  /// GL state, or if you are making GL calls on your own outside of Lullaby.
  void UpdateCachedRenderState(const fplbase::RenderState& render_state);

  /// Creates a render target that can be used in a pass for rendering, and as a
  /// texture on top of an object.
  void CreateRenderTarget(HashValue render_target_name,
                          const RenderTargetCreateParams& create_params);

  void BindUniform(const Uniform& uniform);

 protected:
  /// Structure holding cached gpu state data.
  struct GPUCache {
    std::vector<TexturePtr> bound_textures;
  };

  /// Minimal amount of information needed to render a single "object".
  struct RenderObject {
    MeshPtr mesh = nullptr;
    std::shared_ptr<Material> material;
    int submesh_index;
    RenderSortOrder sort_order = 0;
    float z_sort_order = 0.0f;
    mathfu::mat4 world_from_entity_matrix;
  };

  /// Structure defining data used when rendering a RenderPassObject.
  struct RenderPassObjectDefinition {
    fplbase::RenderState render_state;
    RenderClearParams clear_params;
    SortMode sort_mode = SortMode_None;
    RenderCullMode cull_mode = RenderCullMode::kNone;
    RenderTarget* render_target = nullptr;
  };

  /// Structure holding the render components and definition of a pass.
  struct RenderPassObject {
    RenderPassObject();

    RenderPassObjectDefinition definition;
    ComponentPool<RenderComponent> components;
  };

  /// The blend mode types for objects in a render pass.
  enum class RenderObjectsBlendMode { kOpaque, kBlendEnabled, kNumModes };
  // Container for render objects.
  using RenderObjectVector = std::vector<RenderObject>;

  /// A group of render objects and associated parameters for rendering.
  struct RenderPassDrawGroup {
    /// Render state associated with this draw group.
    fplbase::RenderState render_state;
    /// Sort mode associated with this draw group.
    SortMode sort_mode = SortMode_None;
    /// Objects to be rendered within this draw group.
    RenderObjectVector render_objects;
  };

  /// A container for holding the data to render entities in a render pass.
  struct RenderPassDrawContainer {
    RenderClearParams clear_params;
    RenderCullMode cull_mode = RenderCullMode::kNone;
    RenderTarget* render_target = nullptr;

    std::array<RenderPassDrawGroup,
               static_cast<size_t>(RenderObjectsBlendMode::kNumModes)>
        draw_groups;
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

  void SetRenderPass(const RenderPassDefT& data);
  void ApplyClearParams(const RenderClearParams& clear_params);

  void RenderAt(const RenderObject* render_object,
                const mathfu::mat4& world_from_entity_matrix,
                const RenderView& view);
  void RenderAtMultiview(const RenderObject* render_object,
                         const mathfu::mat4& world_from_entity_matrix,
                         const RenderView* views);

  void RenderObjects(const RenderObjectVector& objects, const RenderView* views,
                     size_t num_views);
  void RenderPanos(const RenderView* views, size_t num_views);
  void SetViewUniforms(const RenderView& view);

  void SetMesh(Entity entity, HashValue pass, const MeshData& mesh,
               HashValue mesh_id);
  void SetMesh(Entity e, MeshPtr mesh);
  void DeformMesh(Entity entity, HashValue pass, MeshData* mesh);
  void BindVertexArray(uint32_t ref);
  void ClearSamplers();
  void ApplyMaterial(const Material& material,
                     fplbase::RenderState render_state);

  // Triggers the generation of a quad mesh for the given entity and deforms
  // that mesh if a deformation exists on that entity.
  void SetQuadImpl(Entity e, HashValue pass, const RenderQuad& quad);

  void CreateDeferredMeshes();

  void UpdateUniformLocations(Material* material);

  void RenderDebugStats(const RenderView* views, size_t num_views);
  void UpdateSortOrder(Entity entity);
  void OnTextureLoaded(const RenderComponent& component, int unit,
                       const TexturePtr& texture);
  void OnMeshLoaded(RenderComponent* render_component, HashValue pass);
  bool IsReadyToRenderImpl(const RenderComponent& component) const;
  void SetShaderUniforms(const std::vector<Uniform>& uniforms);
  void DrawMeshFromComponent(const RenderObject* render_object);
  bool GetUniform(const RenderComponent* render_component, const char* name,
                  size_t length, float* data_out) const;

  /// Correct the front face mode according to the scalar in the matrix. This is
  /// needed for cases where the matrix contains negative scale used to flip the
  /// on an axis.
  void CorrectFrontFaceFromMatrix(const mathfu::mat4& matrix);

  /// Initializes the default render passes. This includes the following passes:
  /// RenderPass_Pano, RenderPass_Opaque, RenderPass_Main, RenderPass_OverDraw
  /// and RenderPass_OverDrawGlow.
  void InitDefaultRenderPassObjectes();

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
  static void SortObjects(RenderObjectVector* objects, SortMode mode);

  /// Sorts a vector of RenderObject by a given view-dependent sort mode.
  static void SortObjectsUsingView(RenderObjectVector* objects, SortMode mode,
                                   const RenderView* views, size_t num_views);

  // Thread-specific render API. Holds rendering context.
  // In multi-threaded rendering, every thread should have one of these classes.
  fplbase::Renderer renderer_;
  fplbase::BlendMode blend_mode_ = fplbase::kBlendModeOff;
  int max_texture_unit_ = 0;

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

  // This lets us know if can skip ResetState() when we're about to start a
  // render pass.
  bool known_state_ = false;

  // This lets us know if the current render call is being done for the right
  // eye instead of the left eye.
  bool rendering_right_eye_ = false;

  /// Is stereoscopic multiview rendering mode enabled?
  bool multiview_enabled_ = false;

  bool initialized_ = false;

  ShaderPtr shader_ = nullptr;

  // The frame buffer that was bound at the beginning of rendering.
  int default_frame_buffer_ = 0;

  // The render state of which materials will inherit states.
  fplbase::RenderState inherited_render_state_;

  // Cached GPU state.
  GPUCache gpu_cache_;

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
