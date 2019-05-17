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

#ifndef LULLABY_SYSTEMS_RENDER_TESTING_MOCK_RENDER_SYSTEM_IMPL_H_
#define LULLABY_SYSTEMS_RENDER_TESTING_MOCK_RENDER_SYSTEM_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/registry.h"

namespace lull {
class Font;

// A mock implementation of the lullaby RenderSystem.
class RenderSystemImplInternal {
 public:
  using FontPtr = std::shared_ptr<Font>;

  explicit RenderSystemImplInternal(
      Registry*, const RenderSystem::InitParams& init_params) {}

  MOCK_METHOD0(Initialize, void());
  MOCK_METHOD0(SubmitRenderData, void());
  MOCK_METHOD0(BeginRendering, void());
  MOCK_METHOD0(EndRendering, void());
  MOCK_METHOD1(SetStereoMultiviewEnabled, void(bool enabled));
  MOCK_METHOD3(Create, void(Entity e, HashValue type, const System::Def* def));
  MOCK_METHOD2(Create, void(Entity e, HashValue));
  MOCK_METHOD3(PostCreateInit,
               void(Entity e, HashValue type, const System::Def* def));
  MOCK_METHOD1(Destroy, void(Entity e));
  MOCK_METHOD2(Destroy, void(Entity e, HashValue pass));

  MOCK_METHOD1(GetRenderPass, HashValue(Entity e));
  MOCK_METHOD1(GetRenderPasses, std::vector<HashValue>(Entity entity));

  MOCK_METHOD1(PreloadFont, void(const char* name));
  MOCK_CONST_METHOD0(GetWhiteTexture, const TexturePtr&());
  MOCK_CONST_METHOD0(GetInvalidTexture, const TexturePtr&());
  MOCK_METHOD1(GetTexture, TexturePtr(HashValue texture_hash));
  MOCK_METHOD2(LoadTexture,
               TexturePtr(const std::string& filename, bool create_mips));
  MOCK_METHOD1(LoadTextureAtlas, void(const std::string& filename));
  MOCK_METHOD2(CreateTexture,
               TexturePtr(const ImageData& image, bool create_mips));
  MOCK_METHOD1(LoadShader, ShaderPtr(const std::string& filename));
  MOCK_METHOD1(LoadMesh, MeshPtr(const std::string& filename));

  MOCK_METHOD0(ProcessTasks, void());
  MOCK_METHOD0(WaitForAssetsToLoad, void());

  MOCK_METHOD3(SetTexture, void(Entity e, int unit, const TexturePtr& texture));
  MOCK_METHOD4(SetTexture, void(Entity e, HashValue pass, int unit,
                                const TexturePtr& texture));
  MOCK_METHOD3(SetTextureExternal, void(Entity e, HashValue pass, int unit));

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
               void(Entity e, HashValue pass, int unit, uint32_t texture_target,
                    uint32_t texture_id));
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
  MOCK_METHOD6(SetUniform, void(Entity e, HashValue pass, const char* name,
                                const float* data, int dimension, int count));
  MOCK_METHOD7(SetUniform,
               void(Entity entity, Optional<HashValue> pass,
                    Optional<int> submesh_index, string_view name,
                    ShaderDataType type, Span<uint8_t> data, int count));
  MOCK_METHOD4(GetUniform, bool(Entity e, const char* name, size_t length,
                                float* data_out));
  MOCK_METHOD5(GetUniform, bool(Entity e, HashValue pass, const char* name,
                                size_t length, float* data_out));
  MOCK_METHOD6(GetUniform, bool(Entity entity, Optional<HashValue> pass,
                                Optional<int> submesh_index, string_view name,
                                size_t length, uint8_t* data_out));
  MOCK_METHOD2(CopyUniforms, void(Entity entity, Entity source));
  MOCK_METHOD3(SetUniformChangedCallback,
               void(Entity entity, HashValue pass,
                    RenderSystem::UniformChangedCallback callback));
  MOCK_METHOD2(SetText, void(Entity e, const std::string& text));

  MOCK_METHOD2(GetQuad, bool(Entity e, RenderQuad* quad));
  MOCK_METHOD2(SetQuad, void(Entity e, const RenderQuad& quad));
  MOCK_METHOD2(SetAndDeformMesh, void(Entity e, const MeshData& mesh));
  MOCK_METHOD3(SetMesh, void(Entity e, HashValue pass, MeshPtr mesh));
  MOCK_METHOD2(SetMesh, void(Entity e, const MeshData& mesh));
  MOCK_METHOD3(SetMesh, void(Entity e, HashValue pass, const MeshData& mesh));
  MOCK_METHOD2(SetMesh, void(Entity e, const std::string& file));
  MOCK_METHOD2(GetMesh, MeshPtr(Entity e, HashValue pass));
  MOCK_METHOD1(GetShader, ShaderPtr(Entity e));
  MOCK_METHOD2(GetShader, ShaderPtr(Entity e, HashValue pass));
  MOCK_METHOD2(SetShader, void(Entity e, const ShaderPtr& shader));
  MOCK_METHOD3(SetShader,
               void(Entity e, HashValue pass, const ShaderPtr& shader));
  MOCK_METHOD4(SetMaterial,
               void(Entity e, Optional<HashValue> pass,
                    Optional<int> submesh_index, const MaterialInfo& material));
  MOCK_CONST_METHOD4(IsShaderFeatureRequested,
                     bool(Entity entity, Optional<HashValue> pass,
                          Optional<int> submesh_index, HashValue feature));
  MOCK_METHOD4(RequestShaderFeature,
               std::set<HashValue>(Entity entity, Optional<HashValue> pass,
                                   Optional<int> submesh_index,
                                   HashValue feature));
  MOCK_METHOD4(ClearShaderFeature,
               std::set<HashValue>(Entity entity, Optional<HashValue> pass,
                                   Optional<int> submesh_index,
                                   HashValue feature));
  MOCK_METHOD3(ClearShaderFeatures,
               bool(Entity entity, Optional<HashValue> pass,
                    Optional<int> submesh_index));
  MOCK_METHOD2(SetSortOrderOffset,
               void(Entity e, RenderSortOrderOffset sort_order_offset));
  MOCK_METHOD3(SetSortOrderOffset,
               void(Entity e, HashValue pass,
                    RenderSortOrderOffset sort_order_offset));
  MOCK_METHOD3(SetStencilMode,
               void(Entity e, RenderStencilMode mode, int value));
  MOCK_METHOD4(SetStencilMode, void(Entity e, HashValue pass,
                                    RenderStencilMode mode, int value));
  MOCK_METHOD2(SetDeformationFunction,
               void(Entity e, const RenderSystem::DeformationFn& deform));
  MOCK_METHOD1(Hide, void(Entity e));
  MOCK_METHOD3(Hide, void(Entity e, Optional<HashValue> pass,
                          Optional<int> submesh_index));
  MOCK_METHOD1(Show, void(Entity e));
  MOCK_METHOD3(Show, void(Entity e, Optional<HashValue> pass,
                          Optional<int> submesh_index));
  MOCK_METHOD2(SetRenderPass, void(Entity e, HashValue pass));
  MOCK_CONST_METHOD1(GetSortMode, SortMode(HashValue pass));
  MOCK_METHOD2(SetSortMode, void(HashValue pass, SortMode mode));
  MOCK_METHOD2(SetSortVector, void(HashValue pass, const mathfu::vec3& vector));
  MOCK_METHOD2(SetRenderState,
               void(HashValue pass, const fplbase::RenderState& render_state));
  MOCK_METHOD2(SetClearParams,
               void(HashValue pass, const RenderClearParams& clear_params));
  MOCK_METHOD2(SetCullMode, void(HashValue pass, RenderCullMode mode));
  MOCK_METHOD1(SetDefaultFrontFace, void(RenderFrontFace face));
  MOCK_METHOD2(CreateRenderTarget,
               void(HashValue render_target_name,
                    const RenderTargetCreateParams& create_params));
  MOCK_METHOD1(SetDepthTest, void(const bool enabled));
  MOCK_METHOD1(SetDepthWrite, void(const bool enabled));
  MOCK_METHOD1(SetBlendMode, void(const fplbase::BlendMode blend_mode));
  MOCK_METHOD1(SetViewport, void(const RenderView& view));
  MOCK_CONST_METHOD1(GetNumBones, int(Entity e));
  MOCK_CONST_METHOD2(GetBoneParents, const uint8_t*(Entity e, int* num));
  MOCK_CONST_METHOD2(GetBoneNames, const std::string*(Entity e, int* num));
  MOCK_CONST_METHOD2(GetDefaultBoneTransformInverses,
                     const mathfu::AffineTransform*(Entity e, int* num));
  MOCK_METHOD3(SetBoneTransforms,
               void(Entity entity, const mathfu::AffineTransform* transforms,
                    int num_transforms));
  MOCK_CONST_METHOD1(GetSortOrder, RenderSortOrder(Entity e));
  MOCK_CONST_METHOD1(GetSortOrderOffset, RenderSortOrderOffset(Entity e));
  MOCK_CONST_METHOD2(IsTextureSet, bool(Entity e, int unit));
  MOCK_CONST_METHOD2(IsTextureLoaded, bool(Entity e, int unit));
  MOCK_CONST_METHOD1(IsTextureLoaded, bool(const TexturePtr&));
  MOCK_CONST_METHOD1(IsReadyToRender, bool(Entity e));
  MOCK_CONST_METHOD2(IsReadyToRender, bool(Entity e, HashValue p));
  MOCK_CONST_METHOD1(IsHidden, bool(Entity e));
  MOCK_CONST_METHOD3(IsHidden, bool(Entity e, Optional<HashValue> pass,
                                    Optional<int> submesh_index));

  MOCK_CONST_METHOD0(GetCachedRenderState, const fplbase::RenderState&());
  MOCK_METHOD1(UpdateCachedRenderState,
               void(const fplbase::RenderState& render_state));

  MOCK_METHOD0(BeginFrame, void());
  MOCK_METHOD0(EndFrame, void());

  MOCK_METHOD6(UpdateDynamicMesh,
               void(Entity entity, MeshData::PrimitiveType primitive_type,
                    const VertexFormat& vertex_format, size_t max_vertices,
                    size_t max_indices,
                    const std::function<void(MeshData*)>& update_mesh));
  MOCK_METHOD8(UpdateDynamicMesh,
               void(Entity entity, MeshData::PrimitiveType primitive_type,
                    const VertexFormat& vertex_format, size_t max_vertices,
                    size_t max_indices, MeshData::IndexType index_type,
                    const size_t max_ranges,
                    const std::function<void(MeshData*)>& update_mesh));

  MOCK_METHOD1(BindShader, void(const ShaderPtr& shader));
  MOCK_METHOD2(BindTexture, void(int unit, const TexturePtr& texture));
  MOCK_METHOD3(BindUniform,
               void(const char* name, const float* data, int dimension));

  MOCK_METHOD2(DrawMesh, void(const MeshData& mesh,
                              Optional<mathfu::mat4> clip_from_model));

  MOCK_CONST_METHOD0(GetClearColor, mathfu::vec4());
  MOCK_METHOD4(SetClearColor, void(float r, float g, float b, float a));

  MOCK_METHOD2(Render, void(const RenderView* views, size_t num_views));
  MOCK_METHOD3(Render,
               void(const RenderView* views, size_t num_views, HashValue pass));

  MOCK_METHOD1(SetDefaultRenderPass, void(HashValue pass));
  MOCK_METHOD0(GetDefaultRenderPass, HashValue());

  MOCK_METHOD2(SetRenderTarget,
               void(HashValue pass, HashValue render_target_name));
  MOCK_METHOD1(GetRenderTargetData, ImageData(HashValue render_target_name));

  MOCK_METHOD1(GetGroupId, Optional<HashValue>(Entity entity));
  MOCK_METHOD2(SetGroupId,
               void(Entity entity, const Optional<HashValue>& group_id));
  MOCK_METHOD1(GetGroupParams,
               const RenderSystem::GroupParams*(HashValue group_id));
  MOCK_METHOD2(SetGroupParams,
               void(HashValue group_id,
                    const RenderSystem::GroupParams& group_params));
  MOCK_CONST_METHOD4(GetShaderString,
                     std::string(Entity entity, HashValue pass,
                                 int submesh_index, ShaderStageType stage));
  MOCK_METHOD2(CompileShaderString,
               ShaderPtr(const std::string& vertex_string,
                         const std::string& fragment_string));
};

using NiceMockRenderSystem = ::testing::NiceMock<RenderSystemImplInternal>;

// To use this implementation in tests, link against the
// lullaby:mock_render_system target and simply instantiate a
// RenderSystem as usual. To setup the behavior of the mock, get a handle to it
// through the RenderGetImpl() method. See the
// mock_render_system_test.cc for an example.
// Uses NiceMock so test won't output unnecessary warnings in test log.
class RenderSystemImpl : public NiceMockRenderSystem {
 public:
  explicit RenderSystemImpl(Registry* registry,
                            const RenderSystem::InitParams& init_params)
      : NiceMockRenderSystem(registry, init_params) {}
};

using MockRenderSystemImpl = RenderSystemImpl;

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_TESTING_MOCK_RENDER_SYSTEM_IMPL_H_
