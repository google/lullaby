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

#include "lullaby/systems/render/fpl/render_system_fpl.h"

#include <inttypes.h>
#include <stdio.h>

#include "fplbase/glplatform.h"
#include "fplbase/internal/type_conversions_gl.h"
#include "fplbase/render_utils.h"
#include "lullaby/events/render_events.h"
#include "lullaby/modules/config/config.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/render/quad_util.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/render/detail/profiler.h"
#include "lullaby/systems/render/fpl/shader.h"
#include "lullaby/systems/render/render_helpers.h"
#include "lullaby/systems/render/render_stats.h"
#include "lullaby/systems/render/simple_font.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/trace.h"
#include "lullaby/generated/render_def_generated.h"

namespace lull {
namespace {
const HashValue kRenderDefHash = ConstHash("RenderDef");
constexpr int kNumVec4sInAffineTransform = 3;
constexpr const char* kColorUniform = "color";
constexpr const char* kTextureBoundsUniform = "uv_bounds";
constexpr const char* kClampBoundsUniform = "clamp_bounds";
constexpr const char* kBoneTransformsUniform = "bone_transforms";
// We break the naming convention here for compatibility with early VR apps.
constexpr const char* kIsRightEyeUniform = "uIsRightEye";
constexpr HashValue kRenderResetStateHash = ConstHash("lull.Render.ResetState");

bool IsSupportedUniformDimension(int dimension) {
  return (dimension == 1 || dimension == 2 || dimension == 3 ||
          dimension == 4 || dimension == 9 || dimension == 16);
}

bool IsSupportedUniformType(ShaderDataType type) {
  return type >= ShaderDataType_MIN && type <= ShaderDataType_Float4x4;
}

void SetDebugUniform(Shader* shader, const char* name, const float values[4]) {
  const fplbase::UniformHandle location = shader->FindUniform(name);
  if (fplbase::ValidUniformHandle(location)) {
    shader->SetUniform(location, values, 4);
  }
}

void UpdateUniformBinding(Uniform::Description* desc, const ShaderPtr& shader) {
  if (!desc) {
    return;
  }

  if (!shader) {
    desc->binding = -1;
  }

  const Shader::UniformHnd handle = shader->FindUniform(desc->name.c_str());
  if (fplbase::ValidUniformHandle(handle)) {
    desc->binding = fplbase::GlUniformHandle(handle);
  } else {
    desc->binding = -1;
  }
}

}  // namespace

RenderSystemFpl::RenderSystemFpl(Registry* registry,
                                 const RenderSystem::InitParams& init_params)
    : System(registry),
      render_component_pools_(registry),
      sort_order_manager_(registry_),
      multiview_enabled_(init_params.enable_stereo_multiview) {
  renderer_.Initialize(mathfu::kZeros2i, "lull::RenderSystem");

  factory_ = registry->Create<RenderFactory>(registry, &renderer_);

  SetSortMode(RenderPass_Opaque, SortMode_AverageSpaceOriginFrontToBack);

  SetSortMode(RenderPass_Main, SortMode_SortOrderIncreasing);
  SetCullMode(RenderPass_Main, CullMode::kNone);

  // Attach to the immediate parent changed event since this has render
  // implications which don't want to be delayed a frame.
  auto* dispatcher = registry->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const ParentChangedImmediateEvent& event) {
    UpdateSortOrder(event.target);
  });
  dispatcher->Connect(this,
                      [this](const ChildIndexChangedImmediateEvent& event) {
                        UpdateSortOrder(event.target);
                      });

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    // TODO Move to render_system.inc if we can have optional args.
    binder->RegisterFunction("lull.Render.GetTextureId", [this](Entity entity) {
      TexturePtr texture = GetTexture(entity, 0);
      return texture ? static_cast<int>(texture->GetResourceId().handle) : 0;
    });
  }

  clear_params_.clear_options =
      ClearParams::kColor | ClearParams::kDepth | ClearParams::kStencil;
}

RenderSystemFpl::~RenderSystemFpl() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Render.GetTextureId");
  }
  registry_->Get<Dispatcher>()->DisconnectAll(this);
}

void RenderSystemFpl::SetStereoMultiviewEnabled(bool enabled) {
  multiview_enabled_ = enabled;
}

void RenderSystemFpl::PreloadFont(const char* name) {
  // TODO Remove after apps use TextSystem directly.
  std::string filename(name);
  if (!EndsWith(filename, ".ttf")) {
    filename.append(".ttf");
  }

  TextSystem* text_system = registry_->Get<TextSystem>();
  CHECK(text_system) << "Missing text system.";
  text_system->LoadFonts({filename});
}

const TexturePtr& RenderSystemFpl::GetWhiteTexture() const {
  return factory_->GetWhiteTexture();
}

const TexturePtr& RenderSystemFpl::GetInvalidTexture() const {
  return factory_->GetInvalidTexture();
}

TexturePtr RenderSystemFpl::GetTexture(HashValue texture_hash) const {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
  return nullptr;
}

TexturePtr RenderSystemFpl::LoadTexture(const std::string& filename,
                                        bool create_mips) {
  return factory_->LoadTexture(filename, create_mips);
}

void RenderSystemFpl::LoadTextureAtlas(const std::string& filename) {
  const bool create_mips = false;
  factory_->LoadTextureAtlas(filename, create_mips);
}

MeshPtr RenderSystemFpl::LoadMesh(const std::string& filename) {
  return factory_->LoadMesh(filename);
}

TexturePtr RenderSystemFpl::CreateTexture(const ImageData& image,
                                          bool create_mips) {
  return factory_->CreateTexture(image, create_mips);
}

ShaderPtr RenderSystemFpl::LoadShader(const std::string& filename) {
  return factory_->LoadShader(filename);
}

void RenderSystemFpl::Create(Entity e, HashValue type, const Def* def) {
  if (type == kRenderDefHash) {
    CreateRenderComponentFromDef(e, *ConvertDef<RenderDef>(def));
  } else {
    LOG(DFATAL) << "Invalid type passed to Create.";
  }
}

void RenderSystemFpl::Create(Entity e, HashValue pass) {
  if (render_component_pools_.GetComponent(e)) {
    SetRenderPass(e, pass);
    return;
  }
  pass = FixRenderPass(pass);
  RenderComponent& component = render_component_pools_.EmplaceComponent(
      e, static_cast<RenderPass>(pass));
  component.pass = static_cast<RenderPass>(pass);

  sort_order_manager_.UpdateSortOrder(
      e, [this](detail::EntityIdPair entity_id_pair) {
        return render_component_pools_.GetComponent(entity_id_pair.entity);
      });
}


void RenderSystemFpl::CreateRenderComponentFromDef(Entity e,
                                                   const RenderDef& data) {
  HashValue pass = FixRenderPass(data.pass());
  RenderComponent* component;
  if (data.hidden()) {
    component = &render_component_pools_.GetPool(RenderPass_Invisible)
                     .EmplaceComponent(e);
  } else {
    component = &render_component_pools_.GetPool(static_cast<RenderPass>(pass))
                     .EmplaceComponent(e);
  }
  component->pass = static_cast<RenderPass>(pass);
  component->hidden = data.hidden();

  // If the def has been generated from a RenderDefT, its members will always
  // be non-null, so check for non-empty, not just not-null.
  if (data.shader() && data.shader()->Length() > 0) {
    SetShader(e, LoadShader(data.shader()->str()));
  }

  if (data.font()) {
    // TODO Remove after apps use TextSystem directly.
    TextSystem* text_system = registry_->Get<TextSystem>();
    CHECK(text_system) << "Missing text system.";
    text_system->CreateFromRenderDef(e, data);
  }

  if (data.textures() && data.textures()->Length() > 0) {
    for (unsigned int i = 0; i < data.textures()->size(); ++i) {
      TexturePtr texture = factory_->LoadTexture(
          data.textures()->Get(i)->c_str(), data.create_mips());
      SetTexture(e, i, texture);
    }
  } else if (data.texture() && data.texture()->size() > 0) {
    TexturePtr texture =
        factory_->LoadTexture(data.texture()->c_str(), data.create_mips());
    SetTexture(e, 0, texture);
  } else if (data.external_texture()) {
#ifdef GL_TEXTURE_EXTERNAL_OES
    GLuint texture_id;
    GL_CALL(glGenTextures(1, &texture_id));
    GL_CALL(glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id));
    GL_CALL(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER,
                            GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER,
                            GL_NEAREST));
    SetTextureId(e, 0, GL_TEXTURE_EXTERNAL_OES, texture_id);
#else
    LOG(WARNING) << "External textures are not available.";
#endif  // GL_TEXTURE_EXTERNAL_OES
  }

  if (data.mesh() && data.mesh()->Length() > 0) {
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
  } else {
    SetUniform(e, kColorUniform, &component->default_color[0], 4, 1);
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
    // If the def has been generated from a RenderDefT, its members will always
    // be non-null, so check for non-empty, not just not-null.
    if (data.text() && data.text()->Length() > 0) {
      SetText(e, data.text()->c_str());
    } else if (data.quad() && data.quad()->verts_x() > 0 &&
               data.quad()->verts_y() > 0) {
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

HashValue RenderSystemFpl::GetRenderPass(Entity entity) const {
  const RenderComponent* component =
      render_component_pools_.GetComponent(entity);
  return component ? component->pass : RenderPass_Invalid;
}

std::vector<HashValue> RenderSystemFpl::GetRenderPasses(Entity entity) const {
  const RenderComponent* component =
      render_component_pools_.GetComponent(entity);
  if (component) {
    return {static_cast<HashValue>(component->pass)};
  } else {
    return {};
  }
}

void RenderSystemFpl::CreateDeferredMeshes() {
  while (!deferred_meshes_.empty()) {
    DeferredMesh& defer = deferred_meshes_.front();
    DeformMesh(defer.e, &defer.mesh);
    SetMesh(defer.e, defer.mesh, defer.mesh_id);
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

void RenderSystemFpl::SetUniform(Entity entity, Optional<HashValue> pass,
                                 Optional<int> submesh_index, string_view name,
                                 ShaderDataType type, Span<uint8_t> data,
                                 int count) {
  auto* render_component = render_component_pools_.GetComponent(entity);
  if (!render_component || !render_component->material.GetShader()) {
    return;
  }

  if (!IsSupportedUniformType(type)) {
    LOG(DFATAL) << "ShaderDataType not supported: " << type;
    return;
  }

  // Do not allow partial data in this function.
  if (data.size() != Uniform::UniformTypeToBytesSize(type) * count) {
    LOG(DFATAL) << "Partial uniform data is not allowed through "
                   "RenderSystem::SetUniform.";
    return;
  }

  Material* material = &render_component->material;

  const Uniform::Description description(std::string(name), type, count);

  Uniform* uniform = material->GetUniformByName(description.name);
  if (!uniform || uniform->GetDescription().type != description.type ||
      uniform->GetDescription().count != count) {
    const Material::UniformIndex index = material->AddUniform(description);
    uniform = material->GetUniformByIndex(index);
  }

  uniform->SetData(data.data(), data.size());

  if (uniform->GetDescription().binding == -1) {
    UpdateUniformBinding(&uniform->GetDescription(), material->GetShader());
  }

  if (render_component->uniform_changed_callback) {
    render_component->uniform_changed_callback(0, name, type, data, count);
  }
}

bool RenderSystemFpl::GetUniform(Entity entity, Optional<HashValue> pass,
                                 Optional<int> submesh_index, string_view name,
                                 size_t length, uint8_t* data_out) const {
  auto* render_component = render_component_pools_.GetComponent(entity);
  if (!render_component) {
    return false;
  }

  const Uniform* uniform = render_component->material.GetUniformByName(name);
  if (!uniform) {
    return false;
  }

  if (length > uniform->Size()) {
    return false;
  }

  memcpy(data_out, uniform->GetData<uint8_t>(), length);

  return true;
}

void RenderSystemFpl::SetUniform(Entity e, const char* name, const float* data,
                                 int dimension, int count) {
  SetUniform(e, NullOpt, NullOpt, name, FloatDimensionsToUniformType(dimension),
             {reinterpret_cast<const uint8_t*>(data),
              dimension * count * sizeof(float)},
             count);
}

bool RenderSystemFpl::GetUniform(Entity e, const char* name, size_t length,
                                 float* data_out) const {
  return GetUniform(e, NullOpt, NullOpt, name, length * sizeof(float),
                    reinterpret_cast<uint8_t*>(data_out));
}

void RenderSystemFpl::CopyUniforms(Entity entity, Entity source) {
  RenderComponent* component = render_component_pools_.GetComponent(entity);
  if (!component) {
    return;
  }

  component->material.ClearUniforms();

  const RenderComponent* source_component =
      render_component_pools_.GetComponent(source);
  if (source_component) {
    const UniformVector& uniforms = source_component->material.GetUniforms();
    for (auto& uniform : uniforms) {
      component->material.AddUniform(uniform);
    }

    if (component->material.GetShader() !=
        source_component->material.GetShader()) {
      // Fix the locations using |entity|'s shader.
      UpdateUniformLocations(component);
    }
  }
}

void RenderSystemFpl::SetUniformChangedCallback(
    Entity entity, HashValue pass,
    RenderSystem::UniformChangedCallback callback) {
  RenderComponent* component = render_component_pools_.GetComponent(entity);
  if (!component) {
    return;
  }
  component->uniform_changed_callback = std::move(callback);
}

void RenderSystemFpl::UpdateUniformLocations(RenderComponent* component) {
  if (!component->material.GetShader()) {
    return;
  }

  auto& uniforms = component->material.GetUniforms();
  for (auto& uniform : uniforms) {
    UpdateUniformBinding(&uniform.GetDescription(),
                         component->material.GetShader());
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
    LOG(ERROR) << "Incorrect bone count. Mesh contains "
               << component->mesh->GetNumBones() << " bones, but was expecting "
               << num_transforms << " bones.";
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
    // TODO Add CheckTextureSizeWarning that does not depend on HMD.

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
  render_component->material.SetTexture(unit, texture);

  if (texture) {
    max_texture_unit_ = std::max(max_texture_unit_, unit);

    // Add subtexture coordinates so the vertex shaders will pick them up. These
    // are known when the texture is created; no need to wait for load.
    SetUniform(e, kTextureBoundsUniform, &texture->UvBounds()[0], 4, 1);

    if (texture->IsLoaded()) {
      OnTextureLoaded(*render_component, unit, texture);
    } else {
      texture->AddOnLoadCallback([this, unit, e, texture]() {
        RenderComponent* render_component =
            render_component_pools_.GetComponent(e);
        if (render_component &&
            render_component->material.GetTexture(unit) == texture) {
          OnTextureLoaded(*render_component, unit, texture);
        }
      });
    }
  }
}

void RenderSystemFpl::SetTextureExternal(Entity e, HashValue pass, int unit) {
  LOG(FATAL) << "Unimplemented.";
}

TexturePtr RenderSystemFpl::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    RenderSystem::TextureProcessor processor) {
  return factory_->CreateProcessedTexture(source_texture, create_mips,
                                          processor);
}

TexturePtr RenderSystemFpl::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    const RenderSystem::TextureProcessor& processor,
    const mathfu::vec2i& output_dimensions) {
  return factory_->CreateProcessedTexture(source_texture, create_mips,
                                          processor, output_dimensions);
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
  return render_component->material.GetTexture(unit);
}


void RenderSystemFpl::SetText(Entity e, const std::string& text) {
  // TODO Remove after apps use TextSystem directly.
  TextSystem* text_system = registry_->Get<TextSystem>();
  CHECK(text_system) << "Missing text system.";
  text_system->SetText(e, text);
}

void RenderSystemFpl::SetQuad(Entity e, const Quad& quad) {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    LOG(WARNING) << "Missing entity for SetQuad: " << e;
    return;
  }
  render_component->quad = quad;

  MeshData mesh;
  if (quad.has_uv) {
    mesh = CreateQuadMesh<VertexPT>(quad.size.x, quad.size.y, quad.verts.x,
                                    quad.verts.y, quad.corner_radius,
                                    quad.corner_verts, quad.corner_mask);
  } else {
    mesh = CreateQuadMesh<VertexP>(quad.size.x, quad.size.y, quad.verts.x,
                                   quad.verts.y, quad.corner_radius,
                                   quad.corner_verts, quad.corner_mask);
  }

  auto iter = deformations_.find(e);
  if (iter != deformations_.end()) {
    DeferredMesh defer;
    defer.e = e;
    defer.mesh_id = quad.id;
    defer.mesh = std::move(mesh);
    deferred_meshes_.push(std::move(defer));
  } else {
    SetMesh(e, mesh, quad.id);
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

void RenderSystemFpl::SetMesh(Entity entity, const MeshData& mesh,
                              HashValue mesh_id) {
  MeshPtr gpu_mesh;
  if (mesh_id != 0) {
    gpu_mesh = factory_->CreateMesh(mesh_id, mesh);
  } else {
    gpu_mesh = factory_->CreateMesh(mesh);
  }
  SetMesh(entity, gpu_mesh);
}

void RenderSystemFpl::SetMesh(Entity e, const MeshData& mesh) {
  const HashValue mesh_id = 0;
  SetMesh(e, mesh, mesh_id);
}

void RenderSystemFpl::SetAndDeformMesh(Entity entity, const MeshData& mesh) {
  if (mesh.GetVertexBytes() == nullptr) {
    LOG(WARNING) << "Can't deform mesh without read access.";
    SetMesh(entity, mesh);
    return;
  }

  auto iter = deformations_.find(entity);
  if (iter != deformations_.end()) {
    DeferredMesh defer;
    defer.e = entity;
    defer.mesh = mesh.CreateHeapCopy();
    deferred_meshes_.push(std::move(defer));
  } else {
    SetMesh(entity, mesh);
  }
}

void RenderSystemFpl::SetMesh(Entity e, const std::string& file) {
  SetMesh(e, factory_->LoadMesh(file));
}

RenderSystemFpl::SortOrder RenderSystemFpl::GetSortOrder(Entity e) const {
  const auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return 0;
  }
  return render_component->sort_order;
}

RenderSystemFpl::SortOrderOffset RenderSystemFpl::GetSortOrderOffset(
    Entity entity) const {
  return sort_order_manager_.GetOffset(entity);
}

void RenderSystemFpl::SetSortOrderOffset(Entity e, SortOrderOffset offset) {
  sort_order_manager_.SetOffset(e, offset);
  sort_order_manager_.UpdateSortOrder(
      e, [this](detail::EntityIdPair entity_id_pair) {
        return render_component_pools_.GetComponent(entity_id_pair.entity);
      });
}

bool RenderSystemFpl::IsTextureSet(Entity e, int unit) const {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return false;
  }
  return render_component->material.GetTexture(unit) != nullptr;
}

bool RenderSystemFpl::IsTextureLoaded(Entity e, int unit) const {
  const auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return false;
  }

  if (!render_component->material.GetTexture(unit)) {
    return false;
  }
  return render_component->material.GetTexture(unit)->IsLoaded();
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
  const auto& textures = component.material.GetTextures();
  for (const auto& pair : textures) {
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

bool RenderSystemFpl::IsHidden(Entity entity, Optional<HashValue> pass,
                               Optional<int> submesh_index) const {
  return IsHidden(entity);
}

ShaderPtr RenderSystemFpl::GetShader(Entity entity) const {
  const RenderComponent* component =
      render_component_pools_.GetComponent(entity);
  return component ? component->material.GetShader() : ShaderPtr();
}

void RenderSystemFpl::SetShader(Entity e, const ShaderPtr& shader) {
  auto* render_component = render_component_pools_.GetComponent(e);
  if (!render_component) {
    return;
  }
  render_component->material.SetShader(shader);

  // Update the uniforms' locations in the new shader.
  UpdateUniformLocations(render_component);
}

void RenderSystemFpl::SetMaterial(Entity e, Optional<HashValue> pass,
                                  Optional<int> submesh_index,
                                  const MaterialInfo& info) {
  LOG(FATAL) << "Unimplemented.";
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
  SendEvent(registry_, e, MeshChangedEvent(e, 0));
}

void RenderSystemFpl::DeformMesh(Entity entity, MeshData* mesh) {
  auto iter = deformations_.find(entity);
  const Deformation deform =
      iter != deformations_.end() ? iter->second : nullptr;
  if (deform) {
    deform(mesh);
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

void RenderSystemFpl::SetStencilMode(Entity e, HashValue pass, StencilMode mode,
                                     int value) {
  LOG(DFATAL) << "This feature is not implemented in RenderSystemFpl.";
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

void RenderSystemFpl::Hide(Entity entity, Optional<HashValue> pass,
                           Optional<int> submesh_index) {
  Hide(entity);
}

void RenderSystemFpl::Show(Entity entity, Optional<HashValue> pass,
                           Optional<int> submesh_index) {
  Show(entity);
}

void RenderSystemFpl::SetRenderPass(Entity e, HashValue pass) {
  pass = FixRenderPass(pass);
  RenderComponent* render_component = render_component_pools_.GetComponent(e);
  if (render_component) {
    render_component->pass = static_cast<RenderPass>(pass);
    if (!render_component->hidden) {
      render_component_pools_.MoveToPool(e, static_cast<RenderPass>(pass));
    }
  }
}

SortMode RenderSystemFpl::GetSortMode(HashValue pass) const {
  pass = FixRenderPass(pass);
  const RenderPool* pool =
      render_component_pools_.GetExistingPool(static_cast<RenderPass>(pass));
  return pool ? pool->GetSortMode() : SortMode_None;
}

void RenderSystemFpl::SetSortMode(HashValue pass, SortMode mode) {
  pass = FixRenderPass(pass);
  RenderPool& pool =
      render_component_pools_.GetPool(static_cast<RenderPass>(pass));
  pool.SetSortMode(mode);
}

void RenderSystemFpl::SetSortVector(HashValue pass,
                                    const mathfu::vec3& vector) {
  pass = FixRenderPass(pass);
  RenderPool& pool =
      render_component_pools_.GetPool(static_cast<RenderPass>(pass));
  pool.SetSortVector(vector);
}

void RenderSystemFpl::SetCullMode(HashValue pass, CullMode mode) {
  pass = FixRenderPass(pass);
  RenderPool& pool =
      render_component_pools_.GetPool(static_cast<RenderPass>(pass));
  pool.SetCullMode(mode);
}

void RenderSystemFpl::SetDefaultFrontFace(FrontFace face) {
  default_front_face_ = face;
}

void RenderSystemFpl::SetDepthTest(const bool enabled) {
  if (enabled) {
#if !defined(NDEBUG) && !defined(__ANDROID__)
    // GL_DEPTH_BITS was deprecated in desktop GL 3.3, so make sure this get
    // succeeds before checking depth_bits.
    GLint depth_bits = 0;
    glGetIntegerv(GL_DEPTH_BITS, &depth_bits);
    if (glGetError() == 0 && depth_bits == 0) {
      // This has been known to cause problems on iOS 10.
      LOG_ONCE(WARNING) << "Enabling depth test without a depth buffer; this "
                           "has known issues on some platforms.";
    }
#endif  // !ION_PRODUCTION && !defined(__ANDROID__)

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
  if (renderer_.feature_level() != fplbase::kFeatureLevel30) {
    return;
  }

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

mathfu::vec4 RenderSystemFpl::GetClearColor() const {
  return clear_params_.color_value;
}

void RenderSystemFpl::SetClearColor(float r, float g, float b, float a) {
  clear_params_.color_value = mathfu::vec4(r, g, b, a);
}

void RenderSystemFpl::SetClearParams(HashValue pass,
                                     const ClearParams& clear_params) {
  clear_params_ = clear_params;
}

void RenderSystemFpl::BeginFrame() {
  LULLABY_CPU_TRACE_CALL();
  GLbitfield options = 0;
  if (CheckBit(clear_params_.clear_options, ClearParams::kColor)) {
    GL_CALL(
        glClearColor(clear_params_.color_value.x, clear_params_.color_value.y,
                     clear_params_.color_value.z, clear_params_.color_value.w));
    options |= GL_COLOR_BUFFER_BIT;
  }
  if (CheckBit(clear_params_.clear_options, ClearParams::kDepth)) {
    options |= GL_DEPTH_BUFFER_BIT;
#ifdef FPLBASE_GLES
    GL_CALL(glClearDepthf(clear_params_.depth_value));
#else
    GL_CALL(glClearDepth(static_cast<double>(clear_params_.depth_value)));
#endif
  }
  if (CheckBit(clear_params_.clear_options, ClearParams::kStencil)) {
    options |= GL_STENCIL_BUFFER_BIT;
    GL_CALL(glClearStencil(clear_params_.stencil_value));
  }
  GL_CALL(glClear(options));
}

void RenderSystemFpl::EndFrame() {
  // Something in later passes seems to expect depth write to be on. Setting
  // this here until the culprit is identified (b/36200233).
  const auto* config = registry_->Get<Config>();
  bool reset_state = true;
  if (config) {
    reset_state = config->Get(kRenderResetStateHash, reset_state);
  }
  if (reset_state) {
    SetDepthWrite(true);
  }
}

void RenderSystemFpl::BeginRendering() {}

void RenderSystemFpl::EndRendering() {
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void RenderSystemFpl::SetViewUniforms(const View& view) {
  renderer_.set_camera_pos(view.world_from_eye_matrix.TranslationVector3D());

  rendering_right_eye_ = view.eye == 1;
}

void RenderSystemFpl::RenderAt(const RenderComponent* component,
                               const mathfu::mat4& world_from_entity_matrix,
                               const View& view) {
  LULLABY_CPU_TRACE_CALL();
  const ShaderPtr& shader = component->material.GetShader();
  if (!shader || !component->mesh) {
    return;
  }

  const mathfu::mat4 clip_from_entity_matrix = CalculateClipFromModelMatrix(
      world_from_entity_matrix, view.clip_from_world_matrix);
  renderer_.set_model_view_projection(clip_from_entity_matrix);
  renderer_.set_model(world_from_entity_matrix);

  BindShader(shader);
  SetShaderUniforms(shader, component->material.GetUniforms());

  const Shader::UniformHnd mat_normal_uniform_handle =
      shader->FindUniform("mat_normal");
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
    GL_CALL(glUniformMatrix3fv(uniform_gl, 1, false, packed[0].data_));
  }
  const Shader::UniformHnd camera_dir_handle =
      shader->FindUniform("camera_dir");
  if (fplbase::ValidUniformHandle(camera_dir_handle)) {
    const int uniform_gl = fplbase::GlUniformHandle(camera_dir_handle);
    mathfu::vec3_packed camera_dir;
    CalculateCameraDirection(view.world_from_eye_matrix).Pack(&camera_dir);
    GL_CALL(glUniform3fv(uniform_gl, 1, camera_dir.data_));
  }

  const auto& textures = component->material.GetTextures();
  for (const auto& texture : textures) {
    texture.second->Bind(texture.first);
  }

  // Bit of magic to determine if the scalar is negative and if so flip the cull
  // face. This possibly be revised (b/38235916).
  if (CalculateDeterminant3x3(world_from_entity_matrix) >= 0.0f) {
    GL_CALL(glFrontFace(default_front_face_ == FrontFace::kClockwise ? GL_CW
                                                                     : GL_CCW));
  } else {
    GL_CALL(glFrontFace(default_front_face_ == FrontFace::kClockwise ? GL_CCW
                                                                     : GL_CW));
  }

  BindStencilMode(component->stencil_mode, component->stencil_value);
  DrawMeshFromComponent(component);
}

void RenderSystemFpl::RenderAtMultiview(
    const RenderComponent* component,
    const mathfu::mat4& world_from_entity_matrix, const View* views) {
  LULLABY_CPU_TRACE_CALL();
  const ShaderPtr& shader = component->material.GetShader();
  if (!shader || !component->mesh) {
    return;
  }

  const mathfu::mat4 clip_from_entity_matrix[] = {
      views[0].clip_from_world_matrix * world_from_entity_matrix,
      views[1].clip_from_world_matrix * world_from_entity_matrix,
  };

  BindShader(shader);
  SetShaderUniforms(shader, component->material.GetUniforms());

  const Shader::UniformHnd mvp_uniform_handle =
      shader->FindUniform("model_view_projection");
  if (fplbase::ValidUniformHandle(mvp_uniform_handle)) {
    const int uniform_gl = fplbase::GlUniformHandle(mvp_uniform_handle);
    GL_CALL(glUniformMatrix4fv(uniform_gl, 2, false,
                               &(clip_from_entity_matrix[0][0])));
  }
  const Shader::UniformHnd mat_normal_uniform_handle =
      shader->FindUniform("mat_normal");
  if (fplbase::ValidUniformHandle(mat_normal_uniform_handle)) {
    const int uniform_gl = fplbase::GlUniformHandle(mat_normal_uniform_handle);
    const mathfu::mat3 normal_matrix =
        ComputeNormalMatrix(world_from_entity_matrix);
    mathfu::VectorPacked<float, 3> packed[3];
    normal_matrix.Pack(packed);
    GL_CALL(glUniformMatrix3fv(uniform_gl, 1, false, packed[0].data_));
  }
  const Shader::UniformHnd camera_dir_handle =
      shader->FindUniform("camera_dir");
  if (fplbase::ValidUniformHandle(camera_dir_handle)) {
    const int uniform_gl = fplbase::GlUniformHandle(camera_dir_handle);
    mathfu::vec3_packed camera_dir[2];
    for (size_t i = 0; i < 2; ++i) {
      CalculateCameraDirection(views[i].world_from_eye_matrix)
          .Pack(&camera_dir[i]);
    }
    GL_CALL(glUniform3fv(uniform_gl, 2, camera_dir[0].data_));
  }

  const auto& textures = component->material.GetTextures();
  for (const auto& texture : textures) {
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

void RenderSystemFpl::SetShaderUniforms(const ShaderPtr& shader,
                                        const UniformVector& uniforms) {
  for (const auto& uniform : uniforms) {
    shader->BindUniform(uniform);
  }
}

void RenderSystemFpl::DrawMeshFromComponent(const RenderComponent* component) {
  if (component->mesh) {
    component->mesh->Render(&renderer_, blend_mode_);
    detail::Profiler* profiler = registry_->Get<detail::Profiler>();
    if (profiler) {
      profiler->RecordDraw(component->material.GetShader(),
                           component->mesh->GetNumVertices(),
                           component->mesh->GetNumTriangles());
    }
  }
}

void RenderSystemFpl::RenderDisplayList(const View& view,
                                        const DisplayList& display_list) {
  LULLABY_CPU_TRACE_CALL();
  const std::vector<DisplayList::Entry>* list = display_list.GetContents();
  std::for_each(
      list->begin(), list->end(), [&](const DisplayList::Entry& info) {
        if (info.component) {
          RenderAt(info.component, info.world_from_entity_matrix, view);
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
                                             size_t num_views, HashValue pass) {
  pass = FixRenderPass(pass);
  const RenderPool& pool =
      render_component_pools_.GetPool(static_cast<RenderPass>(pass));
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
                             HashValue pass) {
  pass = FixRenderPass(pass);
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
      // this here until the culprit is identified (b/36200233). Since not all
      // apps call EndFrame, we can't rely solely on the depth write call there.
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
    if (!multiview_enabled_) {
      GL_CALL(glUniform1i(fplbase::GlUniformHandle(uniform_is_right_eye),
                          static_cast<int>(rendering_right_eye_)));
    } else {
      int right_eye_uniform[] = {0, 1};
      GL_CALL(glUniform1iv(fplbase::GlUniformHandle(uniform_is_right_eye), 2,
                           right_eye_uniform));
    }
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

void RenderSystemFpl::DrawMesh(const MeshData& mesh,
                               Optional<mathfu::mat4> clip_from_model) {
  if (clip_from_model) {
    renderer_.set_model_view_projection(*clip_from_model);
    // Shader needs to be rebound after setting MVP.
    if (shader_) {
      BindShader(shader_);
    }
  }

  if (mesh.GetNumVertices() == 0) {
    return;
  }
  if (!mesh.GetVertexBytes()) {
    LOG(DFATAL) << "Can't draw mesh without vertex read access.";
    return;
  }

  const fplbase::Mesh::Primitive fpl_prim =
      Mesh::GetFplPrimitiveType(mesh.GetPrimitiveType());
  const auto vertex_size =
      static_cast<int>(mesh.GetVertexFormat().GetVertexSize());
  fplbase::Attribute attributes[Mesh::kMaxFplAttributeArraySize];
  Mesh::GetFplAttributes(mesh.GetVertexFormat(), attributes);

  if (mesh.GetNumIndices() > 0) {
    if (!mesh.GetIndexBytes()) {
      LOG(DFATAL) << "Can't draw mesh without index read access.";
      return;
    }
    if (mesh.GetIndexType() == MeshData::kIndexU16) {
      fplbase::RenderArray(fpl_prim, static_cast<int>(mesh.GetNumIndices()),
                           attributes, vertex_size, mesh.GetVertexBytes(),
                           mesh.GetIndexData<uint16_t>());
    } else {
      fplbase::RenderArray(fpl_prim, static_cast<int>(mesh.GetNumIndices()),
                           attributes, vertex_size, mesh.GetVertexBytes(),
                           mesh.GetIndexData<uint32_t>());
    }
  } else {
    fplbase::RenderArray(fpl_prim, static_cast<int>(mesh.GetNumVertices()),
                         attributes, vertex_size, mesh.GetVertexBytes());
  }
}

void RenderSystemFpl::UpdateDynamicMesh(
    Entity entity, PrimitiveType primitive_type,
    const VertexFormat& vertex_format, const size_t max_vertices,
    const size_t max_indices, MeshData::IndexType index_type,
    const size_t max_ranges,
    const std::function<void(MeshData*)>& update_mesh) {
  RenderComponent* component = render_component_pools_.GetComponent(entity);
  if (!component) {
    return;
  }

  if (max_vertices > 0) {
    const MeshData::IndexType index_type = MeshData::kIndexU16;
    DataContainer vertex_data = DataContainer::CreateHeapDataContainer(
        max_vertices * vertex_format.GetVertexSize());
    DataContainer index_data = DataContainer::CreateHeapDataContainer(
        max_indices * MeshData::GetIndexSize(index_type));
    DataContainer range_data = DataContainer::CreateHeapDataContainer(
        max_ranges * sizeof(MeshData::IndexRange));
    MeshData data(primitive_type, vertex_format, std::move(vertex_data),
                  index_type, std::move(index_data), std::move(range_data));
    update_mesh(&data);
    component->mesh = factory_->CreateMesh(data);
  } else {
    component->mesh.reset();
  }
  SendEvent(registry_, entity, MeshChangedEvent(entity, 0));
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
  const bool is_perspective = views[0].clip_from_eye_matrix[15] == 0.0f;
  const bool is_stereo = (num_views == 2 && is_perspective &&
                          views[1].clip_from_eye_matrix[15] == 0.0f);
  mathfu::vec3 start_pos;
  float font_size;

  // TODO Separate, tested matrix decomposition util functions.
  if (is_perspective) {
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
    BindShader(font->GetShader());

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

    DrawMesh(text.GetMesh(), views[i].clip_from_eye_matrix);
  }

  // Cleanup render state.
  SetDepthTest(true);
  SetDepthWrite(true);
}

void RenderSystemFpl::UpdateSortOrder(Entity entity) {
  sort_order_manager_.UpdateSortOrder(
      entity, [this](detail::EntityIdPair entity_id_pair) {
        return render_component_pools_.GetComponent(entity_id_pair.entity);
      });
}

const fplbase::RenderState& RenderSystemFpl::GetCachedRenderState() const {
  return renderer_.GetRenderState();
}

void RenderSystemFpl::UpdateCachedRenderState(
    const fplbase::RenderState& render_state) {
  renderer_.UpdateCachedRenderState(render_state);
}

void RenderSystemFpl::CreateRenderTarget(
    HashValue /*render_target_name*/,
    const RenderTargetCreateParams& /*create_params*/) {
  LOG(DFATAL) << "CreateRenderTarget is not supported with Render System Fpl.";
}

void RenderSystemFpl::SetRenderTarget(HashValue pass,
                                      HashValue render_target_name) {
  LOG(DFATAL) << "SetRenderTarget is not supported with Render System Fpl.";
}

ImageData RenderSystemFpl::GetRenderTargetData(HashValue render_target_name) {
  LOG(DFATAL) << "GetRenderTargetData is not supported with Render System Fpl.";
  return ImageData();
}

void RenderSystemFpl::Destroy(Entity /*e*/, HashValue pass) {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
}

void RenderSystemFpl::SetUniform(Entity e, HashValue pass, const char* name,
                                 const float* data, int dimension,
                                 int count) {
  SetUniform(e, pass, NullOpt, name, FloatDimensionsToUniformType(dimension),
             {reinterpret_cast<const uint8_t*>(data),
              dimension * count * sizeof(float)},
             count);
}
bool RenderSystemFpl::GetUniform(Entity /*e*/, HashValue pass, const char* name,
                                 size_t length, float* data_out) const {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
  return false;
}
void RenderSystemFpl::SetMesh(Entity e, HashValue /*pass*/,
                              const MeshPtr& mesh) {
  SetMesh(e, mesh);
}
void RenderSystemFpl::SetMesh(Entity e, HashValue /*pass*/,
                              const MeshData& mesh) {
  SetMesh(e, mesh);
}
MeshPtr RenderSystemFpl::GetMesh(Entity e, HashValue /*pass*/) {
  RenderComponent* component = render_component_pools_.GetComponent(e);
  return component ? component->mesh : nullptr;
}
ShaderPtr RenderSystemFpl::GetShader(Entity /*entity*/, HashValue pass) const {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
  return nullptr;
}
void RenderSystemFpl::SetShader(Entity entity, HashValue pass,
                                const ShaderPtr& shader) {
  SetShader(entity, shader);
}
void RenderSystemFpl::SetTexture(Entity /*e*/, HashValue pass, int unit,
                                 const TexturePtr& texture) {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
}
void RenderSystemFpl::SetTextureId(Entity /*e*/, HashValue pass, int unit,
                                   uint32_t texture_target,
                                   uint32_t texture_id) {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
}
void RenderSystemFpl::SetSortOrderOffset(Entity /*e*/, HashValue pass,
                                         SortOrderOffset offset) {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
}

void RenderSystemFpl::SetRenderState(HashValue pass,
                                     const fplbase::RenderState& render_state) {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
}

bool RenderSystemFpl::IsShaderFeatureRequested(Entity entity,
                                               Optional<HashValue> pass,
                                               Optional<int> submesh_index,
                                               HashValue feature) const {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
  return false;
}

void RenderSystemFpl::RequestShaderFeature(Entity entity,
                                           Optional<HashValue> pass,
                                           Optional<int> submesh_index,
                                           HashValue feature) {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
}

void RenderSystemFpl::ClearShaderFeatures(Entity entity,
                                          Optional<HashValue> pass,
                                          Optional<int> submesh_index) {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
}

void RenderSystemFpl::ClearShaderFeature(Entity entity,
                                         Optional<HashValue> pass,
                                         Optional<int> submesh_index,
                                         HashValue feature) {
  LOG(DFATAL) << "This feature is only implemented in RenderSystemNext.";
}

Optional<HashValue> RenderSystemFpl::GetGroupId(Entity entity) const {
  // Does nothing.
  return Optional<HashValue>();
}

void RenderSystemFpl::SetGroupId(Entity entity,
                                 const Optional<HashValue>& group_id) {
  // Does nothing.
}

const RenderSystem::GroupParams* RenderSystemFpl::GetGroupParams(
    HashValue group_id) const {
  // Does nothing.
  return nullptr;
}

void RenderSystemFpl::SetGroupParams(
    HashValue group_id, const RenderSystem::GroupParams& group_params) {
  // Does nothing.
}

std::string RenderSystemFpl::GetShaderString(Entity entity, HashValue pass,
                                             int submesh_index,
                                             ShaderStageType stage) const {
  auto* render_component = render_component_pools_.GetComponent(entity);
  if (!render_component || !render_component->material.GetShader()) {
    return "";
  }

  return factory_->GetShaderString(
      render_component->material.GetShader()->Impl()->filename(), stage);
}

ShaderPtr RenderSystemFpl::CompileShaderString(
    const std::string& vertex_string, const std::string& fragment_string) {
  return factory_->CompileShaderFromStrings(vertex_string, fragment_string);
}

}  // namespace lull
