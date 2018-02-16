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
// We break the naming convention here for compatibility with early VR apps.
constexpr const char* kIsRightEyeUniform = "uIsRightEye";

bool IsSupportedUniformDimension(int dimension) {
  return (dimension == 1 || dimension == 2 || dimension == 3 ||
          dimension == 4 || dimension == 16);
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

void UpdateUniformBinding(Uniform::Description* desc, const ShaderPtr& shader) {
  if (!desc) {
    return;
  }

  if (shader) {
    const UniformHnd uniform = shader->FindUniform(desc->name.c_str());
    if (uniform) {
      desc->binding = *uniform;
      return;
    }
  }

  desc->binding = -1;
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

}  // namespace

RenderSystemNext::RenderPassObject::RenderPassObject()
    : components(kRenderPoolPageSize) {}

RenderSystemNext::RenderSystemNext(Registry* registry,
                                   const RenderSystem::InitParams& init_params)
    : System(registry),
      sort_order_manager_(registry_),
      multiview_enabled_(init_params.enable_stereo_multiview),
      clip_from_model_matrix_func_(CalculateClipFromModelMatrix) {
  renderer_.Initialize(mathfu::kZeros2i, "lull::RenderSystem");

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
    binder->RegisterMethod("lull.Render.Show", &lull::RenderSystem::Show);
    binder->RegisterMethod("lull.Render.Hide", &lull::RenderSystem::Hide);
    binder->RegisterFunction("lull.Render.GetTextureId", [this](Entity entity) {
      TexturePtr texture = GetTexture(entity, 0);
      return texture ? static_cast<int>(texture->GetResourceId().Get()) : 0;
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

void RenderSystemNext::Initialize() {
  InitDefaultRenderPassObjectes();
  initialized_ = true;
}

void RenderSystemNext::SetStereoMultiviewEnabled(bool enabled) {
  multiview_enabled_ = enabled;
}

const TexturePtr& RenderSystemNext::GetWhiteTexture() const {
  return texture_factory_->GetWhiteTexture();
}

const TexturePtr& RenderSystemNext::GetInvalidTexture() const {
  return texture_factory_->GetInvalidTexture();
}

TexturePtr RenderSystemNext::LoadTexture(const std::string& filename,
                                         bool create_mips) {
  TextureFactory::CreateParams params;
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
  TextureFactory::CreateParams params;
  params.generate_mipmaps = create_mips;
  return texture_factory_->CreateTexture(&image, params);
}

ShaderPtr RenderSystemNext::LoadShader(const std::string& filename) {
  return shader_factory_->LoadShader(filename);
}

void RenderSystemNext::Create(Entity e, HashValue pass) {
  RenderPassObject* render_pass = GetRenderPassObject(pass);
  if (!render_pass) {
    LOG(DFATAL) << "Render pass " << pass << " does not exist!";
    return;
  }

  RenderComponent* component = render_pass->components.Emplace(e);
  if (!component) {
    LOG(DFATAL) << "RenderComponent for Entity " << e << " inside pass " << pass
                << " already exists.";
    return;
  }

  SetSortOrderOffset(e, 0);
}

void RenderSystemNext::Create(Entity e, HashValue pass, HashValue pass_enum) {
  LOG(WARNING) << "DEPRECATED: use Create(entity, pass) instead!";
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
      TextureFactory::CreateParams params;
      params.generate_mipmaps = data.create_mips();
      TexturePtr texture = texture_factory_->LoadTexture(
          data.textures()->Get(i)->c_str(), params);
      SetTexture(e, pass_hash, i, texture);
    }
  } else if (data.texture() && data.texture()->size() > 0) {
    TextureFactory::CreateParams params;
    params.generate_mipmaps = data.create_mips();
    TexturePtr texture =
        texture_factory_->LoadTexture(data.texture()->c_str(), params);
    SetTexture(e, pass_hash, 0, texture);
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
                            GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR));
    SetTextureId(e, pass_hash, 0, GL_TEXTURE_EXTERNAL_OES, texture_id);
#else
    LOG(DFATAL) << "External textures are not available.";
#endif  // GL_TEXTURE_EXTERNAL_OES
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

      SetQuad(e, pass_hash, quad);
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

void RenderSystemNext::SetUniform(Entity e, const char* name, const float* data,
                                  int dimension, int count) {
  ForEachComponentOfEntity(
      e, [this, e, name, data, dimension, count](
             RenderComponent* render_component, HashValue pass) {
        SetUniform(e, pass, name, data, dimension, count);
      });
}

void RenderSystemNext::SetUniform(Entity e, HashValue pass, const char* name,
                                  const float* data, int dimension, int count) {
  if (!IsSupportedUniformDimension(dimension)) {
    LOG(DFATAL) << "Unsupported uniform dimension " << dimension;
    return;
  }

  auto* render_component = GetComponent(e, pass);
  if (!render_component) {
    return;
  }

  const size_t num_bytes = dimension * count * sizeof(float);
  for (auto& material_ptr : render_component->materials) {
    Material& material = *material_ptr;
    Uniform* uniform = material.GetUniformByName(name);
    if (!uniform || uniform->GetDescription().num_bytes != num_bytes) {
      Uniform::Description desc(
          name,
          (dimension > 4) ? Uniform::Type::kMatrix : Uniform::Type::kFloats,
          num_bytes, count);
      if (!uniform) {
        material.AddUniform(desc);
      } else {
        material.UpdateUniform(desc);
      }
    }

    material.SetUniformByName(name, data, dimension * count);
    uniform = material.GetUniformByName(name);
    if (uniform && uniform->GetDescription().binding == -1) {
      UpdateUniformBinding(&uniform->GetDescription(), material.GetShader());
    }
  }
}

bool RenderSystemNext::GetUniform(const RenderComponent* render_component,
                                  const char* name, size_t length,
                                  float* data_out) const {
  if (!render_component || render_component->materials.empty()) {
    return false;
  }

  // TODO(b/68673841): Deprecate this function and add an index to refer to a
  // specific material.
  const Uniform* uniform =
      render_component->materials[0]->GetUniformByName(name);
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

bool RenderSystemNext::GetUniform(Entity e, const char* name, size_t length,
                                  float* data_out) const {
  return GetUniform(FindRenderComponentForEntity(e), name, length, data_out);
}

bool RenderSystemNext::GetUniform(Entity e, HashValue pass, const char* name,
                                  size_t length, float* data_out) const {
  auto* render_component = GetComponent(e, pass);
  return GetUniform(render_component, name, length, data_out);
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
    destination_material.ClearUniforms();

    const Material& source_material = copy_per_material
                                          ? *source_component->materials[index]
                                          : *source_component->materials[0];
    const std::vector<Uniform>& source_uniforms = source_material.GetUniforms();
    for (auto& uniform : source_uniforms) {
      destination_material.AddUniform(uniform);
    }

    if (destination_material.GetShader() != source_material.GetShader()) {
      // Fix the locations using |entity|'s shader.
      UpdateUniformLocations(&destination_material);
    }
  }
}

void RenderSystemNext::UpdateUniformLocations(Material* material) {
  if (!material || !material->GetShader()) {
    return;
  }

  auto& uniforms = material->GetUniforms();
  for (Uniform& uniform : uniforms) {
    UpdateUniformBinding(&uniform.GetDescription(), material->GetShader());
  }
}

void RenderSystemNext::OnTextureLoaded(const RenderComponent& component,
                                       int unit, const TexturePtr& texture) {
  const Entity entity = component.GetEntity();
  const mathfu::vec4 clamp_bounds = texture->CalculateClampBounds();
  SetUniform(entity, kClampBoundsUniform, &clamp_bounds[0], 4, 1);

  if (texture && texture->GetResourceId()) {
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

  for (auto& material : render_component->materials) {
    material->SetTexture(unit, texture);
  }
  max_texture_unit_ = std::max(max_texture_unit_, unit);

  // Add subtexture coordinates so the vertex shaders will pick them up.  These
  // are known when the texture is created; no need to wait for load.
  SetUniform(e, pass, kTextureBoundsUniform, &texture->UvBounds()[0], 4, 1);

  if (texture->IsLoaded()) {
    OnTextureLoaded(*render_component, unit, texture);
  } else {
    texture->AddOnLoadCallback([this, unit, e, pass, texture]() {
      RenderComponent* render_component = GetComponent(e, pass);
      if (render_component) {
        for (auto& material : render_component->materials) {
          if (material->GetTexture(unit) == texture) {
            OnTextureLoaded(*render_component, unit, texture);
          }
        }
      }
    });
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

  TextureFactory::CreateParams params;
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
  renderer_.SetViewport(fplbase::Viewport(mathfu::kZeros2i, size));

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
  return render_component->materials[0]->GetTexture(unit);
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
        SetQuad(e, pass, quad);
      });
}

void RenderSystemNext::SetQuad(Entity e, HashValue pass,
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
  return render_component->materials[0]->GetTexture(unit) != nullptr;
}

bool RenderSystemNext::IsTextureLoaded(Entity e, int unit) const {
  const auto* render_component = FindRenderComponentForEntity(e);
  if (!render_component || render_component->materials.empty()) {
    return false;
  }

  if (!render_component->materials[0]->GetTexture(unit)) {
    return false;
  }
  return render_component->materials[0]->GetTexture(unit)->IsLoaded();
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

bool RenderSystemNext::IsReadyToRenderImpl(
    const RenderComponent& component) const {
  if (component.mesh && !component.mesh->IsLoaded()) {
    return false;
  }

  for (const auto& material : component.materials) {
    const auto& textures = material->GetTextures();
    for (const auto& pair : textures) {
      const TexturePtr& texture = pair.second;
      if (!texture->IsLoaded()) {
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
  if (render_component->materials.empty()) {
    render_component->materials.resize(1);
  }

  for (auto& material : render_component->materials) {
    if (material.get() == nullptr) {
      material = std::make_shared<Material>();
    }
    material->SetShader(shader);

    // Update the uniforms' locations in the new shader.
    UpdateUniformLocations(material.get());
  }
}

static void SetUniformsFromMaterialInfo(Material* material,
                                        const MaterialInfo& info) {
  if (!material) {
    return;
  }
  ShaderPtr shader = material->GetShader();
  if (!shader) {
    return;
  }

  const Shader::Description& shader_description = shader->GetDescription();
  for (const ShaderUniformDefT& uniform_def : shader_description.uniforms) {
    const HashValue name_hash = Hash(uniform_def.name);
    int dimension = 0;
    const float* data = nullptr;
    switch (uniform_def.type) {
      case ShaderUniformType_Float1: {
        const auto* property = info.GetProperty<float>(name_hash);
        data = property;
        dimension = 1;
        break;
      }
      case ShaderUniformType_Float2: {
        const auto* property = info.GetProperty<mathfu::vec2>(name_hash);
        data = property ? &property->x : nullptr;
        dimension = 2;
        break;
      }
      case ShaderUniformType_Float3: {
        const auto* property = info.GetProperty<mathfu::vec3>(name_hash);
        data = property ? &property->x : nullptr;
        dimension = 3;
        break;
      }
      case ShaderUniformType_Float4: {
        const auto* property = info.GetProperty<mathfu::vec3>(name_hash);
        data = property ? &property->x : nullptr;
        dimension = 4;
        break;
      }
      default:
        break;
    }
    if (data) {
      // Set uniform by name as opposed to hash because it's likely it is not
      // yet created.
      material->SetUniformByName(uniform_def.name, data, dimension, 1);
    }
  }
}

static void SetMaterialFromMaterialInfo(ShaderFactory* shader_factory,
                                        TextureFactoryImpl* texture_factory,
                                        Material* material,
                                        const MaterialInfo& info) {
  const std::string shader_file =
      "shaders/" + info.GetShadingModel() + ".fplshader";
  ShaderPtr shader = shader_factory->LoadShader(shader_file.c_str());
  material->SetShader(shader);
  SetUniformsFromMaterialInfo(material, info);

  for (int i = 0; i < MaterialTextureUsage_MAX; ++i) {
    MaterialTextureUsage usage = static_cast<MaterialTextureUsage>(i);
    const std::string& name = info.GetTexture(usage);
    if (!name.empty()) {
      TextureFactory::CreateParams params;
      params.generate_mipmaps = true;
      TexturePtr texture = texture_factory->LoadTexture(name, params);
      material->SetTexture(i, texture);
      material->SetUniformByName(kTextureBoundsUniform, &texture->UvBounds()[0],
                                 4);
    }
  }
}

void RenderSystemNext::SetMaterial(Entity e, int submesh_index,
                                   const MaterialInfo& material) {
  Material mat;
  SetMaterialFromMaterialInfo(shader_factory_, texture_factory_, &mat,
                              material);
  ForEachComponentOfEntity(
      e, [&](RenderComponent* render_component, HashValue pass_hash) {
        if (render_component->materials.size() <=
            static_cast<size_t>(submesh_index)) {
          render_component->materials.resize(submesh_index + 1);
        }
        // Copy the material into a shared pointer for each submesh. This should
        // allow the materials for submeshes to diverge if necessary.
        render_component->materials[submesh_index] =
            std::make_shared<Material>(mat);
      });
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
      copy_material = std::make_shared<Material>();
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

  auto* rig_system = registry_->Get<RigSystem>();
  const size_t num_shader_bones = mesh->TryUpdateRig(rig_system, entity);
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
    auto* dispatcher_system = registry_->Get<DispatcherSystem>();
    if (dispatcher_system) {
      dispatcher_system->Send(entity, ReadyToRenderEvent(entity));
    }
  }
}

void RenderSystemNext::SetMesh(Entity e, MeshPtr mesh) {
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
  render_pass->definition.clear_params = clear_params;
}

SortMode RenderSystemNext::GetSortMode(HashValue pass) const {
  const RenderPassObject* render_pass_object = FindRenderPassObject(pass);
  if (render_pass_object) {
    return render_pass_object->definition.sort_mode;
  }
  return SortMode_None;
}

void RenderSystemNext::SetSortMode(HashValue pass, SortMode mode) {
  RenderPassObject* render_pass_object = GetRenderPassObject(pass);
  if (render_pass_object) {
    render_pass_object->definition.sort_mode = mode;
  }
}

RenderCullMode RenderSystemNext::GetCullMode(HashValue pass) {
  const RenderPassObject* render_pass_object = FindRenderPassObject(pass);
  if (render_pass_object) {
    return render_pass_object->definition.cull_mode;
  }
  return RenderCullMode::kNone;
}

void RenderSystemNext::SetCullMode(HashValue pass, RenderCullMode mode) {
  RenderPassObject* render_pass_object = GetRenderPassObject(pass);
  if (render_pass_object) {
    render_pass_object->definition.cull_mode = mode;
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
    render_pass_object->definition.render_state = state;
  }
}

const fplbase::RenderState* RenderSystemNext::GetRenderState(
    HashValue pass) const {
  const RenderPassObject* render_pass_object = FindRenderPassObject(pass);
  if (render_pass_object) {
    return &render_pass_object->definition.render_state;
  }
  return nullptr;
}

void RenderSystemNext::SetDepthTest(const bool enabled) {
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
#endif  // !ION_PRODUCTION

    renderer_.SetDepthFunction(fplbase::kDepthFunctionLess);
    return;
  }

  renderer_.SetDepthFunction(fplbase::kDepthFunctionDisabled);
}

void RenderSystemNext::SetDepthWrite(const bool enabled) {
  renderer_.SetDepthWrite(enabled);
}

void RenderSystemNext::SetViewport(const RenderView& view) {
  LULLABY_CPU_TRACE_CALL();
  renderer_.SetViewport(fplbase::Viewport(view.viewport, view.dimensions));
  inherited_render_state_.viewport = GetCachedRenderState().viewport;
}

void RenderSystemNext::SetClipFromModelMatrix(const mathfu::mat4& mvp) {
  renderer_.set_model_view_projection(mvp);
}

void RenderSystemNext::SetClipFromModelMatrixFunction(
    const RenderSystem::ClipFromModelMatrixFn& func) {
  if (!func) {
    clip_from_model_matrix_func_ = CalculateClipFromModelMatrix;
    return;
  }

  clip_from_model_matrix_func_ = func;
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

  RenderClearParams clear_params;
  clear_params.clear_options = RenderClearParams::kColor |
                               RenderClearParams::kDepth |
                               RenderClearParams::kStencil;
  clear_params.color_value = clear_color_;
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
    pass_container.clear_params = iter.second.definition.clear_params;
    pass_container.cull_mode = iter.second.definition.cull_mode;
    pass_container.render_target = iter.second.definition.render_target;

    RenderPassDrawGroup& opaque_draw_group =
        pass_container
            .draw_groups[static_cast<size_t>(RenderObjectsBlendMode::kOpaque)];
    RenderPassDrawGroup& blend_draw_group =
        pass_container.draw_groups[static_cast<size_t>(
            RenderObjectsBlendMode::kBlendEnabled)];

    // Assign sort modes.
    if (iter.second.definition.sort_mode == SortMode_Optimized) {
      // If user set optimized, set the best expected sort modes depending on
      // the blend mode.
      opaque_draw_group.sort_mode = SortMode_AverageSpaceOriginFrontToBack;
      blend_draw_group.sort_mode = SortMode_AverageSpaceOriginBackToFront;
    } else {
      // Otherwise assign the user's chosen sort modes.
      for (auto& draw_group : pass_container.draw_groups) {
        draw_group.sort_mode = iter.second.definition.sort_mode;
      }
    }

    // Set the render states.
    // Opaque remains as is.
    const fplbase::RenderState& pass_render_state =
        iter.second.definition.render_state;
    opaque_draw_group.render_state = pass_render_state;
    // Blend requires z-write off and blend mode on.
    if (!pass_render_state.blend_state.enabled) {
      fplbase::RenderState render_state = pass_render_state;
      render_state.blend_state.enabled = true;
      render_state.blend_state.src_alpha = fplbase::BlendState::kOne;
      render_state.blend_state.src_color = fplbase::BlendState::kOne;
      render_state.blend_state.dst_alpha =
          fplbase::BlendState::kOneMinusSrcAlpha;
      render_state.blend_state.dst_color =
          fplbase::BlendState::kOneMinusSrcAlpha;
      render_state.depth_state.write_enabled = false;

      // Assign the render state.
      blend_draw_group.render_state = std::move(render_state);
    } else {
      blend_draw_group.render_state = pass_render_state;
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

          RenderObject render_obj;
          render_obj.mesh = render_component.mesh;
          render_obj.sort_order = render_component.sort_order;
          render_obj.world_from_entity_matrix = *world_from_entity_matrix;

          // Add each material as a single render object where each material
          // references a submesh.
          for (size_t i = 0; i < render_component.materials.size(); ++i) {
            render_obj.material = render_component.materials[i];
            render_obj.submesh_index = static_cast<int>(i);

            const RenderObjectsBlendMode object_blend_mode =
                (pass_render_state.blend_state.enabled ||
                 IsAlphaEnabled(*render_obj.material))
                    ? RenderObjectsBlendMode::kBlendEnabled
                    : RenderObjectsBlendMode::kOpaque;
            pass_container.draw_groups[static_cast<size_t>(object_blend_mode)]
                .render_objects.emplace_back(render_obj);
          }
        });

    // Sort only objects with "static" sort order, such as explicit sort order
    // or absolute z-position.
    for (auto& draw_group : pass_container.draw_groups) {
      if (IsSortModeViewIndependent(draw_group.sort_mode)) {
        SortObjects(&draw_group.render_objects, draw_group.sort_mode);
      }
    }
  }

  render_data_buffer_.UnlockWriteBuffer();
}

void RenderSystemNext::BeginRendering() {
  LULLABY_CPU_TRACE_CALL();

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

void RenderSystemNext::SetViewUniforms(const RenderView& view) {
  renderer_.set_camera_pos(view.world_from_eye_matrix.TranslationVector3D());

  rendering_right_eye_ = view.eye == 1;
}

void RenderSystemNext::RenderAt(const RenderObject* render_object,
                                const mathfu::mat4& world_from_entity_matrix,
                                const RenderView& view) {
  LULLABY_CPU_TRACE_CALL();
  const Material& material = *render_object->material;
  const ShaderPtr& shader = material.GetShader();
  if (!shader || !render_object->mesh) {
    return;
  }

  const mathfu::mat4 clip_from_entity_matrix = clip_from_model_matrix_func_(
      world_from_entity_matrix, view.clip_from_world_matrix);
  renderer_.set_model_view_projection(clip_from_entity_matrix);
  renderer_.set_model(world_from_entity_matrix);

  ApplyMaterial(material, inherited_render_state_);

  const UniformHnd mat_normal_uniform = shader->FindUniform("mat_normal");
  if (mat_normal_uniform) {
    // Compute the normal matrix. This is the transposed matrix of the inversed
    // world position. This is done to avoid non-uniform scaling of the normal.
    // A good explanation of this can be found here:
    // http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/
    const mathfu::mat3 normal_matrix =
        ComputeNormalMatrix(world_from_entity_matrix);
    mathfu::vec3_packed packed[3];
    normal_matrix.Pack(packed);
    GL_CALL(glUniformMatrix3fv(*mat_normal_uniform, 1, false, packed[0].data));
  }
  const UniformHnd camera_dir_uniform = shader->FindUniform("camera_dir");
  if (camera_dir_uniform) {
    mathfu::vec3_packed camera_dir;
    CalculateCameraDirection(view.world_from_eye_matrix).Pack(&camera_dir);
    GL_CALL(glUniform3fv(*camera_dir_uniform, 1, camera_dir.data));
  }
  const UniformHnd view_uniform = shader->FindUniform("view");
  if (view_uniform) {
    GL_CALL(glUniformMatrix4fv(*view_uniform, 1, false,
                               &view.eye_from_world_matrix[0]));
  }

  // Bit of magic to determine if the scalar is negative and if so flip the cull
  // face. This possibly be revised (b/38235916).
  CorrectFrontFaceFromMatrix(world_from_entity_matrix);

  DrawMeshFromComponent(render_object);
}

void RenderSystemNext::RenderAtMultiview(
    const RenderObject* render_object,
    const mathfu::mat4& world_from_entity_matrix, const RenderView* views) {
  LULLABY_CPU_TRACE_CALL();
  const Material& material = *render_object->material;
  const ShaderPtr& shader = material.GetShader();
  if (!shader || !render_object->mesh) {
    return;
  }

  const mathfu::mat4 clip_from_entity_matrix[] = {
      views[0].clip_from_world_matrix * world_from_entity_matrix,
      views[1].clip_from_world_matrix * world_from_entity_matrix,
  };
  renderer_.set_model(world_from_entity_matrix);

  ApplyMaterial(material, inherited_render_state_);

  const UniformHnd mvp_uniform = shader->FindUniform("model_view_projection");
  if (mvp_uniform) {
    GL_CALL(glUniformMatrix4fv(*mvp_uniform, 2, false,
                               &(clip_from_entity_matrix[0][0])));
  }
  const UniformHnd mat_normal_uniform = shader->FindUniform("mat_normal");
  if (mat_normal_uniform) {
    const mathfu::mat3 normal_matrix =
        ComputeNormalMatrix(world_from_entity_matrix);
    mathfu::VectorPacked<float, 3> packed[3];
    normal_matrix.Pack(packed);
    GL_CALL(glUniformMatrix3fv(*mat_normal_uniform, 1, false, packed[0].data));
  }
  const UniformHnd camera_dir_uniform = shader->FindUniform("camera_dir");
  if (camera_dir_uniform) {
    mathfu::vec3_packed camera_dir[2];
    for (size_t i = 0; i < 2; ++i) {
      CalculateCameraDirection(views[i].world_from_eye_matrix)
          .Pack(&camera_dir[i]);
    }
    GL_CALL(glUniform3fv(*camera_dir_uniform, 2, camera_dir[0].data));
  }

  // Bit of magic to determine if the scalar is negative and if so flip the cull
  // face. This possibly be revised (b/38235916).
  CorrectFrontFaceFromMatrix(world_from_entity_matrix);

  DrawMeshFromComponent(render_object);
}

void RenderSystemNext::SetShaderUniforms(const std::vector<Uniform>& uniforms) {
  for (const auto& uniform : uniforms) {
    BindUniform(uniform);
  }
}

void RenderSystemNext::DrawMeshFromComponent(
    const RenderObject* render_object) {
  if (render_object->mesh) {
    renderer_.SetBlendMode(renderer_.GetBlendMode());
    render_object->mesh->RenderSubmesh(render_object->submesh_index);
    detail::Profiler* profiler = registry_->Get<detail::Profiler>();
    if (profiler) {
      profiler->RecordDraw(render_object->material->GetShader(),
                           render_object->mesh->GetNumVertices(),
                           render_object->mesh->GetNumTriangles());
    }
  }
}

void RenderSystemNext::Render(const RenderView* views, size_t num_views) {
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

  // Set the render target, if needed.
  if (draw_container.render_target) {
    draw_container.render_target->Bind();
  }
  ApplyClearParams(draw_container.clear_params);

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

  // Draw the elements.
  if (pass == ConstHash("Debug")) {
    RenderDebugStats(views, num_views);
  } else {
    for (auto& draw_group : draw_container.draw_groups) {
      renderer_.SetRenderState(draw_group.render_state);
      inherited_render_state_ = draw_group.render_state;

      if (!IsSortModeViewIndependent(draw_group.sort_mode)) {
        SortObjectsUsingView(&draw_group.render_objects, draw_group.sort_mode,
                             views, num_views);
      }
      RenderObjects(draw_group.render_objects, views, num_views);
    }
  }

  // Set the render target back to default, if needed.
  if (draw_container.render_target) {
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
                                     const RenderView* views,
                                     size_t num_views) {
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
      const RenderView& view = views[i];

      SetViewport(view);
      SetViewUniforms(view);

      for (const RenderObject& obj : objects) {
        RenderAt(&obj, obj.world_from_entity_matrix, views[i]);
      }
    }
  }

  // Reset states that are set at the entity level in RenderAt.
  GL_CALL(glFrontFace(GL_CCW));
}

void RenderSystemNext::BindShader(const ShaderPtr& shader) {
  // Don't early exit if shader == shader_, since fplbase::Shader::Set also sets
  // the common fpl uniforms.
  shader_ = shader;
  shader->Bind();

  shader->SetUniform(shader->FindUniform(ConstHash("model_view_projection")),
                     &renderer_.model_view_projection()[0], 16, 1);
  shader->SetUniform(shader->FindUniform(ConstHash("model")),
                     &renderer_.model()[0], 16, 1);
  shader->SetUniform(shader->FindUniform(ConstHash("color")),
                     &renderer_.color()[0], 4, 1);
  shader->SetUniform(shader->FindUniform(ConstHash("light_pos")),
                     &renderer_.light_pos()[0], 3, 1);
  shader->SetUniform(shader->FindUniform(ConstHash("camera_pos")),
                     &renderer_.camera_pos()[0], 3, 1);
  const auto time = static_cast<float>(renderer_.time());
  shader->SetUniform(shader->FindUniform(ConstHash("time")), &time, 1, 1);
  if (renderer_.num_bones() > 0) {
    const int kNumVec4InBoneTransform = 3;
    shader->SetUniform(shader->FindUniform(ConstHash("bone_transforms")),
                       &renderer_.bone_transforms()[0][0], 4,
                       renderer_.num_bones() * kNumVec4InBoneTransform);
  }

  // Bind uniform describing whether or not we're rendering in the right eye.
  // This uniform is an int due to legacy reasons, but there's no pipeline in
  // FPL for setting int uniforms, so we have to make a direct gl call instead.
  const UniformHnd uniform_is_right_eye =
      shader->FindUniform(kIsRightEyeUniform);
  if (uniform_is_right_eye) {
    GL_CALL(glUniform1i(*uniform_is_right_eye,
                        static_cast<int>(rendering_right_eye_)));
  }
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
    const size_t max_indices,
    const std::function<void(MeshData*)>& update_mesh) {
  if (max_vertices > 0) {
    const MeshData::IndexType index_type = MeshData::kIndexU16;
    DataContainer vertex_data = DataContainer::CreateHeapDataContainer(
        max_vertices * vertex_format.GetVertexSize());
    DataContainer index_data = DataContainer::CreateHeapDataContainer(
        max_indices * MeshData::GetIndexSize(index_type));
    MeshData data(primitive_type, vertex_format, std::move(vertex_data),
                  index_type, std::move(index_data));
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
  return renderer_.GetRenderState();
}

void RenderSystemNext::UpdateCachedRenderState(
    const fplbase::RenderState& render_state) {
  renderer_.UpdateCachedRenderState(render_state);
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

void RenderSystemNext::SortObjects(RenderObjectVector* objects, SortMode mode) {
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

    default:
      LOG(DFATAL) << "SortObjects called with unsupported sort mode!";
      break;
  }
}

void RenderSystemNext::SortObjectsUsingView(RenderObjectVector* objects,
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

void RenderSystemNext::InitDefaultRenderPassObjectes() {
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

void RenderSystemNext::SetRenderPass(const RenderPassDefT& data) {
  const HashValue pass = Hash(data.name.c_str());
  RenderPassObject* pass_object = GetRenderPassObject(pass);
  if (!pass_object) {
    return;
  }

  RenderPassObjectDefinition& def = pass_object->definition;
  switch (data.sort_mode) {
    case SortMode_None:
      break;
    case SortMode_SortOrderDecreasing:
      def.sort_mode = SortMode_SortOrderDecreasing;
      break;
    case SortMode_SortOrderIncreasing:
      def.sort_mode = SortMode_SortOrderIncreasing;
      break;
    case SortMode_WorldSpaceZBackToFront:
      def.sort_mode = SortMode_WorldSpaceZBackToFront;
      break;
    case SortMode_WorldSpaceZFrontToBack:
      def.sort_mode = SortMode_WorldSpaceZFrontToBack;
      break;
    case SortMode_AverageSpaceOriginBackToFront:
      def.sort_mode = SortMode_AverageSpaceOriginBackToFront;
      break;
    case SortMode_AverageSpaceOriginFrontToBack:
      def.sort_mode = SortMode_AverageSpaceOriginFrontToBack;
      break;
    case SortMode_Optimized:
      break;
  }
  Apply(&def.render_state, data.render_state);
}

void RenderSystemNext::ApplyClearParams(const RenderClearParams& clear_params) {
  GLbitfield gl_clear_mask = 0;
  if (CheckBit(clear_params.clear_options, RenderClearParams::kColor)) {
    gl_clear_mask |= GL_COLOR_BUFFER_BIT;
    GL_CALL(glClearColor(clear_params.color_value.x, clear_params.color_value.y,
                         clear_params.color_value.z,
                         clear_params.color_value.w));
  }

  if (CheckBit(clear_params.clear_options, RenderClearParams::kDepth)) {
    gl_clear_mask |= GL_DEPTH_BUFFER_BIT;
#ifdef FPLBASE_GLES
    GL_CALL(glClearDepthf(clear_params.depth_value));
#else
    GL_CALL(glClearDepth(static_cast<double>(clear_params.depth_value)));
#endif
  }

  if (CheckBit(clear_params.clear_options, RenderClearParams::kStencil)) {
    gl_clear_mask |= GL_STENCIL_BUFFER_BIT;
    GL_CALL(glClearStencil(clear_params.stencil_value));
  }

  GL_CALL(glClear(gl_clear_mask));
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
  auto iter = render_targets_.find(render_target_name);
  if (iter == render_targets_.end()) {
    LOG(DFATAL) << "SetRenderTarget called with non-existent render target: "
                << render_target_name;
    return;
  }

  RenderPassObject* pass_object = GetRenderPassObject(pass);
  if (!pass_object) {
    return;
  }

  pass_object->definition.render_target = iter->second.get();
}

void RenderSystemNext::CorrectFrontFaceFromMatrix(const mathfu::mat4& matrix) {
  if (CalculateDeterminant3x3(matrix) >= 0.0f) {
    // If the scalar is positive, match the default settings.
    renderer_.SetFrontFace(GetCachedRenderState().cull_state.front);
  } else {
    // Otherwise, reverse the order.
    renderer_.SetFrontFace(static_cast<fplbase::CullState::FrontFace>(
        fplbase::CullState::kFrontFaceCount -
        GetCachedRenderState().cull_state.front - 1));
  }
}

void RenderSystemNext::BindUniform(const Uniform& uniform) {
  const Uniform::Description& desc = uniform.GetDescription();
  int binding = 0;
  if (desc.binding >= 0) {
    binding = desc.binding;
  } else {
    const UniformHnd uniform = shader_->FindUniform(desc.name.c_str());
    if (uniform) {
      binding = *uniform;
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

void RenderSystemNext::ApplyMaterial(const Material& material,
                                     fplbase::RenderState render_state) {
  const ShaderPtr& shader = material.GetShader();
  if (!shader) {
    return;
  }

  BindShader(shader);
  SetShaderUniforms(material.GetUniforms());

  const auto& textures = material.GetTextures();
  for (const auto& texture : textures) {
    texture.second->Bind(texture.first);
  }

  Apply(&render_state, material.GetBlendState());
  Apply(&render_state, material.GetCullState());
  Apply(&render_state, material.GetDepthState());
  Apply(&render_state, material.GetPointState());
  Apply(&render_state, material.GetStencilState());
  renderer_.SetRenderState(render_state);
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

void RenderSystemNext::SetFont(Entity entity, const FontPtr& font) {
  LOG(DFATAL) << "Deprecated.";
}

void RenderSystemNext::SetTextSize(Entity entity, int size) {
  LOG(DFATAL) << "Deprecated.";
}

void RenderSystemNext::RenderPanos(const RenderView* views, size_t num_views) {
  LOG(DFATAL) << "Deprecated.";
}

void RenderSystemNext::PreloadFont(const char* name) {
  LOG(DFATAL) << "Deprecated.";
}

FontPtr RenderSystemNext::LoadFonts(const std::vector<std::string>& names) {
  LOG(DFATAL) << "Deprecated.";
  return nullptr;
}

}  // namespace lull
