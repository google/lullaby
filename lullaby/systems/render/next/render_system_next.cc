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

#include "lullaby/systems/render/next/render_system_next.h"

#include <inttypes.h>
#include <stdio.h>

#include "fplbase/glplatform.h"
#include "fplbase/internal/type_conversions_gl.h"
#include "fplbase/render_utils.h"
#include "lullaby/events/render_events.h"
#include "lullaby/modules/config/config.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/file/file.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/render/mesh_util.h"
#include "lullaby/modules/render/triangle_mesh.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/render/detail/profiler.h"
#include "lullaby/systems/render/next/render_state.h"
#include "lullaby/systems/render/render_stats.h"
#include "lullaby/systems/render/simple_font.h"
#include "lullaby/systems/rig/rig_system.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/math.h"
#include "lullaby/util/trace.h"
#include "lullaby/generated/render_def_generated.h"

namespace lull {
namespace {
constexpr int kInitialRenderPoolSize = 512;
const HashValue kRenderDefHash = Hash("RenderDef");
const RenderSystemNext::RenderComponentID kDefaultRenderId = 0;
constexpr int kNumVec4sInAffineTransform = 3;
constexpr const char* kColorUniform = "color";
constexpr const char* kTextureBoundsUniform = "uv_bounds";
constexpr const char* kClampBoundsUniform = "clamp_bounds";
constexpr const char* kBoneTransformsUniform = "bone_transforms";
// We break the naming convention here for compatibility with early VR apps.
constexpr const char* kIsRightEyeUniform = "uIsRightEye";

template <typename T>
void RemoveFromVector(std::vector<T>* vector, const T& value) {
  if (!vector) {
    return;
  }

  for (auto it = vector->cbegin(); it != vector->cend(); ++it) {
    if (*it == value) {
      vector->erase(it);
      return;
    }
  }
}

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

fplbase::RenderTargetTextureFormat RenderTargetTextureFormatToFpl(
    TextureFormat format) {
  switch (format) {
    case TextureFormat_A8:
      return fplbase::kRenderTargetTextureFormatA8;
      break;
    case TextureFormat_R8:
      return fplbase::kRenderTargetTextureFormatR8;
      break;
    case TextureFormat_RGB8:
      return fplbase::kRenderTargetTextureFormatRGB8;
      break;
    case TextureFormat_RGBA8:
      return fplbase::kRenderTargetTextureFormatRGBA8;
      break;
    default:
      LOG(DFATAL) << "Unknown render target texture format.";
      return fplbase::kRenderTargetTextureFormatCount;
  }
}

fplbase::DepthStencilFormat DepthStencilFormatToFpl(DepthStencilFormat format) {
  switch (format) {
    case DepthStencilFormat_None:
      return fplbase::kDepthStencilFormatNone;
      break;
    case DepthStencilFormat_Depth16:
      return fplbase::kDepthStencilFormatDepth16;
      break;
    case DepthStencilFormat_Depth24:
      return fplbase::kDepthStencilFormatDepth24;
      break;
    case DepthStencilFormat_Depth32F:
      return fplbase::kDepthStencilFormatDepth32F;
      break;
    case DepthStencilFormat_Depth24Stencil8:
      return fplbase::kDepthStencilFormatDepth24Stencil8;
      break;
    case DepthStencilFormat_Depth32FStencil8:
      return fplbase::kDepthStencilFormatDepth32FStencil8;
      break;
    case DepthStencilFormat_Stencil8:
      return fplbase::kDepthStencilFormatStencil8;
      break;
    default:
      LOG(DFATAL) << "Unknown depth stencil format.";
      return fplbase::kDepthStencilFormatCount;
  }
}

void UpdateUniformBinding(Uniform::Description* desc, const ShaderPtr& shader) {
  if (!desc) {
    return;
  }

  if (shader) {
    const Shader::UniformHnd handle = shader->FindUniform(desc->name.c_str());
    if (fplbase::ValidUniformHandle(handle)) {
      desc->binding = fplbase::GlUniformHandle(handle);
      return;
    }
  }

  desc->binding = -1;
}

}  // namespace

RenderSystemNext::RenderSystemNext(Registry* registry)
    : System(registry),
      components_(kInitialRenderPoolSize),
      sort_order_manager_(registry_) {
  renderer_.Initialize(mathfu::kZeros2i, "lull::RenderSystem");

  factory_ = registry->Create<RenderFactory>(registry, &renderer_);
  registry_->Get<Dispatcher>()->Connect(
      this,
      [this](const ParentChangedEvent& event) { OnParentChanged(event); });

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.Render.Show", &lull::RenderSystem::Show);
    binder->RegisterMethod("lull.Render.Hide", &lull::RenderSystem::Hide);
    binder->RegisterFunction("lull.Render.GetTextureId", [this](Entity entity) {
      TexturePtr texture = GetTexture(entity, 0);
      return texture ? static_cast<int>(texture->GetResourceId().handle) : 0;
    });
    binder->RegisterMethod("lull.Render.SetColor",
                           &lull::RenderSystem::SetColor);
  }
}

RenderSystemNext::~RenderSystemNext() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Render.Show");
    binder->UnregisterFunction("lull.Render.Hide");
    binder->UnregisterFunction("lull.Render.GetTextureId");
    binder->UnregisterFunction("lull.Render.SetColor");
  }
  registry_->Get<Dispatcher>()->DisconnectAll(this);
}

void RenderSystemNext::Initialize() { InitDefaultRenderPasses(); }

void RenderSystemNext::SetStereoMultiviewEnabled(bool enabled) {
  multiview_enabled_ = enabled;
}

void RenderSystemNext::PreloadFont(const char* name) {
  LOG(FATAL) << "Deprecated.";
}

FontPtr RenderSystemNext::LoadFonts(const std::vector<std::string>& names) {
  LOG(FATAL) << "Deprecated.";
  return nullptr;
}

const TexturePtr& RenderSystemNext::GetWhiteTexture() const {
  return factory_->GetWhiteTexture();
}

const TexturePtr& RenderSystemNext::GetInvalidTexture() const {
  return factory_->GetInvalidTexture();
}

TexturePtr RenderSystemNext::LoadTexture(const std::string& filename,
                                         bool create_mips) {
  return factory_->LoadTexture(filename, create_mips);
}

TexturePtr RenderSystemNext::GetTexture(HashValue texture_hash) const {
  return factory_->GetCachedTexture(texture_hash);
}

void RenderSystemNext::LoadTextureAtlas(const std::string& filename) {
  const bool create_mips = false;
  factory_->LoadTextureAtlas(filename, create_mips);
}

MeshPtr RenderSystemNext::LoadMesh(const std::string& filename) {
  return factory_->LoadMesh(filename);
}


ShaderPtr RenderSystemNext::LoadShader(const std::string& filename) {
  return factory_->LoadShader(filename);
}

void RenderSystemNext::Create(Entity e, HashValue component_id,
                              HashValue pass) {
  const EntityIdPair entity_id_pair(e, component_id);
  RenderComponent* component = components_.Emplace(entity_id_pair);
  if (!component) {
    LOG(DFATAL) << "RenderComponent for Entity " << e << " with id "
                << component_id << " already exists.";
    return;
  }

  entity_ids_[e].push_back(entity_id_pair);
  component->pass = pass;
  SetSortOrderOffset(e, 0);
}

void RenderSystemNext::Create(Entity e, HashValue pass) {
  Create(e, kDefaultRenderId, pass);
}

void RenderSystemNext::Create(Entity e, HashValue type, const Def* def) {
  if (type != kRenderDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting RenderDef!";
    return;
  }

  const RenderDef& data = *ConvertDef<RenderDef>(def);
  if (data.font()) {
    LOG(FATAL) << "Deprecated.";
  }

  const EntityIdPair entity_id_pair(e, data.id());
  RenderComponent* component = components_.Emplace(entity_id_pair);
  if (!component) {
    LOG(DFATAL) << "RenderComponent for Entity " << e << " with id "
                << data.id() << " already exists.";
    return;
  }

  entity_ids_[e].push_back(entity_id_pair);
  if (data.pass() == RenderPass_Pano) {
    component->pass = ConstHash("Pano");
  } else if (data.pass() == RenderPass_Opaque) {
    component->pass = ConstHash("Opaque");
  } else if (data.pass() == RenderPass_Main) {
    component->pass = ConstHash("Main");
  } else if (data.pass() == RenderPass_OverDraw) {
    component->pass = ConstHash("OverDraw");
  } else if (data.pass() == RenderPass_Debug) {
    component->pass = ConstHash("Debug");
  } else if (data.pass() == RenderPass_Invisible) {
    component->pass = ConstHash("Invisible");
  } else if (data.pass() == RenderPass_OverDrawGlow) {
    component->pass = ConstHash("OverDrawGlow");
  }
  component->hidden = data.hidden();

  if (data.shader()) {
    SetShader(e, data.id(), LoadShader(data.shader()->str()));
  }

  if (data.textures()) {
    for (unsigned int i = 0; i < data.textures()->size(); ++i) {
      TexturePtr texture = factory_->LoadTexture(
          data.textures()->Get(i)->c_str(), data.create_mips());
      SetTexture(e, data.id(), i, texture);
    }
  } else if (data.texture() && data.texture()->size() > 0) {
    TexturePtr texture =
        factory_->LoadTexture(data.texture()->c_str(), data.create_mips());
    SetTexture(e, data.id(), 0, texture);
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
    SetTextureId(e, data.id(), 0, GL_TEXTURE_EXTERNAL_OES, texture_id);
#else
    LOG(WARNING) << "External textures are not available.";
#endif  // GL_TEXTURE_EXTERNAL_OES
  }

  if (data.mesh()) {
    SetMesh(e, data.id(), factory_->LoadMesh(data.mesh()->c_str()));
  }
  if (data.color()) {
    mathfu::vec4 color;
    MathfuVec4FromFbColor(data.color(), &color);
    SetUniform(e, data.id(), kColorUniform, &color[0], 4, 1);
    component->default_color = color;
  } else if (data.color_hex()) {
    mathfu::vec4 color;
    MathfuVec4FromFbColorHex(data.color_hex()->c_str(), &color);
    SetUniform(e, data.id(), kColorUniform, &color[0], 4, 1);
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
      SetUniform(e, data.id(), uniform->name()->c_str(),
                 uniform->float_value()->data(), uniform->dimension(),
                 uniform->count());
    }
  }
  SetSortOrderOffset(e, data.id(), data.sort_order_offset());
}

void RenderSystemNext::PostCreateInit(Entity e, HashValue type,
                                      const Def* def) {
  if (type == kRenderDefHash) {
    auto& data = *ConvertDef<RenderDef>(def);
    if (data.text()) {
      LOG(FATAL) << "Deprecated.";
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
      SetQuad(e, data.id(), quad);
    }
  }
}

void RenderSystemNext::Destroy(Entity e) {
  SetStencilMode(e, StencilMode::kDisabled, 0);

  auto iter = entity_ids_.find(e);
  if (iter != entity_ids_.end()) {
    for (const EntityIdPair& entity_id_pair : iter->second) {
      components_.Destroy(entity_id_pair);
    }
    entity_ids_.erase(iter);
  }
  deformations_.erase(e);
  sort_order_manager_.Destroy(e);
}

void RenderSystemNext::Destroy(Entity e, HashValue component_id) {
  const EntityIdPair entity_id_pair(e, component_id);
  SetStencilMode(e, component_id, StencilMode::kDisabled, 0);

  auto iter = entity_ids_.find(e);
  if (iter != entity_ids_.end()) {
    RemoveFromVector(&iter->second, entity_id_pair);
  }

  deformations_.erase(e);
  sort_order_manager_.Destroy(entity_id_pair);
}

void RenderSystemNext::SetQuadImpl(Entity e, HashValue component_id,
                                   const Quad& quad) {
  if (quad.has_uv) {
    SetMesh(e, component_id, CreateQuad<VertexPT>(e, component_id, quad));
  } else {
    SetMesh(e, component_id, CreateQuad<VertexP>(e, component_id, quad));
  }
}

void RenderSystemNext::CreateDeferredMeshes() {
  while (!deferred_meshes_.empty()) {
    DeferredMesh& defer = deferred_meshes_.front();
    switch (defer.type) {
      case DeferredMesh::kQuad:
        SetQuadImpl(defer.entity_id_pair.entity, defer.entity_id_pair.id,
                    defer.quad);
        break;
      case DeferredMesh::kMesh:
        DeformMesh(defer.entity_id_pair.entity, defer.entity_id_pair.id,
                   &defer.mesh);
        SetMesh(defer.entity_id_pair.entity, defer.entity_id_pair.id,
                defer.mesh);
        break;
    }
    deferred_meshes_.pop();
  }
}

void RenderSystemNext::ProcessTasks() {
  LULLABY_CPU_TRACE_CALL();

  CreateDeferredMeshes();
  factory_->UpdateAssetLoad();
}

void RenderSystemNext::WaitForAssetsToLoad() {
  CreateDeferredMeshes();
  factory_->WaitForAssetsToLoad();
}

const mathfu::vec4& RenderSystemNext::GetDefaultColor(Entity entity) const {
  const RenderComponent* component = components_.Get(entity);
  if (component) {
    return component->default_color;
  }
  return mathfu::kOnes4f;
}

void RenderSystemNext::SetDefaultColor(Entity entity,
                                       const mathfu::vec4& color) {
  RenderComponent* component = components_.Get(entity);
  if (component) {
    component->default_color = color;
  }
}

bool RenderSystemNext::GetColor(Entity entity, mathfu::vec4* color) const {
  return GetUniform(entity, kColorUniform, 4, &(*color)[0]);
}

void RenderSystemNext::SetColor(Entity entity, const mathfu::vec4& color) {
  SetUniform(entity, kColorUniform, &color[0], 4, 1);
}

void RenderSystemNext::SetUniform(Entity e, const char* name, const float* data,
                                  int dimension, int count) {
  SetUniform(e, kDefaultRenderId, name, data, dimension, count);
}

void RenderSystemNext::SetUniform(Entity e, HashValue component_id,
                                  const char* name, const float* data,
                                  int dimension, int count) {
  if (!IsSupportedUniformDimension(dimension)) {
    LOG(DFATAL) << "Unsupported uniform dimension " << dimension;
    return;
  }

  const EntityIdPair entity_id_pair(e, component_id);
  auto* render_component = components_.Get(entity_id_pair);
  if (!render_component || !render_component->material.GetShader()) {
    return;
  }

  const size_t num_bytes = dimension * count * sizeof(float);
  Uniform* uniform = render_component->material.GetUniformByName(name);
  if (!uniform || uniform->GetDescription().num_bytes != num_bytes) {
    Uniform::Description desc(
        name, (dimension > 4) ? Uniform::Type::kMatrix : Uniform::Type::kFloats,
        num_bytes, count);
    if (!uniform) {
      render_component->material.AddUniform(desc);
    } else {
      render_component->material.UpdateUniform(desc);
    }
  }

  render_component->material.SetUniformByName(name, data, dimension * count);
  uniform = render_component->material.GetUniformByName(name);
  if (uniform && uniform->GetDescription().binding == -1) {
    UpdateUniformBinding(&uniform->GetDescription(),
                         render_component->material.GetShader());
  }
}

bool RenderSystemNext::GetUniform(Entity e, const char* name, size_t length,
                                  float* data_out) const {
  return GetUniform(e, kDefaultRenderId, name, length, data_out);
}

bool RenderSystemNext::GetUniform(Entity e, HashValue component_id,
                                  const char* name, size_t length,
                                  float* data_out) const {
  const EntityIdPair entity_id_pair(e, component_id);
  auto* render_component = components_.Get(entity_id_pair);
  if (!render_component) {
    return false;
  }

  const Uniform* uniform = render_component->material.GetUniformByName(name);
  if (!uniform) {
    return false;
  }

  const Uniform::Description& desc = uniform->GetDescription();
  // Length is the number of floats expected. Convert it into size in bytes.
  const size_t expected_bytes = length * sizeof(float);
  if (expected_bytes < desc.num_bytes) {
    return false;
  }

  memcpy(data_out, uniform->GetData<float>(), sizeof(float) * length);

  return true;
}

void RenderSystemNext::CopyUniforms(Entity entity, Entity source) {
  RenderComponent* component = components_.Get(entity);
  if (!component) {
    return;
  }

  component->material.ClearUniforms();

  const RenderComponent* source_component = components_.Get(source);
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

void RenderSystemNext::UpdateUniformLocations(RenderComponent* component) {
  if (!component->material.GetShader()) {
    return;
  }

  auto& uniforms = component->material.GetUniforms();
  for (Uniform& uniform : uniforms) {
    UpdateUniformBinding(&uniform.GetDescription(),
                         component->material.GetShader());
  }
}

int RenderSystemNext::GetNumBones(Entity entity) const {
  const RenderComponent* component = components_.Get(entity);
  if (!component || !component->mesh) {
    return 0;
  }
  return component->mesh->GetNumBones();
}

const uint8_t* RenderSystemNext::GetBoneParents(Entity e, int* num) const {
  auto* render_component = components_.Get(e);
  if (!render_component || !render_component->mesh) {
    if (num) {
      *num = 0;
    }
    return nullptr;
  }
  return render_component->mesh->GetBoneParents(num);
}

const std::string* RenderSystemNext::GetBoneNames(Entity e, int* num) const {
  auto* render_component = components_.Get(e);
  if (!render_component || !render_component->mesh) {
    if (num) {
      *num = 0;
    }
    return nullptr;
  }
  return render_component->mesh->GetBoneNames(num);
}

const mathfu::AffineTransform*
RenderSystemNext::GetDefaultBoneTransformInverses(Entity e, int* num) const {
  auto* render_component = components_.Get(e);
  if (!render_component || !render_component->mesh) {
    if (num) {
      *num = 0;
    }
    return nullptr;
  }
  return render_component->mesh->GetDefaultBoneTransformInverses(num);
}

void RenderSystemNext::SetBoneTransforms(
    Entity entity, const mathfu::AffineTransform* transforms,
    int num_transforms) {
  RenderComponent* component = components_.Get(entity);
  if (!component || !component->mesh) {
    return;
  }

  // GLES2 only supports square matrices, so send the affine transforms as an
  // array of 3 * num_transforms vec4s.
  constexpr int dimension = 4;
  const float* data = nullptr;
  int count = 0;

  if (component->mesh->IsLoaded()) {
    const int num_shader_bones = component->mesh->GetNumShaderBones();
    shader_transforms_.resize(num_shader_bones);

    if (num_transforms != component->mesh->GetNumBones()) {
      LOG(DFATAL) << "Mesh must have " << num_transforms << " bones.";
      return;
    }
    component->mesh->GatherShaderTransforms(transforms,
                                            shader_transforms_.data());

    data = &(shader_transforms_[0][0]);
    count = kNumVec4sInAffineTransform * num_shader_bones;
    component->need_to_gather_bone_transforms = false;
  } else {
    // We can't calculate the actual uniform values until the mesh is loaded, so
    // cache the desired values and we'll correct them when the load is done.
    data = &(transforms[0][0]);
    count = kNumVec4sInAffineTransform * num_transforms;
    component->need_to_gather_bone_transforms = true;
  }

  SetUniform(entity, kBoneTransformsUniform, data, dimension, count);
}

void RenderSystemNext::OnTextureLoaded(const RenderComponent& component,
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

void RenderSystemNext::SetTexture(Entity e, int unit,
                                  const TexturePtr& texture) {
  SetTexture(e, kDefaultRenderId, unit, texture);
}

void RenderSystemNext::SetTexture(Entity e, HashValue component_id, int unit,
                                  const TexturePtr& texture) {
  const EntityIdPair entity_id_pair(e, component_id);
  auto* render_component = components_.Get(entity_id_pair);
  if (!render_component) {
    return;
  }

  render_component->material.SetTexture(unit, texture);
  max_texture_unit_ = std::max(max_texture_unit_, unit);

  // Add subtexture coordinates so the vertex shaders will pick them up.  These
  // are known when the texture is created; no need to wait for load.
  SetUniform(e, component_id, kTextureBoundsUniform, &texture->UvBounds()[0], 4,
             1);

  if (texture->IsLoaded()) {
    OnTextureLoaded(*render_component, unit, texture);
  } else {
    texture->AddOnLoadCallback([this, unit, entity_id_pair, texture]() {
      RenderComponent* render_component = components_.Get(entity_id_pair);
      if (render_component &&
          render_component->material.GetTexture(unit) == texture) {
        OnTextureLoaded(*render_component, unit, texture);
      }
    });
  }
}

TexturePtr RenderSystemNext::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    RenderSystem::TextureProcessor processor) {
  return factory_->CreateProcessedTexture(source_texture, create_mips,
                                          processor);
}

TexturePtr RenderSystemNext::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    const RenderSystem::TextureProcessor& processor,
    const mathfu::vec2i& output_dimensions) {
  return factory_->CreateProcessedTexture(source_texture, create_mips,
                                          processor, output_dimensions);
}

void RenderSystemNext::SetTextureId(Entity e, int unit, uint32_t texture_target,
                                    uint32_t texture_id) {
  SetTextureId(e, kDefaultRenderId, unit, texture_target, texture_id);
}

void RenderSystemNext::SetTextureId(Entity e, HashValue component_id, int unit,
                                    uint32_t texture_target,
                                    uint32_t texture_id) {
  const EntityIdPair entity_id_pair(e, component_id);
  auto* render_component = components_.Get(entity_id_pair);
  if (!render_component) {
    return;
  }
  auto texture = factory_->CreateTexture(texture_target, texture_id);
  SetTexture(e, component_id, unit, texture);
}

TexturePtr RenderSystemNext::GetTexture(Entity entity, int unit) const {
  const auto* render_component = components_.Get(entity);
  if (!render_component) {
    return TexturePtr();
  }
  return render_component->material.GetTexture(unit);
}

void RenderSystemNext::SetPano(Entity entity, const std::string& filename,
                               float heading_offset_deg) {
  LOG(FATAL) << "Deprecated.";
}

void RenderSystemNext::SetText(Entity e, const std::string& text) {
  LOG(FATAL) << "Deprecated.";
}

void RenderSystemNext::SetFont(Entity entity, const FontPtr& font) {
  LOG(FATAL) << "Deprecated.";
}

void RenderSystemNext::SetTextSize(Entity entity, int size) {
  LOG(FATAL) << "Deprecated.";
}

const std::vector<LinkTag>* RenderSystemNext::GetLinkTags(Entity e) const {
  LOG(FATAL) << "Deprecated.";
  return nullptr;
}

const std::vector<mathfu::vec3>* RenderSystemNext::GetCaretPositions(
    Entity e) const {
  LOG(FATAL) << "Deprecated.";
  return nullptr;
}

bool RenderSystemNext::GetQuad(Entity e, Quad* quad) const {
  const auto* render_component = components_.Get(e);
  if (!render_component) {
    return false;
  }
  *quad = render_component->quad;
  return true;
}

void RenderSystemNext::SetQuad(Entity e, const Quad& quad) {
  SetQuad(e, kDefaultRenderId, quad);
}

void RenderSystemNext::SetQuad(Entity e, HashValue component_id,
                               const Quad& quad) {
  const EntityIdPair entity_id_pair(e, component_id);
  auto* render_component = components_.Get(entity_id_pair);
  if (!render_component) {
    LOG(WARNING) << "Missing entity for SetQuad: " << entity_id_pair.entity
                 << ", with id: " << entity_id_pair.id;
    return;
  }
  render_component->quad = quad;

  auto iter = deformations_.find(entity_id_pair);
  if (iter != deformations_.end()) {
    DeferredMesh defer;
    defer.entity_id_pair = entity_id_pair;
    defer.type = DeferredMesh::kQuad;
    defer.quad = quad;
    deferred_meshes_.push(std::move(defer));
  } else {
    SetQuadImpl(e, component_id, quad);
  }
}

void RenderSystemNext::SetMesh(Entity e, const TriangleMesh<VertexPT>& mesh) {
  SetMesh(e, kDefaultRenderId, mesh);
}

void RenderSystemNext::SetMesh(Entity e, HashValue component_id,
                               const TriangleMesh<VertexPT>& mesh) {
  SetMesh(e, component_id, factory_->CreateMesh(mesh));
}

void RenderSystemNext::SetAndDeformMesh(Entity entity,
                                        const TriangleMesh<VertexPT>& mesh) {
  SetAndDeformMesh(entity, kDefaultRenderId, mesh);
}

void RenderSystemNext::SetAndDeformMesh(Entity entity, HashValue component_id,
                                        const TriangleMesh<VertexPT>& mesh) {
  const EntityIdPair entity_id_pair(entity, component_id);
  auto iter = deformations_.find(entity_id_pair);
  if (iter != deformations_.end()) {
    DeferredMesh defer;
    defer.entity_id_pair = entity_id_pair;
    defer.type = DeferredMesh::kMesh;
    defer.mesh.GetVertices() = mesh.GetVertices();
    defer.mesh.GetIndices() = mesh.GetIndices();
    deferred_meshes_.emplace(std::move(defer));
  } else {
    SetMesh(entity, component_id, mesh);
  }
}

void RenderSystemNext::SetMesh(Entity e, const MeshData& mesh) {
  SetMesh(e, factory_->CreateMesh(mesh));
}

void RenderSystemNext::SetMesh(Entity e, const std::string& file) {
  SetMesh(e, factory_->LoadMesh(file));
}

RenderSystemNext::SortOrderOffset RenderSystemNext::GetSortOrderOffset(
    Entity entity) const {
  return sort_order_manager_.GetOffset(entity);
}

void RenderSystemNext::SetSortOrderOffset(Entity e,
                                          SortOrderOffset sort_order_offset) {
  SetSortOrderOffset(e, kDefaultRenderId, sort_order_offset);
}

void RenderSystemNext::SetSortOrderOffset(Entity e, HashValue component_id,
                                          SortOrderOffset sort_order_offset) {
  const EntityIdPair entity_id_pair(e, component_id);
  sort_order_manager_.SetOffset(entity_id_pair, sort_order_offset);
  sort_order_manager_.UpdateSortOrder(entity_id_pair,
                                      [this](EntityIdPair entity_id_pair) {
                                        return components_.Get(entity_id_pair);
                                      });
}

bool RenderSystemNext::IsTextureSet(Entity e, int unit) const {
  auto* render_component = components_.Get(e);
  if (!render_component) {
    return false;
  }
  return render_component->material.GetTexture(unit) != nullptr;
}

bool RenderSystemNext::IsTextureLoaded(Entity e, int unit) const {
  const auto* render_component = components_.Get(e);
  if (!render_component) {
    return false;
  }

  if (!render_component->material.GetTexture(unit)) {
    return false;
  }
  return render_component->material.GetTexture(unit)->IsLoaded();
}

bool RenderSystemNext::IsTextureLoaded(const TexturePtr& texture) const {
  return texture->IsLoaded();
}

bool RenderSystemNext::IsReadyToRender(Entity entity) const {
  const auto* render_component = components_.Get(entity);
  if (!render_component) {
    // No component, no textures, no fonts, no problem.
    return true;
  }
  return IsReadyToRenderImpl(*render_component);
}

bool RenderSystemNext::IsReadyToRenderImpl(
    const RenderComponent& component) const {
  if (component.mesh && !component.mesh->IsLoaded()) {
    return false;
  }
  const auto& textures = component.material.GetTextures();
  for (const auto& pair : textures) {
    const TexturePtr& texture = pair.second;
    if (!texture->IsLoaded() || !factory_->IsTextureValid(texture)) {
      return false;
    }
  }
  return true;
}

bool RenderSystemNext::IsHidden(Entity e) const {
  const auto* render_component = components_.Get(e);
  const bool render_component_hidden =
      render_component && render_component->hidden;

  // If there are no models associated with this entity, then it is hidden.
  // Otherwise, it is hidden if the RenderComponent is hidden.
  return (render_component_hidden || !render_component);
}

ShaderPtr RenderSystemNext::GetShader(Entity entity,
                                      HashValue component_id) const {
  const EntityIdPair entity_id_pair(entity, component_id);
  const RenderComponent* component = components_.Get(entity_id_pair);
  return component ? component->material.GetShader() : ShaderPtr();
}

ShaderPtr RenderSystemNext::GetShader(Entity entity) const {
  const RenderComponent* component = components_.Get(entity);
  return component ? component->material.GetShader() : ShaderPtr();
}

void RenderSystemNext::SetShader(Entity e, const ShaderPtr& shader) {
  SetShader(e, kDefaultRenderId, shader);
}

void RenderSystemNext::SetShader(Entity e, HashValue component_id,
                                 const ShaderPtr& shader) {
  const EntityIdPair entity_id_pair(e, component_id);
  auto* render_component = components_.Get(entity_id_pair);
  if (!render_component) {
    return;
  }
  render_component->material.SetShader(shader);

  // Update the uniforms' locations in the new shader.
  UpdateUniformLocations(render_component);
}

void RenderSystemNext::OnMeshLoaded(RenderComponent* render_component) {
  const Entity entity = render_component->GetEntity();

  auto* transform_system = registry_->Get<TransformSystem>();
  transform_system->SetAabb(entity, render_component->mesh->GetAabb());

  MeshPtr& mesh = render_component->mesh;
  const size_t num_bones = mesh->GetNumBones();
  const size_t num_shader_bones = mesh->GetNumShaderBones();
  if (num_bones > 0 && num_shader_bones > 0) {
    auto* rig_system = registry_->Get<RigSystem>();
    if (rig_system) {
      mesh->UpdateRig(rig_system, entity);
    } else {
      // By default, clear the bone transforms to identity.
      constexpr int dimension = 4;
      const int count =
          kNumVec4sInAffineTransform * static_cast<int>(num_shader_bones);
      const mathfu::AffineTransform identity =
          mathfu::mat4::ToAffineTransform(mathfu::mat4::Identity());
      shader_transforms_.clear();
      shader_transforms_.resize(num_shader_bones, identity);

      // Check if we have existing bone transforms, which can be ungathered.
      const Uniform* uniform =
          render_component->material.GetUniformByName(kBoneTransformsUniform);
      if (uniform && uniform->GetDescription().type == Uniform::Type::kFloats) {
        const float* data = uniform->GetData<float>();
        const mathfu::AffineTransform* transforms =
            reinterpret_cast<const mathfu::AffineTransform*>(data);
        if (render_component->need_to_gather_bone_transforms) {
          const int ungathered_count = kNumVec4sInAffineTransform *
                                       render_component->mesh->GetNumBones();
          if (uniform->GetDescription().count == ungathered_count) {
            render_component->mesh->GatherShaderTransforms(
                transforms, shader_transforms_.data());
            render_component->need_to_gather_bone_transforms = false;
          } else {
            LOG(WARNING) << "Ungathered bone transforms had wrong count";
          }
        } else if (uniform->GetDescription().count == count) {
          for (size_t i = 0; i < num_shader_bones; ++i) {
            shader_transforms_[i] = transforms[i];
          }
        }
      }

      const float* data = &(shader_transforms_[0][0]);
      SetUniform(entity, render_component->id, kBoneTransformsUniform, data,
                 dimension, count);
    }
  }

  if (IsReadyToRenderImpl(*render_component)) {
    auto* dispatcher_system = registry_->Get<DispatcherSystem>();
    if (dispatcher_system) {
      dispatcher_system->Send(entity, ReadyToRenderEvent(entity));
    }
  }
}

void RenderSystemNext::SetMesh(Entity e, MeshPtr mesh) {
  SetMesh(e, kDefaultRenderId, mesh);
}

void RenderSystemNext::SetMesh(Entity e, HashValue component_id,
                               const MeshPtr& mesh) {
  const EntityIdPair entity_id_pair(e, component_id);
  auto* render_component = components_.Get(entity_id_pair);
  if (!render_component) {
    LOG(WARNING) << "Missing RenderComponent, "
                 << "skipping mesh update for entity: " << e
                 << ", with id: " << component_id;
    return;
  }

  render_component->mesh = std::move(mesh);

  if (render_component->mesh) {
    MeshPtr callback_mesh = mesh;
    auto callback = [this, e, component_id, callback_mesh]() {
      RenderComponent* render_component =
          components_.Get(EntityIdPair(e, component_id));
      if (render_component && render_component->mesh == callback_mesh) {
        OnMeshLoaded(render_component);
      }
    };
    render_component->mesh->AddOrInvokeOnLoadCallback(callback);
  }
  SendEvent(registry_, e, MeshChangedEvent(e, component_id));
}

MeshPtr RenderSystemNext::GetMesh(Entity e, HashValue component_id) {
  const EntityIdPair entity_id_pair(e, component_id);
  auto* render_component = components_.Get(entity_id_pair);
  if (!render_component) {
    LOG(WARNING) << "Missing RenderComponent for entity: " << e
                 << ", with id: " << component_id;
    return nullptr;
  }

  return render_component->mesh;
}

template <typename Vertex>
void RenderSystemNext::DeformMesh(Entity entity, HashValue component_id,
                                  TriangleMesh<Vertex>* mesh) {
  const EntityIdPair entity_id_pair(entity, component_id);
  auto iter = deformations_.find(entity_id_pair);
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
MeshPtr RenderSystemNext::CreateQuad(Entity e, HashValue component_id,
                                     const Quad& quad) {
  if (quad.size.x == 0 || quad.size.y == 0) {
    return nullptr;
  }

  TriangleMesh<Vertex> mesh;
  mesh.SetQuad(quad.size.x, quad.size.y, quad.verts.x, quad.verts.y,
               quad.corner_radius, quad.corner_verts, quad.corner_mask);

  DeformMesh<Vertex>(e, component_id, &mesh);

  if (quad.id != 0) {
    return factory_->CreateMesh(quad.id, mesh);
  } else {
    return factory_->CreateMesh(mesh);
  }
}

void RenderSystemNext::SetStencilMode(Entity e, StencilMode mode, int value) {
  SetStencilMode(e, kDefaultRenderId, mode, value);
}

void RenderSystemNext::SetStencilMode(Entity e, HashValue component_id,
                                      StencilMode mode, int value) {
  const EntityIdPair entity_id_pair(e, component_id);
  auto* render_component = components_.Get(entity_id_pair);
  if (!render_component || render_component->stencil_mode == mode) {
    return;
  }

  render_component->stencil_mode = mode;
  render_component->stencil_value = value;
}

void RenderSystemNext::SetDeformationFunction(Entity e,
                                              const Deformation& deform) {
  if (deform) {
    deformations_.emplace(e, deform);
  } else {
    deformations_.erase(e);
  }
}

void RenderSystemNext::Hide(Entity e) {
  auto* render_component = components_.Get(e);
  if (render_component && !render_component->hidden) {
    render_component->hidden = true;
    SendEvent(registry_, e, HiddenEvent(e));
  }
}

void RenderSystemNext::Show(Entity e) {
  auto* render_component = components_.Get(e);
  if (render_component && render_component->hidden) {
    render_component->hidden = false;
    SendEvent(registry_, e, UnhiddenEvent(e));
  }
}

HashValue RenderSystemNext::GetRenderPass(Entity entity) const {
  const RenderComponent* component = components_.Get(entity);
  return component ? component->pass : 0;
}

void RenderSystemNext::SetRenderPass(Entity e, HashValue pass) {
  RenderComponent* render_component = components_.Get(e);
  if (render_component) {
    render_component->pass = pass;
  }
}

RenderSystem::SortMode RenderSystemNext::GetSortMode(HashValue pass) const {
  const auto it = pass_definitions_.find(pass);
  return (it != pass_definitions_.cend()) ? it->second.sort_mode
                                          : SortMode::kNone;
}

void RenderSystemNext::SetSortMode(HashValue pass, SortMode mode) {
  pass_definitions_[pass].sort_mode = mode;
}

RenderSystem::CullMode RenderSystemNext::GetCullMode(HashValue pass) {
  const auto it = pass_definitions_.find(pass);
  return (it != pass_definitions_.cend()) ? it->second.cull_mode
                                          : CullMode::kNone;
}

void RenderSystemNext::SetCullMode(HashValue pass, CullMode mode) {
  pass_definitions_[pass].cull_mode = mode;
}

void RenderSystemNext::SetRenderState(HashValue pass,
                                      const fplbase::RenderState& state) {
  pass_definitions_[pass].render_state = state;
}

const fplbase::RenderState* RenderSystemNext::GetRenderState(
    HashValue pass) const {
  const auto& it = pass_definitions_.find(pass);
  if (it == pass_definitions_.end()) {
    return nullptr;
  }

  return &it->second.render_state;
}

void RenderSystemNext::SetDepthTest(const bool enabled) {
  if (enabled) {
#if !ION_PRODUCTION
    // GL_DEPTH_BITS was deprecated in desktop GL 3.3, so make sure this get
    // succeeds before checking depth_bits.
    GLint depth_bits = 0;
    glGetIntegerv(GL_DEPTH_BITS, &depth_bits);
    if (glGetError() == 0 && depth_bits == 0) {
      // This has been known to cause problems on iOS 10.
      LOG_ONCE(WARNING) << "Enabling depth test without a depth buffer; this "
                           "has known issues on some platforms.";
    }
#endif  // !ION_PRODUCTION

    renderer_.SetDepthFunction(fplbase::kDepthFunctionLess);
    return;
  }

  renderer_.SetDepthFunction(fplbase::kDepthFunctionDisabled);
}

void RenderSystemNext::SetDepthWrite(const bool enabled) {
  renderer_.SetDepthWrite(enabled);
}

void RenderSystemNext::SetViewport(const View& view) {
  LULLABY_CPU_TRACE_CALL();
  renderer_.SetViewport(fplbase::Viewport(view.viewport, view.dimensions));
}

void RenderSystemNext::SetClipFromModelMatrix(const mathfu::mat4& mvp) {
  renderer_.set_model_view_projection(mvp);
}

void RenderSystemNext::BindStencilMode(StencilMode mode, int ref) {
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

void RenderSystemNext::BindVertexArray(uint32_t ref) {
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

void RenderSystemNext::ClearSamplers() {
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

void RenderSystemNext::ResetState() {
  const fplbase::RenderState& render_state = renderer_.GetRenderState();

  // Clear render state.
  SetBlendMode(fplbase::kBlendModeOff);
  renderer_.SetCulling(fplbase::kCullingModeBack);
  SetDepthTest(true);
  GL_CALL(glEnable(GL_DEPTH_TEST));
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

void RenderSystemNext::SetBlendMode(fplbase::BlendMode blend_mode) {
  renderer_.SetBlendMode(blend_mode);
  blend_mode_ = blend_mode;
}

mathfu::vec4 RenderSystemNext::GetClearColor() const { return clear_color_; }

void RenderSystemNext::SetClearColor(float r, float g, float b, float a) {
  clear_color_ = mathfu::vec4(r, g, b, a);
}

void RenderSystemNext::SubmitRenderData() {
  RenderData* data = render_data_buffer_.LockWriteBuffer();
  if (!data) {
    return;
  }
  data->clear();

  const auto* transform_system = registry_->Get<TransformSystem>();
  components_.ForEach([&](RenderComponent& render_component) {
    if (render_component.hidden) {
      return;
    }
    if (render_component.pass == 0) {
      return;
    }
    const Entity entity = render_component.GetEntity();
    if (entity == kNullEntity) {
      return;
    }
    const mathfu::mat4* world_from_entity_matrix =
        transform_system->GetWorldFromEntityMatrix(entity);
    if (world_from_entity_matrix == nullptr ||
        !transform_system->IsEnabled(entity)) {
      return;
    }

    RenderObject render_obj;
    render_obj.mesh = render_component.mesh;
    render_obj.material = render_component.material;
    render_obj.sort_order = render_component.sort_order;
    render_obj.stencil_mode = render_component.stencil_mode;
    render_obj.stencil_value = render_component.stencil_value;
    render_obj.world_from_entity_matrix = *world_from_entity_matrix;

    RenderPassAndObjects& entry = (*data)[render_component.pass];
    entry.render_objects.emplace_back(std::move(render_obj));
  });

  for (auto& iter : *data) {
    const RenderPassDefinition& pass = pass_definitions_[iter.first];
    iter.second.pass_definition = pass;
    // Sort only objects with "static" sort order, such as explicit sort order
    // or absolute z-position.
    if (IsSortModeViewIndependent(pass.sort_mode)) {
      SortObjects(&iter.second.render_objects, pass.sort_mode);
    }
  }

  render_data_buffer_.UnlockWriteBuffer();
}

void RenderSystemNext::BeginRendering() {
  LULLABY_CPU_TRACE_CALL();
  GL_CALL(glClearColor(clear_color_.x, clear_color_.y, clear_color_.z,
                       clear_color_.w));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                  GL_STENCIL_BUFFER_BIT));

  // Retrieve the (current) default frame buffer.
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &default_frame_buffer_);

  active_render_data_ = render_data_buffer_.LockReadBuffer();
}

void RenderSystemNext::EndRendering() {
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
  render_data_buffer_.UnlockReadBuffer();
  active_render_data_ = nullptr;
  default_frame_buffer_ = 0;
}

void RenderSystemNext::SetViewUniforms(const View& view) {
  renderer_.set_camera_pos(view.world_from_eye_matrix.TranslationVector3D());

  rendering_right_eye_ = view.eye == 1;
}

void RenderSystemNext::RenderAt(const RenderObject* component,
                                const mathfu::mat4& world_from_entity_matrix,
                                const View& view) {
  LULLABY_CPU_TRACE_CALL();
  const ShaderPtr& shader = component->material.GetShader();
  if (!shader || !component->mesh) {
    return;
  }

  const mathfu::mat4 clip_from_entity_matrix =
      view.clip_from_world_matrix * world_from_entity_matrix;
  renderer_.set_model_view_projection(clip_from_entity_matrix);
  renderer_.set_model(world_from_entity_matrix);

  BindShader(shader);
  SetShaderUniforms(component->material.GetUniforms());

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
    mathfu::vec3_packed packed[3];
    normal_matrix.Pack(packed);
    GL_CALL(glUniformMatrix3fv(uniform_gl, 1, false, packed[0].data));
  }
  const Shader::UniformHnd camera_dir_handle =
      shader->FindUniform("camera_dir");
  if (fplbase::ValidUniformHandle(camera_dir_handle)) {
    const int uniform_gl = fplbase::GlUniformHandle(camera_dir_handle);
    mathfu::vec3_packed camera_dir;
    CalculateCameraDirection(view.world_from_eye_matrix).Pack(&camera_dir);
    GL_CALL(glUniform3fv(uniform_gl, 1, camera_dir.data));
  }

  const auto& textures = component->material.GetTextures();
  for (const auto& texture : textures) {
    texture.second->Bind(texture.first);
  }

  // Bit of magic to determine if the scalar is negative and if so flip the cull
  // face. This possibly be revised (b/38235916).
  CorrectFrontFaceFromMatrix(world_from_entity_matrix);

  BindStencilMode(component->stencil_mode, component->stencil_value);
  DrawMeshFromComponent(component);
}

void RenderSystemNext::RenderAtMultiview(
    const RenderObject* component, const mathfu::mat4& world_from_entity_matrix,
    const View* views) {
  LULLABY_CPU_TRACE_CALL();
  const ShaderPtr& shader = component->material.GetShader();
  if (!shader || !component->mesh) {
    return;
  }

  const mathfu::mat4 clip_from_entity_matrix[] = {
      views[0].clip_from_world_matrix * world_from_entity_matrix,
      views[1].clip_from_world_matrix * world_from_entity_matrix,
  };

  renderer_.set_model(world_from_entity_matrix);
  BindShader(shader);
  SetShaderUniforms(component->material.GetUniforms());

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
    GL_CALL(glUniformMatrix3fv(uniform_gl, 1, false, packed[0].data));
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
    GL_CALL(glUniform3fv(uniform_gl, 2, camera_dir[0].data));
  }

  const auto& textures = component->material.GetTextures();
  for (const auto& texture : textures) {
    texture.second->Bind(texture.first);
  }

  // Bit of magic to determine if the scalar is negative and if so flip the cull
  // face. This possibly be revised (b/38235916).
  CorrectFrontFaceFromMatrix(world_from_entity_matrix);

  BindStencilMode(component->stencil_mode, component->stencil_value);
  DrawMeshFromComponent(component);
}

void RenderSystemNext::SetShaderUniforms(const UniformVector& uniforms) {
  for (const auto& uniform : uniforms) {
    BindUniform(uniform);
  }
}

void RenderSystemNext::DrawMeshFromComponent(const RenderObject* component) {
  if (component->mesh) {
    component->mesh->Render(&renderer_);
    detail::Profiler* profiler = registry_->Get<detail::Profiler>();
    if (profiler) {
      profiler->RecordDraw(component->material.GetShader(),
                           component->mesh->GetNumVertices(),
                           component->mesh->GetNumTriangles());
    }
  }
}

void RenderSystemNext::RenderPanos(const View* views, size_t num_views) {
  LOG(FATAL) << "Deprecated.";
}

void RenderSystemNext::Render(const View* views, size_t num_views) {
  renderer_.BeginRendering();

  ResetState();
  known_state_ = true;

  // assume a max of 2 views, one for each eye.
  RenderView pano_views[2];
  CHECK_LE(num_views, 2);
  GenerateEyeCenteredViews({views, num_views}, pano_views);
  Render(pano_views, num_views, ConstHash("Pano"));
  Render(views, num_views, ConstHash("Opaque"));
  Render(views, num_views, ConstHash("Main"));
  Render(views, num_views, ConstHash("OverDraw"));
  Render(views, num_views, ConstHash("OverDrawGlow"));

  known_state_ = false;

  renderer_.EndRendering();
}

void RenderSystemNext::Render(const View* views, size_t num_views,
                              HashValue pass) {
  LULLABY_CPU_TRACE_CALL();

  if (!active_render_data_) {
    LOG(DFATAL) << "Render between BeginRendering() and EndRendering()!";
    return;
  }
  auto iter = active_render_data_->find(pass);
  if (iter == active_render_data_->end()) {
    // No data associated with this pass.
    return;
  }
  if (iter->second.render_objects.empty()) {
    // No objects to render with this pass.
    return;
  }

  if (!known_state_) {
    renderer_.BeginRendering();
    if (pass != 0) {
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

  const RenderPassDefinition& pass_definition = iter->second.pass_definition;

  // Set the render target, if needed.
  if (pass_definition.render_target) {
    pass_definition.render_target->SetAsRenderTarget();
  }

  // Prepare the pass.
  renderer_.SetRenderState(pass_definition.render_state);
  cached_render_state_ = pass_definition.render_state;

  // Draw the elements.
  if (pass == ConstHash("Debug")) {
    RenderDebugStats(views, num_views);
  } else {
    RenderObjectList& objects = iter->second.render_objects;
    if (!IsSortModeViewIndependent(pass_definition.sort_mode)) {
      SortObjectsUsingView(&objects, pass_definition.sort_mode, views,
                           num_views);
    }
    RenderObjects(objects, views, num_views);
  }

  // Set the render target back to default, if needed.
  if (pass_definition.render_target) {
    GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, default_frame_buffer_));
  }

  if (reset_state) {
    static const fplbase::RenderState kDefaultRenderState;
    renderer_.SetRenderState(kDefaultRenderState);
  }

  if (!known_state_) {
    renderer_.EndRendering();
  }
}

void RenderSystemNext::RenderObjects(const std::vector<RenderObject>& objects,
                                     const View* views, size_t num_views) {
  if (objects.empty()) {
    return;
  }

  if (multiview_enabled_) {
    SetViewport(views[0]);
    SetViewUniforms(views[0]);

    for (const RenderObject& obj : objects) {
      RenderAtMultiview(&obj, obj.world_from_entity_matrix, views);
    }
  } else {
    for (size_t i = 0; i < num_views; ++i) {
      const View& view = views[i];

      SetViewport(view);
      SetViewUniforms(view);

      for (const RenderObject& obj : objects) {
        RenderAt(&obj, obj.world_from_entity_matrix, views[i]);
      }
    }
  }

  // Reset states that are set at the entity level in RenderAt.
  BindStencilMode(StencilMode::kDisabled, 0);
  GL_CALL(glFrontFace(GL_CCW));
}

void RenderSystemNext::BindShader(const ShaderPtr& shader) {
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

void RenderSystemNext::BindTexture(int unit, const TexturePtr& texture) {
  texture->Bind(unit);
}

void RenderSystemNext::BindUniform(const char* name, const float* data,
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

void RenderSystemNext::DrawPrimitives(PrimitiveType type,
                                      const VertexFormat& format,
                                      const void* vertex_data,
                                      size_t num_vertices) {
  const fplbase::Mesh::Primitive fpl_type = Mesh::GetFplPrimitiveType(type);
  fplbase::Attribute attributes[Mesh::kMaxFplAttributeArraySize];
  Mesh::GetFplAttributes(format, attributes);

  fplbase::RenderArray(fpl_type, static_cast<int>(num_vertices), attributes,
                       static_cast<int>(format.GetVertexSize()), vertex_data);
}

void RenderSystemNext::DrawIndexedPrimitives(
    PrimitiveType type, const VertexFormat& format, const void* vertex_data,
    size_t num_vertices, const uint16_t* indices, size_t num_indices) {
  const fplbase::Mesh::Primitive fpl_type = Mesh::GetFplPrimitiveType(type);
  fplbase::Attribute attributes[Mesh::kMaxFplAttributeArraySize];
  Mesh::GetFplAttributes(format, attributes);

  fplbase::RenderArray(fpl_type, static_cast<int>(num_indices), attributes,
                       static_cast<int>(format.GetVertexSize()), vertex_data,
                       indices);
}

void RenderSystemNext::UpdateDynamicMesh(
    Entity entity, PrimitiveType primitive_type,
    const VertexFormat& vertex_format, const size_t max_vertices,
    const size_t max_indices,
    const std::function<void(MeshData*)>& update_mesh) {
  RenderComponent* component = components_.Get(entity);
  if (!component) {
    return;
  }

  if (max_vertices > 0) {
    DataContainer vertex_data = DataContainer::CreateHeapDataContainer(
        max_vertices * vertex_format.GetVertexSize());
    DataContainer index_data = DataContainer::CreateHeapDataContainer(
        max_indices * sizeof(MeshData::Index));
    MeshData data(primitive_type, vertex_format, std::move(vertex_data),
                  std::move(index_data));
    update_mesh(&data);
    component->mesh = factory_->CreateMesh(data);
  } else {
    component->mesh.reset();
  }
  SendEvent(registry_, entity, MeshChangedEvent(entity, kDefaultRenderId));
}

void RenderSystemNext::RenderDebugStats(const View* views, size_t num_views) {
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

  // TODO(b/29914331) Separate, tested matrix decomposition util functions.
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
    const int padding = 20;
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

void RenderSystemNext::OnParentChanged(const ParentChangedEvent& event) {
  sort_order_manager_.UpdateSortOrder(
      event.target,
      [this](EntityIdPair entity) { return components_.Get(entity); });
}

const fplbase::RenderState& RenderSystemNext::GetRenderState() const {
  return renderer_.GetRenderState();
}

void RenderSystemNext::UpdateCachedRenderState(
    const fplbase::RenderState& render_state) {
  renderer_.UpdateCachedRenderState(render_state);
}

bool RenderSystemNext::IsSortModeViewIndependent(SortMode mode) {
  switch (mode) {
    case SortMode::kAverageSpaceOriginBackToFront:
    case SortMode::kAverageSpaceOriginFrontToBack:
      return false;
    default:
      return true;
  }
}

void RenderSystemNext::SortObjects(RenderObjectList* objects, SortMode mode) {
  switch (mode) {
    case SortMode::kNone:
      // Do nothing.
      break;

    case SortMode::kSortOrderDecreasing:
      std::sort(objects->begin(), objects->end(),
                [](const RenderObject& a, const RenderObject& b) {
                  return a.sort_order > b.sort_order;
                });
      break;

    case SortMode::kSortOrderIncreasing:
      std::sort(objects->begin(), objects->end(),
                [](const RenderObject& a, const RenderObject& b) {
                  return a.sort_order < b.sort_order;
                });
      break;

    case SortMode::kWorldSpaceZBackToFront:
      std::sort(objects->begin(), objects->end(),
                [](const RenderObject& a, const RenderObject& b) {
                  return a.world_from_entity_matrix.TranslationVector3D().z <
                         b.world_from_entity_matrix.TranslationVector3D().z;
                });
      break;

    case SortMode::kWorldSpaceZFrontToBack:
      std::sort(objects->begin(), objects->end(),
                [](const RenderObject& a, const RenderObject& b) {
                  return a.world_from_entity_matrix.TranslationVector3D().z >
                         b.world_from_entity_matrix.TranslationVector3D().z;
                });
      break;

    default:
      LOG(DFATAL) << "SortObjects called with unsupported sort mode!";
      break;
  }
}

void RenderSystemNext::SortObjectsUsingView(RenderObjectList* objects,
                                            SortMode mode, const View* views,
                                            size_t num_views) {
  // Get the average camera position.
  if (num_views <= 0) {
    LOG(DFATAL) << "Must have at least 1 view.";
    return;
  }
  mathfu::vec3 avg_pos = mathfu::kZeros3f;
  mathfu::vec3 avg_z(0, 0, 0);
  for (size_t i = 0; i < num_views; ++i) {
    avg_pos += views[i].world_from_eye_matrix.TranslationVector3D();
    avg_z += GetMatrixColumn3D(views[i].world_from_eye_matrix, 2);
  }
  avg_pos /= static_cast<float>(num_views);
  avg_z.Normalize();

  // Give relative values to the elements.
  for (RenderObject& obj : *objects) {
    const mathfu::vec3 world_pos =
        obj.world_from_entity_matrix.TranslationVector3D();
    obj.z_sort_order = mathfu::vec3::DotProduct(world_pos - avg_pos, avg_z);
  }

  switch (mode) {
    case SortMode::kAverageSpaceOriginBackToFront:
      std::sort(objects->begin(), objects->end(),
                [&](const RenderObject& a, const RenderObject& b) {
                  return a.z_sort_order < b.z_sort_order;
                });
      break;

    case SortMode::kAverageSpaceOriginFrontToBack:
      std::sort(objects->begin(), objects->end(),
                [&](const RenderObject& a, const RenderObject& b) {
                  return a.z_sort_order > b.z_sort_order;
                });
      break;

    default:
      LOG(DFATAL) << "SortObjectsUsingView called with unsupported sort mode!";
      break;
  }
}

void RenderSystemNext::InitDefaultRenderPasses() {
  fplbase::RenderState render_state;

  // RenderPass_Pano. Premultiplied alpha blend state, everything else default.
  render_state.blend_state.enabled = true;
  render_state.blend_state.src_alpha = fplbase::BlendState::kOne;
  render_state.blend_state.src_color = fplbase::BlendState::kOne;
  render_state.blend_state.dst_alpha = fplbase::BlendState::kOneMinusSrcAlpha;
  render_state.blend_state.dst_color = fplbase::BlendState::kOneMinusSrcAlpha;
  SetRenderState(ConstHash("Pano"), render_state);

  // RenderPass_Opaque. Depth test and write on. BlendMode disabled, face cull
  // mode back.
  render_state.blend_state.enabled = false;
  render_state.depth_state.test_enabled = true;
  render_state.depth_state.write_enabled = true;
  render_state.depth_state.function = fplbase::kRenderLessEqual;
  render_state.cull_state.enabled = true;
  render_state.cull_state.face = fplbase::CullState::kBack;
  SetRenderState(ConstHash("Opaque"), render_state);

  // RenderPass_Main. Depth test on, write off. Premultiplied alpha blend state,
  // face cull mode back.
  render_state.blend_state.enabled = true;
  render_state.blend_state.src_alpha = fplbase::BlendState::kOne;
  render_state.blend_state.src_color = fplbase::BlendState::kOne;
  render_state.blend_state.dst_alpha = fplbase::BlendState::kOneMinusSrcAlpha;
  render_state.blend_state.dst_color = fplbase::BlendState::kOneMinusSrcAlpha;
  render_state.depth_state.test_enabled = true;
  render_state.depth_state.function = fplbase::kRenderLessEqual;
  render_state.depth_state.write_enabled = false;
  render_state.cull_state.enabled = true;
  render_state.cull_state.face = fplbase::CullState::kBack;
  SetRenderState(ConstHash("Main"), render_state);

  // RenderPass_OverDraw. Depth test and write false, premultiplied alpha, back
  // face culling.
  render_state.depth_state.test_enabled = false;
  render_state.depth_state.write_enabled = false;
  render_state.blend_state.enabled = true;
  render_state.blend_state.src_alpha = fplbase::BlendState::kOne;
  render_state.blend_state.src_color = fplbase::BlendState::kOne;
  render_state.blend_state.dst_alpha = fplbase::BlendState::kOneMinusSrcAlpha;
  render_state.blend_state.dst_color = fplbase::BlendState::kOneMinusSrcAlpha;
  render_state.cull_state.enabled = true;
  render_state.cull_state.face = fplbase::CullState::kBack;
  SetRenderState(ConstHash("OverDraw"), render_state);

  // RenderPass_OverDrawGlow. Depth test and write off, additive blend mode, no
  // face culling.
  render_state.depth_state.test_enabled = false;
  render_state.depth_state.write_enabled = false;
  render_state.blend_state.enabled = true;
  render_state.blend_state.src_alpha = fplbase::BlendState::kOne;
  render_state.blend_state.src_color = fplbase::BlendState::kOne;
  render_state.blend_state.dst_alpha = fplbase::BlendState::kOne;
  render_state.blend_state.dst_color = fplbase::BlendState::kOne;
  render_state.cull_state.enabled = false;
  SetRenderState(ConstHash("OverDrawGlow"), render_state);

  SetSortMode(ConstHash("Opaque"), SortMode::kAverageSpaceOriginFrontToBack);
  SetSortMode(ConstHash("Main"), SortMode::kSortOrderIncreasing);
}

void RenderSystemNext::SetRenderPass(const RenderPassDefT& data) {
  const HashValue pass = Hash(data.name.c_str());
  RenderPassDefinition& def = pass_definitions_[pass];
  switch (data.sort_mode) {
    case SortMode_None:
      break;
    case SortMode_SortOrderDecreasing:
      def.sort_mode = SortMode::kSortOrderDecreasing;
      break;
    case SortMode_SortOrderIncreasing:
      def.sort_mode = SortMode::kSortOrderIncreasing;
      break;
    case SortMode_WorldSpaceZBackToFront:
      def.sort_mode = SortMode::kWorldSpaceZBackToFront;
      break;
    case SortMode_WorldSpaceZFrontToBack:
      def.sort_mode = SortMode::kWorldSpaceZFrontToBack;
      break;
    case SortMode_AverageSpaceOriginBackToFront:
      def.sort_mode = SortMode::kAverageSpaceOriginBackToFront;
      break;
    case SortMode_AverageSpaceOriginFrontToBack:
      def.sort_mode = SortMode::kAverageSpaceOriginFrontToBack;
      break;
    case SortMode_Optimized:
      break;
  }
  Apply(&def.render_state, data.render_state);
}

void RenderSystemNext::CreateRenderTarget(
    HashValue render_target_name, const mathfu::vec2i& dimensions,
    TextureFormat texture_format, DepthStencilFormat depth_stencil_format) {
  DCHECK_EQ(render_targets_.count(render_target_name), 0);

  // Create the render target.
  auto render_target = MakeUnique<fplbase::RenderTarget>();
  render_target->Initialize(dimensions,
                            RenderTargetTextureFormatToFpl(texture_format),
                            DepthStencilFormatToFpl(depth_stencil_format));

  // Create a bindable texture.
  TexturePtr texture = factory_->CreateTexture(
      GL_TEXTURE_2D, fplbase::GlTextureHandle(render_target->GetTextureId()));
  factory_->CacheTexture(render_target_name, texture);

  // Store the render target.
  render_targets_[render_target_name] = std::move(render_target);
}

void RenderSystemNext::SetRenderTarget(HashValue pass,
                                       HashValue render_target_name) {
  auto iter = render_targets_.find(render_target_name);
  if (iter == render_targets_.end()) {
    LOG(FATAL) << "SetRenderTarget called with non-existent render target: "
               << render_target_name;
    return;
  }
  pass_definitions_[pass].render_target = iter->second.get();
}

void RenderSystemNext::CorrectFrontFaceFromMatrix(const mathfu::mat4& matrix) {
  if (CalculateDeterminant3x3(matrix) >= 0.0f) {
    // If the scalar is positive, match the default settings.
    renderer_.SetFrontFace(cached_render_state_.cull_state.front);
  } else {
    // Otherwise, reverse the order.
    renderer_.SetFrontFace(static_cast<fplbase::CullState::FrontFace>(
        fplbase::CullState::kFrontFaceCount -
        cached_render_state_.cull_state.front - 1));
  }
}

void RenderSystemNext::BindUniform(const Uniform& uniform) {
  const Uniform::Description& desc = uniform.GetDescription();
  int binding = 0;
  if (desc.binding >= 0) {
    binding = desc.binding;
  } else {
    const Shader::UniformHnd handle = shader_->FindUniform(desc.name.c_str());
    if (fplbase::ValidUniformHandle(handle)) {
      binding = fplbase::GlUniformHandle(handle);
    } else {
      return;
    }
  }

  const size_t bytes_per_component = desc.num_bytes / desc.count;
  switch (desc.type) {
    case Uniform::Type::kFloats: {
      switch (bytes_per_component) {
        case 4:
          GL_CALL(glUniform1fv(binding, desc.count, uniform.GetData<float>()));
          break;
        case 8:
          GL_CALL(glUniform2fv(binding, desc.count, uniform.GetData<float>()));
          break;
        case 12:
          GL_CALL(glUniform3fv(binding, desc.count, uniform.GetData<float>()));
          break;
        case 16:
          GL_CALL(glUniform4fv(binding, desc.count, uniform.GetData<float>()));
          break;
        default:
          LOG(DFATAL) << "Uniform named \"" << desc.name
                      << "\" is set to unsupported type floats with size "
                      << desc.num_bytes;
      }
    } break;

    case Uniform::Type::kMatrix: {
      switch (bytes_per_component) {
        case 64:
          GL_CALL(glUniformMatrix4fv(binding, desc.count, false,
                                     uniform.GetData<float>()));
          break;
        case 36:
          GL_CALL(glUniformMatrix3fv(binding, desc.count, false,
                                     uniform.GetData<float>()));
          break;
        case 16:
          GL_CALL(glUniformMatrix2fv(binding, desc.count, false,
                                     uniform.GetData<float>()));
          break;
        default:
          LOG(DFATAL) << "Uniform named \"" << desc.name
                      << "\" is set to unsupported type matrix with size "
                      << desc.num_bytes;
      }
    } break;

    default:
      // Error or missing implementation.
      LOG(DFATAL) << "Trying to bind uniform of unknown type.";
  }
}

}  // namespace lull
