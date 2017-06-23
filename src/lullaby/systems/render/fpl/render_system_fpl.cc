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

#include "lullaby/systems/render/fpl/render_system_fpl.h"

#include <inttypes.h>
#include <stdio.h>

#include "fplbase/glplatform.h"
#include "fplbase/internal/type_conversions_gl.h"
#include "fplbase/render_utils.h"
#include "lullaby/generated/render_def_generated.h"
#include "lullaby/base/dispatcher.h"
#include "lullaby/base/entity_factory.h"
#include "lullaby/events/render_events.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/render/detail/profiler.h"
#include "lullaby/systems/render/render_stats.h"
#include "lullaby/systems/render/simple_font.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/util/config.h"
#include "lullaby/util/file.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/mathfu_fb_conversions.h"
#include "lullaby/util/mesh_util.h"
#include "lullaby/util/trace.h"
#include "lullaby/util/triangle_mesh.h"

namespace lull {
namespace {
const HashValue kRenderDefHash = Hash("RenderDef");
constexpr int kNumVec4sInAffineTransform = 3;
constexpr const char* kColorUniform = "color";
constexpr const char* kTextureBoundsUniform = "uv_bounds";
constexpr const char* kClampBoundsUniform = "clamp_bounds";
constexpr const char* kBoneTransformsUniform = "bone_transforms";
// We break the naming convention here for compatibility with early VR apps.
constexpr const char* kIsRightEyeUniform = "uIsRightEye";

bool IsSupportedUniformDimension(int dimension) {
  return (dimension == 1 || dimension == 2 || dimension == 3 ||
          dimension == 4 || dimension == 16);
}

void SetDebugUniform(Shader* shader, const char* name, const float values[4]) {
  const fplbase::UniformHandle location = shader->FindUniform(name);
  if (fplbase::ValidUniformHandle(location)) {
    shader->SetUniform(location, values, 4);
  }
}

void DrawDynamicMesh(const MeshData* mesh) {
  const fplbase::Mesh::Primitive prim =
      Mesh::GetFplPrimitiveType(mesh->GetPrimitiveType());
  const VertexFormat& vertex_format = mesh->GetVertexFormat();
  const uint32_t vertex_size =
      static_cast<uint32_t>(vertex_format.GetVertexSize());
  fplbase::Attribute fpl_attribs[Mesh::kMaxFplAttributeArraySize];
  Mesh::GetFplAttributes(vertex_format, fpl_attribs);

  if (mesh->GetNumIndices() > 0) {
    fplbase::RenderArray(prim, static_cast<int>(mesh->GetNumIndices()),
                         fpl_attribs, vertex_size, mesh->GetVertexBytes(),
                         mesh->GetIndexData());
  } else {
    fplbase::RenderArray(prim, mesh->GetNumVertices(), fpl_attribs, vertex_size,
                         mesh->GetVertexBytes());
  }
}

}  // namespace

RenderSystemFpl::RenderSystemFpl(Registry* registry)
    : System(registry),
      render_component_pools_(registry),
      sort_order_manager_(registry_) {
  renderer_.Initialize(mathfu::kZeros2i, "lull::RenderSystem");

  factory_ = registry->Create<RenderFactory>(registry, &renderer_);

  SetSortMode(RenderPass_Opaque, SortMode::kAverageSpaceOriginFrontToBack);

  SetSortMode(RenderPass_Main, SortMode::kSortOrderIncreasing);
  SetCullMode(RenderPass_Main, CullMode::kNone);

  registry_->Get<Dispatcher>()->Connect(
      this,
      [this](const ParentChangedEvent& event) { OnParentChanged(event); });
}

RenderSystemFpl::~RenderSystemFpl() {
  registry_->Get<Dispatcher>()->DisconnectAll(this);
}

void RenderSystemFpl::SetStereoMultiviewEnabled(bool enabled) {
  multiview_enabled_ = enabled;
}

void RenderSystemFpl::PreloadFont(const char* name) {
  // TODO(b/33705809) Remove after apps use TextSystem directly.
  std::string filename(name);
  if (!EndsWith(filename, ".ttf")) {
    filename.append(".ttf");
  }

  TextSystem* text_system = registry_->Get<TextSystem>();
  CHECK(text_system) << "Missing text system.";
  text_system->LoadFonts({filename});
}

FontPtr RenderSystemFpl::LoadFonts(const std::vector<std::string>& names) {
  // TODO(b/33705809) Remove after apps use TextSystem directly.
  TextSystem* text_system = registry_->Get<TextSystem>();
  CHECK(text_system) << "Missing text system.";
  return text_system->LoadFonts(names);
}

const TexturePtr& RenderSystemFpl::GetWhiteTexture() const {
  return factory_->GetWhiteTexture();
}

const TexturePtr& RenderSystemFpl::GetInvalidTexture() const {
  return factory_->GetInvalidTexture();
}

TexturePtr RenderSystemFpl::LoadTexture(const std::string& filename,
                                        bool create_mips) {
  return factory_->LoadTexture(filename, create_mips);
}

void RenderSystemFpl::LoadTextureAtlas(const std::string& filename) {
  const bool create_mips = false;
  factory_->LoadTextureAtlas(filename, create_mips);
}


ShaderPtr RenderSystemFpl::LoadShader(const std::string& filename) {
  return factory_->LoadShader(filename);
}

void RenderSystemFpl::Create(Entity e, HashValue type, const Def* def) {
  if (type == kRenderDefHash) {
    CreateRenderComponentFromDef(e, *ConvertDef<RenderDef>(def));
  } else {
    LOG(DFATAL)
        << "Invalid type passed to Create.";
  }
}

void RenderSystemFpl::Create(Entity e, RenderPass pass) {
  RenderComponent& component =
      render_component_pools_.EmplaceComponent(e, pass);
  component.pass = pass;

  sort_order_manager_.UpdateSortOrder(
      e, [this](Entity entity) {
        return render_component_pools_.GetComponent(entity);
      });
}


void RenderSystemFpl::CreateRenderComponentFromDef(Entity e,
                                                   const RenderDef& data) {
  RenderComponent* component;
  if (data.hidden()) {
    component = &render_component_pools_.GetPool(RenderPass_Invisible)
        .EmplaceComponent(e);
  } else {
    component = &render_component_pools_.GetPool(data.pass())
        .EmplaceComponent(e);
  }
  component->pass = data.pass();
  component->hidden = data.hidden();

  if (data.shader()) {
    SetShader(e, LoadShader(data.shader()->str()));
  }

  if (data.font()) {
    // TODO(b/33705809) Remove after apps use TextSystem directly.
    TextSystem* text_system = registry_->Get<TextSystem>();
    CHECK(text_system) << "Missing text system.";
    text_system->CreateFromRenderDef(e, data);
  }

  if (data.texture() && data.texture()->size() > 0) {
    TexturePtr texture =
        factory_->LoadTexture(data.texture()->c_str(), data.create_mips());
    SetTexture(e, 0, texture);
  }

  if (data.mesh()) {
    SetMesh(e, factory_->LoadMesh(data.mesh()->c_str()));
  }

  if (data.color()) {
    mathfu::vec4 color;
    MathfuVec4FromFbColor(data.color(), &color);
    SetUniform(e, kColorUniform, &color[0], 4, 1);
    component->default_color = color;
  } else if (data.color_hex()) {
    mathfu::vec4 color;
    MathfuVec4FromFbColorHex(data.color_hex()->c_str(), &color);
    SetUniform(e, kColorUniform, &color[0], 4, 1);
    component->default_color = color;
  }

  if (data.uniforms()) {
    for (const UniformDef* uniform : *data.uniforms()) {
      if (!uniform->name() || !uniform->float_value()) {
        LOG(DFATAL) << "Missing required uniform name or value";
        continue;
      }
      if (uniform->dimension() <= 0) {
        LOG(DFATAL) << "Uniform dimension must be positive: "
                    << uniform->dimension();
        continue;
      }
      if (uniform->count() <= 0) {
        LOG(DFATAL) << "Uniform count must be positive: " << uniform->count();
        continue;
      }
      if (uniform->float_value()->size() !=
          static_cast<size_t>(uniform->dimension() * uniform->count())) {
        LOG(DFATAL) << "Uniform must have dimension x count values: "
                    << uniform->float_value()->size();
        continue;
      }
      SetUniform(e, uniform->name()->c_str(), uniform->float_value()->data(),
                 uniform->dimension(), uniform->count());
    }
  }

  SetSortOrderOffset(e, data.sort_order_offset());
}

void RenderSystemFpl::PostCreateInit(Entity e, HashValue type, const Def* def) {
  if (type == kRenderDefHash) {
    auto& data = *ConvertDef<RenderDef>(def);
    if (data.text()) {
      SetText(e, data.text()->c_str());
    } else if (data.quad()) {
      const QuadDef& quad_def = *data.quad();

      Quad quad;
      quad.size = mathfu::vec2(quad_def.size_x(), quad_def.size_y());
      quad.verts = mathfu::vec2i(quad_def.verts_x(), quad_def.verts_y());
      quad.has_uv = quad_def.has_uv();
      quad.corner_radius = quad_def.corner_radius();
      quad.corner_verts = quad_def.corner_verts();
      if (data.shape_id()) {
        quad.id = Hash(data.shape_id()->c_str());
      }
      SetQuad(e, quad);
    }
  }
}

void RenderSystemFpl::Destroy(Entity e) {
  SetStencilMode(e, StencilMode::kDisabled, 0);
  render_component_pools_.DestroyComponent(e);
  deformations_.erase(e);
  sort_order_manager_.Destroy(e);
}

RenderPass RenderSystemFpl::GetRenderPass(Entity entity) const {
  const RenderComponent* component =
      render_component_pools_.GetComponent(entity);
  return component ? component->pass : RenderPass_Invalid;
}

void RenderSystemFpl::SetQuadImpl(Entity e, const Quad& quad) {
  if (quad.has_uv) {
    SetMesh(e, CreateQuad<VertexPT>(e, quad));
  } else {
    SetMesh(e, CreateQuad<VertexP>(e, quad));
  }
}

void RenderSystemFpl::CreateDeferredMeshes() {
  while (!deferred_meshes_.empty()) {
    DeferredMesh& defer = deferred_meshes_.front();
    switch (defer.type) {
      case DeferredMesh::kQuad:
        SetQuadImpl(defer.e, defer.quad);
        break;
      case DeferredMesh::kMesh:
        DeformMesh(defer.e, &defer.mesh);
        SetMesh(defer.e, defer.mesh);
        break;
    }
    deferred_meshes_.pop();
  }
}

void RenderSystemFpl::ProcessTasks() {
  LULLABY_CPU_TRACE_CALL();

  CreateDeferredMeshes();
  factory_->UpdateAssetLoad();
}

void RenderSystemFpl::WaitForAssetsToLoad() {
  CreateDeferredMeshes();
  factory_->WaitForAssetsToLoad();
}

const mathfu::vec4& RenderSystemFpl::GetDefaultColor(Entity entity) const {
  const RenderComponent* component =
      render_component_pools_.GetComponent(entity);
  if (component) {
    return component->default_color;
  }
  return mathfu::kOnes4f;
}

void RenderSystemFpl::SetDefaultColor(Entity entity,
                                      const mathfu::vec4& color) {
  RenderComponent* component = render_component_pools_.GetComponent(entity);
  if (component) {
    component->default_color = color;
  }
}

bool RenderSystemFpl::GetColor(Entity entity, mathfu::vec4* color) const {
  return GetUniform(entity, kColorUniform, 4, &(*color)[0]);
}

void RenderSystemFpl::SetColor(Entity entity, const mathfu::vec4& color) {
  SetUniform(entity, kColorUniform, &color[0], 4, 1);
}

void RenderSystemFpl::SetUniform(Entity e, const char* name, const float* data,
                                 int dimension, int count) {
  if (!IsSupportedUniformDimension(dimension)) {
    LOG(DFATAL) << "Unsupported uniform dimension " << dimension;
    return;
  }
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component || !render_component->shader) {
    return;
  }

  const lull::HashValue key = Hash(name);
  const bool is_new = (render_component->uniforms.count(key) == 0);
  auto& uniform = render_component->uniforms[key];
  uniform.name = name;
  uniform.values.clear();
  uniform.values.assign(data, data + (dimension * count));
  if (is_new) {
    uniform.location = render_component->shader->FindUniform(name);
  }
  uniform.count = count;
  uniform.dimension = dimension;
}

bool RenderSystemFpl::GetUniform(Entity e, const char* name, size_t length,
                                 float* data_out) const {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return false;
  }

  const auto& iter = render_component->uniforms.find(Hash(name));
  if (iter == render_component->uniforms.end()) {
    return false;
  }

  const RenderComponent::UniformData& uniform = iter->second;
  if (length < uniform.values.size()) {
    return false;
  }
  for (size_t i = 0; i < uniform.values.size(); ++i) {
    data_out[i] = uniform.values[i];
  }
  return true;
}

void RenderSystemFpl::CopyUniforms(Entity entity, Entity source) {
  RenderComponent* component = render_component_pools_.GetComponent(entity);
  if (!component) {
    return;
  }

  component->uniforms.clear();

  const RenderComponent* source_component =
      render_component_pools_.GetComponent(source);
  if (source_component) {
    component->uniforms = source_component->uniforms;

    if (component->shader != source_component->shader) {
      // Fix the locations using |entity|'s shader.
      UpdateUniformLocations(component);
    }
  }
}

void RenderSystemFpl::UpdateUniformLocations(RenderComponent* component) {
  if (!component->shader) {
    return;
  }

  for (auto& iter : component->uniforms) {
    RenderComponent::UniformData& uniform = iter.second;
    uniform.location = component->shader->FindUniform(uniform.name.c_str());
  }
}

int RenderSystemFpl::GetNumBones(Entity entity) const {
  const RenderComponent* component =
      render_component_pools_.GetComponent(entity);
  if (!component || !component->mesh) {
    return 0;
  }
  return component->mesh->GetNumBones();
}

const uint8_t* RenderSystemFpl::GetBoneParents(Entity e, int* num) const {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component || !render_component->mesh) {
    if (num) {
      *num = 0;
    }
    return nullptr;
  }
  return render_component->mesh->GetBoneParents(num);
}

const std::string* RenderSystemFpl::GetBoneNames(Entity e, int* num) const {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component || !render_component->mesh) {
    if (num) {
      *num = 0;
    }
    return nullptr;
  }
  return render_component->mesh->GetBoneNames(num);
}

const mathfu::AffineTransform* RenderSystemFpl::GetDefaultBoneTransformInverses(
    Entity e, int* num) const {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component || !render_component->mesh) {
    if (num) {
      *num = 0;
    }
    return nullptr;
  }
  return render_component->mesh->GetDefaultBoneTransformInverses(num);
}

void RenderSystemFpl::SetBoneTransforms(
    Entity entity, const mathfu::AffineTransform* transforms,
    int num_transforms) {
  RenderComponent* component = render_component_pools_.GetComponent(entity);
  if (!component || !component->mesh) {
    return;
  }

  const int num_shader_bones = component->mesh->GetNumShaderBones();
  shader_transforms_.resize(num_shader_bones);

  if (num_transforms != component->mesh->GetNumBones()) {
    LOG(DFATAL) << "Mesh must have " << num_transforms << " bones.";
    return;
  }
  component->mesh->GatherShaderTransforms(transforms,
                                          shader_transforms_.data());

  // GLES2 only supports square matrices, so send the affine transforms as an
  // array of 3 * num_transforms vec4s.
  const float* data = &(shader_transforms_[0][0]);
  const int dimension = 4;
  const int count = kNumVec4sInAffineTransform * num_shader_bones;
  SetUniform(entity, kBoneTransformsUniform, data, dimension, count);
}

void RenderSystemFpl::OnTextureLoaded(const RenderComponent& component,
                                      int unit, const TexturePtr& texture) {
  const Entity entity = component.GetEntity();
  const mathfu::vec4 clamp_bounds = texture->CalculateClampBounds();
  SetUniform(entity, kClampBoundsUniform, &clamp_bounds[0], 4, 1);

  if (factory_->IsTextureValid(texture)) {
    // TODO(b/38130323) Add CheckTextureSizeWarning that does not depend on HMD.

    auto* dispatcher_system = registry_->Get<DispatcherSystem>();
    if (dispatcher_system) {
      dispatcher_system->Send(entity, TextureReadyEvent(entity, unit));
      if (IsReadyToRenderImpl(component)) {
        dispatcher_system->Send(entity, ReadyToRenderEvent(entity));
      }
    }
  }
}

void RenderSystemFpl::SetTexture(Entity e, int unit,
                                 const TexturePtr& texture) {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return;
  }

  if (!texture) {
    render_component->textures.erase(unit);
    return;
  }

  render_component->textures[unit] = texture;
  max_texture_unit_ = std::max(max_texture_unit_, unit);

  // Add subtexture coordinates so the vertex shaders will pick them up.  These
  // are known when the texture is created; no need to wait for load.
  SetUniform(e, kTextureBoundsUniform, &texture->UvBounds()[0], 4, 1);

  if (texture->IsLoaded()) {
    OnTextureLoaded(*render_component, unit, texture);
  } else {
    texture->AddOnLoadCallback([this, unit, e, texture]() {
      RenderComponent* render_component =
          render_component_pools_.GetComponent(e);
      if (render_component && render_component->textures.count(unit) != 0 &&
          render_component->textures[unit] == texture) {
        OnTextureLoaded(*render_component, unit, texture);
      }
    });
  }
}

TexturePtr RenderSystemFpl::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    RenderSystem::TextureProcessor processor) {
  return factory_->CreateProcessedTexture(source_texture, create_mips,
                                          processor);
}

void RenderSystemFpl::SetTextureId(Entity e, int unit, uint32_t texture_target,
                                   uint32_t texture_id) {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return;
  }
  auto texture = factory_->CreateTexture(texture_target, texture_id);
  SetTexture(e, unit, texture);
}

TexturePtr RenderSystemFpl::GetTexture(Entity entity, int unit) const {
  const auto* render_component = render_component_pools_.GetComponent(entity);
  if (!render_component) {
    return TexturePtr();
  }
  auto iter = render_component->textures.find(unit);
  if (iter == render_component->textures.end()) {
    return TexturePtr();
  }
  return iter->second;
}


void RenderSystemFpl::SetText(Entity e, const std::string& text) {
  // TODO(b/33705809) Remove after apps use TextSystem directly.
  TextSystem* text_system = registry_->Get<TextSystem>();
  CHECK(text_system) << "Missing text system.";
  text_system->SetText(e, text);
}

const std::vector<LinkTag>* RenderSystemFpl::GetLinkTags(Entity e) const {
  // TODO(b/33705809) Remove after apps use TextSystem directly.
  const TextSystem* text_system = registry_->Get<TextSystem>();
  CHECK(text_system) << "Missing text system.";
  return text_system->GetLinkTags(e);
}

void RenderSystemFpl::SetQuad(Entity e, const Quad& quad) {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    LOG(WARNING) << "Missing entity for SetQuad: " << e;
    return;
  }
  render_component->quad = quad;

  auto iter = deformations_.find(e);
  if (iter != deformations_.end()) {
    DeferredMesh defer;
    defer.e = e;
    defer.type = DeferredMesh::kQuad;
    defer.quad = quad;
    deferred_meshes_.push(std::move(defer));
  } else {
    SetQuadImpl(e, quad);
  }
}

bool RenderSystemFpl::GetQuad(Entity e, Quad* quad) const {
  const auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return false;
  }
  *quad = render_component->quad;
  return true;
}

void RenderSystemFpl::SetMesh(Entity e, const TriangleMesh<VertexPT>& mesh) {
  SetMesh(e, factory_->CreateMesh(mesh));
}

void RenderSystemFpl::SetAndDeformMesh(Entity entity,
                                       const TriangleMesh<VertexPT>& mesh) {
  auto iter = deformations_.find(entity);
  if (iter != deformations_.end()) {
    DeferredMesh defer;
    defer.e = entity;
    defer.type = DeferredMesh::kMesh;
    defer.mesh.GetVertices() = mesh.GetVertices();
    defer.mesh.GetIndices() = mesh.GetIndices();
    deferred_meshes_.emplace(std::move(defer));
  } else {
    SetMesh(entity, mesh);
  }
}

void RenderSystemFpl::SetMesh(Entity e, const MeshData& mesh) {
  SetMesh(e, factory_->CreateMesh(mesh));
}

void RenderSystemFpl::SetMesh(Entity e, const std::string& file) {
  SetMesh(e, factory_->LoadMesh(file));
}

RenderSystemFpl::SortOrderOffset RenderSystemFpl::GetSortOrderOffset(
    Entity entity) const {
  return sort_order_manager_.GetOffset(entity);
}

void RenderSystemFpl::SetSortOrderOffset(Entity e, SortOrderOffset offset) {
  sort_order_manager_.SetOffset(e, offset);
  sort_order_manager_.UpdateSortOrder(
      e, [this](Entity entity) {
        return render_component_pools_.GetComponent(entity);
      });
}

bool RenderSystemFpl::IsTextureSet(Entity e, int unit) const {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return false;
  }
  return render_component->textures.count(unit) != 0;
}

bool RenderSystemFpl::IsTextureLoaded(Entity e, int unit) const {
  const auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return false;
  }

  if (render_component->textures.count(unit) == 0) {
    return false;
  }
  return render_component->textures.at(unit)->IsLoaded();
}

bool RenderSystemFpl::IsTextureLoaded(const TexturePtr& texture) const {
  return texture->IsLoaded();
}

bool RenderSystemFpl::IsReadyToRender(Entity entity) const {
  const auto* render_component = render_component_pools_.GetComponent(entity);
  if (!render_component) {
    // No component, no textures, no fonts, no problem.
    return true;
  }
  return IsReadyToRenderImpl(*render_component);
}

bool RenderSystemFpl::IsReadyToRenderImpl(
    const RenderComponent& component) const {
  for (const auto& pair : component.textures) {
    const TexturePtr& texture = pair.second;
    if (!texture->IsLoaded() || !factory_->IsTextureValid(texture)) {
      return false;
    }
  }
  return true;
}

bool RenderSystemFpl::IsHidden(Entity e) const {
  const auto* render_component = render_component_pools_.GetComponent(e);
  bool component_exists = (render_component != nullptr);
  bool component_hidden = render_component && render_component->hidden;


  // If there are no models associated with this entity, then it is hidden.
  // Otherwise, it is hidden if component is hidden.
  return !component_exists || component_hidden;
}

ShaderPtr RenderSystemFpl::GetShader(Entity entity) const {
  const RenderComponent* component =
      render_component_pools_.GetComponent(entity);
  return component ? component->shader : ShaderPtr();
}

void RenderSystemFpl::SetShader(Entity e, const ShaderPtr& shader) {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return;
  }
  render_component->shader = shader;

  // Update the uniforms' locations in the new shader.
  UpdateUniformLocations(render_component);
}

void RenderSystemFpl::SetMesh(Entity e, MeshPtr mesh) {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    LOG(WARNING) << "Missing RenderComponent, "
                 << "skipping mesh update for entity: " << e;
    return;
  }

  render_component->mesh = std::move(mesh);
  if (render_component->mesh) {
    auto& transform_system = *registry_->Get<TransformSystem>();
    transform_system.SetAabb(e, render_component->mesh->GetAabb());

    const int num_shader_bones = render_component->mesh->GetNumShaderBones();
    if (num_shader_bones > 0) {
      const mathfu::AffineTransform identity =
          mathfu::mat4::ToAffineTransform(mathfu::mat4::Identity());
      shader_transforms_.clear();
      shader_transforms_.resize(num_shader_bones, identity);

      const float* data = &(shader_transforms_[0][0]);
      const int dimension = 4;
      const int count = kNumVec4sInAffineTransform * num_shader_bones;
      SetUniform(e, kBoneTransformsUniform, data, dimension, count);
    }
  }
}

void RenderSystemFpl::SetFont(Entity entity, const FontPtr& font) {
  // TODO(b/33705809) Remove after apps use TextSystem directly.
  TextSystem* text_system = registry_->Get<TextSystem>();
  CHECK(text_system) << "Missing text system.";
  text_system->SetFont(entity, font);
}

void RenderSystemFpl::SetTextSize(Entity entity, int size) {
  // TODO(b/33705809) Remove after apps use TextSystem directly.
  const float kMetersFromMillimeters = .001f;
  TextSystem* text_system = registry_->Get<TextSystem>();
  CHECK(text_system) << "Missing text system.";
  text_system->SetLineHeight(entity,
                             static_cast<float>(size) * kMetersFromMillimeters);
}

template <typename Vertex>
void RenderSystemFpl::DeformMesh(Entity entity, TriangleMesh<Vertex>* mesh) {
  auto iter = deformations_.find(entity);
  const Deformation deform =
      iter != deformations_.end() ? iter->second : nullptr;
  if (deform) {
    // TODO(b/28313614) Use TriangleMesh::ApplyDeformation.
    if (sizeof(Vertex) % sizeof(float) == 0) {
      const int stride = static_cast<int>(sizeof(Vertex) / sizeof(float));
      std::vector<Vertex>& vertices = mesh->GetVertices();
      deform(reinterpret_cast<float*>(vertices.data()),
             vertices.size() * stride, stride);
    } else {
      LOG(ERROR) << "Tried to deform an unsupported vertex format.";
    }
  }
}

template <typename Vertex>
MeshPtr RenderSystemFpl::CreateQuad(Entity e, const Quad& quad) {
  if (quad.size.x == 0 || quad.size.y == 0) {
    return nullptr;
  }

  TriangleMesh<Vertex> mesh;
  mesh.SetQuad(quad.size.x, quad.size.y, quad.verts.x, quad.verts.y,
               quad.corner_radius, quad.corner_verts, quad.corner_mask);

  DeformMesh<Vertex>(e, &mesh);

  if (quad.id != 0) {
    return factory_->CreateMesh(quad.id, mesh);
  } else {
    return factory_->CreateMesh(mesh);
  }
}

void RenderSystemFpl::SetStencilMode(Entity e, StencilMode mode, int value) {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component || render_component->stencil_mode == mode) {
    return;
  }

  render_component->stencil_mode = mode;
  render_component->stencil_value = value;
}

void RenderSystemFpl::SetDeformationFunction(Entity e,
                                             const Deformation& deform) {
  if (deform) {
    deformations_.emplace(e, deform);
  } else {
    deformations_.erase(e);
  }
}

void RenderSystemFpl::Hide(Entity e) {
  auto* render_component = render_component_pools_.GetComponent(e);
  bool newly_hidden = false;
  if (render_component && !render_component->hidden) {
    render_component->hidden = true;
    render_component_pools_.MoveToPool(e, RenderPass_Invisible);
    newly_hidden = true;
  }


  if (newly_hidden) {
    SendEvent(registry_, e, HiddenEvent(e));
  }
}

void RenderSystemFpl::Show(Entity e) {
  auto* render_component = render_component_pools_.GetComponent(e);
  bool newly_unhidden = false;
  if (render_component && render_component->hidden) {
    render_component->hidden = false;
    render_component_pools_.MoveToPool(e, render_component->pass);
    newly_unhidden = true;
  }


  if (newly_unhidden) {
    SendEvent(registry_, e, UnhiddenEvent(e));
  }
}

void RenderSystemFpl::SetRenderPass(Entity e, RenderPass pass) {
  RenderComponent* render_component = render_component_pools_.GetComponent(e);
  if (render_component) {
    render_component->pass = pass;
    if (!render_component->hidden) {
      render_component_pools_.MoveToPool(e, pass);
    }
  }
}

RenderSystem::SortMode RenderSystemFpl::GetSortMode(RenderPass pass) const {
  const RenderPool* pool = render_component_pools_.GetExistingPool(pass);
  return pool ? pool->GetSortMode() : SortMode::kNone;
}

void RenderSystemFpl::SetSortMode(RenderPass pass, SortMode mode) {
  RenderPool& pool = render_component_pools_.GetPool(pass);
  pool.SetSortMode(mode);
}

void RenderSystemFpl::SetCullMode(RenderPass pass, CullMode mode) {
  RenderPool& pool = render_component_pools_.GetPool(pass);
  pool.SetCullMode(mode);
}

void RenderSystemFpl::SetDepthTest(const bool enabled) {
  if (enabled) {
#if !ION_PRODUCTION
    GLint depth_bits = 0;
    glGetIntegerv(GL_DEPTH_BITS, &depth_bits);
    if (depth_bits == 0) {
      // This has been known to cause problems on iOS 10.
      LOG(WARNING) << "Enabling depth test without a depth buffer; this "
                      "has known issues on some platforms.";
    }
#endif  // !ION_PRODUCTION

    renderer_.SetDepthFunction(fplbase::kDepthFunctionLess);
    return;
  }

  renderer_.SetDepthFunction(fplbase::kDepthFunctionDisabled);
}

void RenderSystemFpl::SetDepthWrite(const bool enabled) {
  renderer_.SetDepthWrite(enabled);
}

void RenderSystemFpl::SetViewport(const View& view) {
  LULLABY_CPU_TRACE_CALL();
  renderer_.SetViewport(fplbase::Viewport(view.viewport, view.dimensions));
}

void RenderSystemFpl::SetClipFromModelMatrix(const mathfu::mat4& mvp) {
  renderer_.set_model_view_projection(mvp);
}

void RenderSystemFpl::BindStencilMode(StencilMode mode, int ref) {
  // Stencil mask setting all the bits to be 1.
  static const fplbase::StencilMask kStencilMaskAllBits = ~0;

  switch (mode) {
    case StencilMode::kDisabled:
      renderer_.SetStencilMode(fplbase::kStencilDisabled, ref,
                               kStencilMaskAllBits);
      break;
    case StencilMode::kTest:
      renderer_.SetStencilMode(fplbase::kStencilCompareEqual, ref,
                               kStencilMaskAllBits);
      break;
    case StencilMode::kWrite:
      renderer_.SetStencilMode(fplbase::kStencilWrite, ref,
                               kStencilMaskAllBits);
      break;
  }
}

void RenderSystemFpl::BindVertexArray(uint32_t ref) {
  // VAOs are part of the GLES3 & GL3 specs.
  if (renderer_.feature_level() == fplbase::kFeatureLevel30) {
#if GL_ES_VERSION_3_0 || defined(GL_VERSION_3_0)
    GL_CALL(glBindVertexArray(ref));
#endif
    return;
  }

// VAOs were available prior to GLES3 using an extension.
#if GL_OES_vertex_array_object
#ifndef GL_GLEXT_PROTOTYPES
  static PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES = []() {
    return (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress(
        "glBindVertexArrayOES");
  };
  if (glBindVertexArrayOES) {
    GL_CALL(glBindVertexArrayOES(ref));
  }
#else   // GL_GLEXT_PROTOTYPES
  GL_CALL(glBindVertexArrayOES(ref));
#endif  // !GL_GLEXT_PROTOTYPES
#endif  // GL_OES_vertex_array_object
}

void RenderSystemFpl::ClearSamplers() {
// Samplers are part of GLES3 & GL3.3 specs.
#if GL_ES_VERSION_3_0 || defined(GL_VERSION_3_3)
  for (int i = 0; i <= max_texture_unit_; ++i) {
    // Confusingly, glBindSampler takes an index, not the raw texture unit
    // (GL_TEXTURE0 + index).
    GL_CALL(glBindSampler(i, 0));
  }
#endif  // GL_ES_VERSION_3_0 || GL_VERSION_3_3
}

void RenderSystemFpl::ResetState() {
  const fplbase::RenderState& render_state = renderer_.GetRenderState();

  // Clear render state.
  SetBlendMode(fplbase::kBlendModeOff);
  renderer_.SetCulling(fplbase::kCullingModeBack);
  SetDepthTest(true);
  renderer_.ScissorOff();
  GL_CALL(glDisable(GL_STENCIL_TEST));
  GL_CALL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
  GL_CALL(
      glDepthMask(render_state.depth_state.write_enabled ? GL_TRUE : GL_FALSE));
  GL_CALL(glStencilMask(~0));
  GL_CALL(glFrontFace(GL_CCW));
  GL_CALL(glPolygonOffset(0, 0));

  // Clear sampler objects, since FPL doesn't use them.
  ClearSamplers();

  // Clear VAO since it overrides VBOs.
  BindVertexArray(0);

  // Clear attributes, though we can leave position.
  GL_CALL(glDisableVertexAttribArray(fplbase::Mesh::kAttributeNormal));
  GL_CALL(glDisableVertexAttribArray(fplbase::Mesh::kAttributeTangent));
  GL_CALL(glDisableVertexAttribArray(fplbase::Mesh::kAttributeTexCoord));
  GL_CALL(glDisableVertexAttribArray(fplbase::Mesh::kAttributeTexCoordAlt));
  GL_CALL(glDisableVertexAttribArray(fplbase::Mesh::kAttributeColor));
  GL_CALL(glDisableVertexAttribArray(fplbase::Mesh::kAttributeBoneIndices));
  GL_CALL(glDisableVertexAttribArray(fplbase::Mesh::kAttributeBoneWeights));

  shader_.reset();
}

void RenderSystemFpl::SetBlendMode(fplbase::BlendMode blend_mode) {
  renderer_.SetBlendMode(blend_mode);
  blend_mode_ = blend_mode;
}

void RenderSystemFpl::SetClearColor(float r, float g, float b, float a) {
  clear_color_ = mathfu::vec4(r, g, b, a);
}

void RenderSystemFpl::BeginFrame() {
  LULLABY_CPU_TRACE_CALL();
  GL_CALL(glClearColor(clear_color_.x, clear_color_.y, clear_color_.z,
                       clear_color_.w));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                  GL_STENCIL_BUFFER_BIT));
}

void RenderSystemFpl::EndFrame() {}

void RenderSystemFpl::SetViewUniforms(const View& view) {
  renderer_.set_camera_pos(view.world_from_eye_matrix.TranslationVector3D());

  rendering_right_eye_ = view.eye == 1;
}

void RenderSystemFpl::RenderAt(const RenderComponent* component,
                               const mathfu::mat4& world_from_entity_matrix,
                               const View& view) {
  LULLABY_CPU_TRACE_CALL();
  if (!component->shader || (!component->mesh && !component->dynamic_mesh)) {
    return;
  }

  const mathfu::mat4 clip_from_entity_matrix =
      view.clip_from_world_matrix * world_from_entity_matrix;
  renderer_.set_model_view_projection(clip_from_entity_matrix);
  renderer_.set_model(world_from_entity_matrix);

  BindShader(component->shader);
  SetShaderUniforms(component->uniforms);

  const Shader::UniformHnd mat_normal_uniform_handle =
      component->shader->FindUniform("mat_normal");
  if (fplbase::ValidUniformHandle(mat_normal_uniform_handle)) {
    const int uniform_gl = fplbase::GlUniformHandle(mat_normal_uniform_handle);
    // Compute the normal matrix. This is the transposed matrix of the inversed
    // world position. This is done to avoid non-uniform scaling of the normal.
    // A good explanation of this can be found here:
    // http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/
    const mathfu::mat3 normal_matrix =
        ComputeNormalMatrix(world_from_entity_matrix);
    mathfu::VectorPacked<float, 3> packed[3];
    normal_matrix.Pack(packed);
    glUniformMatrix3fv(uniform_gl, 1, false, packed[0].data);
  }
  const Shader::UniformHnd camera_dir_handle =
      component->shader->FindUniform("camera_dir");
  if (fplbase::ValidUniformHandle(camera_dir_handle)) {
    const int uniform_gl = fplbase::GlUniformHandle(camera_dir_handle);
    mathfu::vec3_packed camera_dir;
    CalculateCameraDirection(view.world_from_eye_matrix).Pack(&camera_dir);
    glUniform3fv(uniform_gl, 1, camera_dir.data);
  }

  for (const auto& texture : component->textures) {
    texture.second->Bind(texture.first);
  }

  // Bit of magic to determine if the scalar is negative and if so flip the cull
  // face. This possibly be revised (b/38235916).
  if (CalculateDeterminant3x3(world_from_entity_matrix) >= 0.0f) {
    GL_CALL(glFrontFace(GL_CCW));
  } else {
    GL_CALL(glFrontFace(GL_CW));
  }

  BindStencilMode(component->stencil_mode, component->stencil_value);
  DrawMeshFromComponent(component);
}

void RenderSystemFpl::RenderAtMultiview(
    const RenderComponent* component,
    const mathfu::mat4& world_from_entity_matrix, const View* views) {
  LULLABY_CPU_TRACE_CALL();
  if (!component->shader || (!component->mesh && !component->dynamic_mesh)) {
    return;
  }

  const mathfu::mat4 clip_from_entity_matrix[] = {
      views[0].clip_from_world_matrix * world_from_entity_matrix,
      views[1].clip_from_world_matrix * world_from_entity_matrix,
  };

  BindShader(component->shader);
  SetShaderUniforms(component->uniforms);

  const Shader::UniformHnd mvp_uniform_handle =
      component->shader->FindUniform("model_view_projection");
  if (fplbase::ValidUniformHandle(mvp_uniform_handle)) {
    const int uniform_gl = fplbase::GlUniformHandle(mvp_uniform_handle);
    glUniformMatrix4fv(uniform_gl, 2, false, &(clip_from_entity_matrix[0][0]));
  }
  const Shader::UniformHnd mat_normal_uniform_handle =
      component->shader->FindUniform("mat_normal");
  if (fplbase::ValidUniformHandle(mat_normal_uniform_handle)) {
    const int uniform_gl = fplbase::GlUniformHandle(mat_normal_uniform_handle);
    const mathfu::mat3 normal_matrix =
        ComputeNormalMatrix(world_from_entity_matrix);
    mathfu::VectorPacked<float, 3> packed[3];
    normal_matrix.Pack(packed);
    glUniformMatrix3fv(uniform_gl, 1, false, packed[0].data);
  }
  const Shader::UniformHnd camera_dir_handle =
      component->shader->FindUniform("camera_dir");
  if (fplbase::ValidUniformHandle(camera_dir_handle)) {
    const int uniform_gl = fplbase::GlUniformHandle(camera_dir_handle);
    mathfu::vec3_packed camera_dir[2];
    for (size_t i = 0; i < 2; ++i) {
      CalculateCameraDirection(views[i].world_from_eye_matrix)
          .Pack(&camera_dir[i]);
    }
    glUniform3fv(uniform_gl, 2, camera_dir[0].data);
  }

  for (const auto& texture : component->textures) {
    texture.second->Bind(texture.first);
  }

  // Bit of magic to determine if the scalar is negative and if so flip the cull
  // face. This possibly be revised (b/38235916).
  if (CalculateDeterminant3x3(world_from_entity_matrix) >= 0.0f) {
    GL_CALL(glFrontFace(GL_CCW));
  } else {
    GL_CALL(glFrontFace(GL_CW));
  }

  BindStencilMode(component->stencil_mode, component->stencil_value);
  DrawMeshFromComponent(component);
}

void RenderSystemFpl::SetShaderUniforms(const UniformMap& uniforms) {
  for (const auto& iter : uniforms) {
    const RenderComponent::UniformData& uniform = iter.second;
    if (fplbase::ValidUniformHandle(uniform.location)) {
      const float* values = uniform.values.data();
      // TODO(b/62000164): Add a `count` parameter to
      // fplbase::Shader::SetUniform() so that we don't have to make OpenGL
      // calls here.
      const int uniform_gl = fplbase::GlUniformHandle(uniform.location);
      switch (uniform.dimension) {
        case 1:
          GL_CALL(glUniform1fv(uniform_gl, uniform.count, values));
          break;
        case 2:
          GL_CALL(glUniform2fv(uniform_gl, uniform.count, values));
          break;
        case 3:
          GL_CALL(glUniform3fv(uniform_gl, uniform.count, values));
          break;
        case 4:
          GL_CALL(glUniform4fv(uniform_gl, uniform.count, values));
          break;
        case 16:
          GL_CALL(glUniformMatrix4fv(uniform_gl, uniform.count, false, values));
          break;
      }
    }
  }
}

void RenderSystemFpl::DrawMeshFromComponent(const RenderComponent* component) {
  if (component->mesh) {
    component->mesh->Render(&renderer_, blend_mode_);
    detail::Profiler* profiler = registry_->Get<detail::Profiler>();
    if (profiler) {
      profiler->RecordDraw(component->shader, component->mesh->GetNumVertices(),
                           component->mesh->GetNumTriangles());
    }
  }

  if (component->dynamic_mesh) {
    MeshData* mesh = component->dynamic_mesh.get();

    DrawDynamicMesh(mesh);

    detail::Profiler* profiler = registry_->Get<detail::Profiler>();
    if (profiler) {
      profiler->RecordDraw(component->shader, mesh->GetNumVertices(),
                           static_cast<int>(mesh->GetNumIndices() / 3));
    }
  }
}

const std::vector<mathfu::vec3>* RenderSystemFpl::GetCaretPositions(
    Entity e) const {
  // TODO(b/33705809) Remove after apps use TextSystem directly.
  const TextSystem* text_system = registry_->Get<TextSystem>();
  CHECK(text_system) << "Missing text system.";
  return text_system->GetCaretPositions(e);
}

void RenderSystemFpl::RenderDisplayList(const View& view,
                                        const DisplayList& display_list) {
  LULLABY_CPU_TRACE_CALL();
  const std::vector<DisplayList::Entry>* list = display_list.GetContents();
  std::for_each(list->begin(), list->end(),
                [&](const DisplayList::Entry& info) {
                  if (info.component) {
                    RenderAt(info.component, info.world_from_entity_matrix,
                             view);
                  }
                });
}

void RenderSystemFpl::RenderDisplayListMultiview(
    const View* views, const DisplayList& display_list) {
  LULLABY_CPU_TRACE_CALL();
  const std::vector<DisplayList::Entry>* list = display_list.GetContents();
  std::for_each(list->begin(), list->end(),
                [&](const DisplayList::Entry& info) {
                  if (info.component) {
                    RenderAtMultiview(info.component,
                                      info.world_from_entity_matrix, views);
                  }
                });
}

void RenderSystemFpl::RenderComponentsInPass(const View* views,
                                             size_t num_views,
                                             RenderPass pass) {
  const RenderPool& pool = render_component_pools_.GetPool(pass);
  DisplayList display_list(registry_);
  display_list.Populate(pool, views, num_views);

  if (multiview_enabled_) {
    SetViewport(views[0]);
    SetViewUniforms(views[0]);
    RenderDisplayListMultiview(views, display_list);

  } else {
    for (size_t j = 0; j < num_views; ++j) {
      SetViewport(views[j]);
      SetViewUniforms(views[j]);
      RenderDisplayList(views[j], display_list);
    }
  }

  // Reset states that are set at the entity level in RenderAt.
  BindStencilMode(StencilMode::kDisabled, 0);
  GL_CALL(glFrontFace(GL_CCW));
}


void RenderSystemFpl::Render(const View* views, size_t num_views) {
  renderer_.BeginRendering();

  ResetState();
  known_state_ = true;

  Render(views, num_views, RenderPass_Pano);
  Render(views, num_views, RenderPass_Opaque);
  Render(views, num_views, RenderPass_Main);
  Render(views, num_views, RenderPass_OverDraw);
  Render(views, num_views, RenderPass_OverDrawGlow);

  known_state_ = false;

  renderer_.EndRendering();
}

void RenderSystemFpl::Render(const View* views, size_t num_views,
                             RenderPass pass) {
  LULLABY_CPU_TRACE_CALL();

  if (!known_state_) {
    renderer_.BeginRendering();

    if (pass < RenderPass_NumPredefinedPasses) {
      ResetState();
    }
  }

  bool reset_state = true;
  auto* config = registry_->Get<Config>();
  if (config) {
    static const HashValue kRenderResetStateHash =
        Hash("lull.Render.ResetState");
    reset_state = config->Get(kRenderResetStateHash, reset_state);
  }

  switch (pass) {
    case RenderPass_Pano: {
      SetDepthTest(false);
      SetBlendMode(fplbase::kBlendModePreMultipliedAlpha);  // (1, 1-SrcAlpha)
      RenderComponentsInPass(views, num_views, pass);
      break;
    }
    case RenderPass_Opaque: {
      SetDepthTest(true);
      SetDepthWrite(true);
      SetBlendMode(fplbase::kBlendModeOff);
      renderer_.SetCulling(fplbase::kCullingModeBack);

      RenderComponentsInPass(views, num_views, pass);

      if (reset_state) {
        SetDepthTest(false);
        renderer_.SetCulling(fplbase::kCullingModeNone);
      }
      break;
    }
    case RenderPass_Main: {
      SetDepthTest(true);
      SetBlendMode(fplbase::kBlendModePreMultipliedAlpha);  // (1, 1-SrcAlpha)
      renderer_.SetCulling(fplbase::kCullingModeBack);
      SetDepthWrite(false);

      RenderComponentsInPass(views, num_views, pass);

      if (reset_state) {
        SetBlendMode(fplbase::kBlendModeOff);
        renderer_.SetCulling(fplbase::kCullingModeNone);
      }
      break;
    }
    case RenderPass_OverDraw: {
      // Allow OverDraw to draw over anything that has been rendered by
      // disabling the depth test.
      SetDepthTest(false);
      SetBlendMode(fplbase::kBlendModePreMultipliedAlpha);  // (1, 1-SrcAlpha)
      renderer_.SetCulling(fplbase::kCullingModeBack);
      SetDepthWrite(false);

      RenderComponentsInPass(views, num_views, pass);

      if (reset_state) {
        renderer_.SetCulling(fplbase::kCullingModeNone);
        SetBlendMode(fplbase::kBlendModeOff);
      }
      break;
    }
    case RenderPass_OverDrawGlow: {
      // Allow OverDrawGlow to draw over anything that has been rendered by
      // disabling the depth test. Set alpha mode to additive and remove
      // culling.
      SetDepthTest(false);
      SetBlendMode(fplbase::kBlendModeAdd);  // (1, 1)
      renderer_.SetCulling(fplbase::kCullingModeNone);
      SetDepthWrite(false);

      RenderComponentsInPass(views, num_views, pass);

      if (reset_state) {
        SetBlendMode(fplbase::kBlendModeOff);
      }

      // Something in later passes seems to expect depth write to be on. Setting
      // this here until the culprit is identified (b/36200233).
      SetDepthWrite(true);
      break;
    }
    case RenderPass_Invisible: {
      // Do nothing.
      break;
    }
    case RenderPass_Debug: {
      RenderDebugStats(views, num_views);
      break;
    }
    default:
      RenderComponentsInPass(views, num_views, pass);
      break;
  }

  if (!known_state_) {
    renderer_.EndRendering();
  }
}

void RenderSystemFpl::BindShader(const ShaderPtr& shader) {
  // Don't early exit if shader == shader_, since fplbase::Shader::Set also sets
  // the common fpl uniforms.
  shader_ = shader;
  shader->Bind();

  // Bind uniform describing whether or not we're rendering in the right eye.
  // This uniform is an int due to legacy reasons, but there's no pipeline in
  // FPL for setting int uniforms, so we have to make a direct gl call instead.
  fplbase::UniformHandle uniform_is_right_eye =
      shader->FindUniform(kIsRightEyeUniform);
  if (fplbase::ValidUniformHandle(uniform_is_right_eye)) {
    GL_CALL(glUniform1i(fplbase::GlUniformHandle(uniform_is_right_eye),
                        static_cast<int>(rendering_right_eye_)));
  }
}

void RenderSystemFpl::BindTexture(int unit, const TexturePtr& texture) {
  texture->Bind(unit);
}

void RenderSystemFpl::BindUniform(const char* name, const float* data,
                                  int dimension) {
  if (!IsSupportedUniformDimension(dimension)) {
    LOG(DFATAL) << "Unsupported uniform dimension " << dimension;
    return;
  }
  if (!shader_) {
    LOG(DFATAL) << "Cannot bind uniform on unbound shader!";
    return;
  }
  const fplbase::UniformHandle location = shader_->FindUniform(name);
  if (fplbase::ValidUniformHandle(location)) {
    shader_->SetUniform(location, data, dimension);
  }
}

void RenderSystemFpl::DrawPrimitives(PrimitiveType type,
                                     const VertexFormat& format,
                                     const void* vertex_data,
                                     size_t num_vertices) {
  const fplbase::Mesh::Primitive fpl_type = Mesh::GetFplPrimitiveType(type);
  fplbase::Attribute attributes[Mesh::kMaxFplAttributeArraySize];
  Mesh::GetFplAttributes(format, attributes);

  fplbase::RenderArray(fpl_type, static_cast<int>(num_vertices), attributes,
                       static_cast<int>(format.GetVertexSize()), vertex_data);
}

void RenderSystemFpl::DrawIndexedPrimitives(
    PrimitiveType type, const VertexFormat& format, const void* vertex_data,
    size_t num_vertices, const uint16_t* indices, size_t num_indices) {
  const fplbase::Mesh::Primitive fpl_type = Mesh::GetFplPrimitiveType(type);
  fplbase::Attribute attributes[Mesh::kMaxFplAttributeArraySize];
  Mesh::GetFplAttributes(format, attributes);

  fplbase::RenderArray(fpl_type, static_cast<int>(num_indices), attributes,
                       static_cast<int>(format.GetVertexSize()), vertex_data,
                       indices);
}

void RenderSystemFpl::UpdateDynamicMesh(
    Entity entity, PrimitiveType primitive_type,
    const VertexFormat& vertex_format, const size_t max_vertices,
    const size_t max_indices,
    const std::function<void(MeshData*)>& update_mesh) {
  RenderComponent* component = render_component_pools_.GetComponent(entity);
  if (!component) {
    return;
  }

  if (max_vertices > 0) {
    DataContainer vertex_data = DataContainer::CreateHeapDataContainer(
        max_vertices * vertex_format.GetVertexSize());
    DataContainer index_data = DataContainer::CreateHeapDataContainer(
        max_indices * sizeof(MeshData::Index));
    component->dynamic_mesh.reset(new MeshData(primitive_type, vertex_format,
                                               std::move(vertex_data),
                                               std::move(index_data)));
    update_mesh(component->dynamic_mesh.get());
  } else {
    component->dynamic_mesh.reset();
  }
}

void RenderSystemFpl::RenderDebugStats(const View* views, size_t num_views) {
  RenderStats* render_stats = registry_->Get<RenderStats>();
  if (!render_stats || num_views == 0) {
    return;
  }
  const bool stats_enabled =
      render_stats->IsLayerEnabled(RenderStats::Layer::kRenderStats);
  const bool fps_counter =
      render_stats->IsLayerEnabled(RenderStats::Layer::kFpsCounter);
  if (!stats_enabled && !fps_counter) {
    return;
  }

  SimpleFont* font = render_stats->GetFont();
  if (!font || !font->GetShader()) {
    return;
  }

  // Calculate the position and size of the text from the projection matrix.
  const bool is_stereo =
      (num_views == 2 && views[0].clip_from_eye_matrix[15] == 0.0f &&
       views[1].clip_from_eye_matrix[15] == 0.0f);
  mathfu::vec3 start_pos;
  float font_size;

  // TODO(b/29914331) Separate, tested matrix decomposition util functions.
  if (is_stereo) {
    const float kTopOfTextScreenScale = .45f;
    const float kFontScreenScale = .075f;
    const float z = -1.0f;
    const float tan_half_fov = 1.0f / views[0].clip_from_eye_matrix[5];
    font_size = .5f * kFontScreenScale * -z * tan_half_fov;
    start_pos =
        mathfu::vec3(-.5f, kTopOfTextScreenScale * -z * tan_half_fov, z);
  } else {
    const float kNearPlaneOffset = .0001f;
    const float bottom = (-1.0f - views[0].clip_from_eye_matrix[13]) /
                         views[0].clip_from_eye_matrix[5];
    const float top = bottom + 2.0f / views[0].clip_from_eye_matrix[5];
    const float near_z = (1.0f + views[0].clip_from_eye_matrix[14]) /
                         views[0].clip_from_eye_matrix[10];
    const float padding = 20;
    font_size = 16;
    start_pos =
        mathfu::vec3(padding, top - padding, -(near_z - kNearPlaneOffset));
  }

  // Setup shared render state.
  font->GetTexture()->Bind(0);
  font->SetSize(font_size);

  const float uv_bounds[4] = {0, 0, 1, 1};
  SetDebugUniform(font->GetShader().get(), kTextureBoundsUniform, uv_bounds);

  const float color[4] = {1, 1, 1, 1};
  SetDebugUniform(font->GetShader().get(), kColorUniform, color);

  SetDepthTest(false);
  SetDepthWrite(false);

  char buf[512] = "";

  // Draw in each view.
  for (size_t i = 0; i < num_views; ++i) {
    SetViewport(views[i]);
    SetViewUniforms(views[i]);

    renderer_.set_model_view_projection(views[i].clip_from_eye_matrix);
    BindShader(
        font->GetShader());  // Shader needs to be bound after setting MVP.

    mathfu::vec3 pos = start_pos;
    if (is_stereo && i > 0) {
      // Reposition text so that it's consistently placed in both eye views.
      pos = views[i].world_from_eye_matrix.Inverse() *
            (views[0].world_from_eye_matrix * start_pos);
    }

    SimpleFontRenderer text(font);
    text.SetCursor(pos);

    // Draw basic render stats.
    const detail::Profiler* profiler = registry_->Get<detail::Profiler>();
    if (profiler && stats_enabled) {
      snprintf(buf, sizeof(buf),
               "FPS            %0.2f\n"
               "CPU ms         %0.2f\n"
               "GPU ms         %0.2f\n"
               "# draws        %d\n"
               "# shader swaps %d\n"
               "# verts        %d\n"
               "# tris         %d",
               profiler->GetFilteredFps(), profiler->GetCpuFrameMs(),
               profiler->GetGpuFrameMs(), profiler->GetNumDraws(),
               profiler->GetNumShaderSwaps(), profiler->GetNumVerts(),
               profiler->GetNumTris());
      text.Print(buf);
    } else if (profiler) {
      DCHECK(fps_counter);
      snprintf(buf, sizeof(buf), "FPS %0.2f\n", profiler->GetFilteredFps());
      text.Print(buf);
    }


    if (!text.GetMesh().IsEmpty()) {
      const TriangleMesh<VertexPT>& mesh = text.GetMesh();
      auto vertices = mesh.GetVertices();
      auto indices = mesh.GetIndices();
      DrawIndexedPrimitives(MeshData::PrimitiveType::kTriangles,
                            VertexPT::kFormat, vertices.data(), vertices.size(),
                            indices.data(), indices.size());
    }
  }

  // Cleanup render state.
  SetDepthTest(true);
  SetDepthWrite(true);
}

void RenderSystemFpl::OnParentChanged(const ParentChangedEvent& event) {
  sort_order_manager_.UpdateSortOrder(
      event.target, [this](Entity entity) {
        return render_component_pools_.GetComponent(entity);
      });
}

const fplbase::RenderState& RenderSystemFpl::GetRenderState() const {
  return renderer_.GetRenderState();
}

void RenderSystemFpl::UpdateCachedRenderState(
    const fplbase::RenderState& render_state) {
  renderer_.UpdateCachedRenderState(render_state);
}

}  // namespace lull
