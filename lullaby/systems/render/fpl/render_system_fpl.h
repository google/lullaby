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

#ifndef LULLABY_SYSTEMS_RENDER_FPL_RENDER_SYSTEM_FPL_H_
#define LULLABY_SYSTEMS_RENDER_FPL_RENDER_SYSTEM_FPL_H_

#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "lullaby/events/entity_events.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/render/detail/display_list.h"
#include "lullaby/systems/render/detail/render_pool_map.h"
#include "lullaby/systems/render/detail/sort_order.h"
#include "lullaby/systems/render/fpl/mesh.h"
#include "lullaby/util/async_processor.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/render/fpl/render_component.h"
#include "lullaby/systems/render/fpl/render_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

// The FPL implementation of RenderSystem.  For documentation of the public
// functions, refer to the RenderSystem class declaration.
class RenderSystemFpl : public System {
 public:
  using CullMode = RenderCullMode;
  using FrontFace = RenderFrontFace;
  using StencilMode = RenderStencilMode;
  using Deformation = RenderSystem::DeformationFn;
  using PrimitiveType = MeshData::PrimitiveType;
  using Quad = RenderQuad;
  using SortOrder = RenderSortOrder;
  using SortOrderOffset = RenderSortOrderOffset;
  using View = RenderView;
  using ClearParams = RenderClearParams;

  explicit RenderSystemFpl(Registry* registry,
                           const RenderSystem::InitParams& init_params);
  ~RenderSystemFpl() override;

  void SetStereoMultiviewEnabled(bool enabled);

  void BeginRendering();
  void EndRendering();
  void SubmitRenderData() {}

  void PreloadFont(const char* name);
  const TexturePtr& GetWhiteTexture() const;
  const TexturePtr& GetInvalidTexture() const;
  TexturePtr GetTexture(HashValue texture_hash) const;
  TexturePtr LoadTexture(const std::string& filename, bool create_mips);
  void LoadTextureAtlas(const std::string& filename);
  MeshPtr LoadMesh(const std::string& filename);
  TexturePtr CreateTexture(const ImageData& image, bool create_mips);

  ShaderPtr LoadShader(const std::string& filename);

  void Create(Entity e, HashValue type, const Def* def) override;
  void Create(Entity e, HashValue pass);
  void PostCreateInit(Entity e, HashValue type, const Def* def) override;
  void Destroy(Entity e) override;
  void Destroy(Entity e, HashValue pass);

  void ProcessTasks();
  void WaitForAssetsToLoad();

  HashValue GetRenderPass(Entity entity) const;
  std::vector<HashValue> GetRenderPasses(Entity entity) const;

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
  void SetBoneTransforms(Entity entity,
                         const mathfu::AffineTransform* transforms,
                         int num_transforms);

  void SetTexture(Entity e, int unit, const TexturePtr& texture);
  void SetTexture(Entity e, HashValue pass, int unit,
                  const TexturePtr& texture);
  void SetTextureExternal(Entity e, HashValue pass, int unit);

  TexturePtr CreateProcessedTexture(const TexturePtr& source_texture,
                                    bool create_mips,
                                    RenderSystem::TextureProcessor processor);

  TexturePtr CreateProcessedTexture(
      const TexturePtr& source_texture, bool create_mips,
      const RenderSystem::TextureProcessor& processor,
      const mathfu::vec2i& output_dimensions);

  void SetTextureId(Entity e, int unit, uint32_t texture_target,
                    uint32_t texture_id);
  void SetTextureId(Entity e, HashValue pass, int unit, uint32_t texture_target,
                    uint32_t texture_id);

  TexturePtr GetTexture(Entity entity, int unit) const;


  void SetText(Entity e, const std::string& text);

  bool GetQuad(Entity e, Quad* quad) const;
  void SetQuad(Entity e, const Quad& quad);

  void SetMesh(Entity e, const MeshData& mesh);
  void SetMesh(Entity e, HashValue pass, const MeshData& mesh);
  void SetAndDeformMesh(Entity entity, const MeshData& mesh);

  void SetMesh(Entity e, const std::string& file);
  void SetMesh(Entity e, HashValue pass, const MeshPtr& mesh);
  MeshPtr GetMesh(Entity e, HashValue pass);

  ShaderPtr GetShader(Entity entity) const;
  ShaderPtr GetShader(Entity entity, HashValue pass) const;
  void SetShader(Entity e, const ShaderPtr& shader);
  void SetShader(Entity e, HashValue pass, const ShaderPtr& shader);

  void SetMaterial(Entity e, Optional<HashValue> pass,
                   Optional<int> submesh_index, const MaterialInfo& material);
  bool IsShaderFeatureRequested(Entity entity, Optional<HashValue> pass,
                                Optional<int> submesh_index,
                                HashValue feature) const;
  void RequestShaderFeature(Entity entity, Optional<HashValue> pass,
                            Optional<int> submesh_index, HashValue feature);
  void ClearShaderFeatures(Entity entity, Optional<HashValue> pass,
                           Optional<int> submesh_index);
  void ClearShaderFeature(Entity entity, Optional<HashValue> pass,
                          Optional<int> submesh_index, HashValue feature);

  SortOrder GetSortOrder(Entity e) const;
  SortOrderOffset GetSortOrderOffset(Entity e) const;
  void SetSortOrderOffset(Entity e, SortOrderOffset sort_order_offset);
  void SetSortOrderOffset(Entity e, HashValue pass, SortOrderOffset offset);

  void SetStencilMode(Entity e, StencilMode mode, int value);
  void SetStencilMode(Entity e, HashValue pass, StencilMode mode, int value);

  bool IsTextureSet(Entity e, int unit) const;

  bool IsTextureLoaded(Entity e, int unit) const;

  bool IsTextureLoaded(const TexturePtr& texture) const;

  bool IsReadyToRender(Entity entity) const;

  bool IsReadyToRender(Entity entity, HashValue pass) const {
    return IsReadyToRender(entity);
  }

  bool IsHidden(Entity e) const;
  bool IsHidden(Entity entity, Optional<HashValue> pass,
                Optional<int> submesh_index) const;

  void SetDeformationFunction(Entity e, const Deformation& deform);

  void Hide(Entity e);
  void Hide(Entity entity, Optional<HashValue> pass,
            Optional<int> submesh_index);

  void Show(Entity e);
  void Show(Entity entity, Optional<HashValue> pass,
            Optional<int> submesh_index);

  void SetRenderPass(Entity e, HashValue pass);

  void SetRenderState(HashValue pass, const fplbase::RenderState& render_state);

  SortMode GetSortMode(HashValue pass) const;
  void SetSortMode(HashValue pass, SortMode mode);

  void SetSortVector(HashValue pass, const mathfu::vec3& vector);

  void SetClearParams(HashValue pass, const ClearParams& clear_params);

  void SetCullMode(HashValue pass, CullMode mode);

  void SetDefaultFrontFace(FrontFace face);

  void SetRenderTarget(HashValue pass, HashValue render_target_name);

  ImageData GetRenderTargetData(HashValue render_target_name);

  void SetDepthTest(const bool enabled);
  void SetDepthWrite(const bool enabled);

  void SetViewport(const View& view);

  mathfu::vec4 GetClearColor() const;
  void SetClearColor(float r, float g, float b, float a);

  void BeginFrame();
  void EndFrame();

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
                         MeshData::IndexType index_type,
                         const size_t max_ranges,
                         const std::function<void(MeshData*)>& update_mesh);

  void BindShader(const ShaderPtr& shader);

  void BindTexture(int unit, const TexturePtr& texture);

  void BindUniform(const char* name, const float* data, int dimension);

  void DrawMesh(const MeshData& mesh, Optional<mathfu::mat4> clip_from_model);

  /// Returns the render state cached by the FPL renderer.
  const fplbase::RenderState& GetCachedRenderState() const;

  /// Updates the render state cached in the renderer. This should be used if
  /// your app is sharing a GL context with another framework which affects the
  /// GL state, or if you are making GL calls on your own outside of Lullaby.
  void UpdateCachedRenderState(const fplbase::RenderState& render_state);

  void CreateRenderTarget(HashValue render_target_name,
                          const RenderTargetCreateParams& create_params);

  Optional<HashValue> GetGroupId(Entity entity) const;
  void SetGroupId(Entity entity, const Optional<HashValue>& group_id);
  const RenderSystem::GroupParams* GetGroupParams(HashValue group_id) const;
  void SetGroupParams(HashValue group_id,
                      const RenderSystem::GroupParams& group_params);
  std::string GetShaderString(Entity entity, HashValue pass, int submesh_index,
                              ShaderStageType stage) const;
  ShaderPtr CompileShaderString(const std::string& vertex_string,
                                const std::string& fragment_string);

 protected:
  using RenderComponent = detail::RenderComponent;
  using DisplayList = detail::DisplayList<RenderComponent>;
  using RenderPool = detail::RenderPool<RenderComponent>;
  using RenderPoolMap = detail::RenderPoolMap<RenderComponent>;
  using UniformVector = std::vector<Uniform>;

  // Stores a mesh to be deformed at a later time.
  struct DeferredMesh {
    Entity e = kNullEntity;
    HashValue mesh_id = 0;
    MeshData mesh;
  };

  void CreateRenderComponentFromDef(Entity e, const RenderDef& data);
  void RenderAt(const RenderComponent* component,
                const mathfu::mat4& world_from_entity_matrix, const View& view);
  void RenderAtMultiview(const RenderComponent* component,
                         const mathfu::mat4& world_from_entity_matrix,
                         const View* views);
  void RenderComponentsInPass(const View* views, size_t num_views,
                              HashValue pass);
  void RenderDisplayList(const View& view, const DisplayList& display_list);
  void RenderDisplayListMultiview(const View* views,
                                  const DisplayList& display_list);
  void SetViewUniforms(const View& view);

  void SetMesh(Entity entity, const MeshData& mesh, HashValue mesh_id);
  void SetMesh(Entity e, MeshPtr mesh);
  void DeformMesh(Entity entity, MeshData* mesh);
  void BindStencilMode(StencilMode mode, int ref);
  void BindVertexArray(uint32_t ref);
  void ClearSamplers();

  void CreateDeferredMeshes();

  void UpdateUniformLocations(RenderComponent* component);

  void RenderDebugStats(const View* views, size_t num_views);
  void UpdateSortOrder(Entity entity);
  void OnTextureLoaded(const RenderComponent& component, int unit,
                       const TexturePtr& texture);
  bool IsReadyToRenderImpl(const RenderComponent& component) const;
  void SetShaderUniforms(const ShaderPtr& shader,
                         const UniformVector& uniforms);
  void DrawMeshFromComponent(const RenderComponent* component);

  // Thread-specific render API. Holds rendering context.
  // In multi-threaded rendering, every thread should have one of these classes.
  fplbase::Renderer renderer_;

  RenderFactory* factory_;
  RenderPoolMap render_component_pools_;
  fplbase::BlendMode blend_mode_ = fplbase::kBlendModeOff;
  int max_texture_unit_ = 0;

  std::unordered_map<Entity, Deformation> deformations_;
  // Since deformations require transforms and meshes can be set before the
  // transform system has initialized, we need to delay deformations until we
  // can be sure that the transforms are valid.
  std::queue<DeferredMesh> deferred_meshes_;

  std::vector<mathfu::AffineTransform> shader_transforms_;

  ClearParams clear_params_;

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

  // The winding order / GL front face to use by default.
  FrontFace default_front_face_ = FrontFace::kCounterClockwise;

  RenderSystemFpl(const RenderSystemFpl&) = delete;
  RenderSystemFpl& operator=(const RenderSystemFpl&) = delete;
};

#ifdef LULLABY_RENDER_BACKEND_FPL
class RenderSystemImpl : public RenderSystemFpl {
 public:
  using RenderSystemFpl::RenderSystemFpl;
};
#endif  // LULLABY_RENDER_BACKEND_FPL

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RenderSystemFpl);

#endif  // LULLABY_SYSTEMS_RENDER_FPL_RENDER_SYSTEM_FPL_H_
