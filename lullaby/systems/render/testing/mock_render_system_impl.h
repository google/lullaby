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

#ifndef LULLABY_SYSTEMS_RENDER_TESTING_MOCK_RENDER_SYSTEM_IMPL_H_
#define LULLABY_SYSTEMS_RENDER_TESTING_MOCK_RENDER_SYSTEM_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "googlemock/include/gmock/gmock.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/registry.h"

namespace lull {
class Font;

// A mock implementation of the lullaby RenderSystem.
class RenderSystemImplInternal {
 public:
  using FontPtr = std::shared_ptr<Font>;

  explicit RenderSystemImplInternal(Registry*) {}

  MOCK_METHOD0(Initialize, void());
  MOCK_METHOD0(SubmitRenderData, void());
  MOCK_METHOD0(BeginRendering, void());
  MOCK_METHOD0(EndRendering, void());
  MOCK_METHOD1(SetStereoMultiviewEnabled, void(bool enabled));
  MOCK_METHOD3(Create,
               void(Entity e, HashValue type, const RenderSystem::Def* def));
  MOCK_METHOD2(Create, void(Entity e, RenderPass));
  MOCK_METHOD3(Create, void(Entity e, HashValue component_id, RenderPass));
  MOCK_METHOD3(PostCreateInit,
               void(Entity e, HashValue type, const RenderSystem::Def* def));
  MOCK_METHOD1(Destroy, void(Entity e));
  MOCK_METHOD2(Destroy, void(Entity e, HashValue component_id));

  MOCK_METHOD1(GetRenderPass, RenderPass(Entity e));

  MOCK_METHOD1(PreloadFont, void(const char* name));
  MOCK_METHOD1(LoadFonts, FontPtr(const std::vector<std::string>& names));
  MOCK_CONST_METHOD0(GetWhiteTexture, const TexturePtr&());
  MOCK_CONST_METHOD0(GetInvalidTexture, const TexturePtr&());
  MOCK_METHOD1(GetTexture, TexturePtr(HashValue texture_hash));
  MOCK_METHOD2(LoadTexture,
               TexturePtr(const std::string& filename, bool create_mips));
  MOCK_METHOD1(LoadTextureAtlas, void(const std::string& filename));
  MOCK_METHOD1(LoadShader, ShaderPtr(const std::string& filename));
  MOCK_METHOD1(LoadMesh, MeshPtr(const std::string& filename));

  MOCK_METHOD0(ProcessTasks, void());
  MOCK_METHOD0(WaitForAssetsToLoad, void());

  MOCK_METHOD3(SetTexture, void(Entity e, int unit, const TexturePtr& texture));
  MOCK_METHOD4(SetTexture, void(Entity e, HashValue component_id, int unit,
                                const TexturePtr& texture));

  MOCK_METHOD3(CreateProcessedTexture,
               TexturePtr(const TexturePtr& source_texture, bool create_mips,
                   RenderSystem::TextureProcessor processor));

  MOCK_METHOD4(CreateProcessedTexture,
               TexturePtr(const TexturePtr& source_texture, bool create_mips,
                          RenderSystem::TextureProcessor processor,
                          const mathfu::vec2i& output_dimensions));

  MOCK_METHOD4(SetTextureId, void(Entity e, int unit, uint32_t texture_target,
      uint32_t texture_id));
  MOCK_METHOD5(SetTextureId,
               void(Entity e, HashValue component_id, int unit,
                    uint32_t texture_target, uint32_t texture_id));
  MOCK_METHOD2(GetTexture, TexturePtr(Entity e, int unit));
  MOCK_METHOD3(SetPano, void(Entity entity, const std::string& filename,
      float heading_offset_deg));

  MOCK_CONST_METHOD1(GetDefaultColor, const mathfu::vec4&(Entity entity));
  MOCK_METHOD2(SetDefaultColor, void(Entity e, const mathfu::vec4& color));

  MOCK_METHOD2(GetColor, bool(Entity entity, mathfu::vec4* color));
  MOCK_METHOD2(SetColor, void(Entity e, const mathfu::vec4& color));

  MOCK_METHOD4(SetUniform, void(Entity e, const char* name, const float* data,
      int dimension));
  MOCK_METHOD5(SetUniform, void(Entity e, const char* name, const float* data,
      int dimension, int count));
  MOCK_METHOD6(SetUniform,
               void(Entity e, HashValue component_id, const char* name,
                    const float* data, int dimension, int count));
  MOCK_METHOD4(GetUniform, bool(Entity e, const char* name, size_t length,
      float* data_out));
  MOCK_METHOD5(GetUniform,
               bool(Entity e, HashValue component_id, const char* name,
                    size_t length, float* data_out));
  MOCK_METHOD2(CopyUniforms, void(Entity entity, Entity source));
  MOCK_METHOD2(SetText, void(Entity e, const std::string& text));
  MOCK_CONST_METHOD1(GetLinkTags, std::vector<LinkTag>*(Entity e));

  MOCK_METHOD2(GetQuad, bool(Entity e, RenderSystem::Quad* quad));
  MOCK_METHOD2(SetQuad, void(Entity e, const RenderSystem::Quad& quad));
  MOCK_METHOD3(SetQuad, void(Entity e, HashValue component_id,
                             const RenderSystem::Quad& quad));
  MOCK_METHOD2(SetMesh, void(Entity e, const TriangleMesh<VertexPT>& mesh));
  MOCK_METHOD3(SetMesh, void(Entity e, HashValue component_id,
                             const TriangleMesh<VertexPT>& mesh));
  MOCK_METHOD2(SetAndDeformMesh, void(Entity e, const MeshData& mesh));
  MOCK_METHOD2(SetAndDeformMesh,
               void(Entity e, const TriangleMesh<VertexPT>& mesh));
  MOCK_METHOD3(SetAndDeformMesh, void(Entity e, HashValue component_id,
                                      const TriangleMesh<VertexPT>& mesh));
  MOCK_METHOD3(SetMesh, void(Entity e, HashValue component_id, MeshPtr mesh));
  MOCK_METHOD2(SetMesh, void(Entity e, const MeshData& mesh));
  MOCK_METHOD2(SetMesh, void(Entity e, const std::string& file));
  MOCK_METHOD2(GetMesh, MeshPtr(Entity e, HashValue component_id));
  MOCK_METHOD1(GetShader, ShaderPtr(Entity e));
  MOCK_METHOD2(GetShader, ShaderPtr(Entity e, HashValue component_id));
  MOCK_METHOD2(SetShader, void(Entity e, const ShaderPtr& shader));
  MOCK_METHOD3(SetShader,
               void(Entity e, HashValue component_id, const ShaderPtr& shader));
  MOCK_METHOD2(SetFont, void(Entity entity, const FontPtr& font));
  MOCK_METHOD2(SetTextSize, void(Entity entity, int size));
  MOCK_METHOD2(SetSortOrderOffset,
               void(Entity e, RenderSystem::SortOrderOffset sort_order_offset));
  MOCK_METHOD3(SetSortOrderOffset,
               void(Entity e, HashValue component_id,
                    RenderSystem::SortOrderOffset sort_order_offset));
  MOCK_METHOD3(SetStencilMode, void(Entity e, StencilMode mode, int value));
  MOCK_METHOD2(SetDeformationFunction,
               void(Entity e, const RenderSystem::Deformation& deform));
  MOCK_METHOD1(Hide, void(Entity e));
  MOCK_METHOD1(Show, void(Entity e));
  MOCK_METHOD2(SetRenderPass, void(Entity e, RenderPass pass));
  MOCK_CONST_METHOD1(GetSortMode, RenderSystem::SortMode(RenderPass pass));
  MOCK_METHOD2(SetSortMode, void(RenderPass pass, RenderSystem::SortMode mode));
  MOCK_METHOD2(SetRenderState,
               void(HashValue pass, const fplbase::RenderState& render_state));
  MOCK_METHOD2(SetClearParams,
               void(HashValue pass, const ClearParams& clear_params));
  MOCK_METHOD2(SetCullMode, void(RenderPass pass, RenderSystem::CullMode mode));
  MOCK_METHOD4(CreateRenderTarget,
               void(HashValue render_target_name,
                    const mathfu::vec2i& dimensions,
                    TextureFormat texture_format,
                    DepthStencilFormat depth_stencil_format));
  MOCK_METHOD1(SetDepthTest, void(const bool enabled));
  MOCK_METHOD1(SetDepthWrite, void(const bool enabled));
  MOCK_METHOD1(SetBlendMode, void(const fplbase::BlendMode blend_mode));
  MOCK_METHOD1(SetViewport, void(const RenderSystem::View& view));
  MOCK_METHOD1(SetClipFromModelMatrix, void(const mathfu::mat4& mvp));
  MOCK_METHOD1(SetClipFromModelMatrixFunction,
               void(const CalculateClipFromModelMatrixFunc& func));

  MOCK_CONST_METHOD1(GetNumBones, int(Entity e));
  MOCK_CONST_METHOD2(GetBoneParents, const uint8_t*(Entity e, int* num));
  MOCK_CONST_METHOD2(GetBoneNames, const std::string*(Entity e, int* num));
  MOCK_CONST_METHOD2(GetDefaultBoneTransformInverses,
                     const mathfu::AffineTransform*(Entity e, int* num));
  MOCK_METHOD3(SetBoneTransforms,
               void(Entity entity, const mathfu::AffineTransform* transforms,
                   int num_transforms));
  MOCK_CONST_METHOD1(GetSortOrderOffset,
                     RenderSystem::SortOrderOffset(Entity e));
  MOCK_CONST_METHOD2(IsTextureSet, bool(Entity e, int unit));
  MOCK_CONST_METHOD2(IsTextureLoaded, bool(Entity e, int unit));
  MOCK_CONST_METHOD1(IsTextureLoaded, bool(const TexturePtr&));
  MOCK_CONST_METHOD1(IsReadyToRender, bool(Entity e));
  MOCK_CONST_METHOD1(IsHidden, bool(Entity e));

  MOCK_CONST_METHOD0(GetCachedRenderState, const fplbase::RenderState&());
  MOCK_METHOD1(UpdateCachedRenderState,
               void(const fplbase::RenderState& render_state));

  MOCK_METHOD0(BeginFrame, void());
  MOCK_METHOD0(EndFrame, void());

  MOCK_METHOD6(UpdateDynamicMesh,
               void(Entity entity, RenderSystem::PrimitiveType primitive_type,
                    const VertexFormat& vertex_format, size_t max_vertices,
                    size_t max_indices,
                    const std::function<void(MeshData*)>& update_mesh));

  MOCK_METHOD1(BindShader, void(const ShaderPtr& shader));
  MOCK_METHOD2(BindTexture, void(int unit, const TexturePtr& texture));
  MOCK_METHOD3(BindUniform,
               void(const char* name, const float* data, int dimension));
  MOCK_METHOD4(DrawPrimitives,
               void(RenderSystem::PrimitiveType primitive_type,
                   const VertexFormat& vertex_format, const void* vertex_data,
                   size_t num_vertices));

  MOCK_METHOD6(DrawIndexedPrimitives,
               void(RenderSystem::PrimitiveType primitive_type,
                   const VertexFormat& vertex_format, const void* vertex_data,
                   size_t num_vertices, const uint16_t* indices,
                   size_t num_indices));

  MOCK_CONST_METHOD0(GetClearColor, mathfu::vec4());
  MOCK_METHOD4(SetClearColor, void(float r, float g, float b, float a));

  MOCK_METHOD2(Render, void(const RenderSystem::View* views, size_t num_views));
  MOCK_METHOD3(Render, void(const RenderSystem::View* views, size_t num_views,
      RenderPass pass));

  MOCK_CONST_METHOD1(GetCaretPositions,
                     const std::vector<mathfu::vec3>*(Entity e));
  MOCK_METHOD2(SetRenderTarget,
               void(HashValue pass, HashValue render_target_name));

  MOCK_METHOD1(BindUniform, void(const Uniform& uniform));
};

using NiceMockRenderSystem = ::testing::NiceMock<RenderSystemImplInternal>;

// To use this implementation in tests, link against the
// third_party/lullaby:mock_render_system target and simply instantiate a
// RenderSystem as usual. To setup the behavior of the mock, get a handle to it
// through the RenderSystem::GetImpl() method. See the
// mock_render_system_test.cc for an example.
// Uses NiceMock so test won't output unnecessary warnings in test log.
class RenderSystemImpl : public NiceMockRenderSystem {
 public:
  explicit RenderSystemImpl(Registry* registry)
      : NiceMockRenderSystem(registry) {}
};

using MockRenderSystemImpl = RenderSystemImpl;

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_TESTING_MOCK_RENDER_SYSTEM_IMPL_H_
