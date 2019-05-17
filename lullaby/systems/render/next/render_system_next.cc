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

#include "lullaby/systems/render/next/render_system_next.h"

#include <inttypes.h>
#include <stdio.h>
#include <algorithm>
#include <cctype>
#include <memory>

#include "lullaby/events/render_events.h"
#include "lullaby/modules/config/config.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/render/image_decode.h"
#include "lullaby/modules/render/quad_util.h"
#include "lullaby/modules/render/vertex_format_util.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/render/detail/profiler.h"
#include "lullaby/systems/render/next/detail/glplatform.h"
#include "lullaby/systems/render/next/gl_helpers.h"
#include "lullaby/systems/render/next/render_component.h"
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
constexpr const char* kColorUniform = "color";
constexpr const char* kTextureBoundsUniform = "uv_bounds";
constexpr const char* kClampBoundsUniform = "clamp_bounds";
constexpr const char* kDefaultMaterialShaderDirectory = "shaders/";

constexpr HashValue kFeatureHashUniformColor = ConstHash("UniformColor");
constexpr HashValue kFeatureHashIBL = ConstHash("IBL");
constexpr HashValue kFeatureHashOcclusion = ConstHash("Occlusion");

bool IsSupportedUniformDimension(int dimension) {
  return (dimension == 1 || dimension == 2 || dimension == 3 ||
          dimension == 4 || dimension == 9 || dimension == 16);
}

// TODO Add more types when supported.
bool IsSupportedUniformType(ShaderDataType type) {
  return (type >= ShaderDataType_MIN && type <= ShaderDataType_Float4x4) ||
         (type == ShaderDataType_BufferObject);
}

void SetDebugUniform(Shader* shader, const char* name, const float values[4]) {
  shader->SetUniform(Hash(name), values, 4);
}

void SetUniformVec4(RenderComponent* component, HashValue pass, HashValue name,
                    const mathfu::vec4 value) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value[0]);
  const size_t num_bytes = sizeof(value);
  for (const std::shared_ptr<Material>& material : component->materials) {
    material->SetUniform(name, ShaderDataType_Float4, {bytes, num_bytes});
  }
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
  const detail::UniformData* color =
      material.GetUniformData(ConstHash("color"));
  if (color && color->GetData<float>()[3] < (1.0f - kDefaultEpsilon)) {
    return true;
  }
  return false;
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
      renderer_(init_params.gl_major_version_override),
      sort_order_manager_(registry_),
      shading_model_path_(kDefaultMaterialShaderDirectory) {
  if (init_params.enable_stereo_multiview) {
    renderer_.EnableMultiview();
  }

  mesh_factory_ = new MeshFactoryImpl(registry);
  registry->Register(std::unique_ptr<MeshFactory>(mesh_factory_));

  texture_factory_ = new TextureFactoryImpl(registry);
  registry->Register(std::unique_ptr<TextureFactory>(texture_factory_));

  animated_texture_processor_ = new AnimatedTextureProcessor(registry);
  registry->Register(
      std::unique_ptr<AnimatedTextureProcessor>(animated_texture_processor_));

  shader_factory_ = registry->Create<ShaderFactory>(registry);

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
      TexturePtr texture = GetTexture(entity, TextureUsageInfo(0));
      return texture ? static_cast<int>(texture->GetResourceId().Get()) : 0;
    });
  }
}

RenderSystemNext::~RenderSystemNext() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Render.GetTextureId");
  }
  registry_->Get<Dispatcher>()->DisconnectAll(this);
}

void RenderSystemNext::Initialize() {
  SetGpuDecodingEnabled(NextRenderer::SupportsAstc());
  InitDefaultRenderPassObjects();
}

void RenderSystemNext::SetStereoMultiviewEnabled(bool enabled) {
  if (enabled) {
    renderer_.EnableMultiview();
  } else {
    renderer_.DisableMultiview();
  }
}

ShaderPtr RenderSystemNext::LoadShader(const std::string& filename) {
  ShaderCreateParams params =
      CreateShaderParams(filename, nullptr, -1, nullptr);
  return LoadShader(params);
}

ShaderPtr RenderSystemNext::LoadShader(const ShaderCreateParams& params) {
  return shader_factory_->LoadShader(params);
}

void RenderSystemNext::Create(Entity entity, HashValue pass) {
  RenderPassObject* render_pass = GetRenderPassObject(pass);
  if (!render_pass) {
    LOG(DFATAL) << "Render pass " << pass << " does not exist!";
    return;
  }

  const RenderComponent* source_component = GetComponent(entity);

  RenderComponent* component = render_pass->components.Emplace(entity);
  if (component == nullptr) {
    return;
  }

  SetSortOrderOffset(entity, pass, 0);
  if (source_component) {
    component->default_material.CopyUniforms(
        source_component->default_material);
  }
}

void RenderSystemNext::Create(Entity entity, HashValue type, const Def* def) {
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

  RenderComponent* component = render_pass->components.Emplace(entity);
  if (!component) {
    LOG(DFATAL) << "RenderComponent for Entity " << entity << " inside pass "
                << pass_hash << " already exists.";
    return;
  }

  if (data.hidden()) {
    component->default_material.Hide();
  }

  if (data.shader()) {
    ShaderPtr shader = LoadShader(data.shader()->str());
    SetShader({entity, pass_hash}, shader);
  }

  if (data.textures()) {
    for (unsigned int i = 0; i < data.textures()->size(); ++i) {
      TextureParams params;
      params.generate_mipmaps = data.create_mips();
      TexturePtr texture = texture_factory_->LoadTexture(
          data.textures()->Get(i)->c_str(), params);
      SetTexture({entity, pass_hash}, TextureUsageInfo(i), texture);
    }
  } else if (data.texture() && data.texture()->size() > 0) {
    TextureParams params;
    params.generate_mipmaps = data.create_mips();
    TexturePtr texture =
        texture_factory_->LoadTexture(data.texture()->c_str(), params);
    SetTexture({entity, pass_hash}, TextureUsageInfo(0), texture);
  } else if (data.external_texture()) {
    SetTextureExternal({entity, pass_hash}, TextureUsageInfo(0));
  }

  if (data.color()) {
    mathfu::vec4 color;
    MathfuVec4FromFbColor(data.color(), &color);
    SetUniformVec4(component, pass_hash, Hash(kColorUniform), color);
    component->default_color = color;
  } else if (data.color_hex()) {
    mathfu::vec4 color;
    MathfuVec4FromFbColorHex(data.color_hex()->c_str(), &color);
    SetUniformVec4(component, pass_hash, Hash(kColorUniform), color);
    component->default_color = color;
  } else {
    SetUniformVec4(component, pass_hash, Hash(kColorUniform),
                   component->default_color);
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

      ShaderDataType type = FloatDimensionsToUniformType(uniform->dimension());
      const uint8_t* bytes =
          reinterpret_cast<const uint8_t*>(uniform->float_value()->data());
      const size_t num_bytes = uniform->float_value()->size() * sizeof(float);
      SetUniform({entity, pass_hash}, uniform->name()->c_str(), type,
                 {bytes, num_bytes}, uniform->count());
    }
  }
  SetSortOrderOffset(entity, pass_hash, data.sort_order_offset());
}

void RenderSystemNext::PostCreateInit(Entity entity, HashValue type,
                                      const Def* def) {
  if (type == kRenderDefHash) {
    auto& data = *ConvertDef<RenderDef>(def);

    HashValue pass_hash = RenderPassObjectEnumToHashValue(data.pass());
    if (data.mesh()) {
      MeshPtr mesh = mesh_factory_->LoadMesh(data.mesh()->c_str());
      SetMesh({entity, pass_hash}, mesh);
    } else if (data.quad()) {
      const QuadDef& quad_def = *data.quad();
      MeshData mesh_data;
      if (quad_def.has_uv()) {
        mesh_data = CreateQuadMesh<VertexPT>(
            quad_def.size_x(), quad_def.size_y(), quad_def.verts_x(),
            quad_def.verts_y(), quad_def.corner_radius(),
            quad_def.corner_verts());
      } else {
        mesh_data = CreateQuadMesh<VertexP>(
            quad_def.size_x(), quad_def.size_y(), quad_def.verts_x(),
            quad_def.verts_y(), quad_def.corner_radius(),
            quad_def.corner_verts());
      }

      MeshPtr mesh;
      if (data.shape_id()) {
        const HashValue id = Hash(data.shape_id()->c_str());
        mesh = mesh_factory_->CreateMesh(id, &mesh_data);
      } else {
        mesh = mesh_factory_->CreateMesh(&mesh_data);
      }
      SetMesh({entity, pass_hash}, mesh);
    } else if (data.text()) {
      LOG(DFATAL) << "Deprecated.";
    }
  }
}

void RenderSystemNext::Destroy(Entity entity) {
  for (auto& pass : render_passes_) {
    const detail::EntityIdPair entity_id_pair(entity, pass.first);
    pass.second.components.Destroy(entity);
    sort_order_manager_.Destroy(entity_id_pair);
  }
}

void RenderSystemNext::Destroy(Entity entity, HashValue pass) {
  const detail::EntityIdPair entity_id_pair(entity, pass);

  RenderPassObject* render_pass = FindRenderPassObject(pass);
  if (!render_pass) {
    LOG(DFATAL) << "Render pass " << pass << " does not exist!";
    return;
  }
  render_pass->components.Destroy(entity);
  sort_order_manager_.Destroy(entity_id_pair);
}


const mathfu::vec4& RenderSystemNext::GetDefaultColor(Entity entity) const {
  const RenderComponent* component = GetComponent(entity);
  if (component) {
    return component->default_color;
  }
  return mathfu::kOnes4f;
}

void RenderSystemNext::SetDefaultColor(Entity entity,
                                       const mathfu::vec4& color) {
  ForEachComponent(
      entity, [color](RenderComponent* component, HashValue pass_hash) {
        component->default_color = color;
      });
}

bool RenderSystemNext::GetColor(Entity entity, mathfu::vec4* color) const {
  return GetUniform(entity, kColorUniform, sizeof(mathfu::vec4),
                    reinterpret_cast<uint8_t*>(color));
}

void RenderSystemNext::SetColor(Entity entity, const mathfu::vec4& color) {
  SetUniform(entity, kColorUniform, ShaderDataType_Float4,
             {reinterpret_cast<const uint8_t*>(&color[0]), sizeof(color)});
}

void RenderSystemNext::SetUniform(const Drawable& drawable, string_view name,
                                  ShaderDataType type, Span<uint8_t> data,
                                  int count) {
  const int material_index = drawable.index ? *drawable.index : -1;
  if (drawable.pass) {
    RenderComponent* component = GetComponent(drawable.entity, *drawable.pass);
    if (component) {
      SetUniformImpl(component, material_index, name, type, data, count);
    }
  } else {
    ForEachComponent(
        drawable.entity, [=](RenderComponent* component, HashValue pass) {
          SetUniformImpl(component, material_index, name, type, data, count);
        });
  }
}

bool RenderSystemNext::GetUniform(const Drawable& drawable, string_view name,
                                  size_t length, uint8_t* data_out) const {
  const int material_index = drawable.index ? *drawable.index : -1;
  const RenderComponent* component =
      drawable.pass ? GetComponent(drawable.entity, *drawable.pass)
                    : GetComponent(drawable.entity);
  return GetUniformImpl(component, material_index, name, length, data_out);
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
  if (type != ShaderDataType_BufferObject &&
      data.size() !=
          detail::UniformData::ShaderDataTypeToBytesSize(type) * count) {
    LOG(DFATAL) << "Partial uniform data is not allowed through "
                   "RenderSystem::SetUniform.";
    return;
  }

  CHECK(type == ShaderDataType_BufferObject ||
        (count ==
         data.size() / detail::UniformData::ShaderDataTypeToBytesSize(type)));

  const HashValue name_hash = Hash(name);
  if (material_index < 0) {
    for (const std::shared_ptr<Material>& material : component->materials) {
      material->SetUniform(name_hash, type, data);
    }
    component->default_material.SetUniform(name_hash, type, data);
  } else if (material_index < static_cast<int>(component->materials.size())) {
    const std::shared_ptr<Material>& material =
        component->materials[material_index];
    material->SetUniform(name_hash, type, data);
  } else {
    LOG(DFATAL);
    return;
  }

  if (component->uniform_changed_callback) {
    component->uniform_changed_callback(material_index, name, type, data,
                                        count);
  }
}

bool RenderSystemNext::GetUniformImpl(const RenderComponent* component,
                                      int material_index, string_view name,
                                      size_t length, uint8_t* data_out) const {
  if (component == nullptr) {
    return false;
  }

  const HashValue name_hash = Hash(name);
  if (material_index < 0) {
    for (const std::shared_ptr<Material>& material : component->materials) {
      if (material->ReadUniformData(name_hash, length, data_out)) {
        return true;
      }
    }
    return component->default_material.ReadUniformData(name_hash, length,
                                                       data_out);
  } else if (material_index < static_cast<int>(component->materials.size())) {
    const std::shared_ptr<Material>& material =
        component->materials[material_index];
    return material->ReadUniformData(name_hash, length, data_out);
  }
  return false;
}

void RenderSystemNext::CopyUniforms(Entity entity, Entity source) {
  LOG(WARNING) << "CopyUniforms is going to be deprecated! Do not use this.";

  RenderComponent* component = GetComponent(entity);
  const RenderComponent* source_component =
      GetComponent(source);
  if (!component || component->materials.empty() || !source_component ||
      source_component->materials.empty()) {
    return;
  }

  // TODO: Copied data may not be correct.
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

void RenderSystemNext::OnTextureLoaded(RenderComponent* component,
                                       HashValue pass, TextureUsageInfo usage,
                                       const TexturePtr& texture) {
  const Entity entity = component->GetEntity();
  const mathfu::vec4 clamp_bounds = texture->CalculateClampBounds();
  SetUniformVec4(component, pass, Hash(kClampBoundsUniform), clamp_bounds);

  if (texture && texture->GetResourceId()) {
    // Ideally, we'd actually send the usage itself as part of the
    // TextureReadyEvent, but we need the texture unit for backwards
    // compatibility. We assume the unit is the same as the 0th usage. This is
    // the reverse of the what the unit-to-usage constructor does for
    // TextureUsageInfo.
    static const int unit = usage.GetChannelUsage(0);
    SendEvent(registry_, entity, TextureReadyEvent(entity, unit));
    if (IsReadyToRenderImpl(*component)) {
      SendEvent(registry_, entity, ReadyToRenderEvent(entity, pass));
    }
  }
}

void RenderSystemNext::SetTexture(const Drawable& drawable,
                                  TextureUsageInfo usage,
                                  const TexturePtr& texture) {
  ForEachMaterial(drawable, [=](Material* material) {
    material->SetTexture(usage, texture);
  });

  if (texture) {
    auto callback = [=]() {
      ForEachComponent(drawable,
                       [&](RenderComponent* component, HashValue pass) {
                         OnTextureLoaded(component, pass, usage, texture);
                       });
    };
    texture->AddOrInvokeOnLoadCallback(callback);
  }
}

void RenderSystemNext::SetTextureId(const Drawable& drawable,
                                    TextureUsageInfo usage,
                                    uint32_t texture_target,
                                    uint32_t texture_id) {
  auto texture = texture_factory_->CreateTexture(texture_target, texture_id);
  SetTexture(drawable, usage, texture);
}

void RenderSystemNext::SetTextureExternal(const Drawable& drawable,
                                          TextureUsageInfo usage) {
  const TexturePtr texture = texture_factory_->CreateExternalTexture();
  SetTexture(drawable, usage, texture);
}

TexturePtr RenderSystemNext::GetTexture(const Drawable& drawable,
                                        TextureUsageInfo usage) const {
  const Material* material = GetMaterial(drawable);
  return material ? material->GetTexture(usage) : nullptr;
}

void RenderSystemNext::SetMesh(const Drawable& drawable,
                               const MeshPtr& mesh) {
  ForEachComponent(drawable, [&](RenderComponent* component, HashValue pass) {
    SetMeshImpl(drawable.entity, pass, mesh);
  });
}

void RenderSystemNext::SetMesh(const Drawable& drawable, const MeshData& mesh) {
  SetMesh(drawable, mesh_factory_->CreateMesh(&mesh));
}

MeshPtr RenderSystemNext::GetMesh(const Drawable& drawable) {
  auto* component = GetComponent(drawable);
  return component ? component->mesh : nullptr;
}

RenderSortOrder RenderSystemNext::GetSortOrder(Entity entity) const {
  const auto* component = GetComponent(entity);
  return component ? component->sort_order : 0;
}

RenderSortOrderOffset RenderSystemNext::GetSortOrderOffset(
    Entity entity) const {
  return sort_order_manager_.GetOffset(entity);
}

void RenderSystemNext::SetSortOrderOffset(
    Entity entity, RenderSortOrderOffset sort_order_offset) {
  bool found_component = false;
  ForEachComponent(
      entity, [this, entity, sort_order_offset, &found_component](
                  RenderComponent* render_component, HashValue pass) {
        SetSortOrderOffset(entity, pass, sort_order_offset);
        found_component = true;
      });
  if (!found_component) {
    // NOTE: If you see this error, you're calling this function on an entity
    // with no render components, which is a null operation.  If you want to
    // change the sort order of child entities, you need to use the version of
    // SetSortOrderOffset that takes a pass, using the pass that the child's
    // RenderComponent is in.
    LOG(WARNING) << "No RenderComponent found in SetSortOrderOffset.";
  }
}

void RenderSystemNext::SetSortOrderOffset(
    Entity entity, HashValue pass, RenderSortOrderOffset sort_order_offset) {
  const detail::EntityIdPair entity_id_pair(entity, pass);
  sort_order_manager_.SetOffset(entity_id_pair, sort_order_offset);
  sort_order_manager_.UpdateSortOrder(
      entity_id_pair, [this](detail::EntityIdPair entity_id_pair) {
        return GetComponent(entity_id_pair.entity, entity_id_pair.id);
      });
}

bool RenderSystemNext::IsReadyToRender(const Drawable& drawable) const {
  const RenderComponent* component = GetComponent(drawable);
  // No component, no textures, no geometry, no problem.
  return component ? IsReadyToRenderImpl(*component) : true;
}

bool RenderSystemNext::IsReadyToRenderImpl(
    const RenderComponent& component) const {
  if (component.mesh && !component.mesh->IsLoaded()) {
    return false;
  }

  for (const auto& material : component.materials) {
    if (!material->IsLoaded()) {
      return false;
    }
  }
  return true;
}

bool RenderSystemNext::IsHidden(const Drawable& drawable) const {
  bool is_hidden = true;
  ForEachMaterial(drawable, [&](const Material& material) {
    is_hidden &= material.IsHidden();
  });
  return is_hidden;
}

ShaderPtr RenderSystemNext::GetShader(const Drawable& drawable) const {
  const Material* material = GetMaterial(drawable);
  return material ? material->GetShader() : nullptr;
}

void RenderSystemNext::SetShader(const Drawable& drawable,
                                 const ShaderPtr& shader) {
  auto set_shader = [&](RenderComponent* render_component, HashValue) {
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
  };

  if (!drawable.pass) {
    ForEachComponent(drawable.entity, set_shader);
  } else {
    auto* render_component = GetComponent(drawable);
    if (render_component) {
      set_shader(render_component, *drawable.pass);
    }
  }
}

void RenderSystemNext::SetShadingModelPath(const std::string& path) {
  shading_model_path_ = path;
}

std::shared_ptr<Material> RenderSystemNext::CreateMaterialFromMaterialInfo(
    RenderComponent* render_component, int submesh_index,
    const MaterialInfo& info) {
  std::shared_ptr<Material> material =
      std::make_shared<Material>(render_component->default_material);

  for (const auto& pair : info.GetTextureInfos()) {
    const TextureUsageInfo& usage_info = pair.first;
    const std::string& name = pair.second;
    // TODO:  Propagate texture def params properly.
    TextureParams params;
    params.generate_mipmaps = true;
    TexturePtr texture = texture_factory_->LoadTexture(name, params);
    material->SetTexture(usage_info, texture);
  }

  material->ApplyProperties(info.GetProperties());

  const std::string shading_model =
      shading_model_path_ + info.GetShadingModel();
  ShaderPtr shader;
  if (render_component->mesh && render_component->mesh->IsLoaded()) {
    const ShaderCreateParams shader_params =
        CreateShaderParams(shading_model, render_component, submesh_index,
                           material);
    shader = shader_factory_->LoadShader(shader_params);
  } else {
    shader = std::make_shared<Shader>(ShaderDescription(shading_model));
  }
  material->SetShader(shader);
  return material;
}

void RenderSystemNext::SetMaterial(const Drawable& drawable,
                                   const MaterialInfo& info) {
  ForEachComponent(
      drawable, [&](RenderComponent* component, HashValue pass_hash) {
        if (drawable.index) {
          SetMaterialImpl(component, pass_hash, *drawable.index, info);
        } else {
          SetMaterialImpl(component, pass_hash, info);
        }
      });
}

void RenderSystemNext::SetMaterialImpl(RenderComponent* render_component,
                                       HashValue pass_hash,
                                       const MaterialInfo& material_info) {
  if (render_component->materials.empty()) {
    render_component->default_material =
        *CreateMaterialFromMaterialInfo(render_component, 0, material_info);
  } else {
    for (int i = 0; i < render_component->materials.size(); ++i) {
      SetMaterialImpl(render_component, pass_hash, i, material_info);
    }
  }
}

void RenderSystemNext::SetMaterialImpl(RenderComponent* render_component,
                                       HashValue pass_hash, int submesh_index,
                                       const MaterialInfo& material_info) {
  std::shared_ptr<Material> material =
      CreateMaterialFromMaterialInfo(render_component, submesh_index,
                                     material_info);

  if (render_component->materials.size() <=
      static_cast<size_t>(submesh_index)) {
    render_component->materials.resize(submesh_index + 1);
  }
  render_component->materials[submesh_index] = material;

  for (int i = MaterialTextureUsage_MIN; i <= MaterialTextureUsage_MAX; ++i) {
    const TextureUsageInfo usage = TextureUsageInfo(i);
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
            OnTextureLoaded(render_component, pass_hash, usage, texture);
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

  for (int i = 0; i < render_component->materials.size(); ++i) {
    auto& material = render_component->materials[i];
    RebuildShader(render_component, i, material);
  }

  if (IsReadyToRenderImpl(*render_component)) {
    SendEvent(registry_, entity, ReadyToRenderEvent(entity, pass));
  }
}

void RenderSystemNext::SetMeshImpl(Entity entity, HashValue pass,
                                   const MeshPtr& mesh) {
  auto* render_component = GetComponent(entity, pass);
  if (!render_component) {
    LOG(WARNING) << "Cannot find component to set mesh";
    LOG(WARNING) << "  Entity: " << entity << ", Pass:  " << pass;
    return;
  }

  render_component->mesh = mesh;

  if (render_component->mesh) {
    MeshPtr callback_mesh = render_component->mesh;
    auto callback = [this, entity, pass, callback_mesh]() {
      // Re-acquire the component because this function can get called after
      // the component has been moved to a new memory address.
      RenderComponent* render_component = GetComponent(entity, pass);
      if (render_component && render_component->mesh == callback_mesh) {
        OnMeshLoaded(render_component, pass);
      }
    };
    render_component->mesh->AddOrInvokeOnLoadCallback(callback);
  }
  SendEvent(registry_, entity, MeshChangedEvent(entity, pass));
}

void RenderSystemNext::SetStencilMode(Entity entity, RenderStencilMode mode,
                                      int value) {
  ForEachComponent(
      entity, [this, entity, mode, value](RenderComponent* render_component,
                                          HashValue pass) {
        SetStencilMode(entity, pass, mode, value);
      });
}

void RenderSystemNext::SetStencilMode(Entity entity, HashValue pass,
                                      RenderStencilMode mode, int value) {
  // Stencil mask setting all the bits to be 1.
  static constexpr uint32_t kStencilMaskAllBits = ~0;

  auto* render_component = GetComponent(entity, pass);
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

void RenderSystemNext::Hide(const Drawable& drawable) {
  ForEachMaterial(drawable, [&](Material* material) {
    material->Hide();
  }, false);

  // For wholesale Hide, we trigger an event.
  if (drawable.entity && !drawable.pass && !drawable.index) {
    SendEvent(registry_, drawable.entity, HiddenEvent(drawable.entity));
  }
}

void RenderSystemNext::Show(const Drawable& drawable) {
  ForEachMaterial(drawable, [&](Material* material) {
    material->Show();
  }, false);

  // For wholesale Show, we trigger an event.
  if (drawable.entity && !drawable.pass && !drawable.index) {
    SendEvent(registry_, drawable.entity, UnhiddenEvent(drawable.entity));
  }
}

std::vector<HashValue> RenderSystemNext::GetRenderPasses(Entity entity) const {
  std::vector<HashValue> passes;
  ForEachComponent(
      entity, [&passes](const RenderComponent& /*component*/, HashValue pass) {
        passes.emplace_back(pass);
      });
  return passes;
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

void RenderSystemNext::SetCullMode(HashValue pass, RenderCullMode mode) {
  RenderPassObject* render_pass_object = GetRenderPassObject(pass);
  if (render_pass_object) {
    render_pass_object->cull_mode = mode;
  }
}

void RenderSystemNext::SetDefaultFrontFace(RenderFrontFace face) {
  CHECK(render_passes_.empty())
      << "Must set the default FrontFace before Initializing passes.";
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
  LULLABY_CPU_TRACE("SetViewport");
  SetViewport(mathfu::recti(view.viewport, view.dimensions));
}

mathfu::vec4 RenderSystemNext::GetClearColor() const {
  auto iter = render_passes_.find(ConstHash("ClearDisplay"));
  return iter != render_passes_.end() ? iter->second.clear_params.color_value
                                      : mathfu::kZeros4f;
}

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
      pass_container.render_target = target->second;
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
        layer.sort_mode = iter.second.sort_mode;
      }
    }
    opaque_layer.cull_mode = iter.second.cull_mode;
    blend_layer.cull_mode = iter.second.cull_mode;

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
          obj.sort_order = render_component.sort_order;

          // Add each material as a single render object where each material
          // references a submesh.
          for (size_t i = 0; i < render_component.materials.size(); ++i) {
            obj.material = render_component.materials[i];
            if (!obj.material || obj.material->IsHidden() ||
                !obj.material->IsLoaded()) {
              continue;
            }
            obj.submesh_index = static_cast<int>(i);

            const Aabb aabb = render_component.mesh->GetSubmeshAabb(i);
            obj.world_position =
                obj.world_from_entity_matrix * (aabb.min + aabb.max) * .5f;

            const RenderPassDrawContainer::LayerType type =
                ((pass_render_state.blend_state &&
                  pass_render_state.blend_state->enabled) ||
                 IsAlphaEnabled(*obj.material))
                    ? RenderPassDrawContainer::kBlendEnabled
                    : RenderPassDrawContainer::kOpaque;
            if (type == RenderPassDrawContainer::kBlendEnabled) {
              const BlendStateT* blend_state = obj.material->GetBlendState();
              if (blend_state && !blend_state->enabled) {
                // Set the blend state to null, effectively letting the layer
                // use its own blend state.
                obj.material->SetBlendState(nullptr);
              }
            }
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
                                const RenderStateT& render_state,
                                const RenderView* views, size_t num_views) {
  LULLABY_CPU_TRACE("RenderAt");
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
  shader->SetUniform(kMatNormal, normal_matrix[0].data_, 9);

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
        CalculateClipFromModelMatrix(render_object->world_from_entity_matrix,
                                     views[i].clip_from_world_matrix);
  }

  constexpr HashValue kView = ConstHash("view");
  shader->SetUniform(kView, &(view_matrix[0][0]), 16, count);

  constexpr HashValue kModelViewProjection = ConstHash("model_view_projection");
  shader->SetUniform(kModelViewProjection, &(clip_from_entity_matrix[0][0]), 16,
                     count);

  constexpr HashValue kCameraDir = ConstHash("camera_dir");
  shader->SetUniform(kCameraDir, camera_dir[0].data_, 3, count);

  constexpr HashValue kCameraPos = ConstHash("camera_pos");
  // camera_pos is not currently multiview enabled, so only set 1 value.
  shader->SetUniform(kCameraPos, camera_pos[0].data_, 3, 1);

  // We break the naming convention here for compatibility with early VR apps.
  constexpr HashValue kIsRightEye = ConstHash("uIsRightEye");
  shader->SetUniform(kIsRightEye, is_right_eye, 1, count);

  // Add subtexture coordinates so the vertex shaders will pick them up.
  // These are known when the texture is created; no need to wait for load.
  // We only do this for texture unit 0 for legacy reasons.
  auto texture = material->GetTexture(TextureUsageInfo(0));
  if (texture) {
    constexpr HashValue kUvBounds = ConstHash("uv_bounds");
    const mathfu::vec4 bounds = texture->UvBounds();
    shader->SetUniform(kUvBounds, bounds.data_, 4);
  }

  renderer_.GetRenderStateManager().SetRenderState(render_state);
  renderer_.ApplyMaterial(render_object->material);
  renderer_.Draw(mesh, render_object->world_from_entity_matrix,
                 render_object->submesh_index);

  detail::Profiler* profiler = registry_->Get<detail::Profiler>();
  if (profiler) {
    profiler->RecordDraw(material->GetShader(), mesh->GetNumVertices(),
                         mesh->GetNumPrimitives());
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
  LULLABY_CPU_TRACE_FORMAT("Render(pass=0x%08x)", pass);

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

  renderer_.Begin(draw_container.render_target.get());
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


  if (renderer_.IsMultiviewEnabled()) {
    SetViewport(views[0]);
    for (const RenderObject& obj : objects) {
      RenderAt(&obj, render_state, views, num_views);
    }
  } else {
    for (size_t i = 0; i < num_views; ++i) {
      SetViewport(views[i]);
      for (const RenderObject& obj : objects) {
        RenderAt(&obj, render_state, &views[i], 1);
      }
    }
  }
}

void RenderSystemNext::BindShader(const ShaderPtr& shader) {
  bound_shader_ = shader;
  if (shader) {
    shader->Bind();

    // Initialize color to white and allow individual materials to set it
    // differently.
    constexpr HashValue kColor = ConstHash("color");
    shader->SetUniform(kColor, &mathfu::kOnes4f[0], 4);
  }
}

void RenderSystemNext::BindTexture(int unit, const TexturePtr& texture) {
  if (unit < 0) {
    LOG(ERROR) << "BindTexture called with negative unit.";
    return;
  }

  // Ensure there is enough size in the cache.
  if (bound_textures_.size() <= unit) {
    bound_textures_.resize(unit + 1);
  }

  if (texture && texture->GetResourceId()) {
    GL_CALL(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CALL(glBindTexture(texture->GetTarget(), *texture->GetResourceId()));
  } else if (bound_textures_[unit] && bound_textures_[unit]->GetResourceId()) {
    GL_CALL(glActiveTexture(GL_TEXTURE0 + unit));
    GL_CALL(glBindTexture(bound_textures_[unit]->GetTarget(), 0));
  }
  bound_textures_[unit] = texture;
}

void RenderSystemNext::BindUniform(const char* name, const float* data,
                                   int dimension) {
  if (!IsSupportedUniformDimension(dimension)) {
    LOG(DFATAL) << "Unsupported uniform dimension " << dimension;
    return;
  }
  if (!bound_shader_) {
    LOG(DFATAL) << "Cannot bind uniform on unbound shader!";
    return;
  }
  bound_shader_->SetUniform(Hash(name), data, dimension);
}

void RenderSystemNext::DrawMesh(const MeshData& mesh,
                                Optional<mathfu::mat4> clip_from_model) {
  if (bound_shader_ && clip_from_model) {
    constexpr HashValue kModelViewProjection =
        ConstHash("model_view_projection");
    bound_shader_->SetUniform(kModelViewProjection, &(*clip_from_model)[0], 16);
  }

  renderer_.DrawMeshData(mesh);
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
    const float padding = 20.f;
    font_size = 16.f;
    start_pos =
        mathfu::vec3(padding, top - padding, -(near_z - kNearPlaneOffset));
  }

  // Setup shared render state.
  BindTexture(0, font->GetTexture());
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

void RenderSystemNext::UpdateSortOrder(Entity entity) {
  ForEachComponent(
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
  bound_shader_.reset();

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
#endif
}

void RenderSystemNext::SetDepthTest(bool enabled) {
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

void RenderSystemNext::SetDepthWrite(bool enabled) {
  fplbase::DepthState depth_state = render_state_.depth_state;
  depth_state.write_enabled = enabled;
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
  mathfu::vec3 avg_camera_pos = mathfu::kZeros3f;
  mathfu::vec3 avg_camera_dir = mathfu::kZeros3f;
  for (size_t i = 0; i < num_views; ++i) {
    avg_camera_pos += views[i].world_from_eye_matrix.TranslationVector3D();
    avg_camera_dir += CalculateCameraDirection(views[i].world_from_eye_matrix);
  }
  avg_camera_pos /= static_cast<float>(num_views);
  avg_camera_dir.Normalize();

  // Give relative values to the elements.
  for (RenderObject& obj : *objects) {
    const mathfu::vec3 world_pos = obj.world_position;

    obj.depth_sort_order =
        mathfu::vec3::DotProduct(world_pos - avg_camera_pos, avg_camera_dir);
  }

  switch (mode) {
    case SortMode_AverageSpaceOriginBackToFront:
      std::sort(objects->begin(), objects->end(),
                [&](const RenderObject& a, const RenderObject& b) {
                  return a.depth_sort_order > b.depth_sort_order;
                });
      break;

    case SortMode_AverageSpaceOriginFrontToBack:
      std::sort(objects->begin(), objects->end(),
                [&](const RenderObject& a, const RenderObject& b) {
                  return a.depth_sort_order < b.depth_sort_order;
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
  DCHECK(create_params.replace_existing ||
         render_targets_.count(render_target_name) == 0);

  // Create the render target.
  auto render_target = MakeUnique<RenderTarget>(create_params);

  // Create a bindable texture.
  TexturePtr texture = texture_factory_->CreateTexture(
      GL_TEXTURE_2D, *render_target->GetTextureId(), create_params.dimensions);
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

void RenderSystemNext::ForEachComponent(const Drawable& drawable,
                                        const OnMutableComponentFn& fn) {
  if (drawable.pass) {
    RenderComponent* component = GetComponent(drawable);
    if (component) {
      fn(component, *drawable.pass);
    }
  } else {
    for (auto& pass : render_passes_) {
      RenderComponent* component = pass.second.components.Get(drawable.entity);
      if (component) {
        fn(component, pass.first);
      }
    }
  }
}

void RenderSystemNext::ForEachComponent(const Drawable& drawable,
                                        const OnComponentFn& fn) const {
  if (drawable.pass) {
    const RenderComponent* component = GetComponent(drawable);
    if (component) {
      fn(*component, *drawable.pass);
    }
  } else {
    for (const auto& pass : render_passes_) {
      const RenderComponent* component =
          pass.second.components.Get(drawable.entity);
      if (component) {
        fn(*component, pass.first);
      }
    }
  }
}

void RenderSystemNext::ForEachMaterial(const Drawable& drawable,
                                       const OnMutableMaterialFn& fn,
                                       bool rebuild_shader) {
  ForEachComponent(drawable, [&](RenderComponent* component, HashValue pass) {
    if (!drawable.index) {
      fn(&component->default_material);
      for (int i = 0; i < component->materials.size(); ++i) {
        auto& material = component->materials[i];
        fn(material.get());
        if (rebuild_shader) {
          RebuildShader(component, i, material);
        }
      }
    } else if (*drawable.index <
               static_cast<int>(component->materials.size())) {
      auto& material = component->materials[*drawable.index];
      fn(material.get());
      if (rebuild_shader) {
        RebuildShader(component, *drawable.index, material);
      }
    } else {
      fn(&component->default_material);
    }
  });
}

void RenderSystemNext::ForEachMaterial(const Drawable& drawable,
                                       const OnMaterialFn& fn) const {
  ForEachComponent(drawable, [&](const RenderComponent& component,
                                 HashValue pass) {
    if (!drawable.index) {
      for (auto& material : component.materials) {
        fn(*material);
      }
    } else if (*drawable.index < static_cast<int>(component.materials.size())) {
      auto& material = component.materials[*drawable.index];
      fn(*material);
    } else {
      fn(component.default_material);
    }
  });
}

RenderComponent* RenderSystemNext::GetComponent(const Drawable& drawable) {
  if (drawable.pass) {
    return GetComponent(drawable.entity, *drawable.pass);
  }
  RenderComponent* component =
      GetComponent(drawable.entity, ConstHash("Opaque"));
  if (component) {
    return component;
  }
  component = GetComponent(drawable.entity, ConstHash("Main"));
  if (component) {
    return component;
  }
  for (auto& pass : render_passes_) {
    component = pass.second.components.Get(drawable.entity);
    if (component) {
      return component;
    }
  }
  return nullptr;
}

const RenderComponent* RenderSystemNext::GetComponent(
    const Drawable& drawable) const {
  if (drawable.pass) {
    return GetComponent(drawable.entity, *drawable.pass);
  }
  const RenderComponent* component =
      GetComponent(drawable.entity, ConstHash("Opaque"));
  if (component) {
    return component;
  }
  component = GetComponent(drawable.entity, ConstHash("Main"));
  if (component) {
    return component;
  }
  for (const auto& pass : render_passes_) {
    component = pass.second.components.Get(drawable.entity);
    if (component) {
      return component;
    }
  }

  return nullptr;
}

RenderComponent* RenderSystemNext::GetComponent(Entity entity, HashValue pass) {
  // TODO some apps are feeding pass=0 because they are used to
  // component id.
  if (pass == 0) {
    LOG(WARNING) << "Tried find render component by using pass = 0. Support "
                    "for this will be deprecated. Apps should identify the "
                    "correct pass the entity lives in.";
    return GetComponent(entity);
  }

  RenderPassObject* pass_object = FindRenderPassObject(pass);
  return pass_object ? pass_object->components.Get(entity) : nullptr;
}

const RenderComponent* RenderSystemNext::GetComponent(Entity entity,
                                                      HashValue pass) const {
  // TODO some apps are feeding pass=0 because they are used to
  // component id.
  if (pass == 0) {
    LOG(WARNING) << "Tried find render component by using pass = 0. Support "
                    "for this will be deprecated. Apps should identify the "
                    "correct pass the entity lives in.";
    return GetComponent(entity);
  }

  const RenderPassObject* pass_object = FindRenderPassObject(pass);
  return pass_object ? pass_object->components.Get(entity) : nullptr;
}

Material* RenderSystemNext::GetMaterial(const Drawable& drawable) {
  RenderComponent* component = GetComponent(drawable);
  if (!component) {
    return nullptr;
  }

  if (!drawable.index) {
    if (component->materials.empty()) {
      return &component->default_material;
    } else {
      return component->materials[0].get();
    }
  } else if (*drawable.index <
             static_cast<int>(component->materials.size())) {
    return component->materials[*drawable.index].get();
  } else {
    return &component->default_material;
  }
}

const Material* RenderSystemNext::GetMaterial(const Drawable& drawable) const {
  const RenderComponent* component = GetComponent(drawable);
  if (!component) {
    return nullptr;
  }

  if (!drawable.index) {
    if (component->materials.empty()) {
      return &component->default_material;
    } else {
      return component->materials[0].get();
    }
  } else if (*drawable.index <
             static_cast<int>(component->materials.size())) {
    return component->materials[*drawable.index].get();
  } else {
    return &component->default_material;
  }
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

ShaderCreateParams RenderSystemNext::CreateShaderParams(
    const std::string& shading_model, const RenderComponent* component,
    int submesh_index, const std::shared_ptr<Material>& material) const {
  ShaderCreateParams params;
  params.selection_params.max_shader_version = NextRenderer::MaxShaderVersion();
  params.shading_model.resize(shading_model.size());
  params.selection_params.lang = GetShaderLanguage();
  std::transform(shading_model.begin(), shading_model.end(),
                 params.shading_model.begin(), ::tolower);
  if (component) {
    if (component->mesh && submesh_index >= 0) {
      const VertexFormat& vertex_format =
          component->mesh->GetVertexFormat(submesh_index);
      SetFeatureFlags(vertex_format, &params.selection_params.features);
      SetEnvironmentFlags(
          vertex_format, &params.selection_params.environment);
    }
  }
  if (material) {
    material->AddEnvironmentFlags(&params.selection_params.environment);
    material->AddFeatureFlags(&params.selection_params.features);
  }
  // (b/110715643): These should come through user API.
  params.selection_params.features.insert(kFeatureHashUniformColor);
  params.selection_params.features.insert(kFeatureHashIBL);
  params.selection_params.features.insert(kFeatureHashOcclusion);
  params.selection_params.environment.insert(
      renderer_.GetEnvironmentFlags().cbegin(),
      renderer_.GetEnvironmentFlags().cend());

  return params;
}

std::string RenderSystemNext::GetShaderString(Entity entity, HashValue pass,
                                              int submesh_index,
                                              ShaderStageType stage) const {
  const RenderComponent* component = GetComponent(entity, pass);
  if (!component || submesh_index < 0 ||
      submesh_index >= component->materials.size()) {
    LOG(INFO) << " No entity such entity submesh.";
    return "";
  }
  const std::shared_ptr<Material>& material =
      component->materials[submesh_index];

  ShaderPtr shader = material->GetShader();
  if (!shader) {
    LOG(INFO) << " No shader.";
    return "";
  }
  if (shader->GetDescription().shading_model.empty()) {
    LOG(INFO) << " No shading model.";
    return "";
  }

  const ShaderCreateParams shader_params = CreateShaderParams(
      shader->GetDescription().shading_model, component, submesh_index,
      material);
  return shader_factory_->GetShaderString(shader_params, stage);
}

ShaderPtr RenderSystemNext::CompileShaderString(
    const std::string& vertex_string, const std::string& fragment_string) {
  return shader_factory_->CompileShader(vertex_string, fragment_string);
}

bool RenderSystemNext::IsShaderFeatureRequested(const Drawable& drawable,
                                                HashValue feature) const {
  const RenderComponent* component = GetComponent(drawable);
  if (!component) {
    return 0;
  }
  if (!drawable.index ||
      *drawable.index >= static_cast<int>(component->materials.size())) {
    return component->default_material.IsShaderFeatureRequested(feature);
  }
  return component->materials[*drawable.index]->IsShaderFeatureRequested(
      feature);
}

void RenderSystemNext::RequestShaderFeature(const Drawable& drawable,
                                            HashValue feature) {
  ForEachMaterial(drawable, [&](Material* material) {
    material->RequestShaderFeature(feature);
  });
}

void RenderSystemNext::ClearShaderFeature(const Drawable& drawable,
                                          HashValue feature) {
  ForEachMaterial(drawable, [&](Material* material) {
    material->ClearShaderFeature(feature);
  });
}

void RenderSystemNext::RebuildShader(RenderComponent* component,
                                     int submesh_index,
                                     std::shared_ptr<Material> material) {
  if (!material || !component) {
    return;
  }

  ShaderPtr shader = material->GetShader();
  if (shader && !shader->GetDescription().shading_model.empty()) {
    const ShaderCreateParams shader_params = CreateShaderParams(
        shader->GetDescription().shading_model, component, submesh_index,
        material);
    shader = shader_factory_->LoadShader(shader_params);
    material->SetShader(shader);
  }
}
}  // namespace lull
