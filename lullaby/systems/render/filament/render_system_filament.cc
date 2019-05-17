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

#include "lullaby/systems/render/filament/render_system_filament.h"

#include "lullaby/generated/flatbuffers/material_def_generated.h"
#include "lullaby/generated/light_def_generated.h"
#include "lullaby/events/render_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/render/shader_snippets_selector.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/render/filament/filament_utils.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/flatbuffer_reader.h"
#include "lullaby/util/make_unique.h"
#include "mathfu/io.h"

namespace lull {
namespace {
constexpr HashValue kFeatureHashUniformColor = ConstHash("UniformColor");
}  // namespace

RenderSystemFilament::RenderSystemFilament(
    Registry* registry, const RenderSystem::InitParams& init_params)
    : System(registry) {
  RegisterDef<AmbientLightDefT>((RenderSystem*)nullptr);
  RegisterDef<DirectionalLightDefT>((RenderSystem*)nullptr);
  RegisterDef<EnvironmentLightDefT>((RenderSystem*)nullptr);
  RegisterDef<PointLightDefT>((RenderSystem*)nullptr);

  renderer_ = MakeUnique<Renderer>();
  filament::Engine* engine = renderer_->GetEngine();

  mesh_factory_ = new MeshFactoryImpl(registry, engine);
  registry->Register(std::unique_ptr<MeshFactory>(mesh_factory_));

  texture_factory_ = new TextureFactoryImpl(registry, engine);
  registry->Register(std::unique_ptr<TextureFactory>(texture_factory_));

  shader_factory_ = MakeUnique<ShaderFactory>(registry, engine);

  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->Connect(this, [this](const SetNativeWindowEvent& event) {
      SetNativeWindow(event.native_window);
    });
  }
}

RenderSystemFilament::~RenderSystemFilament() {
  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->DisconnectAll(this);
  }
}

void RenderSystemFilament::Initialize() {
}

void RenderSystemFilament::SetNativeWindow(void* native_window) {
  renderer_->SetNativeWindow(native_window);
}

void RenderSystemFilament::SetStereoMultiviewEnabled(bool enabled) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetDefaultFrontFace(RenderFrontFace face) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetShadingModelPath(const std::string& path) {
  shader_factory_->SetShadingModelPath(path);
}

ShaderPtr RenderSystemFilament::LoadShader(const std::string& filename) {
  return shader_factory_->CreateShader(filename);
}

void RenderSystemFilament::Create(Entity entity, HashValue pass) {
  RenderPassObject& render_pass = GetRenderPassObject(pass);
  RenderComponent* component = render_pass.components.Emplace(entity);
  if (component) {
    ResizeRenderables(component, 1);
  }
}

template <typename T>
HashValue GetPassFromDef(T value) {
  // Map legacy enum types to the corresponding hash value.
  HashValue pass = static_cast<HashValue>(value);
  if (pass == RenderPass_Pano) {
    pass = ConstHash("Pano");
  } else if (pass == RenderPass_Opaque) {
    pass = ConstHash("Opaque");
  } else if (pass == RenderPass_Main) {
    pass = ConstHash("Main");
  } else if (pass == RenderPass_OverDraw) {
    pass = ConstHash("OverDraw");
  } else if (pass == RenderPass_Debug) {
    pass = ConstHash("Debug");
  } else if (pass == RenderPass_Invisible) {
    pass = ConstHash("Invisible");
  } else if (pass == RenderPass_OverDrawGlow) {
    pass = ConstHash("OverDrawGlow");
  }
  return pass;
}

void RenderSystemFilament::Create(Entity entity, HashValue type,
                                  const Def* def) {
  if (type == ConstHash("RenderDef")) {
    auto data = ConvertDef<RenderDef>(def);

    // Map legacy enum types to the corresponding hash value.
    const HashValue pass = GetPassFromDef(data->pass());
    Create(entity, pass);

    if (data->shader() != nullptr) {
      ShaderPtr shader = LoadShader(data->shader()->str());
      SetShader({entity, pass}, shader);
    }

    if (data->color()) {
      SetColor(entity, mathfu::vec4(data->color()->r(), data->color()->g(),
                                    data->color()->b(), data->color()->a()));
    }

    if (data->mesh()) {
      LOG(FATAL) << "RenderDef mesh deprecated.";
    }
    if (data->font()) {
      LOG(FATAL) << "RenderDef font deprecated.";
    }
    if (data->text()) {
      LOG(FATAL) << "RenderDef text deprecated.";
    }
    if (data->quad()) {
      const QuadDef& quad_def = *data->quad();
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
      SetMesh({entity, pass}, mesh_data);
    }

    TextureParams params;
    params.generate_mipmaps = data->create_mips();

    if (data->external_texture()) {
      TexturePtr texture = texture_factory_->CreateExternalTexture();
      SetTexture({entity, pass}, TextureUsageInfo(0), texture);
    } else if (data->texture()) {
      TexturePtr texture =
          texture_factory_->LoadTexture(data->texture()->c_str(), params);
      SetTexture({entity, pass}, TextureUsageInfo(0), texture);
    } else if (data->textures()) {
      for (unsigned int i = 0; i < data->textures()->size(); ++i) {
        TexturePtr texture = texture_factory_->LoadTexture(
            data->textures()->Get(i)->c_str(), params);
        SetTexture({entity, pass}, TextureUsageInfo(i), texture);
      }
    }
  } else if (type == ConstHash("AmbientLightDef")) {
    LOG(DFATAL) << "Ambient light is not supported in filament. "
                   "Use environmental lighting instead.";
  } else if (type == ConstHash("DirectionalLightDef")) {
    auto data = ConvertDef<DirectionalLightDef>(def);
    const HashValue pass = GetPassFromDef(data->group());
    RenderPassObject& obj = GetRenderPassObject(pass);
    obj.sceneview->CreateLight(entity, *data);
  } else if (type == ConstHash("EnvironmentLightDef")) {
    auto data = ConvertDef<EnvironmentLightDef>(def);
    const HashValue pass = GetPassFromDef(data->group());
    RenderPassObject& obj = GetRenderPassObject(pass);
    obj.sceneview->CreateLight(entity, *data);
  } else if (type == ConstHash("PointLightDef")) {
    auto data = ConvertDef<PointLightDef>(def);
    const HashValue pass = GetPassFromDef(data->group());
    RenderPassObject& obj = GetRenderPassObject(pass);
    obj.sceneview->CreateLight(entity, *data);
  }
}

void RenderSystemFilament::PostCreateInit(Entity entity, HashValue type,
                                          const Def*) {}

void RenderSystemFilament::Destroy(Entity entity) {
  for (auto& iter : render_passes_) {
    iter.second.components.Destroy(entity);
    iter.second.sceneview->DestroyLight(entity);
  }
}

void RenderSystemFilament::Destroy(Entity entity, HashValue pass) {
  RenderPassObject* render_pass = FindRenderPassObject(pass);
  if (render_pass) {
    render_pass->components.Destroy(entity);
    render_pass->sceneview->DestroyLight(entity);
  }
}

void RenderSystemFilament::Hide(const Drawable& drawable) {
  ForEachRenderable(drawable, [&](Renderable* renderable) {
    renderable->Hide();
  });
}

void RenderSystemFilament::Show(const Drawable& drawable) {
  ForEachRenderable(drawable,
                    [&](Renderable* renderable) { renderable->Show(); });
}

bool RenderSystemFilament::IsHidden(const Drawable& drawable) const {
  bool hidden = true;
  ForEachRenderable(drawable, [&](const Renderable& renderable) {
    hidden &= renderable.IsHidden();
  });
  return hidden;
}

bool RenderSystemFilament::IsReadyToRender(const Drawable& drawable) const {
  bool ready = true;
  ForEachRenderable(drawable, [&](const Renderable& renderable) {
    ready &= renderable.IsReadyToRender();
  });
  return ready;
}

std::vector<HashValue> RenderSystemFilament::GetRenderPasses(
    Entity entity) const {
  std::vector<HashValue> result;
  for (auto& iter : render_passes_) {
    if (iter.second.components.Contains(entity)) {
      result.push_back(iter.first);
    }
  }
  return result;
}

const mathfu::vec4& RenderSystemFilament::GetDefaultColor(Entity entity) const {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
  return mathfu::kZeros4f;
}

void RenderSystemFilament::SetDefaultColor(Entity entity,
                                           const mathfu::vec4& color) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

bool RenderSystemFilament::GetColor(Entity entity, mathfu::vec4* color) const {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
  return false;
}

void RenderSystemFilament::SetColor(Entity entity, const mathfu::vec4& color) {
  ForEachRenderable(entity, [=](Renderable* renderable) {
    renderable->SetColor(color);
  });
}

void RenderSystemFilament::SetUniform(const Drawable& drawable,
                                      string_view name, ShaderDataType type,
                                      Span<uint8_t> data, int count) {
  ForEachRenderable(drawable, [=](Renderable* renderable) {
    renderable->SetUniform(Hash(name), type, data);
  });
}

bool RenderSystemFilament::GetUniform(const Drawable& drawable,
                                      string_view name, size_t length,
                                      uint8_t* data_out) const {
  const RenderComponent* component = GetRenderComponent(drawable);
  if (component == nullptr) {
    return false;
  }
  if (component->renderables.empty()) {
    return false;
  }

  const int index = drawable.index ? *drawable.index : 0;
  const RenderablePtr renderable = index < component->renderables.size()
                                       ? component->renderables[index]
                                       : component->renderables[0];
  return renderable->ReadUniformData(Hash(name), length, data_out);
}

void RenderSystemFilament::CopyUniforms(Entity entity, Entity source) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetUniformChangedCallback(
    Entity entity, HashValue pass,
    RenderSystem::UniformChangedCallback callback) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetTexture(const Drawable& drawable,
                                      TextureUsageInfo usage,
                                      const TexturePtr& texture) {
  ForEachRenderable(drawable, [&](Renderable* renderable) {
    SetTextureImpl(renderable, usage, texture);
  });
  if (texture) {
    texture->AddOrInvokeOnLoadCallback([=]() { OnTextureLoaded(drawable); });
  }
}

void RenderSystemFilament::SetTextureExternal(const Drawable& drawable,
                                              TextureUsageInfo usage) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetTextureId(const Drawable& drawable,
                                        TextureUsageInfo usage,
                                        uint32_t texture_target,
                                        uint32_t texture_id) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

TexturePtr RenderSystemFilament::GetTexture(const Drawable& drawable,
                                            TextureUsageInfo usage) const {
  TexturePtr texture;
  ForEachRenderable(drawable, [&](const Renderable& renderable) {
    if (texture == nullptr) {
      auto t = renderable.GetTexture(usage);
      if (t) {
        texture = t;
      }
    }
  });
  return texture;
}

void RenderSystemFilament::SetTextureImpl(Renderable* renderable,
                                          const TextureUsageInfo& usage_info,
                                          const TexturePtr& texture) {
  renderable->SetTexture(usage_info, texture);
  RebuildShader(renderable);
}

void RenderSystemFilament::OnTextureLoaded(const Drawable& drawable) {
  RenderComponent* component = GetRenderComponent(drawable);
  if (component == nullptr) {
    return;
  }

  if (IsReadyToRender(drawable)) {
    ReadyToRenderEvent event(drawable.entity);
    if (drawable.pass) {
      event.pass = *drawable.pass;
    }
    SendEvent(registry_, drawable.entity, event);
  }

  // TODO: Send TextureReadyEvent.
}

void RenderSystemFilament::SetMesh(const Drawable& drawable,
                                   const MeshData& mesh) {
  SetMesh(drawable, mesh_factory_->CreateMesh(mesh.CreateHeapCopy()));
}

void RenderSystemFilament::SetMesh(const Drawable& drawable,
                                   const MeshPtr& mesh) {
  SetMeshImpl(drawable, mesh);
}

void RenderSystemFilament::SetMeshImpl(const Drawable& drawable,
                                       const MeshPtr& mesh) {
  RenderComponent* component = GetRenderComponent(drawable);
  if (component == nullptr) {
    return;
  }

  if (component->mesh != mesh) {
    component->mesh = mesh;
    if (mesh) {
      mesh->AddOrInvokeOnLoadCallback([=]() { OnMeshLoaded(drawable); });
    }

    const HashValue pass = drawable.pass ? *drawable.pass : ConstHash("Opaque");
    SendEvent(registry_, drawable.entity,
              MeshChangedEvent(drawable.entity, pass));
  }
}

void RenderSystemFilament::ResizeRenderables(RenderComponent* component,
                                             size_t count) {
  component->renderables.resize(count);
  for (size_t i = 0; i < count; ++i) {
    auto& renderable = component->renderables[i];
    if (!renderable) {
      renderable = std::make_shared<Renderable>(renderer_->GetEngine());
      if (i > 0) {
        renderable->CopyFrom(*component->renderables[0]);
      }
    }
  }
}

void RenderSystemFilament::OnMeshLoaded(const Drawable& drawable) {
  RenderComponent* component = GetRenderComponent(drawable);
  if (component == nullptr) {
    return;
  }

  MeshPtr mesh = component->mesh;
  if (mesh) {
    const size_t count = mesh->GetNumSubMeshes();
    ResizeRenderables(component, count);

    for (size_t i = 0; i < count; ++i) {
      RenderablePtr renderable = component->renderables[i];
      renderable->SetGeometry(mesh, i);
      RebuildShader(renderable.get());
    }
  } else {
    for (auto& renderable : component->renderables) {
      renderable->SetGeometry(mesh, 0);
    }
  }

  if (IsReadyToRender(drawable)) {
    ReadyToRenderEvent event(drawable.entity);
    if (drawable.pass) {
      event.pass = *drawable.pass;
    }
    SendEvent(registry_, drawable.entity, event);
  }
}

MeshPtr RenderSystemFilament::GetMesh(const Drawable& drawable) {
  const RenderComponent* component = GetRenderComponent(drawable);
  return component ? component->mesh : nullptr;
}

ShaderPtr RenderSystemFilament::GetShader(const Drawable& drawable) const {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
  return nullptr;
}

void RenderSystemFilament::SetShader(const Drawable& drawable,
                                     const ShaderPtr& shader) {
  RenderComponent* component = GetRenderComponent(drawable);
  if (component) {
    SetShaderImpl(component, shader);
  }
  ForEachRenderable(drawable, [&](Renderable* renderable) {
    RebuildShader(renderable);
  });
}

void RenderSystemFilament::SetShaderImpl(RenderComponent* component,
                                         ShaderPtr shader) {
  for (auto& renderable : component->renderables) {
    renderable->SetShader(shader);
  }
}

void RenderSystemFilament::SetMaterial(const Drawable& drawable,
                                       const MaterialInfo& info) {
  ForEachRenderable(drawable, [&](Renderable* renderable) {
    SetMaterialImpl(renderable, info);
  });
}

void RenderSystemFilament::SetMaterialImpl(Renderable* renderable,
                                           const MaterialInfo& info) {
  for (const auto& pair : info.GetTextureInfos()) {
    // TODO:  Propagate texture def params properly.
    TextureParams params;
    params.generate_mipmaps = true;
    TexturePtr texture = texture_factory_->LoadTexture(pair.second, params);
    SetTextureImpl(renderable, pair.first, texture);
  }

  for (const auto& iter : info.GetProperties()) {
    const HashValue name = iter.first;
    const Variant& var = iter.second;
    const TypeId type = var.GetTypeId();
    if (type == GetTypeId<float>()) {
      auto bytes = reinterpret_cast<const uint8_t*>(var.Get<float>());
      const size_t num_bytes = sizeof(float);
      renderable->SetUniform(name, ShaderDataType_Float1, {bytes, num_bytes});
    } else if (type == GetTypeId<mathfu::vec2>()) {
      auto bytes = reinterpret_cast<const uint8_t*>(var.Get<mathfu::vec2>());
      const size_t num_bytes = sizeof(float) * 2;
      renderable->SetUniform(name, ShaderDataType_Float2, {bytes, num_bytes});
    } else if (type == GetTypeId<mathfu::vec3>()) {
      auto bytes = reinterpret_cast<const uint8_t*>(var.Get<mathfu::vec3>());
      const size_t num_bytes = sizeof(float) * 3;
      renderable->SetUniform(name, ShaderDataType_Float3, {bytes, num_bytes});
    } else if (type == GetTypeId<mathfu::vec4>()) {
      auto bytes = reinterpret_cast<const uint8_t*>(var.Get<mathfu::vec4>());
      const size_t num_bytes = sizeof(float) * 4;
      renderable->SetUniform(name, ShaderDataType_Float4, {bytes, num_bytes});
    } else if (type == GetTypeId<bool>()) {
      renderable->RequestFeature(name);
    }
  }

  ShaderPtr shader = BuildShader(info.GetShadingModel(), renderable);
  renderable->SetShader(shader);
}

ShaderPtr RenderSystemFilament::BuildShader(const std::string& shading_model,
                                            const Renderable* renderable) {
  ShaderSelectionParams params;
  params.lang = ShaderLanguage_GLSL_ES;
  params.features.insert(kFeatureHashUniformColor);
  if (renderable) {
    renderable->ReadEnvironmentFlags(&params.environment);
    renderable->ReadFeatureFlags(&params.features);
  }
  return shader_factory_->CreateShader(shading_model, params);
}

void RenderSystemFilament::RebuildShader(Renderable* renderable) {
  const std::string& shading_model = renderable->GetShadingModel();
  if (!shading_model.empty()) {
    ShaderPtr shader = BuildShader(shading_model, renderable);
    renderable->SetShader(shader);
  }
}

bool RenderSystemFilament::IsShaderFeatureRequested(const Drawable& drawable,
                                                    HashValue feature) const {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
  return false;
}

void RenderSystemFilament::RequestShaderFeature(const Drawable& drawable,
                                                HashValue feature) {
  ForEachRenderable(drawable, [&](Renderable* renderable) {
    renderable->RequestFeature(feature);
  });
}

void RenderSystemFilament::ClearShaderFeature(const Drawable& drawable,
                                              HashValue feature) {
  ForEachRenderable(drawable, [&](Renderable* renderable) {
    renderable->ClearFeature(feature);
  });
}

void RenderSystemFilament::SetStencilMode(Entity entity, RenderStencilMode mode,
                                          int value) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetStencilMode(Entity entity, HashValue pass,
                                          RenderStencilMode mode, int value) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

RenderSortOrder RenderSystemFilament::GetSortOrder(Entity entity) const {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
  return 0;
}

RenderSortOrderOffset RenderSystemFilament::GetSortOrderOffset(
    Entity entity) const {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
  return 0;
}

void RenderSystemFilament::SetSortOrderOffset(
    Entity entity, RenderSortOrderOffset sort_order_offset) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetSortOrderOffset(
    Entity entity, HashValue pass, RenderSortOrderOffset sort_order_offset) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

mathfu::vec4 RenderSystemFilament::GetClearColor() const {
  return renderer_->GetClearColor();
}

void RenderSystemFilament::SetClearColor(float r, float g, float b, float a) {
  renderer_->SetClearColor({r, g, b, a});
}

void RenderSystemFilament::SubmitRenderData() {}

void RenderSystemFilament::BeginRendering() {}

void RenderSystemFilament::EndRendering() {}

void RenderSystemFilament::Render(const RenderView* views, size_t num_views) {
  // TODO: render all passes.
  Render(views, num_views, ConstHash("Opaque"));
}

void RenderSystemFilament::Render(const RenderView* views, size_t num_views,
                                  HashValue pass) {
  RenderPassObject* render_pass = FindRenderPassObject(pass);
  if (render_pass) {
    Render(render_pass, {views, num_views});
  }
}

void RenderSystemFilament::Render(RenderPassObject* render_pass,
                                  Span<RenderView> views) {
  auto transform_system = registry_->Get<TransformSystem>();
  if (transform_system == nullptr) {
    LOG(DFATAL) << "Need transform system for rendering.";
    return;
  }

  filament::Scene* scene = render_pass->sceneview->GetScene();
  render_pass->components.ForEach([&](RenderComponent& component) {
    const mathfu::mat4* transform =
        transform_system->GetWorldFromEntityMatrix(component.GetEntity());
    for (auto& renderable : component.renderables) {
      renderable->PrepareForRendering(scene, transform);
    }
  });

  renderer_->Render(render_pass->sceneview.get(), views);
}

void RenderSystemFilament::SetClearParams(
    HashValue pass, const RenderClearParams& clear_params) {
  LOG(WARNING) << "Only clear color is implemented.";
  SetClearColor(clear_params.color_value.x, clear_params.color_value.y,
                clear_params.color_value.z, clear_params.color_value.w);
}

void RenderSystemFilament::SetDefaultRenderPass(HashValue pass) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

HashValue RenderSystemFilament::GetDefaultRenderPass() const {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
  return 0;
}

void RenderSystemFilament::SetSortMode(HashValue pass, SortMode mode) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetSortVector(HashValue pass,
                                         const mathfu::vec3& vector) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetCullMode(HashValue pass, RenderCullMode mode) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetRenderState(HashValue pass,
                                          const fplbase::RenderState& state) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::CreateRenderTarget(
    HashValue render_target_name,
    const RenderTargetCreateParams& create_params) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetRenderTarget(HashValue pass,
                                           HashValue render_target_name) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

ImageData RenderSystemFilament::GetRenderTargetData(
    HashValue render_target_name) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
  return ImageData();
}

void RenderSystemFilament::SetDepthTest(bool enabled) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetDepthWrite(bool enabled) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetViewport(const RenderView& view) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::SetBlendMode(fplbase::BlendMode blend_mode) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::BindShader(const ShaderPtr& shader) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::BindTexture(int unit, const TexturePtr& texture) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::BindUniform(const char* name, const float* data,
                                       int dimension) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

void RenderSystemFilament::DrawMesh(const MeshData&,
                                    Optional<mathfu::mat4> clip_from_model) {
  LOG(ERROR) << "Unimplemented: " << __FUNCTION__;
}

const fplbase::RenderState& RenderSystemFilament::GetCachedRenderState() const {
  LOG_ONCE(ERROR) << "Cannot access filament render state.";
  static const fplbase::RenderState kRenderState;
  return kRenderState;
}

void RenderSystemFilament::UpdateCachedRenderState(
    const fplbase::RenderState& render_state) {
  LOG_ONCE(ERROR) << "Cannot modify filament renderstate.";
}

std::string RenderSystemFilament::GetShaderString(Entity entity, HashValue pass,
                                                  int submesh_index,
                                                  ShaderStageType stage) const {
  return "";
}

ShaderPtr RenderSystemFilament::CompileShaderString(
    const std::string& vertex_string, const std::string& fragment_string) {
  return nullptr;
}

RenderSystemFilament::RenderPassObject&
RenderSystemFilament::GetRenderPassObject(HashValue pass) {
  auto iter = render_passes_.find(pass);
  if (iter != render_passes_.end()) {
    return iter->second;
  } else {
    RenderPassObject& render_pass = render_passes_[pass];
    render_pass.sceneview =
        MakeUnique<SceneView>(registry_, renderer_->GetEngine());
    return render_pass;
  }
}

RenderSystemFilament::RenderPassObject*
RenderSystemFilament::FindRenderPassObject(HashValue pass) {
  auto iter = render_passes_.find(pass);
  return iter != render_passes_.end() ? &iter->second : nullptr;
}

const RenderSystemFilament::RenderPassObject*
RenderSystemFilament::FindRenderPassObject(HashValue pass) const {
  auto iter = render_passes_.find(pass);
  return iter != render_passes_.end() ? &iter->second : nullptr;
}

void RenderSystemFilament::ForEachRenderable(const Drawable& drawable,
                                             OnMutableRenderableFn fn) {
  for (auto& pass_iter : render_passes_) {
    if (drawable.pass && *drawable.pass != pass_iter.first) {
      continue;
    }
    RenderComponent* component = pass_iter.second.components.Get(drawable.entity);
    if (component == nullptr) {
      continue;
    }
    if (drawable.index) {
      if (component->renderables.size() <= *drawable.index) {
        ResizeRenderables(component, *drawable.index + 1);
      }
      fn(component->renderables[*drawable.index].get());
    } else {
      for (auto& renderable : component->renderables) {
        fn(renderable.get());
      }
    }
  }
}

void RenderSystemFilament::ForEachRenderable(const Drawable& drawable,
                                             OnRenderableFn fn) const {
  for (auto& pass_iter : render_passes_) {
    if (drawable.pass && *drawable.pass != pass_iter.first) {
      continue;
    }
    const RenderComponent* component = pass_iter.second.components.Get(drawable.entity);
    if (component == nullptr) {
      continue;
    }
    if (drawable.index && *drawable.index < component->renderables.size()) {
      fn(*component->renderables[*drawable.index].get());
    } else {
      for (auto& renderable : component->renderables) {
        fn(*renderable.get());
      }
    }
  }
}

RenderSystemFilament::RenderComponent* RenderSystemFilament::GetRenderComponent(
    const Drawable& drawable) {
  const HashValue pass = drawable.pass ? *drawable.pass : ConstHash("Opaque");
  RenderPassObject* render_pass = FindRenderPassObject(pass);
  return render_pass ? render_pass->components.Get(drawable.entity) : nullptr;
}

const RenderSystemFilament::RenderComponent*
RenderSystemFilament::GetRenderComponent(const Drawable& drawable) const {
  const HashValue pass = drawable.pass ? *drawable.pass : ConstHash("Opaque");
  const RenderPassObject* render_pass = FindRenderPassObject(pass);
  return render_pass ? render_pass->components.Get(drawable.entity) : nullptr;
}
}  // namespace lull
