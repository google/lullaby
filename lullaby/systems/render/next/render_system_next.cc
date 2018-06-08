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
#include <algorithm>
#include <cctype>
#include <memory>

#include "fplbase/gpu_debug.h"
#include "lullaby/events/render_events.h"
#include "lullaby/modules/config/config.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/render/mesh_util.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/render/detail/profiler.h"
#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/systems/render/next/gl_helpers.h"
#include "lullaby/systems/render/next/render_state.h"
#include "lullaby/systems/render/render_helpers.h"
#include "lullaby/systems/render/render_stats.h"
#include "lullaby/systems/render/simple_font.h"
#include "lullaby/util/filename.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/math.h"
#include "lullaby/util/trace.h"
#include "lullaby/generated/render_def_generated.h"

namespace lull {
namespace {
constexpr int kRenderPoolPageSize = 32;
const HashValue kRenderDefHash = ConstHash("RenderDef");
constexpr int kNumVec4sInAffineTransform = 3;
constexpr const char* kColorUniform = "color";
constexpr const char* kTextureBoundsUniform = "uv_bounds";
constexpr const char* kClampBoundsUniform = "clamp_bounds";
constexpr const char* kBoneTransformsUniform = "bone_transforms";
constexpr const char* kDefaultMaterialShaderDirectory = "shaders/";

// Shader attribute hashes.
constexpr HashValue kAttributeHashPosition = ConstHash("ATTR_POSITION");
constexpr HashValue kAttributeHashUV = ConstHash("ATTR_UV");
constexpr HashValue kAttributeHashColor = ConstHash("ATTR_COLOR");
constexpr HashValue kAttributeHashNormal = ConstHash("ATTR_NORMAL");
constexpr HashValue kAttributeHashOrientation = ConstHash("ATTR_ORIENTATION");
constexpr HashValue kAttributeHashTangent = ConstHash("ATTR_TANGENT");
constexpr HashValue kAttributeHashBoneIndices = ConstHash("ATTR_BONE_INDICES");
constexpr HashValue kAttributeHashBoneWeights = ConstHash("ATTR_BONE_WEIGHTS");

// Shader feature hashes.
constexpr HashValue kFeatureHashTransform = ConstHash("Transform");
constexpr HashValue kFeatureHashVertexColor = ConstHash("VertexColor");
constexpr HashValue kFeatureHashTexture = ConstHash("Texture");
constexpr HashValue kFeatureHashLight = ConstHash("Light");
constexpr HashValue kFeatureHashSkin = ConstHash("Skin");
constexpr HashValue kFeatureHashUniformColor = ConstHash("UniformColor");

bool IsSupportedUniformDimension(int dimension) {
  return (dimension == 1 || dimension == 2 || dimension == 3 ||
          dimension == 4 || dimension == 9 || dimension == 16);
}

// TODO(b/78301332) Add more types when supported.
bool IsSupportedUniformType(ShaderDataType type) {
  return type >= ShaderDataType_MIN && type <= ShaderDataType_Float4x4;
}

void SetDebugUniform(Shader* shader, const char* name, const float values[4]) {
  shader->SetUniform(shader->FindUniform(name), values, 4);
}

HashValue RenderPassObjectEnumToHashValue(RenderPass pass) {
  if (pass == RenderPass_Pano) {
    return ConstHash("Pano");
  } else if (pass == RenderPass_Opaque) {
    return ConstHash("Opaque");
  } else if (pass == RenderPass_Main) {
    return ConstHash("Main");
  } else if (pass == RenderPass_OverDraw) {
    return ConstHash("OverDraw");
  } else if (pass == RenderPass_Debug) {
    return ConstHash("Debug");
  } else if (pass == RenderPass_Invisible) {
    return ConstHash("Invisible");
  } else if (pass == RenderPass_OverDrawGlow) {
    return ConstHash("OverDrawGlow");
  }

  return static_cast<HashValue>(pass);
}

fplbase::CullState::FrontFace CullStateFrontFaceFromFrontFace(
    RenderFrontFace face) {
  switch (face) {
    case RenderFrontFace::kClockwise:
      return fplbase::CullState::FrontFace::kClockWise;
    case RenderFrontFace::kCounterClockwise:
      return fplbase::CullState::FrontFace::kCounterClockWise;
  }
  return fplbase::CullState::FrontFace::kCounterClockWise;
}

bool IsAlphaEnabled(const Material& material) {
  auto* blend_state = material.GetBlendState();
  if (blend_state && blend_state->enabled) {
    return true;
  }
  return false;
}

bool CheckMeshForBoneIndices(const MeshPtr& mesh) {
  if (!mesh) {
    return false;
  }

  return mesh->GetVertexFormat().GetAttributeWithUsage(
             VertexAttributeUsage_BoneIndices) != nullptr;
}

// Return environment flags constructed from a vertex format.
std::set<HashValue> CreateEnvironmentFlagsFromVertexFormat(
    const VertexFormat& vertex_format) {
  std::set<HashValue> environment_flags;
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_Position) !=
      nullptr) {
    environment_flags.insert(kAttributeHashPosition);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_TexCoord) !=
      nullptr) {
    environment_flags.insert(kAttributeHashUV);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_Color) !=
      nullptr) {
    environment_flags.insert(kAttributeHashColor);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_Normal) !=
      nullptr) {
    environment_flags.insert(kAttributeHashNormal);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_Orientation) !=
      nullptr) {
    environment_flags.insert(kAttributeHashOrientation);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_Tangent) !=
      nullptr) {
    environment_flags.insert(kAttributeHashTangent);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_BoneIndices) !=
      nullptr) {
    environment_flags.insert(kAttributeHashBoneIndices);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_BoneWeights) !=
      nullptr) {
    environment_flags.insert(kAttributeHashBoneWeights);
  }

  return environment_flags;
}

// Return feature flags constructed from a vertex format.
std::set<HashValue> CreateFeatureFlagsFromVertexFormat(
    const VertexFormat& vertex_format) {
  std::set<HashValue> feature_flags;
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_Position) !=
      nullptr) {
    feature_flags.insert(kFeatureHashTransform);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_TexCoord) !=
      nullptr) {
    feature_flags.insert(kFeatureHashTexture);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_Color) !=
      nullptr) {
    feature_flags.insert(kFeatureHashVertexColor);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_Normal) !=
      nullptr) {
    feature_flags.insert(kFeatureHashLight);
  }
  if (vertex_format.GetAttributeWithUsage(VertexAttributeUsage_BoneIndices) !=
      nullptr) {
    feature_flags.insert(kFeatureHashSkin);
  }

  return feature_flags;
}

ShaderDataType FloatDimensionsToUniformType(int dimensions) {
  switch (dimensions) {
    case 1:
      return ShaderDataType_Float1;
    case 2:
      return ShaderDataType_Float2;
    case 3:
      return ShaderDataType_Float3;
    case 4:
      return ShaderDataType_Float4;
    case 9:
      return ShaderDataType_Float3x3;
    case 16:
      return ShaderDataType_Float4x4;
    default:
      LOG(DFATAL) << "Failed to convert dimensions to uniform type.";
      return ShaderDataType_Float4;
  }
}
}  // namespace

RenderSystemNext::RenderPassObject::RenderPassObject()
    : components(kRenderPoolPageSize) {
  // Initialize the default render state based on an older implementation to
  // maintain backwards compatibility.
  render_state.cull_state.emplace();
  render_state.cull_state->enabled = true;
  render_state.cull_state->face = CullFace_Back;
  render_state.cull_state->front = FrontFace_CounterClockwise;

  render_state.depth_state.emplace();
  render_state.depth_state->test_enabled = true;
  render_state.depth_state->write_enabled = true;
  render_state.depth_state->function = RenderFunction_Less;

  render_state.stencil_state.emplace();
  render_state.stencil_state->front_function.mask = ~0u;
  render_state.stencil_state->back_function.mask = ~0u;
}

RenderSystemNext::RenderSystemNext(Registry* registry,
                                   const RenderSystem::InitParams& init_params)
    : System(registry),
      sort_order_manager_(registry_),
      shading_model_path_(kDefaultMaterialShaderDirectory),
      clip_from_model_matrix_func_(CalculateClipFromModelMatrix) {
  texture_flag_hashes_.resize(
      static_cast<size_t>(MaterialTextureUsage_MAX + 1));
  for (int i = MaterialTextureUsage_MIN; i <= MaterialTextureUsage_MAX; ++i) {
    const MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(i);
    texture_flag_hashes_[i] =
        Hash("Texture_" + std::string(EnumNameMaterialTextureUsage(usage)));
  }

  if (init_params.enable_stereo_multiview) {
    renderer_.EnableMultiview();
  }

  mesh_factory_ = new MeshFactoryImpl(registry);
  registry->Register(std::unique_ptr<MeshFactory>(mesh_factory_));

  texture_factory_ = new TextureFactoryImpl(registry);
  registry->Register(std::unique_ptr<TextureFactory>(texture_factory_));

  shader_factory_ = registry->Create<ShaderFactory>(registry);
  texture_atlas_factory_ = registry->Create<TextureAtlasFactory>(registry);

  auto* dispatcher = registry->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const ParentChangedEvent& event) {
    UpdateSortOrder(event.target);
  });
  dispatcher->Connect(this, [this](const ChildIndexChangedEvent& event) {
    UpdateSortOrder(event.target);
  });

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.Render.Show", &RenderSystem::Show);
    binder->RegisterMethod("lull.Render.Hide", &RenderSystem::Hide);
    binder->RegisterFunction("lull.Render.GetTextureId", [this](Entity entity) {
      TexturePtr texture = GetTexture(entity, 0);
      return texture ? static_cast<int>(texture->GetResourceId().Get()) : 0;
    });
    binder->RegisterMethod("lull.Render.SetColor", &RenderSystem::SetColor);
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

void RenderSystemNext::Initialize() {
  InitDefaultRenderPassObjects();
  initialized_ = true;
}

void RenderSystemNext::SetStereoMultiviewEnabled(bool enabled) {
  if (enabled) {
    renderer_.EnableMultiview();
  } else {
    renderer_.DisableMultiview();
  }
}

const TexturePtr& RenderSystemNext::GetWhiteTexture() const {
  return texture_factory_->GetWhiteTexture();
}

const TexturePtr& RenderSystemNext::GetInvalidTexture() const {
  return texture_factory_->GetInvalidTexture();
}

TexturePtr RenderSystemNext::LoadTexture(const std::string& filename,
                                         bool create_mips) {
  TextureParams params;
  params.generate_mipmaps = create_mips;
  return texture_factory_->LoadTexture(filename, params);
}

TexturePtr RenderSystemNext::GetTexture(HashValue texture_hash) const {
  return texture_factory_->GetTexture(texture_hash);
}

void RenderSystemNext::LoadTextureAtlas(const std::string& filename) {
  const bool create_mips = false;
  texture_atlas_factory_->LoadTextureAtlas(filename, create_mips);
}

MeshPtr RenderSystemNext::LoadMesh(const std::string& filename) {
  return mesh_factory_->LoadMesh(filename);
}

TexturePtr RenderSystemNext::CreateTexture(const ImageData& image,
                                           bool create_mips) {
  TextureParams params;
  params.generate_mipmaps = create_mips;
  return texture_factory_->CreateTexture(&image, params);
}

ShaderPtr RenderSystemNext::LoadShader(const std::string& filename) {
  ShaderCreateParams params;
  params.shading_model = RemoveExtensionFromFilename(filename);
  return LoadShader(params);
}

ShaderPtr RenderSystemNext::LoadShader(const ShaderCreateParams& params) {
  return shader_factory_->LoadShader(params);
}

void RenderSystemNext::Create(Entity e, HashValue pass) {
  RenderPassObject* render_pass = GetRenderPassObject(pass);
  if (!render_pass) {
    LOG(DFATAL) << "Render pass " << pass << " does not exist!";
    return;
  }

  RenderComponent* component = render_pass->components.Emplace(e);
  if (component) {
    SetSortOrderOffset(e, pass, 0);
  }
}

void RenderSystemNext::Create(Entity e, HashValue type, const Def* def) {
  if (type != kRenderDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting RenderDef!";
    return;
  }

  const RenderDef& data = *ConvertDef<RenderDef>(def);
  if (data.font()) {
    LOG(DFATAL) << "Deprecated.";
  }

  HashValue pass_hash = RenderPassObjectEnumToHashValue(data.pass());
  RenderPassObject* render_pass = GetRenderPassObject(pass_hash);
  if (!render_pass) {
    LOG(DFATAL) << "Render pass " << pass_hash << " does not exist!";
  }

  RenderComponent* component = render_pass->components.Emplace(e);
  if (!component) {
    LOG(DFATAL) << "RenderComponent for Entity " << e << " inside pass "
                << pass_hash << " already exists.";
    return;
  }

  component->hidden = data.hidden();

  if (data.shader()) {
    SetShader(e, pass_hash, LoadShader(data.shader()->str()));
  }

  if (data.textures()) {
    for (unsigned int i = 0; i < data.textures()->size(); ++i) {
      TextureParams params;
      params.generate_mipmaps = data.create_mips();
      TexturePtr texture = texture_factory_->LoadTexture(
          data.textures()->Get(i)->c_str(), params);
      SetTexture(e, pass_hash, i, texture);
    }
  } else if (data.texture() && data.texture()->size() > 0) {
    TextureParams params;
    params.generate_mipmaps = data.create_mips();
    TexturePtr texture =
        texture_factory_->LoadTexture(data.texture()->c_str(), params);
    SetTexture(e, pass_hash, 0, texture);
  } else if (data.external_texture()) {
    SetTextureExternal(e, pass_hash, 0);
  }

  if (data.color()) {
    mathfu::vec4 color;
    MathfuVec4FromFbColor(data.color(), &color);
    SetUniform(e, pass_hash, kColorUniform, &color[0], 4, 1);
    component->default_color = color;
  } else if (data.color_hex()) {
    mathfu::vec4 color;
    MathfuVec4FromFbColorHex(data.color_hex()->c_str(), &color);
    SetUniform(e, pass_hash, kColorUniform, &color[0], 4, 1);
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
      SetUniform(e, pass_hash, uniform->name()->c_str(),
                 uniform->float_value()->data(), uniform->dimension(),
                 uniform->count());
    }
  }
  SetSortOrderOffset(e, pass_hash, data.sort_order_offset());
}

void RenderSystemNext::SetTextureExternal(Entity e, HashValue pass, int unit) {
  const TexturePtr texture = texture_factory_->CreateExternalTexture();
  SetTexture(e, pass, unit, texture);
}

void RenderSystemNext::PostCreateInit(Entity e, HashValue type,
                                      const Def* def) {
  if (type == kRenderDefHash) {
    auto& data = *ConvertDef<RenderDef>(def);

    HashValue pass_hash = RenderPassObjectEnumToHashValue(data.pass());
    if (data.mesh()) {
      SetMesh(e, pass_hash, mesh_factory_->LoadMesh(data.mesh()->c_str()));
    }

    if (data.text()) {
      LOG(DFATAL) << "Deprecated.";
    } else if (data.quad()) {
      const QuadDef& quad_def = *data.quad();

      RenderQuad quad;
      quad.size = mathfu::vec2(quad_def.size_x(), quad_def.size_y());
      quad.verts = mathfu::vec2i(quad_def.verts_x(), quad_def.verts_y());
      quad.has_uv = quad_def.has_uv();
      quad.corner_radius = quad_def.corner_radius();
      quad.corner_verts = quad_def.corner_verts();
      if (data.shape_id()) {
        quad.id = Hash(data.shape_id()->c_str());
      }

      SetQuadImpl(e, pass_hash, quad);
    }
  }
}

void RenderSystemNext::Destroy(Entity e) {
  deformations_.erase(e);
  for (auto& pass : render_passes_) {
    const detail::EntityIdPair entity_id_pair(e, pass.first);
    pass.second.components.Destroy(e);
    sort_order_manager_.Destroy(entity_id_pair);
  }
}

void RenderSystemNext::Destroy(Entity e, HashValue pass) {
  const detail::EntityIdPair entity_id_pair(e, pass);

  RenderPassObject* render_pass = FindRenderPassObject(pass);
  if (!render_pass) {
    LOG(DFATAL) << "Render pass " << pass << " does not exist!";
    return;
  }
  render_pass->components.Destroy(e);
  deformations_.erase(e);
  sort_order_manager_.Destroy(entity_id_pair);
}

void RenderSystemNext::CreateDeferredMeshes() {
  while (!deferred_meshes_.empty()) {
    DeferredMesh& defer = deferred_meshes_.front();
    DeformMesh(defer.entity, defer.pass, &defer.mesh);
    SetMesh(defer.entity, defer.pass, defer.mesh, defer.mesh_id);
    deferred_meshes_.pop();
  }
}

void RenderSystemNext::ProcessTasks() {
  LULLABY_CPU_TRACE_CALL();

  CreateDeferredMeshes();
}

void RenderSystemNext::WaitForAssetsToLoad() {
  CreateDeferredMeshes();
  while (registry_->Get<AssetLoader>()->Finalize()) {
  }
}

const mathfu::vec4& RenderSystemNext::GetDefaultColor(Entity entity) const {
  const RenderComponent* component = FindRenderComponentForEntity(entity);
  if (component) {
    return component->default_color;
  }
  return mathfu::kOnes4f;
}

void RenderSystemNext::SetDefaultColor(Entity entity,
                                       const mathfu::vec4& color) {
  ForEachComponentOfEntity(
      entity, [color](RenderComponent* component, HashValue pass_hash) {
        component->default_color = color;
      });
}

bool RenderSystemNext::GetColor(Entity entity, mathfu::vec4* color) const {
  return GetUniform(entity, kColorUniform, 4, &(*color)[0]);
}

void RenderSystemNext::SetColor(Entity entity, const mathfu::vec4& color) {
  SetUniform(entity, kColorUniform, &color[0], 4, 1);
}

void RenderSystemNext::SetUniform(Entity entity, Optional<HashValue> pass,
                                  Optional<int> submesh_index, string_view name,
                                  ShaderDataType type, Span<uint8_t> data,
                                  int count) {
  const int material_index = submesh_index ? *submesh_index : -1;
  if (pass) {
    RenderComponent* component = GetComponent(entity, *pass);
    if (component) {
      SetUniformImpl(component, material_index, name, type, data, count);
    }
  } else {
    ForEachComponentOfEntity(
        entity, [=](RenderComponent* component, HashValue pass) {
          SetUniformImpl(component, material_index, name, type, data, count);
        });
  }
}

void RenderSystemNext::SetUniform(Entity e, const char* name, const float* data,
                                  int dimension, int count) {
  SetUniform(e, NullOpt, NullOpt, name, FloatDimensionsToUniformType(dimension),
             {reinterpret_cast<const uint8_t*>(data),
              static_cast<size_t>(dimension * count * sizeof(float))},
             count);
}

void RenderSystemNext::SetUniform(Entity e, HashValue pass, const char* name,
                                  const float* data, int dimension, int count) {
  SetUniform(e, pass, NullOpt, name, FloatDimensionsToUniformType(dimension),
             {reinterpret_cast<const uint8_t*>(data),
              static_cast<size_t>(dimension * count * sizeof(float))},
             count);
}

bool RenderSystemNext::GetUniform(Entity entity, Optional<HashValue> pass,
                                  Optional<int> submesh_index, string_view name,
                                  size_t length, uint8_t* data_out) const {
  const int material_index = submesh_index ? *submesh_index : -1;
  const RenderComponent* component =
      pass ? GetComponent(entity, *pass) : FindRenderComponentForEntity(entity);
  return GetUniformImpl(component, material_index, name, length, data_out);
}

bool RenderSystemNext::GetUniform(Entity e, const char* name, size_t length,
                                  float* data_out) const {
  return GetUniform(e, NullOpt, NullOpt, name, length * sizeof(float),
                    reinterpret_cast<uint8_t*>(data_out));
}

bool RenderSystemNext::GetUniform(Entity e, HashValue pass, const char* name,
                                  size_t length, float* data_out) const {
  return GetUniform(e, pass, NullOpt, name, length * sizeof(float),
                    reinterpret_cast<uint8_t*>(data_out));
}

void RenderSystemNext::SetUniformImpl(RenderComponent* component,
                                      int material_index, string_view name,
                                      ShaderDataType type, Span<uint8_t> data,
                                      int count) {
  if (component == nullptr) {
    return;
  }

  if (!IsSupportedUniformType(type)) {
    LOG(DFATAL) << "ShaderDataType not supported: " << type;
    return;
  }

  // Do not allow partial data in this function.
  if (data.size() != UniformData::ShaderDataTypeToBytesSize(type) * count) {
    LOG(DFATAL) << "Partial uniform data is not allowed through "
                   "RenderSystem::SetUniform.";
    return;
  }

  if (material_index < 0) {
    for (const std::shared_ptr<Material>& material : component->materials) {
      SetUniformImpl(material.get(), name, type, data, count);
    }
    SetUniformImpl(&component->default_material, name, type, data, count);
  } else if (material_index < static_cast<int>(component->materials.size())) {
    const std::shared_ptr<Material>& material =
        component->materials[material_index];
    SetUniformImpl(material.get(), name, type, data, count);
  } else {
    LOG(DFATAL);
    return;
  }

  if (component->uniform_changed_callback) {
    component->uniform_changed_callback(material_index, name, type, data,
                                        count);
  }
}

void RenderSystemNext::SetUniformImpl(Material* material,
                                      string_view name, ShaderDataType type,
                                      Span<uint8_t> data, int count) {
  if (material == nullptr) {
    return;
  }
  CHECK(count == data.size() / UniformData::ShaderDataTypeToBytesSize(type));
  material->SetUniform(Hash(name), type, data);
}

bool RenderSystemNext::GetUniformImpl(const RenderComponent* component,
                                      int material_index, string_view name,
                                      size_t length, uint8_t* data_out) const {
  if (component == nullptr) {
    return false;
  }

  if (material_index < 0) {
    for (const std::shared_ptr<Material>& material : component->materials) {
      if (GetUniformImpl(material.get(), name, length, data_out)) {
        return true;
      }
    }
    return GetUniformImpl(&component->default_material, name, length, data_out);
  } else if (material_index < static_cast<int>(component->materials.size())) {
    const std::shared_ptr<Material>& material =
        component->materials[material_index];
    return GetUniformImpl(material.get(), name, length, data_out);
  }
  return false;
}

bool RenderSystemNext::GetUniformImpl(const Material* material,
                                      string_view name, size_t length,
                                      uint8_t* data_out) const {
  if (material == nullptr) {
    return false;
  }
  const UniformData* uniform = material->GetUniformData(Hash(name));
  if (uniform == nullptr) {
    return false;
  }

  if (length > uniform->Size()) {
    return false;
  }

  memcpy(data_out, uniform->GetData<uint8_t>(), length);
  return true;
}

void RenderSystemNext::CopyUniforms(Entity entity, Entity source) {
  LOG(WARNING) << "CopyUniforms is going to be deprecated! Do not use this.";

  RenderComponent* component = FindRenderComponentForEntity(entity);
  const RenderComponent* source_component =
      FindRenderComponentForEntity(source);
  if (!component || component->materials.empty() || !source_component ||
      source_component->materials.empty()) {
    return;
  }

  // TODO(b/68673920): Copied data may not be correct.
  const bool copy_per_material =
      (source_component->materials.size() == component->materials.size());
  for (size_t index = 0; index < component->materials.size(); ++index) {
    Material& destination_material = *component->materials[index];
    const Material& source_material = copy_per_material
                                          ? *source_component->materials[index]
                                          : *source_component->materials[0];
    destination_material.CopyUniforms(source_material);
  }
}

void RenderSystemNext::SetUniformChangedCallback(
    Entity entity, HashValue pass,
    RenderSystem::UniformChangedCallback callback) {
  RenderComponent* component = GetComponent(entity, pass);
  if (!component) {
    return;
  }
  component->uniform_changed_callback = std::move(callback);
}

void RenderSystemNext::OnTextureLoaded(const RenderComponent& component,
                                       HashValue pass, int unit,
                                       const TexturePtr& texture) {
  const Entity entity = component.GetEntity();
  const mathfu::vec4 clamp_bounds = texture->CalculateClampBounds();
  SetUniform(entity, kClampBoundsUniform, &clamp_bounds[0], 4, 1);

  if (texture && texture->GetResourceId()) {
    // TODO(b/38130323) Add CheckTextureSizeWarning that does not depend on HMD.

    SendEvent(registry_, entity, TextureReadyEvent(entity, unit));
    if (IsReadyToRenderImpl(component)) {
      SendEvent(registry_, entity, ReadyToRenderEvent(entity, pass));
    }
  }
}

void RenderSystemNext::SetTexture(Entity e, int unit,
                                  const TexturePtr& texture) {
  ForEachComponentOfEntity(e, [&](RenderComponent* component, HashValue pass) {
    SetTexture(e, pass, unit, texture);
  });
}

void RenderSystemNext::SetTexture(Entity e, HashValue pass, int unit,
                                  const TexturePtr& texture) {
  auto* render_component = GetComponent(e, pass);
  if (!render_component) {
    return;
  }

  const MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(unit);

  render_component->default_material.SetTexture(usage, texture);
  for (auto& material : render_component->materials) {
    const bool new_texture_usage =
        material->GetTexture(usage) != nullptr && texture != nullptr;
    material->SetTexture(usage, texture);

    if (new_texture_usage) {
      // Update the shader to add the new texture usage environment flag.
      ShaderPtr shader = material->GetShader();
      if (shader && !shader->GetDescription().shading_model.empty()) {
        const ShaderCreateParams shader_params = CreateShaderParams(
            shader->GetDescription().shading_model, render_component, material);
        shader = shader_factory_->LoadShader(shader_params);
        material->SetShader(shader);
      }
    }
  }

  // Add subtexture coordinates so the vertex shaders will pick them up.  These
  // are known when the texture is created; no need to wait for load.
  if (texture) {
    SetUniform(e, pass, kTextureBoundsUniform, &texture->UvBounds()[0], 4, 1);
  }
}

TexturePtr RenderSystemNext::CreateProcessedTexture(
    const TexturePtr& texture, bool create_mips,
    const RenderSystem::TextureProcessor& processor,
    const mathfu::vec2i& output_dimensions) {
  LULLABY_CPU_TRACE_CALL();

  if (!texture) {
    LOG(DFATAL) << "null texture passed to CreateProcessedTexture()";
    return TexturePtr();
  }
  // Make and bind a framebuffer for rendering to texture.
  GLuint framebuffer_id, current_framebuffer_id;
  GL_CALL(glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
                        reinterpret_cast<GLint*>(&current_framebuffer_id)));
  GL_CALL(glGenFramebuffers(1, &framebuffer_id));
  GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id));

  // Make an empty FPL texture for the render target, sized to the specified
  // dimensions.
  mathfu::vec2i size = output_dimensions;
  bool target_is_subtexture = false;
  float texture_u_bound = 1.f;
  float texture_v_bound = 1.f;

  // If the input texture is a subtexture, scale the size appropriately.
  if (texture->IsSubtexture()) {
    auto scale = mathfu::vec2(texture->UvBounds().z, texture->UvBounds().w);
    auto size_f = scale * mathfu::vec2(static_cast<float>(size.x),
                                       static_cast<float>(size.y));
    size =
        mathfu::vec2i(static_cast<int>(size_f.x), static_cast<int>(size_f.y));
  }

  // If we don't support NPOT and the texture is NPOT, use UV bounds to work
  // around this.
  if (!GlSupportsTextureNpot() &&
      (!IsPowerOf2(size.x) || !IsPowerOf2(size.y))) {
    target_is_subtexture = true;
    uint32_t next_power_of_two_x =
        mathfu::RoundUpToPowerOf2(static_cast<uint32_t>(size.x));
    uint32_t next_power_of_two_y =
        mathfu::RoundUpToPowerOf2(static_cast<uint32_t>(size.y));
    texture_u_bound =
        static_cast<float>(size.x) / static_cast<float>(next_power_of_two_x);
    texture_v_bound =
        static_cast<float>(size.y) / static_cast<float>(next_power_of_two_y);
    size = mathfu::vec2i(next_power_of_two_x, next_power_of_two_y);
  }

  TextureParams params;
  params.format = ImageData::kRgba8888;
  params.generate_mipmaps = create_mips;
  TexturePtr out_ptr = texture_factory_->CreateTexture(size, params);
  if (target_is_subtexture) {
    const mathfu::vec4 bounds(0.f, 0.f, texture_u_bound, texture_v_bound);
    out_ptr = texture_factory_->CreateSubtexture(out_ptr, bounds);
  }

  // Bind the output texture to the framebuffer as the color attachment.
  GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, out_ptr->GetResourceId().Get(),
                                 0));

#if defined(DEBUG)
  // Check for completeness of the framebuffer.
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    LOG(DFATAL) << "Failed to create offscreen framebuffer: " << std::hex
                << glCheckFramebufferStatus(GL_FRAMEBUFFER);
  }
#endif

  // Subtexturing on output texture can pick up sample noise around the edges
  // of the rendered area. Clear to transparent black.
  if (target_is_subtexture) {
    GL_CALL(glClearColor(0.f, 0.f, 0.f, 0.f));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
  }

  processor(out_ptr);

  // Setup viewport, input texture, shader, and draw quad.
  SetViewport(mathfu::recti(mathfu::kZeros2i, size));

  // We render a quad starting in the lower left corner and extending up and
  // right for as long as the output subtexture is needed.
  const float width = texture_u_bound * 2.f;
  const float height = texture_v_bound * 2.f;
  const mathfu::vec4& uv_rect = texture->UvBounds();
  DrawQuad(mathfu::vec3(-1.f, -1.f, 0.f),
           mathfu::vec3(width - 1.f, height - 1.f, 0),
           mathfu::vec2(uv_rect.x, uv_rect.y),
           mathfu::vec2(uv_rect.x + uv_rect.z, uv_rect.y + uv_rect.w));

  // Delete framebuffer, we retain the texture.
  GL_CALL(glDeleteFramebuffers(1, &framebuffer_id));

  // Regenerate Mipmaps on the processed texture.
  if (create_mips) {
    out_ptr->Bind(0);
    GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
  }

  GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, current_framebuffer_id));

  return out_ptr;
}

TexturePtr RenderSystemNext::CreateProcessedTexture(
    const TexturePtr& texture, bool create_mips,
    const RenderSystem::TextureProcessor& processor) {
  auto size = texture->GetDimensions();
  return CreateProcessedTexture(texture, create_mips, processor, size);
}

void RenderSystemNext::SetTextureId(Entity e, int unit, uint32_t texture_target,
                                    uint32_t texture_id) {
  ForEachComponentOfEntity(
      e, [this, e, unit, texture_target, texture_id](
             RenderComponent* render_component, HashValue pass) {
        SetTextureId(e, pass, unit, texture_target, texture_id);
      });
}

void RenderSystemNext::SetTextureId(Entity e, HashValue pass, int unit,
                                    uint32_t texture_target,
                                    uint32_t texture_id) {
  auto* render_component = GetComponent(e, pass);
  if (!render_component) {
    return;
  }
  auto texture = texture_factory_->CreateTexture(texture_target, texture_id);
  SetTexture(e, pass, unit, texture);
}

TexturePtr RenderSystemNext::GetTexture(Entity entity, int unit) const {
  const auto* render_component = FindRenderComponentForEntity(entity);
  if (!render_component || render_component->materials.empty()) {
    return TexturePtr();
  }
  const MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(unit);
  return render_component->materials[0]->GetTexture(usage);
}

bool RenderSystemNext::GetQuad(Entity e, RenderQuad* quad) const {
  const auto* render_component = FindRenderComponentForEntity(e);
  if (!render_component) {
    return false;
  }
  *quad = render_component->quad;
  return true;
}

void RenderSystemNext::SetQuad(Entity e, const RenderQuad& quad) {
  ForEachComponentOfEntity(
      e, [this, e, quad](RenderComponent* render_component, HashValue pass) {
        SetQuadImpl(e, pass, quad);
      });
}

void RenderSystemNext::SetQuadImpl(Entity e, HashValue pass,
                                   const RenderQuad& quad) {
  const detail::EntityIdPair entity_id_pair(e, pass);
  auto* render_component = GetComponent(e, pass);
  if (!render_component) {
    LOG(WARNING) << "Missing entity for SetQuad: " << entity_id_pair.entity
                 << ", with id: " << entity_id_pair.id;
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
    defer.entity = e;
    defer.pass = pass;
    defer.mesh_id = quad.id;
    defer.mesh = std::move(mesh);
    deferred_meshes_.push(std::move(defer));
  } else {
    SetMesh(e, pass, mesh, quad.id);
  }
}

void RenderSystemNext::SetMesh(Entity e, const MeshData& mesh) {
  ForEachComponentOfEntity(e, [&](RenderComponent* render_component,
                                  HashValue pass) { SetMesh(e, pass, mesh); });
}

void RenderSystemNext::SetMesh(Entity entity, HashValue pass,
                               const MeshData& mesh, HashValue mesh_id) {
  MeshPtr gpu_mesh;
  if (mesh_id != 0) {
    gpu_mesh = mesh_factory_->CreateMesh(mesh_id, &mesh);
  } else {
    gpu_mesh = mesh_factory_->CreateMesh(&mesh);
  }
  SetMesh(entity, pass, gpu_mesh);
}

void RenderSystemNext::SetMesh(Entity entity, HashValue pass,
                               const MeshData& mesh) {
  const HashValue mesh_id = 0;
  SetMesh(entity, pass, mesh, mesh_id);
}

void RenderSystemNext::SetAndDeformMesh(Entity entity, const MeshData& mesh) {
  ForEachComponentOfEntity(
      entity, [&](RenderComponent* render_component, HashValue pass) {
        SetAndDeformMesh(entity, pass, mesh);
      });
}

void RenderSystemNext::SetAndDeformMesh(Entity entity, HashValue pass,
                                        const MeshData& mesh) {
  if (mesh.GetVertexBytes() == nullptr) {
    LOG(WARNING) << "Can't deform mesh without read access.";
    SetMesh(entity, pass, mesh);
    return;
  }

  auto iter = deformations_.find(entity);
  if (iter != deformations_.end()) {
    DeferredMesh defer;
    defer.entity = entity;
    defer.pass = pass;
    defer.mesh = mesh.CreateHeapCopy();
    deferred_meshes_.emplace(std::move(defer));
  } else {
    SetMesh(entity, pass, mesh);
  }
}

void RenderSystemNext::SetMesh(Entity e, const std::string& file) {
  SetMesh(e, mesh_factory_->LoadMesh(file));
}

RenderSortOrder RenderSystemNext::GetSortOrder(Entity e) const {
  const auto* component = FindRenderComponentForEntity(e);
  if (!component) {
    return 0;
  }
  return component->sort_order;
}

RenderSortOrderOffset RenderSystemNext::GetSortOrderOffset(Entity e) const {
  return sort_order_manager_.GetOffset(e);
}

void RenderSystemNext::SetSortOrderOffset(
    Entity e, RenderSortOrderOffset sort_order_offset) {
  ForEachComponentOfEntity(
      e, [this, e, sort_order_offset](RenderComponent* render_component,
                                      HashValue pass) {
        SetSortOrderOffset(e, pass, sort_order_offset);
      });
}

void RenderSystemNext::SetSortOrderOffset(
    Entity e, HashValue pass, RenderSortOrderOffset sort_order_offset) {
  const detail::EntityIdPair entity_id_pair(e, pass);
  sort_order_manager_.SetOffset(entity_id_pair, sort_order_offset);
  sort_order_manager_.UpdateSortOrder(
      entity_id_pair, [this](detail::EntityIdPair entity_id_pair) {
        return GetComponent(entity_id_pair.entity, entity_id_pair.id);
      });
}

bool RenderSystemNext::IsTextureSet(Entity e, int unit) const {
  auto* render_component = FindRenderComponentForEntity(e);
  if (!render_component || render_component->materials.empty()) {
    return false;
  }
  const MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(unit);
  return render_component->materials[0]->GetTexture(usage) != nullptr;
}

bool RenderSystemNext::IsTextureLoaded(Entity e, int unit) const {
  const auto* render_component = FindRenderComponentForEntity(e);
  if (!render_component || render_component->materials.empty()) {
    return false;
  }

  const MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(unit);
  if (!render_component->materials[0]->GetTexture(usage)) {
    return false;
  }
  return render_component->materials[0]->GetTexture(usage)->IsLoaded();
}

bool RenderSystemNext::IsTextureLoaded(const TexturePtr& texture) const {
  return texture->IsLoaded();
}

bool RenderSystemNext::IsReadyToRender(Entity entity) const {
  const auto* render_component = FindRenderComponentForEntity(entity);
  if (!render_component) {
    // No component, no textures, no fonts, no problem.
    return true;
  }
  return IsReadyToRenderImpl(*render_component);
}

bool RenderSystemNext::IsReadyToRender(Entity entity, HashValue pass) const {
  const auto* render_component = GetComponent(entity, pass);
  return !render_component || IsReadyToRenderImpl(*render_component);
}

bool RenderSystemNext::IsReadyToRenderImpl(
    const RenderComponent& component) const {
  if (component.mesh && !component.mesh->IsLoaded()) {
    return false;
  }

  for (const auto& material : component.materials) {
    if (material->GetShader() == nullptr) {
      return false;
    }
    for (int i = MaterialTextureUsage_MIN; i <= MaterialTextureUsage_MAX; ++i) {
      const MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(i);
      TexturePtr texture = material->GetTexture(usage);
      if (texture && !texture->IsLoaded()) {
        return false;
      }
    }
  }
  return true;
}

bool RenderSystemNext::IsHidden(Entity e) const {
  const auto* render_component = FindRenderComponentForEntity(e);
  const bool render_component_hidden =
      render_component && render_component->hidden;

  // If there are no models associated with this entity, then it is hidden.
  // Otherwise, it is hidden if the RenderComponent is hidden.
  return (render_component_hidden || !render_component);
}

ShaderPtr RenderSystemNext::GetShader(Entity entity, HashValue pass) const {
  const RenderComponent* component = GetComponent(entity, pass);
  if (!component || component->materials.empty()) {
    return ShaderPtr();
  }

  return component->materials[0]->GetShader();
}

ShaderPtr RenderSystemNext::GetShader(Entity entity) const {
  const RenderComponent* component = FindRenderComponentForEntity(entity);
  if (!component || component->materials.empty()) {
    return ShaderPtr();
  }

  return component->materials[0]->GetShader();
}

void RenderSystemNext::SetShader(Entity e, const ShaderPtr& shader) {
  ForEachComponentOfEntity(
      e, [this, e, shader](RenderComponent* render_component, HashValue pass) {
        SetShader(e, pass, shader);
      });
}

void RenderSystemNext::SetShader(Entity e, HashValue pass,
                                 const ShaderPtr& shader) {
  auto* render_component = GetComponent(e, pass);
  if (!render_component) {
    return;
  }

  // If there are no materials, resize to have at least one. This is needed
  // because the mesh loading is asynchronous and the materials may not have
  // been set. Without this, the shader setting will not be saved.
  // This is also needed when only a RenderDef is used.
  if (render_component->materials.empty()) {
    render_component->materials.emplace_back(
        std::make_shared<Material>(render_component->default_material));
  }

  for (auto& material : render_component->materials) {
    material->SetShader(shader);
  }
  render_component->default_material.SetShader(shader);
}

void RenderSystemNext::SetShadingModelPath(const std::string& path) {
  shading_model_path_ = path;
}

std::shared_ptr<Material> RenderSystemNext::CreateMaterialFromMaterialInfo(
    RenderComponent* render_component, const MaterialInfo& info) {
  std::shared_ptr<Material> material =
      std::make_shared<Material>(render_component->default_material);

  for (int i = MaterialTextureUsage_MIN; i <= MaterialTextureUsage_MAX; ++i) {
    MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(i);
    const std::string& name = info.GetTexture(usage);
    if (!name.empty()) {
      TextureParams params;
      params.generate_mipmaps = true;
      TexturePtr texture = texture_factory_->LoadTexture(name, params);
      const MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(i);
      material->SetTexture(usage, texture);
      material->SetUniform<float>(Hash(kTextureBoundsUniform),
                                  ShaderDataType_Float4,
                                  {&texture->UvBounds()[0], 1});
    }
  }

  material->ApplyProperties(info.GetProperties());

  const std::string shading_model =
      shading_model_path_ + info.GetShadingModel();
  ShaderPtr shader;
  if (render_component->mesh && render_component->mesh->IsLoaded()) {
    const ShaderCreateParams shader_params =
        CreateShaderParams(shading_model, render_component, material);
    shader = shader_factory_->LoadShader(shader_params);
  } else {
    shader = std::make_shared<Shader>(Shader::Description(shading_model));
  }
  material->SetShader(shader);
  return material;
}

void RenderSystemNext::SetMaterial(Entity e, Optional<HashValue> pass,
                                   Optional<int> submesh_index,
                                   const MaterialInfo& material_info) {
  if (pass) {
    auto* render_component = GetComponent(e, *pass);
    if (submesh_index) {
      SetMaterialImpl(render_component, *pass, *submesh_index, material_info);
    } else {
      SetMaterialImpl(render_component, *pass, material_info);
    }
  } else {
    ForEachComponentOfEntity(
        e, [&](RenderComponent* render_component, HashValue pass_hash) {
          if (submesh_index) {
            SetMaterialImpl(render_component, pass_hash, *submesh_index,
                            material_info);
          } else {
            SetMaterialImpl(render_component, pass_hash, material_info);
          }
        });
  }
}

void RenderSystemNext::SetMaterialImpl(RenderComponent* render_component,
                                       HashValue pass_hash,
                                       const MaterialInfo& material_info) {
  if (!render_component) {
    return;
  }

  if (render_component->materials.empty()) {
    render_component->default_material =
        *CreateMaterialFromMaterialInfo(render_component, material_info);
  } else {
    for (int i = 0; i < render_component->materials.size(); ++i) {
      SetMaterialImpl(render_component, pass_hash, i, material_info);
    }
  }
}

void RenderSystemNext::SetMaterialImpl(RenderComponent* render_component,
                                       HashValue pass_hash, int submesh_index,
                                       const MaterialInfo& material_info) {
  if (!render_component) {
    return;
  }
  std::shared_ptr<Material> material =
      CreateMaterialFromMaterialInfo(render_component, material_info);

  if (render_component->materials.size() <=
      static_cast<size_t>(submesh_index)) {
    render_component->materials.resize(submesh_index + 1);
  }
  render_component->materials[submesh_index] = material;

  for (int i = MaterialTextureUsage_MIN; i <= MaterialTextureUsage_MAX; ++i) {
    const auto usage = static_cast<MaterialTextureUsage>(i);
    TexturePtr texture = material->GetTexture(usage);
    if (!texture) {
      continue;
    }

    const Entity entity = render_component->GetEntity();
    auto callback = [this, usage, entity, pass_hash, texture]() {
      RenderComponent* render_component = GetComponent(entity, pass_hash);
      if (render_component) {
        for (auto& material : render_component->materials) {
          if (material->GetTexture(usage) == texture) {
            OnTextureLoaded(*render_component, pass_hash, usage, texture);
          }
        }
      }
    };
    texture->AddOrInvokeOnLoadCallback(callback);
  }
}

void RenderSystemNext::OnMeshLoaded(RenderComponent* render_component,
                                    HashValue pass) {
  const Entity entity = render_component->GetEntity();

  auto* transform_system = registry_->Get<TransformSystem>();
  transform_system->SetAabb(entity, render_component->mesh->GetAabb());

  MeshPtr& mesh = render_component->mesh;
  if (render_component->materials.size() != mesh->GetNumSubmeshes()) {
    std::shared_ptr<Material> copy_material;
    if (render_component->materials.empty()) {
      copy_material =
          std::make_shared<Material>(render_component->default_material);
    } else {
      copy_material = render_component->materials[0];
    }

    render_component->materials.resize(mesh->GetNumSubmeshes());
    if (mesh->GetNumSubmeshes() != 0) {
      render_component->materials[0] = copy_material;
      for (size_t i = 1; i < mesh->GetNumSubmeshes(); ++i) {
        render_component->materials[i] =
            std::make_shared<Material>(*copy_material);
      }
    }
  }

  for (auto& material : render_component->materials) {
    if (material->GetShader() &&
        !material->GetShader()->GetDescription().shading_model.empty()) {
      const ShaderCreateParams shader_params = CreateShaderParams(
          material->GetShader()->GetDescription().shading_model,
          render_component, material);
      material->SetShader(shader_factory_->LoadShader(shader_params));
    }
  }

  auto* rig_system = registry_->Get<RigSystem>();
  const size_t num_shader_bones =
      std::max(mesh->TryUpdateRig(rig_system, entity),
               CheckMeshForBoneIndices(mesh) ? size_t(1) : size_t(0));
  if (num_shader_bones > 0) {
    // By default, clear the bone transforms to identity.
    const int count =
        kNumVec4sInAffineTransform * static_cast<int>(num_shader_bones);
    const mathfu::AffineTransform identity =
        mathfu::mat4::ToAffineTransform(mathfu::mat4::Identity());
    shader_transforms_.clear();
    shader_transforms_.resize(num_shader_bones, identity);
    constexpr int dimension = 4;
    const float* data = &(shader_transforms_[0][0]);
    SetUniform(entity, pass, kBoneTransformsUniform, data, dimension, count);
  }

  if (IsReadyToRenderImpl(*render_component)) {
    SendEvent(registry_, entity, ReadyToRenderEvent(entity, pass));
  }
}

void RenderSystemNext::SetMesh(Entity e, const MeshPtr& mesh) {
  ForEachComponentOfEntity(e, [&](RenderComponent* render_component,
                                  HashValue pass) { SetMesh(e, pass, mesh); });
}

void RenderSystemNext::SetMesh(Entity e, HashValue pass, const MeshPtr& mesh) {
  auto* render_component = GetComponent(e, pass);
  if (!render_component) {
    LOG(WARNING) << "Missing RenderComponent, "
                 << "skipping mesh update for entity: " << e
                 << ", inside pass: " << pass;
    return;
  }

  render_component->mesh = mesh;

  if (render_component->mesh) {
    MeshPtr callback_mesh = render_component->mesh;
    auto callback = [this, e, pass, callback_mesh]() {
      RenderComponent* render_component = GetComponent(e, pass);
      if (render_component && render_component->mesh == callback_mesh) {
        OnMeshLoaded(render_component, pass);
      }
    };
    render_component->mesh->AddOrInvokeOnLoadCallback(callback);
  }
  SendEvent(registry_, e, MeshChangedEvent(e, pass));
}

MeshPtr RenderSystemNext::GetMesh(Entity e, HashValue pass) {
  auto* render_component = GetComponent(e, pass);
  if (!render_component) {
    LOG(WARNING) << "Missing RenderComponent for entity: " << e
                 << ", inside pass: " << pass;
    return nullptr;
  }

  return render_component->mesh;
}

void RenderSystemNext::DeformMesh(Entity entity, HashValue pass,
                                  MeshData* mesh) {
  auto iter = deformations_.find(entity);
  const RenderSystem::DeformationFn deform =
      iter != deformations_.end() ? iter->second : nullptr;
  if (deform) {
    deform(mesh);
  }
}

void RenderSystemNext::SetStencilMode(Entity e, RenderStencilMode mode,
                                      int value) {
  ForEachComponentOfEntity(
      e, [this, e, mode, value](RenderComponent* render_component,
                                HashValue pass) {
        SetStencilMode(e, pass, mode, value);
      });
}

void RenderSystemNext::SetStencilMode(Entity e, HashValue pass,
                                      RenderStencilMode mode, int value) {
  // Stencil mask setting all the bits to be 1.
  static constexpr uint32_t kStencilMaskAllBits = ~0;

  auto* render_component = GetComponent(e, pass);
  if (!render_component) {
    return;
  }

  StencilStateT stencil_state;

  switch (mode) {
    case RenderStencilMode::kDisabled:
      stencil_state.enabled = false;
      break;

    case RenderStencilMode::kTest:
      stencil_state.enabled = true;

      stencil_state.front_function.function = RenderFunction_Equal;
      stencil_state.front_function.ref = value;
      stencil_state.front_function.mask = kStencilMaskAllBits;
      stencil_state.back_function = stencil_state.front_function;

      stencil_state.front_op.stencil_fail = StencilAction_Keep;
      stencil_state.front_op.depth_fail = StencilAction_Keep;
      stencil_state.front_op.pass = StencilAction_Keep;
      stencil_state.back_op = stencil_state.front_op;
      break;

    case RenderStencilMode::kWrite:
      stencil_state.enabled = true;

      stencil_state.front_function.function = RenderFunction_Always;
      stencil_state.front_function.ref = value;
      stencil_state.front_function.mask = kStencilMaskAllBits;
      stencil_state.back_function = stencil_state.front_function;

      stencil_state.front_op.stencil_fail = StencilAction_Keep;
      stencil_state.front_op.depth_fail = StencilAction_Keep;
      stencil_state.front_op.pass = StencilAction_Replace;
      stencil_state.back_op = stencil_state.front_op;
      break;
  }

  for (auto& material : render_component->materials) {
    material->SetStencilState(&stencil_state);
  }
}

void RenderSystemNext::SetDeformationFunction(
    Entity e, const RenderSystem::DeformationFn& deform) {
  if (deform) {
    deformations_.emplace(e, deform);
  } else {
    deformations_.erase(e);
  }
}

void RenderSystemNext::Hide(Entity e) {
  ForEachComponentOfEntity(
      e, [this, e](RenderComponent* render_component, HashValue pass_hash) {
        if (!render_component->hidden) {
          render_component->hidden = true;
          SendEvent(registry_, e, HiddenEvent(e));
        }
      });
}

void RenderSystemNext::Show(Entity e) {
  ForEachComponentOfEntity(
      e, [this, e](RenderComponent* render_component, HashValue pass_hash) {
        if (render_component->hidden) {
          render_component->hidden = false;
          SendEvent(registry_, e, UnhiddenEvent(e));
        }
      });
}

HashValue RenderSystemNext::GetRenderPass(Entity entity) const {
  LOG(ERROR) << "GetRenderPass is not supported in render system next.";
  return 0;
}

std::vector<HashValue> RenderSystemNext::GetRenderPasses(Entity entity) const {
  std::vector<HashValue> passes;
  ForEachComponentOfEntity(
      entity, [&passes](const RenderComponent& /*component*/, HashValue pass) {
        passes.emplace_back(pass);
      });
  return passes;
}

void RenderSystemNext::SetRenderPass(Entity e, HashValue pass) {
  LOG(ERROR) << "SetRenderPass is not supported in render system next.";
}

void RenderSystemNext::SetClearParams(HashValue pass,
                                      const RenderClearParams& clear_params) {
  RenderPassObject* render_pass = GetRenderPassObject(pass);
  if (!render_pass) {
    LOG(DFATAL) << "Render pass " << pass << " does not exist!";
    return;
  }
  render_pass->clear_params = clear_params;
}

void RenderSystemNext::SetDefaultRenderPass(HashValue pass) {
  default_pass_ = pass;
}

HashValue RenderSystemNext::GetDefaultRenderPass() const {
  return default_pass_;
}

SortMode RenderSystemNext::GetSortMode(HashValue pass) const {
  const RenderPassObject* render_pass_object = FindRenderPassObject(pass);
  if (render_pass_object) {
    return render_pass_object->sort_mode;
  }
  return SortMode_None;
}

void RenderSystemNext::SetSortMode(HashValue pass, SortMode mode) {
  RenderPassObject* render_pass_object = GetRenderPassObject(pass);
  if (render_pass_object) {
    render_pass_object->sort_mode = mode;
  }
}

void RenderSystemNext::SetSortVector(HashValue pass,
                                     const mathfu::vec3& vector) {
  LOG(DFATAL) << "Unimplemented";
}

RenderCullMode RenderSystemNext::GetCullMode(HashValue pass) {
  const RenderPassObject* render_pass_object = FindRenderPassObject(pass);
  if (render_pass_object) {
    return render_pass_object->cull_mode;
  }
  return RenderCullMode::kNone;
}

void RenderSystemNext::SetCullMode(HashValue pass, RenderCullMode mode) {
  RenderPassObject* render_pass_object = GetRenderPassObject(pass);
  if (render_pass_object) {
    render_pass_object->cull_mode = mode;
  }
}

void RenderSystemNext::SetDefaultFrontFace(RenderFrontFace face) {
  CHECK(!initialized_) << "Must set the default FrontFace before Initialize";
  default_front_face_ = face;
}

void RenderSystemNext::SetRenderState(HashValue pass,
                                      const fplbase::RenderState& state) {
  RenderPassObject* render_pass_object = GetRenderPassObject(pass);
  if (render_pass_object) {
    render_pass_object->render_state = Convert(state);
  }
}

void RenderSystemNext::SetViewport(const RenderView& view) {
  LULLABY_CPU_TRACE_CALL();
  SetViewport(mathfu::recti(view.viewport, view.dimensions));
}

void RenderSystemNext::SetClipFromModelMatrix(const mathfu::mat4& mvp) {
  model_view_projection_ = mvp;
}

void RenderSystemNext::SetClipFromModelMatrixFunction(
    const RenderSystem::ClipFromModelMatrixFn& func) {
  if (!func) {
    clip_from_model_matrix_func_ = CalculateClipFromModelMatrix;
    return;
  }

  clip_from_model_matrix_func_ = func;
}

mathfu::vec4 RenderSystemNext::GetClearColor() const { return clear_color_; }

void RenderSystemNext::SetClearColor(float r, float g, float b, float a) {
  RenderClearParams clear_params;
  clear_params.clear_options = RenderClearParams::kColor |
                               RenderClearParams::kDepth |
                               RenderClearParams::kStencil;
  clear_params.color_value = mathfu::vec4(r, g, b, a);
  SetClearParams(ConstHash("ClearDisplay"), clear_params);
}

void RenderSystemNext::SubmitRenderData() {
  RenderData* data = render_data_buffer_.LockWriteBuffer();
  if (!data) {
    return;
  }
  data->clear();

  const auto* transform_system = registry_->Get<TransformSystem>();

  for (const auto& iter : render_passes_) {
    RenderPassDrawContainer& pass_container = (*data)[iter.first];

    // Copy the pass' properties.
    pass_container.clear_params = iter.second.clear_params;

    auto target = render_targets_.find(iter.second.render_target);
    if (target != render_targets_.end()) {
      pass_container.render_target = target->second.get();
    }

    RenderLayer& opaque_layer =
        pass_container.layers[RenderPassDrawContainer::kOpaque];
    RenderLayer& blend_layer =
        pass_container.layers[RenderPassDrawContainer::kBlendEnabled];

    // Assign sort modes.
    if (iter.second.sort_mode == SortMode_Optimized) {
      // If user set optimized, set the best expected sort modes depending on
      // the blend mode.
      opaque_layer.sort_mode = SortMode_AverageSpaceOriginFrontToBack;
      blend_layer.sort_mode = SortMode_AverageSpaceOriginBackToFront;
    } else {
      // Otherwise assign the user's chosen sort modes.
      for (RenderLayer& layer : pass_container.layers) {
        layer.cull_mode = iter.second.cull_mode;
        layer.sort_mode = iter.second.sort_mode;
      }
    }

    // Set the render states.
    // Opaque remains as is.
    const RenderStateT& pass_render_state = iter.second.render_state;

    opaque_layer.render_state = pass_render_state;

    if (pass_render_state.blend_state &&
        pass_render_state.blend_state->enabled) {
      blend_layer.render_state = pass_render_state;
    } else {
      // Blend requires z-write off and blend mode on.
      RenderStateT render_state = pass_render_state;
      if (!render_state.blend_state) {
        render_state.blend_state.emplace();
      }
      if (!render_state.depth_state) {
        render_state.depth_state.emplace();
      }

      render_state.blend_state->enabled = true;
      render_state.blend_state->src_alpha = BlendFactor_One;
      render_state.blend_state->src_color = BlendFactor_One;
      render_state.blend_state->dst_alpha = BlendFactor_OneMinusSrcAlpha;
      render_state.blend_state->dst_color = BlendFactor_OneMinusSrcAlpha;
      render_state.depth_state->write_enabled = false;

      // Assign the render state.
      blend_layer.render_state = render_state;
    }

    iter.second.components.ForEach(
        [&](const RenderComponent& render_component) {
          if (render_component.hidden) {
            return;
          }
          if (!render_component.mesh ||
              render_component.mesh->GetNumSubmeshes() == 0) {
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

          RenderObject obj;
          obj.mesh = render_component.mesh;
          obj.world_from_entity_matrix = *world_from_entity_matrix;

          // Add each material as a single render object where each material
          // references a submesh.
          for (size_t i = 0; i < render_component.materials.size(); ++i) {
            obj.material = render_component.materials[i];
            if (!obj.material || !obj.material->GetShader()) {
              continue;
            }
            obj.submesh_index = static_cast<int>(i);

            const RenderPassDrawContainer::LayerType type =
                ((pass_render_state.blend_state &&
                  pass_render_state.blend_state->enabled) ||
                 IsAlphaEnabled(*obj.material))
                    ? RenderPassDrawContainer::kBlendEnabled
                    : RenderPassDrawContainer::kOpaque;
            pass_container.layers[type].render_objects.push_back(obj);
          }
        });

    // Sort only objects with "static" sort order, such as explicit sort order
    // or absolute z-position.
    for (RenderLayer& layer : pass_container.layers) {
      if (IsSortModeViewIndependent(layer.sort_mode)) {
        SortObjects(&layer.render_objects, layer.sort_mode);
      }
    }
  }

  render_data_buffer_.UnlockWriteBuffer();
}

void RenderSystemNext::BeginRendering() {
  active_render_data_ = render_data_buffer_.LockReadBuffer();
}

void RenderSystemNext::EndRendering() {
  render_data_buffer_.UnlockReadBuffer();
  active_render_data_ = nullptr;
}

void RenderSystemNext::RenderAt(const RenderObject* render_object,
                                const RenderView* views, size_t num_views) {
  LULLABY_CPU_TRACE_CALL();
  static const size_t kMaxNumViews = 2;
  if (views == nullptr || num_views == 0 || num_views > kMaxNumViews) {
    return;
  }

  const std::shared_ptr<Mesh>& mesh = render_object->mesh;
  if (mesh == nullptr) {
    return;
  }
  const std::shared_ptr<Material>& material = render_object->material;
  if (material == nullptr) {
    return;
  }
  const std::shared_ptr<Shader>& shader = material->GetShader();
  if (shader == nullptr) {
    return;
  }

  BindShader(shader);

  constexpr HashValue kModel = ConstHash("model");
  shader->SetUniform(kModel, &render_object->world_from_entity_matrix[0], 16);

  // Compute the normal matrix. This is the transposed matrix of the inversed
  // world position. This is done to avoid non-uniform scaling of the normal.
  // A good explanation of this can be found here:
  // http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/
  constexpr HashValue kMatNormal = ConstHash("mat_normal");
  mathfu::vec3_packed normal_matrix[3];
  ComputeNormalMatrix(render_object->world_from_entity_matrix)
      .Pack(normal_matrix);
  shader->SetUniform(kMatNormal, normal_matrix[0].data, 9);

  // The following uniforms support multiview arrays.  Ensure data is tightly
  // packed so that it correctly gets set in the uniform.
  int is_right_eye[kMaxNumViews] = {0};
  mathfu::vec3_packed camera_dir[kMaxNumViews];
  mathfu::vec3_packed camera_pos[kMaxNumViews];
  mathfu::mat4 clip_from_entity_matrix[kMaxNumViews];
  mathfu::mat4 view_matrix[kMaxNumViews];

  const int count = static_cast<int>(num_views);
  for (int i = 0; i < count; ++i) {
    is_right_eye[i] = views[i].eye == 1 ? 1 : 0;
    view_matrix[i] = views[i].eye_from_world_matrix;
    views[i].world_from_eye_matrix.TranslationVector3D().Pack(&camera_pos[i]);
    CalculateCameraDirection(views[i].world_from_eye_matrix)
        .Pack(&camera_dir[i]);
    clip_from_entity_matrix[i] =
        clip_from_model_matrix_func_(render_object->world_from_entity_matrix,
                                     views[i].clip_from_world_matrix);
  }

  constexpr HashValue kView = ConstHash("view");
  shader->SetUniform(kView, &(view_matrix[0][0]), 16, count);

  constexpr HashValue kModelViewProjection = ConstHash("model_view_projection");
  shader->SetUniform(kModelViewProjection, &(clip_from_entity_matrix[0][0]), 16,
                     count);

  constexpr HashValue kCameraDir = ConstHash("camera_dir");
  shader->SetUniform(kCameraDir, camera_dir[0].data, 3, count);

  constexpr HashValue kCameraPos = ConstHash("camera_pos");
  // camera_pos is not currently multiview enabled, so only set 1 value.
  shader->SetUniform(kCameraPos, camera_pos[0].data, 3, 1);

  // We break the naming convention here for compatibility with early VR apps.
  constexpr HashValue kIsRightEye = ConstHash("uIsRightEye");
  shader->SetUniform(kIsRightEye, is_right_eye, 1, count);

  renderer_.ApplyMaterial(render_object->material);
  renderer_.Draw(mesh, render_object->world_from_entity_matrix,
                 render_object->submesh_index);

  detail::Profiler* profiler = registry_->Get<detail::Profiler>();
  if (profiler) {
    profiler->RecordDraw(material->GetShader(), mesh->GetNumVertices(),
                         mesh->GetNumTriangles());
  }
}

void RenderSystemNext::Render(const RenderView* views, size_t num_views) {
  // Assume a max of 2 views, one for each eye.
  RenderView pano_views[2];
  CHECK_LE(num_views, 2);
  GenerateEyeCenteredViews({views, num_views}, pano_views);
  Render(pano_views, num_views, ConstHash("Pano"));
  Render(views, num_views, ConstHash("Opaque"));
  Render(views, num_views, ConstHash("Main"));
  Render(views, num_views, ConstHash("OverDraw"));
  Render(views, num_views, ConstHash("OverDrawGlow"));
}

void RenderSystemNext::Render(const RenderView* views, size_t num_views,
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

  RenderPassDrawContainer& draw_container = iter->second;

  renderer_.Begin(draw_container.render_target);
  renderer_.Clear(draw_container.clear_params);

  // Draw the elements.
  if (pass == ConstHash("Debug")) {
    RenderDebugStats(views, num_views);
  } else {
    for (RenderLayer& layer : draw_container.layers) {
      if (!IsSortModeViewIndependent(layer.sort_mode)) {
        SortObjectsUsingView(&layer.render_objects, layer.sort_mode, views,
                             num_views);
      }
      RenderObjects(layer.render_objects, layer.render_state, views, num_views);
    }
  }

  renderer_.End();
}

void RenderSystemNext::RenderObjects(const std::vector<RenderObject>& objects,
                                     const RenderStateT& render_state,
                                     const RenderView* views,
                                     size_t num_views) {
  if (objects.empty()) {
    return;
  }

  renderer_.GetRenderStateManager().SetRenderState(render_state);

  if (renderer_.IsMultiviewEnabled()) {
    SetViewport(views[0]);
    for (const RenderObject& obj : objects) {
      RenderAt(&obj, views, num_views);
    }
  } else {
    for (size_t i = 0; i < num_views; ++i) {
      SetViewport(views[i]);
      for (const RenderObject& obj : objects) {
        RenderAt(&obj, &views[i], 1);
      }
    }
  }
}

void RenderSystemNext::BindShader(const ShaderPtr& shader) {
  shader_ = shader;
  shader->Bind();

  // Initialize color to white and allow individual materials to set it
  // differently.
  constexpr HashValue kColor = ConstHash("color");
  shader->SetUniform(kColor, &mathfu::kOnes4f[0], 4);

  // Set the MVP matrix to the one explicitly set by the user and override
  // during draw calls if needed.
  constexpr HashValue kModelViewProjection = ConstHash("model_view_projection");
  shader->SetUniform(kModelViewProjection, &model_view_projection_[0], 16);
}

void RenderSystemNext::BindTexture(int unit, const TexturePtr& texture) {
  if (unit < 0) {
    LOG(ERROR) << "BindTexture called with negative unit.";
    return;
  }

  // Ensure there is enough size in the cache.
  if (gpu_cache_.bound_textures.size() <= unit) {
    gpu_cache_.bound_textures.resize(unit + 1);
  }

  if (texture) {
    texture->Bind(unit);
  } else if (gpu_cache_.bound_textures[unit]) {
    GL_CALL(glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit)));
    GL_CALL(glBindTexture(gpu_cache_.bound_textures[unit]->GetTarget(), 0));
  }
  gpu_cache_.bound_textures[unit] = texture;
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
  shader_->SetUniform(shader_->FindUniform(name), data, dimension);
}

void RenderSystemNext::DrawMesh(const MeshData& mesh) { DrawMeshData(mesh); }

void RenderSystemNext::UpdateDynamicMesh(
    Entity entity, MeshData::PrimitiveType primitive_type,
    const VertexFormat& vertex_format, const size_t max_vertices,
    const size_t max_indices, MeshData::IndexType index_type,
    const size_t max_ranges,
    const std::function<void(MeshData*)>& update_mesh) {
  if (max_vertices > 0) {
    DataContainer vertex_data = DataContainer::CreateHeapDataContainer(
        max_vertices * vertex_format.GetVertexSize());
    DataContainer index_data = DataContainer::CreateHeapDataContainer(
        max_indices * MeshData::GetIndexSize(index_type));
    DataContainer range_data = DataContainer::CreateHeapDataContainer(
        max_ranges * sizeof(MeshData::IndexRange));
    MeshData data(primitive_type, vertex_format, std::move(vertex_data),
                  index_type, std::move(index_data), std::move(range_data));
    update_mesh(&data);
    MeshPtr mesh = mesh_factory_->CreateMesh(std::move(data));
    ForEachComponentOfEntity(
        entity,
        [this, entity, mesh](RenderComponent* component, HashValue pass) {
          component->mesh = mesh;
          SendEvent(registry_, entity, MeshChangedEvent(entity, pass));
        });
  } else {
    ForEachComponentOfEntity(
        entity, [this, entity](RenderComponent* component, HashValue pass) {
          component->mesh.reset();
          SendEvent(registry_, entity, MeshChangedEvent(entity, pass));
        });
  }
}

void RenderSystemNext::RenderDebugStats(const RenderView* views,
                                        size_t num_views) {
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
    const float padding = 20.f;
    font_size = 16.f;
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

    model_view_projection_ = views[i].clip_from_eye_matrix;
    // Shader needs to be bound after setting MVP.
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

    registry_->Get<RenderSystem>()->DrawMesh(text.GetMesh());
  }

  // Cleanup render state.
  SetDepthTest(true);
  SetDepthWrite(true);
}

void RenderSystemNext::UpdateSortOrder(Entity entity) {
  ForEachComponentOfEntity(
      entity, [&](RenderComponent* render_component, HashValue pass) {
        const detail::EntityIdPair entity_id_pair(entity, pass);
        sort_order_manager_.UpdateSortOrder(
            entity_id_pair, [this](detail::EntityIdPair entity_id_pair) {
              return GetComponent(entity_id_pair.entity, entity_id_pair.id);
            });
      });
}

const fplbase::RenderState& RenderSystemNext::GetCachedRenderState() const {
  return render_state_;
}

void RenderSystemNext::UpdateCachedRenderState(
    const fplbase::RenderState& render_state) {
  renderer_.GetRenderStateManager().Reset();
  renderer_.ResetGpuState();
  UnsetDefaultAttributes();
  shader_.reset();

  render_state_ = render_state;
  SetAlphaTestState(render_state.alpha_test_state);
  SetBlendState(render_state.blend_state);
  SetCullState(render_state.cull_state);
  SetDepthState(render_state.depth_state);
  SetPointState(render_state.point_state);
  SetScissorState(render_state.scissor_state);
  SetStencilState(render_state.stencil_state);
  SetViewport(render_state.viewport);
  ValidateRenderState();
}

void RenderSystemNext::ValidateRenderState() {
#ifdef LULLABY_VERIFY_GPU_STATE
  DCHECK(renderer_.GetRenderStateManager().Validate());
  fplbase::ValidateRenderState(render_state_);
#endif
}

void RenderSystemNext::SetDepthTest(const bool enabled) {
  if (enabled) {
    SetDepthFunction(fplbase::kDepthFunctionLess);
  } else {
    SetDepthFunction(fplbase::kDepthFunctionDisabled);
  }
}

void RenderSystemNext::SetViewport(const mathfu::recti& viewport) {
  renderer_.GetRenderStateManager().SetViewport(viewport);
  render_state_.viewport = viewport;
}

void RenderSystemNext::SetDepthWrite(bool depth_write) {
  fplbase::DepthState depth_state = render_state_.depth_state;
  depth_state.write_enabled = depth_write;
  SetDepthState(depth_state);
}

void RenderSystemNext::SetDepthFunction(fplbase::DepthFunction depth_function) {
  fplbase::DepthState depth_state = render_state_.depth_state;

  switch (depth_function) {
    case fplbase::kDepthFunctionDisabled:
      depth_state.test_enabled = false;
      break;

    case fplbase::kDepthFunctionNever:
      depth_state.test_enabled = true;
      depth_state.function = fplbase::kRenderNever;
      break;

    case fplbase::kDepthFunctionAlways:
      depth_state.test_enabled = true;
      depth_state.function = fplbase::kRenderAlways;
      break;

    case fplbase::kDepthFunctionLess:
      depth_state.test_enabled = true;
      depth_state.function = fplbase::kRenderLess;
      break;

    case fplbase::kDepthFunctionLessEqual:
      depth_state.test_enabled = true;
      depth_state.function = fplbase::kRenderLessEqual;
      break;

    case fplbase::kDepthFunctionGreater:
      depth_state.test_enabled = true;
      depth_state.function = fplbase::kRenderGreater;
      break;

    case fplbase::kDepthFunctionGreaterEqual:
      depth_state.test_enabled = true;
      depth_state.function = fplbase::kRenderGreaterEqual;
      break;

    case fplbase::kDepthFunctionEqual:
      depth_state.test_enabled = true;
      depth_state.function = fplbase::kRenderEqual;
      break;

    case fplbase::kDepthFunctionNotEqual:
      depth_state.test_enabled = true;
      depth_state.function = fplbase::kRenderNotEqual;
      break;

    case fplbase::kDepthFunctionUnknown:
      // Do nothing.
      break;

    default:
      assert(false);  // Invalid depth function.
      break;
  }

  SetDepthState(depth_state);
}

void RenderSystemNext::SetBlendMode(fplbase::BlendMode blend_mode) {
  fplbase::AlphaTestState alpha_test_state = render_state_.alpha_test_state;
  fplbase::BlendState blend_state = render_state_.blend_state;

  switch (blend_mode) {
    case fplbase::kBlendModeOff:
      alpha_test_state.enabled = false;
      blend_state.enabled = false;
      break;

    case fplbase::kBlendModeTest:
      alpha_test_state.enabled = true;
      alpha_test_state.function = fplbase::kRenderGreater;
      alpha_test_state.ref = 0.5f;
      blend_state.enabled = false;
      break;

    case fplbase::kBlendModeAlpha:
      alpha_test_state.enabled = false;
      blend_state.enabled = true;
      blend_state.src_alpha = fplbase::BlendState::kSrcAlpha;
      blend_state.src_color = fplbase::BlendState::kSrcAlpha;
      blend_state.dst_alpha = fplbase::BlendState::kOneMinusSrcAlpha;
      blend_state.dst_color = fplbase::BlendState::kOneMinusSrcAlpha;
      break;

    case fplbase::kBlendModeAdd:
      alpha_test_state.enabled = false;
      blend_state.enabled = true;
      blend_state.src_alpha = fplbase::BlendState::kOne;
      blend_state.src_color = fplbase::BlendState::kOne;
      blend_state.dst_alpha = fplbase::BlendState::kOne;
      blend_state.dst_color = fplbase::BlendState::kOne;
      break;

    case fplbase::kBlendModeAddAlpha:
      alpha_test_state.enabled = false;
      blend_state.enabled = true;
      blend_state.src_alpha = fplbase::BlendState::kSrcAlpha;
      blend_state.src_color = fplbase::BlendState::kSrcAlpha;
      blend_state.dst_alpha = fplbase::BlendState::kOne;
      blend_state.dst_color = fplbase::BlendState::kOne;
      break;

    case fplbase::kBlendModeMultiply:
      alpha_test_state.enabled = false;
      blend_state.enabled = true;
      blend_state.src_alpha = fplbase::BlendState::kDstColor;
      blend_state.src_color = fplbase::BlendState::kDstColor;
      blend_state.dst_alpha = fplbase::BlendState::kZero;
      blend_state.dst_color = fplbase::BlendState::kZero;
      break;

    case fplbase::kBlendModePreMultipliedAlpha:
      alpha_test_state.enabled = false;
      blend_state.enabled = true;
      blend_state.src_alpha = fplbase::BlendState::kOne;
      blend_state.src_color = fplbase::BlendState::kOne;
      blend_state.dst_alpha = fplbase::BlendState::kOneMinusSrcAlpha;
      blend_state.dst_color = fplbase::BlendState::kOneMinusSrcAlpha;
      break;

    case fplbase::kBlendModeUnknown:
      // Do nothing.
      break;

    default:
      assert(false);  // Not yet implemented.
      break;
  }

  SetBlendState(blend_state);
  SetAlphaTestState(alpha_test_state);
}

void RenderSystemNext::SetStencilMode(fplbase::StencilMode mode, int ref,
                                      uint32_t mask) {
  fplbase::StencilState stencil_state = render_state_.stencil_state;

  switch (mode) {
    case fplbase::kStencilDisabled:
      stencil_state.enabled = false;
      break;

    case fplbase::kStencilCompareEqual:
      stencil_state.enabled = true;

      stencil_state.front_function.function = fplbase::kRenderEqual;
      stencil_state.front_function.ref = ref;
      stencil_state.front_function.mask = mask;
      stencil_state.back_function = stencil_state.front_function;

      stencil_state.front_op.stencil_fail = fplbase::StencilOperation::kKeep;
      stencil_state.front_op.depth_fail = fplbase::StencilOperation::kKeep;
      stencil_state.front_op.pass = fplbase::StencilOperation::kKeep;
      stencil_state.back_op = stencil_state.front_op;
      break;

    case fplbase::kStencilWrite:
      stencil_state.enabled = true;

      stencil_state.front_function.function = fplbase::kRenderAlways;
      stencil_state.front_function.ref = ref;
      stencil_state.front_function.mask = mask;
      stencil_state.back_function = stencil_state.front_function;

      stencil_state.front_op.stencil_fail = fplbase::StencilOperation::kKeep;
      stencil_state.front_op.depth_fail = fplbase::StencilOperation::kKeep;
      stencil_state.front_op.pass = fplbase::StencilOperation::kReplace;
      stencil_state.back_op = stencil_state.front_op;
      break;

    case fplbase::kStencilUnknown:
      // Do nothing.
      break;

    default:
      assert(false);
  }

  SetStencilState(stencil_state);
}

void RenderSystemNext::SetCulling(fplbase::CullingMode mode) {
  fplbase::CullState cull_state = render_state_.cull_state;

  switch (mode) {
    case fplbase::kCullingModeNone:
      cull_state.enabled = false;
      break;
    case fplbase::kCullingModeBack:
      cull_state.enabled = true;
      cull_state.face = fplbase::CullState::kBack;
      break;
    case fplbase::kCullingModeFront:
      cull_state.enabled = true;
      cull_state.face = fplbase::CullState::kFront;
      break;
    case fplbase::kCullingModeFrontAndBack:
      cull_state.enabled = true;
      cull_state.face = fplbase::CullState::kFrontAndBack;
      break;
    case fplbase::kCullingModeUnknown:
      // Do nothing.
      break;
    default:
      // Unknown culling mode.
      assert(false);
  }

  SetCullState(cull_state);
}

void RenderSystemNext::SetAlphaTestState(
    const fplbase::AlphaTestState& alpha_test_state) {
  AlphaTestStateT state = Convert(alpha_test_state);
  renderer_.GetRenderStateManager().SetAlphaTestState(state);
  render_state_.alpha_test_state = alpha_test_state;
}

void RenderSystemNext::SetBlendState(const fplbase::BlendState& blend_state) {
  BlendStateT state = Convert(blend_state);
  renderer_.GetRenderStateManager().SetBlendState(state);
  render_state_.blend_state = blend_state;
}

void RenderSystemNext::SetCullState(const fplbase::CullState& cull_state) {
  CullStateT state = Convert(cull_state);
  renderer_.GetRenderStateManager().SetCullState(state);
  render_state_.cull_state = cull_state;
}

void RenderSystemNext::SetDepthState(const fplbase::DepthState& depth_state) {
  DepthStateT state = Convert(depth_state);
  renderer_.GetRenderStateManager().SetDepthState(state);
  render_state_.depth_state = depth_state;
}

void RenderSystemNext::SetPointState(const fplbase::PointState& point_state) {
  PointStateT state = Convert(point_state);
  renderer_.GetRenderStateManager().SetPointState(state);
  render_state_.point_state = point_state;
}

void RenderSystemNext::SetScissorState(
    const fplbase::ScissorState& scissor_state) {
  ScissorStateT state = Convert(scissor_state);
  renderer_.GetRenderStateManager().SetScissorState(state);
  render_state_.scissor_state = scissor_state;
}

void RenderSystemNext::SetStencilState(
    const fplbase::StencilState& stencil_state) {
  StencilStateT state = Convert(stencil_state);
  renderer_.GetRenderStateManager().SetStencilState(state);
  render_state_.stencil_state = stencil_state;
}

bool RenderSystemNext::IsSortModeViewIndependent(SortMode mode) {
  switch (mode) {
    case SortMode_AverageSpaceOriginBackToFront:
    case SortMode_AverageSpaceOriginFrontToBack:
      return false;
    default:
      return true;
  }
}

namespace {
inline float GetZBackToFrontXOutToMiddleDistance(const mathfu::vec3& pos) {
  return pos.z - std::abs(pos.x);
}
}  // namespace

void RenderSystemNext::SortObjects(std::vector<RenderObject>* objects,
                                   SortMode mode) {
  switch (mode) {
    case SortMode_Optimized:
    case SortMode_None:
      // Do nothing.
      break;

    case SortMode_SortOrderDecreasing:
      std::sort(objects->begin(), objects->end(),
                [](const RenderObject& a, const RenderObject& b) {
                  return a.sort_order > b.sort_order;
                });
      break;

    case SortMode_SortOrderIncreasing:
      std::sort(objects->begin(), objects->end(),
                [](const RenderObject& a, const RenderObject& b) {
                  return a.sort_order < b.sort_order;
                });
      break;

    case SortMode_WorldSpaceZBackToFront:
      std::sort(objects->begin(), objects->end(),
                [](const RenderObject& a, const RenderObject& b) {
                  return a.world_from_entity_matrix.TranslationVector3D().z <
                         b.world_from_entity_matrix.TranslationVector3D().z;
                });
      break;

    case SortMode_WorldSpaceZFrontToBack:
      std::sort(objects->begin(), objects->end(),
                [](const RenderObject& a, const RenderObject& b) {
                  return a.world_from_entity_matrix.TranslationVector3D().z >
                         b.world_from_entity_matrix.TranslationVector3D().z;
                });
      break;

    case SortMode_WorldSpaceZBackToFrontXOutToMiddle:
      std::sort(objects->begin(), objects->end(),
                [](const RenderObject& a, const RenderObject& b) {
                  return GetZBackToFrontXOutToMiddleDistance(
                             a.world_from_entity_matrix.TranslationVector3D()) <
                         GetZBackToFrontXOutToMiddleDistance(
                             b.world_from_entity_matrix.TranslationVector3D());
                });
      break;

    default:
      LOG(DFATAL) << "SortObjects called with unsupported sort mode!";
      break;
  }
}

void RenderSystemNext::SortObjectsUsingView(std::vector<RenderObject>* objects,
                                            SortMode mode,
                                            const RenderView* views,
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
    case SortMode_AverageSpaceOriginBackToFront:
      std::sort(objects->begin(), objects->end(),
                [&](const RenderObject& a, const RenderObject& b) {
                  return a.z_sort_order < b.z_sort_order;
                });
      break;

    case SortMode_AverageSpaceOriginFrontToBack:
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

void RenderSystemNext::InitDefaultRenderPassObjects() {
  fplbase::RenderState render_state;
  RenderClearParams clear_params;

  // Create a pass that clears the display.
  clear_params.clear_options = RenderClearParams::kColor |
                               RenderClearParams::kDepth |
                               RenderClearParams::kStencil;
  SetClearParams(ConstHash("ClearDisplay"), clear_params);

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
  render_state.cull_state.front =
      CullStateFrontFaceFromFrontFace(default_front_face_);
  render_state.cull_state.face = fplbase::CullState::kBack;
  render_state.point_state.point_sprite_enabled = true;
  render_state.point_state.program_point_size_enabled = true;
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
  render_state.cull_state.front =
      CullStateFrontFaceFromFrontFace(default_front_face_);
  render_state.cull_state.face = fplbase::CullState::kBack;
  render_state.point_state.point_sprite_enabled = true;
  render_state.point_state.program_point_size_enabled = true;
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
  render_state.cull_state.front =
      CullStateFrontFaceFromFrontFace(default_front_face_);
  render_state.cull_state.face = fplbase::CullState::kBack;
  render_state.point_state.point_sprite_enabled = false;
  render_state.point_state.program_point_size_enabled = false;
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
  render_state.cull_state.front =
      CullStateFrontFaceFromFrontFace(default_front_face_);
  render_state.cull_state.enabled = false;
  SetRenderState(ConstHash("OverDrawGlow"), render_state);

  SetSortMode(ConstHash("Opaque"), SortMode_AverageSpaceOriginFrontToBack);
  SetSortMode(ConstHash("Main"), SortMode_SortOrderIncreasing);
}

void RenderSystemNext::CreateRenderTarget(
    HashValue render_target_name,
    const RenderTargetCreateParams& create_params) {
  DCHECK_EQ(render_targets_.count(render_target_name), 0);

  // Create the render target.
  auto render_target = MakeUnique<RenderTarget>(create_params);

  // Create a bindable texture.
  TexturePtr texture = texture_factory_->CreateTexture(
      GL_TEXTURE_2D, *render_target->GetTextureId());
  texture_factory_->CacheTexture(render_target_name, texture);

  // Store the render target.
  render_targets_[render_target_name] = std::move(render_target);
}

void RenderSystemNext::SetRenderTarget(HashValue pass,
                                       HashValue render_target_name) {
  RenderPassObject* pass_object = GetRenderPassObject(pass);
  if (!pass_object) {
    return;
  }

  pass_object->render_target = render_target_name;
}

ImageData RenderSystemNext::GetRenderTargetData(HashValue render_target_name) {
  auto iter = render_targets_.find(render_target_name);
  if (iter == render_targets_.end()) {
    LOG(DFATAL) << "SetRenderTarget called with non-existent render target: "
                << render_target_name;
    return ImageData();
  }
  return iter->second->GetFrameBufferData();
}

void RenderSystemNext::ForEachComponentOfEntity(
    Entity entity, const std::function<void(RenderComponent*, HashValue)>& fn) {
  for (auto& pass : render_passes_) {
    RenderComponent* component = pass.second.components.Get(entity);
    if (component) {
      fn(component, pass.first);
    }
  }
}

void RenderSystemNext::ForEachComponentOfEntity(
    Entity entity,
    const std::function<void(const RenderComponent&, HashValue)>& fn) const {
  for (const auto& pass : render_passes_) {
    const RenderComponent* component = pass.second.components.Get(entity);
    if (component) {
      fn(*component, pass.first);
    }
  }
}

RenderComponent* RenderSystemNext::FindRenderComponentForEntity(Entity e) {
  RenderComponent* component = GetComponent(e, ConstHash("Opaque"));
  if (component) {
    return component;
  }

  component = GetComponent(e, ConstHash("Main"));
  if (component) {
    return component;
  }

  for (auto& pass : render_passes_) {
    component = pass.second.components.Get(e);
    if (component) {
      return component;
    }
  }

  return nullptr;
}

const RenderComponent* RenderSystemNext::FindRenderComponentForEntity(
    Entity e) const {
  const RenderComponent* component = GetComponent(e, ConstHash("Opaque"));
  if (component) {
    return component;
  }

  component = GetComponent(e, ConstHash("Main"));
  if (component) {
    return component;
  }

  for (const auto& pass : render_passes_) {
    component = pass.second.components.Get(e);
    if (component) {
      return component;
    }
  }

  return nullptr;
}

RenderComponent* RenderSystemNext::GetComponent(Entity e, HashValue pass) {
  // TODO(b/68871848) some apps are feeding pass=0 because they are used to
  // component id.
  if (pass == 0) {
    LOG(WARNING) << "Tried find render component by using pass = 0. Support "
                    "for this will be deprecated. Apps should identify the "
                    "correct pass the entity lives in.";
    return FindRenderComponentForEntity(e);
  }

  RenderPassObject* pass_object = FindRenderPassObject(pass);
  if (!pass_object) {
    return nullptr;
  }

  return pass_object->components.Get(e);
}

const RenderComponent* RenderSystemNext::GetComponent(Entity e,
                                                      HashValue pass) const {
  // TODO(b/68871848) some apps are feeding pass=0 because they are used to
  // component id.
  if (pass == 0) {
    LOG(WARNING) << "Tried find render component by using pass = 0. Support "
                    "for this will be deprecated. Apps should identify the "
                    "correct pass the entity lives in.";
    return FindRenderComponentForEntity(e);
  }

  const RenderPassObject* pass_object = FindRenderPassObject(pass);
  if (!pass_object) {
    return nullptr;
  }

  return pass_object->components.Get(e);
}

RenderSystemNext::RenderPassObject* RenderSystemNext::GetRenderPassObject(
    HashValue pass) {
  if (pass == RenderSystem::kDefaultPass) {
    pass = default_pass_;
  }
  return &render_passes_[pass];
}

RenderSystemNext::RenderPassObject* RenderSystemNext::FindRenderPassObject(
    HashValue pass) {
  if (pass == RenderSystem::kDefaultPass) {
    pass = default_pass_;
  }
  auto iter = render_passes_.find(pass);
  if (iter == render_passes_.end()) {
    return nullptr;
  }

  return &iter->second;
}

const RenderSystemNext::RenderPassObject*
RenderSystemNext::FindRenderPassObject(HashValue pass) const {
  if (pass == RenderSystem::kDefaultPass) {
    pass = default_pass_;
  }
  const auto iter = render_passes_.find(pass);
  if (iter == render_passes_.end()) {
    return nullptr;
  }

  return &iter->second;
}

int RenderSystemNext::GetNumBones(Entity entity) const {
  LOG(DFATAL) << "Deprecated.";
  return 0;
}

const uint8_t* RenderSystemNext::GetBoneParents(Entity e, int* num) const {
  LOG(DFATAL) << "Deprecated.";
  return nullptr;
}

const std::string* RenderSystemNext::GetBoneNames(Entity e, int* num) const {
  LOG(DFATAL) << "Deprecated.";
  return nullptr;
}

const mathfu::AffineTransform*
RenderSystemNext::GetDefaultBoneTransformInverses(Entity e, int* num) const {
  LOG(DFATAL) << "Deprecated.";
  return nullptr;
}

void RenderSystemNext::SetBoneTransforms(
    Entity entity, const mathfu::AffineTransform* transforms,
    int num_transforms) {
  LOG(DFATAL) << "Deprecated.";
}

void RenderSystemNext::SetBoneTransforms(
    Entity entity, HashValue component_id,
    const mathfu::AffineTransform* transforms, int num_transforms) {
  LOG(DFATAL) << "Deprecated.";
}

void RenderSystemNext::SetPano(Entity entity, const std::string& filename,
                               float heading_offset_deg) {
  LOG(DFATAL) << "Deprecated.";
}

void RenderSystemNext::SetText(Entity e, const std::string& text) {
  LOG(DFATAL) << "Deprecated.";
}

void RenderSystemNext::PreloadFont(const char* name) {
  LOG(DFATAL) << "Deprecated.";
}

Optional<HashValue> RenderSystemNext::GetGroupId(Entity entity) const {
  // Does nothing.
  return Optional<HashValue>();
}

void RenderSystemNext::SetGroupId(Entity entity,
                                  const Optional<HashValue>& group_id) {
  // Does nothing.
}

const RenderSystem::GroupParams* RenderSystemNext::GetGroupParams(
    HashValue group_id) const {
  // Does nothing.
  return nullptr;
}

void RenderSystemNext::SetGroupParams(
    HashValue group_id, const RenderSystem::GroupParams& group_params) {
  // Does nothing.
}

ShaderCreateParams RenderSystemNext::CreateShaderParams(
    const std::string& shading_model, const RenderComponent* component,
    const std::shared_ptr<Material>& material) {
  ShaderCreateParams params;
  params.shading_model.resize(shading_model.size());
  std::transform(shading_model.begin(), shading_model.end(),
                 params.shading_model.begin(), ::tolower);
  if (component) {
    if (component->mesh) {
      params.environment = CreateEnvironmentFlagsFromVertexFormat(
          component->mesh->GetVertexFormat());
      params.features = CreateFeatureFlagsFromVertexFormat(
          component->mesh->GetVertexFormat());
    }
  }
  if (material) {
    for (int i = MaterialTextureUsage_MIN; i <= MaterialTextureUsage_MAX; ++i) {
      const MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(i);
      if (material->GetTexture(usage)) {
        params.environment.insert(texture_flag_hashes_[i]);
        params.features.insert(texture_flag_hashes_[i]);
      }
    }
  }
  params.features.insert(kFeatureHashUniformColor);
  params.environment.insert(renderer_.GetEnvironmentFlags().cbegin(),
                            renderer_.GetEnvironmentFlags().cend());

  return params;
}

}  // namespace lull
