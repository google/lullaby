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

#ifndef LULLABY_SYSTEMS_RENDER_FPL_RENDER_SYSTEM_FPL_H_
#define LULLABY_SYSTEMS_RENDER_FPL_RENDER_SYSTEM_FPL_H_

#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "lullaby/generated/render_def_generated.h"
#include "lullaby/base/async_processor.h"
#include "lullaby/base/system.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/systems/render/detail/display_list.h"
#include "lullaby/systems/render/detail/render_pool_map.h"
#include "lullaby/systems/render/detail/sort_order.h"
#include "lullaby/systems/render/fpl/mesh.h"
#include "lullaby/systems/render/fpl/render_component.h"
#include "lullaby/systems/render/fpl/render_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/text/html_tags.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/mesh_data.h"
#include "lullaby/util/triangle_mesh.h"
#include "lullaby/util/vertex.h"

namespace lull {

// The FPL implementation of RenderSystem.  For documentation of the public
// functions, refer to the RenderSystem class declaration.
class RenderSystemFpl : public System {
 public:
  using CullMode = RenderSystem::CullMode;
  using Deformation = RenderSystem::Deformation;
  using PrimitiveType = RenderSystem::PrimitiveType;
  using Quad = RenderSystem::Quad;
  using SortMode = RenderSystem::SortMode;
  using SortOrder = RenderSystem::SortOrder;
  using SortOrderOffset = RenderSystem::SortOrderOffset;
  using View = RenderSystem::View;

  explicit RenderSystemFpl(Registry* registry);
  ~RenderSystemFpl() override;

  void SetStereoMultiviewEnabled(bool enabled);

  void BeginRendering() {}
  void EndRendering() {}
  void SubmitRenderData() {}

  void PreloadFont(const char* name);
  FontPtr LoadFonts(const std::vector<std::string>& names);
  const TexturePtr& GetWhiteTexture() const;
  const TexturePtr& GetInvalidTexture() const;
  TexturePtr LoadTexture(const std::string& filename, bool create_mips);
  void LoadTextureAtlas(const std::string& filename);


  ShaderPtr LoadShader(const std::string& filename);

  void Create(Entity e, HashValue type, const Def* def) override;
  void Create(Entity e, RenderPass pass);
  void PostCreateInit(Entity e, HashValue type, const Def* def) override;
  void Destroy(Entity e) override;

  void ProcessTasks();
  void WaitForAssetsToLoad();

  RenderPass GetRenderPass(Entity entity) const;

  const mathfu::vec4& GetDefaultColor(Entity entity) const;
  void SetDefaultColor(Entity entity, const mathfu::vec4& color);

  bool GetColor(Entity entity, mathfu::vec4* color) const;
  void SetColor(Entity entity, const mathfu::vec4& color);

  void SetUniform(Entity e, const char* name, const float* data, int dimension,
                  int count);
  bool GetUniform(Entity e, const char* name, size_t length,
                  float* data_out) const;
  void CopyUniforms(Entity entity, Entity source);

  int GetNumBones(Entity entity) const;
  const uint8_t* GetBoneParents(Entity e, int* num) const;
  const std::string* GetBoneNames(Entity e, int* num) const;
  const mathfu::AffineTransform* GetDefaultBoneTransformInverses(
      Entity e, int* num) const;
  void SetBoneTransforms(Entity entity,
                         const mathfu::AffineTransform* transforms,
                         int num_transforms);

  void SetTexture(Entity e, int unit, const TexturePtr& texture);

  TexturePtr CreateProcessedTexture(const TexturePtr& source_texture,
                                    bool create_mips,
                                    RenderSystem::TextureProcessor processor);

  void SetTextureId(Entity e, int unit, uint32_t texture_target,
                    uint32_t texture_id);

  TexturePtr GetTexture(Entity entity, int unit) const;


  void SetText(Entity e, const std::string& text);
  const std::vector<LinkTag>* GetLinkTags(Entity e) const;

  bool GetQuad(Entity e, Quad* quad) const;
  void SetQuad(Entity e, const Quad& quad);

  // TODO(b/31523782): Remove once pipeline for MeshData is stable.
  void SetMesh(Entity e, const TriangleMesh<VertexPT>& mesh);
  void SetAndDeformMesh(Entity entity, const TriangleMesh<VertexPT>& mesh);

  void SetMesh(Entity e, const MeshData& mesh);

  void SetMesh(Entity e, const std::string& file);

  ShaderPtr GetShader(Entity entity) const;
  void SetShader(Entity e, const ShaderPtr& shader);

  void SetFont(Entity entity, const FontPtr& font);
  void SetTextSize(Entity entity, int size);

  SortOrderOffset GetSortOrderOffset(Entity e) const;
  void SetSortOrderOffset(Entity e, SortOrderOffset sort_order_offset);

  void SetStencilMode(Entity e, StencilMode mode, int value);

  bool IsTextureSet(Entity e, int unit) const;

  bool IsTextureLoaded(Entity e, int unit) const;

  bool IsTextureLoaded(const TexturePtr& texture) const;

  bool IsReadyToRender(Entity entity) const;

  bool IsHidden(Entity e) const;

  void SetDeformationFunction(Entity e, const Deformation& deform);

  void Hide(Entity e);
  void Show(Entity e);

  void SetRenderPass(Entity e, RenderPass pass);

  SortMode GetSortMode(RenderPass pass) const;
  void SetSortMode(RenderPass pass, SortMode mode);

  void SetCullMode(RenderPass pass, CullMode mode);

  void SetDepthTest(const bool enabled);
  void SetDepthWrite(const bool enabled);

  void SetViewport(const View& view);

  // Sets the model_view_projection uniform.  Doesn't take effect until the next
  // call to BindShader.
  void SetClipFromModelMatrix(const mathfu::mat4& mvp);

  void SetClearColor(float r, float g, float b, float a);

  void BeginFrame();
  void EndFrame();

  void Render(const View* views, size_t num_views);
  void Render(const View* views, size_t num_views, RenderPass pass);

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
  const fplbase::RenderState& GetRenderState() const;

  /// Updates the render state cached in the renderer. This should be used if
  /// your app is sharing a GL context with another framework which affects the
  /// GL state, or if you are making GL calls on your own outside of Lullaby.
  void UpdateCachedRenderState(const fplbase::RenderState& render_state);

 protected:
  using RenderComponent = detail::RenderComponent;
  using DisplayList = detail::DisplayList<RenderComponent>;
  using RenderPool = detail::RenderPool<RenderComponent>;
  using RenderPoolMap = detail::RenderPoolMap<RenderComponent>;
  using UniformMap = detail::RenderComponent::UniformMap;

  struct DeferredMesh {
    enum Type { kQuad, kMesh };
    Entity e = kNullEntity;
    Type type;
    RenderSystem::Quad quad;
    TriangleMesh<VertexPT> mesh;
  };

  void CreateRenderComponentFromDef(Entity e, const RenderDef& data);
  void RenderAt(const RenderComponent* component,
                const mathfu::mat4& world_from_entity_matrix, const View& view);
  void RenderAtMultiview(const RenderComponent* component,
                         const mathfu::mat4& world_from_entity_matrix,
                         const View* views);
  void RenderComponentsInPass(const View* views, size_t num_views,
                              RenderPass pass);
  void RenderDisplayList(const View& view, const DisplayList& display_list);
  void RenderDisplayListMultiview(const View* views,
                                  const DisplayList& display_list);
  void SetViewUniforms(const View& view);

  void SetMesh(Entity e, MeshPtr mesh);
  template <typename Vertex>
  MeshPtr CreateQuad(Entity e, const Quad& quad);
  template <typename Vertex>
  void DeformMesh(Entity entity, TriangleMesh<Vertex>* mesh);
  void BindStencilMode(StencilMode mode, int ref);
  void BindVertexArray(uint32_t ref);
  void ClearSamplers();

  // Triggers the generation of a quad mesh for the given entity and deforms
  // that mesh if a deformation exists on that entity.
  void SetQuadImpl(Entity e, const Quad& quad);

  void CreateDeferredMeshes();

  void UpdateUniformLocations(RenderComponent* component);

  void RenderDebugStats(const View* views, size_t num_views);
  void OnParentChanged(const ParentChangedEvent& event);
  void OnTextureLoaded(const RenderComponent& component, int unit,
                       const TexturePtr& texture);
  bool IsReadyToRenderImpl(const RenderComponent& component) const;
  void SetShaderUniforms(const UniformMap& uniforms);
  void DrawMeshFromComponent(const RenderComponent* component);

  // Thread-specific render API. Holds rendering context.
  // In multi-threaded rendering, every thread should have one of these classes.
  fplbase::Renderer renderer_;

  RenderFactory* factory_;
  RenderPoolMap render_component_pools_;
  fplbase::BlendMode blend_mode_ = fplbase::kBlendModeOff;
  int max_texture_unit_ = 0;

  std::unordered_map<Entity, Deformation> deformations_;
  std::queue<DeferredMesh> deferred_meshes_;

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
