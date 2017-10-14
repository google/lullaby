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

#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "fplbase/render_target.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/generated/render_pass_def_generated.h"
#include "lullaby/generated/render_target_def_generated.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/triangle_mesh.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/render/detail/sort_order.h"
#include "lullaby/systems/render/next/mesh.h"
#include "lullaby/systems/render/next/render_component.h"
#include "lullaby/systems/render/next/render_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/uniform.h"
#include "lullaby/systems/text/html_tags.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/async_processor.h"
#include "lullaby/util/buffered_data.h"
#include "lullaby/util/string_view.h"

namespace lull {

// The FPL implementation of RenderSystem.  For documentation of the public
// functions, refer to the RenderSystem class declaration.
class RenderSystemNext : public System {
 public:
  using CullMode = RenderSystem::CullMode;
  using Deformation = RenderSystem::Deformation;
  using PrimitiveType = RenderSystem::PrimitiveType;
  using Quad = RenderSystem::Quad;
  using SortMode = RenderSystem::SortMode;
  using SortOrder = RenderSystem::SortOrder;
  using SortOrderOffset = RenderSystem::SortOrderOffset;
  using View = RenderSystem::View;
  using RenderComponentID = HashValue;

  explicit RenderSystemNext(Registry* registry);
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

  ShaderPtr LoadShader(const std::string& filename);

  void Create(Entity e, HashValue type, const Def* def) override;
  void Create(Entity e, HashValue component_id, HashValue pass);
  void Create(Entity e, HashValue pass);
  void PostCreateInit(Entity e, HashValue type, const Def* def) override;
  void Destroy(Entity e) override;
  void Destroy(Entity e, HashValue component_id);

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
  void SetUniform(Entity e, HashValue component_id, const char* name,
                  const float* data, int dimension, int count);
  bool GetUniform(Entity e, const char* name, size_t length,
                  float* data_out) const;
  bool GetUniform(Entity e, HashValue component_id, const char* name,
                  size_t length, float* data_out) const;
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
  void SetBoneTransforms(Entity entity, HashValue component_id,
                         const mathfu::AffineTransform* transforms,
                         int num_transforms);

  void SetTexture(Entity e, int unit, const TexturePtr& texture);
  void SetTexture(Entity e, HashValue component_id, int unit,
                  const TexturePtr& texture);

  TexturePtr CreateProcessedTexture(const TexturePtr& source_texture,
                                    bool create_mips,
                                    RenderSystem::TextureProcessor processor);

  TexturePtr CreateProcessedTexture(
      const TexturePtr& source_texture, bool create_mips,
      const RenderSystem::TextureProcessor& processor,
      const mathfu::vec2i& output_dimensions);

  void SetTextureId(Entity e, int unit, uint32_t texture_target,
                    uint32_t texture_id);
  void SetTextureId(Entity e, HashValue component_id, int unit,
                    uint32_t texture_target, uint32_t texture_id);

  TexturePtr GetTexture(Entity entity, int unit) const;

  void SetPano(Entity entity, const std::string& filename,
               float heading_offset_deg);

  void SetText(Entity e, const std::string& text);
  const std::vector<LinkTag>* GetLinkTags(Entity e) const;

  bool GetQuad(Entity e, Quad* quad) const;
  void SetQuad(Entity e, const Quad& quad);
  void SetQuad(Entity e, HashValue component_id, const Quad& quad);

  // TODO(b/31523782): Remove once pipeline for MeshData is stable.
  void SetMesh(Entity e, const TriangleMesh<VertexPT>& mesh);
  void SetMesh(Entity e, HashValue component_id,
               const TriangleMesh<VertexPT>& mesh);
  void SetAndDeformMesh(Entity entity, const TriangleMesh<VertexPT>& mesh);
  void SetAndDeformMesh(Entity entity, HashValue component_id,
                        const TriangleMesh<VertexPT>& mesh);

  void SetMesh(Entity e, const MeshData& mesh);
  void SetMesh(Entity entity, HashValue component_id, const MeshData& mesh);
  void SetAndDeformMesh(Entity entity, const MeshData& mesh);
  void SetAndDeformMesh(Entity entity, HashValue component_id,
                        const MeshData& mesh);

  void SetMesh(Entity e, const std::string& file);
  void SetMesh(Entity e, HashValue component_id, const MeshPtr& mesh);
  MeshPtr GetMesh(Entity e, HashValue component_id);

  ShaderPtr GetShader(Entity entity) const;
  ShaderPtr GetShader(Entity entity, HashValue component_id) const;
  void SetShader(Entity e, HashValue component_id, const ShaderPtr& shader);
  void SetShader(Entity e, const ShaderPtr& shader);

  void SetFont(Entity entity, const FontPtr& font);
  void SetTextSize(Entity entity, int size);

  SortOrderOffset GetSortOrderOffset(Entity e) const;
  void SetSortOrderOffset(Entity e, SortOrderOffset sort_order_offset);
  void SetSortOrderOffset(Entity e, HashValue component_id,
                          SortOrderOffset sort_order_offset);

  void SetStencilMode(Entity e, StencilMode mode, int value);
  void SetStencilMode(Entity e, HashValue component_id, StencilMode mode,
                      int value);

  bool IsTextureSet(Entity e, int unit) const;

  bool IsTextureLoaded(Entity e, int unit) const;
  bool IsTextureLoaded(const TexturePtr& texture) const;

  bool IsReadyToRender(Entity entity) const;

  bool IsHidden(Entity e) const;

  void SetDeformationFunction(Entity e, const Deformation& deform);

  void Hide(Entity e);
  void Show(Entity e);

  HashValue GetRenderPass(Entity entity) const;
  void SetRenderPass(Entity e, HashValue pass);

  void SetClearParams(HashValue pass, const ClearParams& clear_params);

  SortMode GetSortMode(HashValue pass) const;
  void SetSortMode(HashValue pass, SortMode mode);

  CullMode GetCullMode(HashValue pass);
  void SetCullMode(HashValue pass, CullMode mode);

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

  void SetViewport(const View& view);

  // Sets the model_view_projection uniform.  Doesn't take effect until the next
  // call to BindShader.
  void SetClipFromModelMatrix(const mathfu::mat4& mvp);

  void SetClipFromModelMatrixFunction(
      const CalculateClipFromModelMatrixFunc& func);

  mathfu::vec4 GetClearColor() const;
  void SetClearColor(float r, float g, float b, float a);

  void BeginFrame() {}
  void EndFrame() {}

  void Render(const View* views, size_t num_views);
  void Render(const View* views, size_t num_views, HashValue pass);

  // Resets the GL state to default.  It's not necessary to call this for any
  // predefined render passes, but this can be useful for any custom ones.
  void ResetState();

  // Sets the GL blend mode to |blend_mode|.
  void SetBlendMode(fplbase::BlendMode blend_mode);

  void UpdateDynamicMesh(Entity entity, PrimitiveType primitive_type,
                         const VertexFormat& vertex_format,
                         const size_t max_vertices, const size_t max_indices,
                         const std::function<void(MeshData*)>& update_mesh);

  void BindShader(const ShaderPtr& shader);

  void BindTexture(int unit, const TexturePtr& texture);

  void BindUniform(const char* name, const float* data, int dimension);

  void DrawPrimitives(PrimitiveType primitive_type,
                      const VertexFormat& vertex_format,
                      const void* vertex_data, size_t num_vertices);

  void DrawIndexedPrimitives(PrimitiveType primitive_type,
                             const VertexFormat& vertex_format,
                             const void* vertex_data, size_t num_vertices,
                             const uint16_t* indices, size_t num_indices);

  const std::vector<mathfu::vec3>* GetCaretPositions(Entity e) const;

  /// Returns the render state cached by the FPL renderer.
  const fplbase::RenderState& GetCachedRenderState() const;

  /// Updates the render state cached in the renderer. This should be used if
  /// your app is sharing a GL context with another framework which affects the
  /// GL state, or if you are making GL calls on your own outside of Lullaby.
  void UpdateCachedRenderState(const fplbase::RenderState& render_state);

  /// Creates a render target that can be used in a pass for rendering, and as a
  /// texture on top of an object.
  void CreateRenderTarget(HashValue render_target_name,
                          const mathfu::vec2i& dimensions,
                          TextureFormat texture_format,
                          DepthStencilFormat depth_stencil_format);

  void BindUniform(const Uniform& uniform);

 protected:
  using RenderComponent = detail::RenderComponent;
  using RenderComponentPool = detail::RenderComponentPool;
  using UniformVector = std::vector<Uniform>;

  /// Minimal amount of information needed to render a single "object".
  struct RenderObject {
    MeshPtr mesh = nullptr;
    std::shared_ptr<MeshData> dynamic_mesh;
    Material material;
    RenderSystem::SortOrder sort_order = 0;
    float z_sort_order = 0.0f;
    StencilMode stencil_mode = StencilMode::kDisabled;
    int stencil_value = 0;
    mathfu::mat4 world_from_entity_matrix;
  };

  /// Structure defining data used when rendering a RenderPass.
  struct RenderPassDefinition {
    fplbase::RenderState render_state;
    ClearParams clear_params;
    SortMode sort_mode = SortMode::kNone;
    CullMode cull_mode = CullMode::kNone;
    fplbase::RenderTarget* render_target = nullptr;
  };

  /// A collection of RenderObjects and a RenderPassDefinition.
  using RenderObjectList = std::vector<RenderObject>;
  struct RenderPassAndObjects {
    RenderObjectList render_objects;
    RenderPassDefinition pass_definition;
  };

  /// The complete set of passes and objects to render.
  using RenderData = std::unordered_map<HashValue, RenderPassAndObjects>;

  /// Stores a mesh to be deformed at a later time.
  struct DeferredMesh {
    EntityIdPair entity_id_pair = kNullEntity;
    HashValue mesh_id = 0;
    MeshData mesh;
  };

  void SetRenderPass(const RenderPassDefT& data);
  void ApplyClearParams(const ClearParams& clear_params);

  void RenderAt(const RenderObject* component,
                const mathfu::mat4& world_from_entity_matrix, const View& view);
  void RenderAtMultiview(const RenderObject* component,
                         const mathfu::mat4& world_from_entity_matrix,
                         const View* views);

  void RenderObjects(const RenderObjectList& objects, const View* views,
                     size_t num_views);
  void RenderPanos(const View* views, size_t num_views);
  void SetViewUniforms(const View& view);

  void SetMesh(Entity entity, HashValue component_id, const MeshData& mesh,
               HashValue mesh_id);
  void SetMesh(Entity e, MeshPtr mesh);
  void DeformMesh(Entity entity, HashValue component_id, MeshData* mesh);
  void BindStencilMode(StencilMode mode, int ref);
  void BindVertexArray(uint32_t ref);
  void ClearSamplers();

  // Triggers the generation of a quad mesh for the given entity and deforms
  // that mesh if a deformation exists on that entity.
  void SetQuadImpl(Entity e, HashValue component_id, const Quad& quad);

  void CreateDeferredMeshes();

  void UpdateUniformLocations(RenderComponent* component);

  void RenderDebugStats(const View* views, size_t num_views);
  void OnParentChanged(const ParentChangedEvent& event);
  void OnTextureLoaded(const RenderComponent& component, int unit,
                       const TexturePtr& texture);
  void OnMeshLoaded(RenderComponent* render_component);
  bool IsReadyToRenderImpl(const RenderComponent& component) const;
  void SetShaderUniforms(const UniformVector& uniforms);
  void DrawMeshFromComponent(const RenderObject* component);

  /// Correct the front face mode according to the scalar in the matrix. This is
  /// needed for cases where the matrix contains negative scale used to flip the
  /// on an axis.
  void CorrectFrontFaceFromMatrix(const mathfu::mat4& matrix);

  /// Initializes the default render passes. This includes the following passes:
  /// RenderPass_Pano, RenderPass_Opaque, RenderPass_Main, RenderPass_OverDraw
  /// and RenderPass_OverDrawGlow.
  void InitDefaultRenderPasses();

  /// Returns true if the SortMode is independent of the view.
  static bool IsSortModeViewIndependent(SortMode mode);

  /// Sorts a vector of RenderObject by a given (non view-dependent) sort mode.
  static void SortObjects(RenderObjectList* objects, SortMode mode);

  /// Sorts a vector of RenderObject by a given view-dependent sort mode.
  static void SortObjectsUsingView(RenderObjectList* objects, SortMode mode,
                                   const RenderSystem::View* views,
                                   size_t num_views);

  // Thread-specific render API. Holds rendering context.
  // In multi-threaded rendering, every thread should have one of these classes.
  fplbase::Renderer renderer_;

  /// The set of data the renderer will use to draw things onto the screen. The
  /// RenderData is created by SubmitRenderData, and this pointer is set at
  /// BeginFrame and released at EndFrame.
  RenderData* active_render_data_ = nullptr;

  /// Buffer of RenderData objects so one thread can write data while another
  /// can use data for rendering.
  BufferedData<RenderData> render_data_buffer_;

  /// Definitions of Render Passes.
  std::unordered_map<HashValue, RenderPassDefinition> pass_definitions_;

  RenderFactory* factory_;
  RenderComponentPool components_;
  std::unordered_map<Entity, std::vector<EntityIdPair>> entity_ids_;
  fplbase::BlendMode blend_mode_ = fplbase::kBlendModeOff;
  int max_texture_unit_ = 0;

  std::unordered_map<EntityIdPair, Deformation, EntityIdPairHash> deformations_;
  /// Since deformations require transforms and meshes can be set before the
  /// transform system has initialized, we need to delay deformations until we
  /// can be sure that the transforms are valid.
  std::queue<DeferredMesh> deferred_meshes_;

  std::unordered_map<HashValue, std::unique_ptr<fplbase::RenderTarget>>
      render_targets_;

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

  ShaderPtr shader_ = nullptr;

  // The frame buffer that was bound at the beginning of rendering.
  int default_frame_buffer_ = 0;

  // The last render state bound.
  fplbase::RenderState cached_render_state_;

  // The function used to calculate the clip_from_model_matrix just before
  // setting the associated uniform.
  CalculateClipFromModelMatrixFunc clip_from_model_matrix_func_;

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
