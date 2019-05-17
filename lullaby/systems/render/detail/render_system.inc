#include "lullaby/systems/render/render_system.h"

#include "lullaby/events/render_events.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/render/render_helpers.h"
#include "lullaby/systems/render/render_stats.h"
#include "lullaby/systems/render/texture_factory.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/color.h"
#include "lullaby/generated/pano_def_generated.h"

#if defined(LULLABY_RENDER_BACKEND_NEXT)
#define LULLABY_RENDER_BACKEND_DEPRECATED 0
#elif defined(LULLABY_RENDER_BACKEND_FILAMENT)
#define LULLABY_RENDER_BACKEND_DEPRECATED 0
#elif defined(LULLABY_RENDER_BACKEND_MOCK)
#define LULLABY_RENDER_BACKEND_DEPRECATED 1
#elif defined(LULLABY_RENDER_BACKEND_FPL)
#define LULLABY_RENDER_BACKEND_DEPRECATED 1
#elif defined(LULLABY_RENDER_BACKEND_ION)
#define LULLABY_RENDER_BACKEND_DEPRECATED 1
#else
#define LULLABY_RENDER_BACKEND_DEPRECATED 1
#endif

namespace lull {

RenderSystem::RenderSystem(Registry* registry)
    : RenderSystem(registry, InitParams()) {}

RenderSystem::RenderSystem(Registry* registry, const InitParams& init_params)
    : System(registry), impl_(new RenderSystemImpl(registry, init_params)) {
  RegisterDef<RenderDefT>(this);

  RegisterDependency<TransformSystem>(this);

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.Render.BeginFrame",
                           &lull::RenderSystem::BeginFrame);
    binder->RegisterMethod("lull.Render.EndFrame",
                           &lull::RenderSystem::EndFrame);
    binder->RegisterMethod("lull.Render.BeginRendering",
                           &lull::RenderSystem::BeginRendering);
    binder->RegisterMethod("lull.Render.EndRendering",
                           &lull::RenderSystem::EndRendering);
    binder->RegisterMethod("lull.Render.SubmitRenderData",
                           &lull::RenderSystem::SubmitRenderData);
    binder->RegisterMethod("lull.Render.ProcessTasks",
                           &lull::RenderSystem::ProcessTasks);
    binder->RegisterFunction("lull.Render.Show",
        [this](Entity entity) {
          Show(entity);
        });
    binder->RegisterFunction("lull.Render.ShowSubmesh",
        [this](Entity entity, Optional<HashValue> pass, Optional<int> idx) {
          Drawable drawable(entity);
          drawable.pass = pass;
          drawable.index = idx;
          Show(drawable);
        });
    binder->RegisterFunction("lull.Render.Hide",
        [this](Entity entity) {
          Hide(entity);
        });
    binder->RegisterFunction("lull.Render.HideSubmesh",
        [this](Entity entity, Optional<HashValue> pass, Optional<int> idx) {
          Drawable drawable(entity);
          drawable.pass = pass;
          drawable.index = idx;
          Hide(drawable);
        });
    binder->RegisterFunction(
        "lull.Render.RenderPassesWithView",
        [this](std::vector<HashValue> passes, mathfu::vec4 vp,
               mathfu::mat4 world_from_eye_matrix, mathfu::rectf fov,
               float near_clip_plane, float far_clip_plane) {
          RenderView view;
          const mathfu::vec4i vpi(vp);
          const mathfu::recti viewport(vpi);
          PopulateRenderView(&view, viewport, world_from_eye_matrix, fov,
                             near_clip_plane, far_clip_plane,
                             static_cast<InputManager::EyeType>(0));
          for (const HashValue pass : passes) {
            Render(&view, 1, pass);
          }
        });
    binder->RegisterFunction(
        "lull.Render.SetClearParams",
        [this](HashValue pass, int clear_options, mathfu::vec4 color_value,
               float depth_value, int stencil_value) {
          RenderClearParams clear_params;
          clear_params.clear_options = static_cast<Bits>(clear_options);
          clear_params.color_value = color_value;
          clear_params.depth_value = depth_value;
          clear_params.stencil_value = stencil_value;
          SetClearParams(pass, clear_params);
        });
    binder->RegisterFunction(
        "lull.Render.SetQuad",
        [this](Entity entity, const mathfu::vec2& size,
               const mathfu::vec2i& verts, float corner_radius,
               int corner_verts, bool has_uv, int corner_mask) {
          RenderQuad quad;
          quad.size = size;
          quad.verts = verts;
          quad.corner_radius = corner_radius;
          quad.corner_verts = corner_verts;
          quad.has_uv = has_uv;
          quad.corner_mask = static_cast<CornerMask>(corner_mask);
          SetQuad(entity, quad);
        });

    // Expose enums for use in scripts.  These are functions you will need to
    // call (with parentheses)
    binder->RegisterFunction("lull.Render.CornerMask.None", []() {
      return static_cast<int>(CornerMask::kNone);
    });
    binder->RegisterFunction("lull.Render.CornerMask.TopRight", []() {
      return static_cast<int>(CornerMask::kTopRight);
    });
    binder->RegisterFunction("lull.Render.CornerMask.BottomRight", []() {
      return static_cast<int>(CornerMask::kBottomRight);
    });
    binder->RegisterFunction("lull.Render.CornerMask.BottomLeft", []() {
      return static_cast<int>(CornerMask::kBottomLeft);
    });
    binder->RegisterFunction("lull.Render.CornerMask.TopLeft", []() {
      return static_cast<int>(CornerMask::kTopLeft);
    });
    binder->RegisterFunction("lull.Render.CornerMask.All", []() {
      return static_cast<int>(CornerMask::kAll);
    });
    binder->RegisterFunction(
        "lull.Render.SetUniform1f",
        [this](Entity entity, const std::string& name, float x) {
          SetUniform(entity, name, {&x, 1});
        });
    binder->RegisterFunction(
        "lull.Render.SetUniform2f",
        [this](Entity entity, const std::string& name, float x, float y) {
          const mathfu::vec2 v2(x, y);
          SetUniform(entity, name, {&v2.x, 2});
        });
    binder->RegisterFunction("lull.Render.SetUniform3f",
                             [this](Entity entity, const std::string& name,
                                    float x, float y, float z) {
                               const mathfu::vec3 v3(x, y, z);
                               SetUniform(entity, name, {&v3.x, 3});
                             });
    binder->RegisterFunction("lull.Render.SetUniform4f",
                             [this](Entity entity, const std::string& name,
                                    float x, float y, float z, float w) {
                               const mathfu::vec4 v4(x, y, z, w);
                               SetUniform(entity, name, {&v4.x, 4});
                             });
    binder->RegisterFunction(
        "lull.Render.SetSubmeshUniform1f",
        [this](Entity entity, int submesh, const std::string& name, float x) {
          SetUniform({entity, DrawableIndex(submesh)}, name, ShaderDataType_Float1,
                     {reinterpret_cast<const uint8_t*>(&x), sizeof(float)});
        });
    binder->RegisterFunction(
        "lull.Render.SetSubmeshUniform2f",
        [this](Entity entity, int submesh, const std::string& name,
               float x, float y) {
          const mathfu::vec2 v2(x, y);
          SetUniform({entity, DrawableIndex(submesh)}, name, ShaderDataType_Float2,
                     {reinterpret_cast<const uint8_t*>(&v2.x), 2 * sizeof(float)});
        });
    binder->RegisterFunction(
        "lull.Render.SetSubmeshUniform3f",
        [this](Entity entity, int submesh, const std::string& name,
               float x, float y, float z) {
          const mathfu::vec3 v3(x, y, z);
          SetUniform({entity, DrawableIndex(submesh)}, name, ShaderDataType_Float3,
                     {reinterpret_cast<const uint8_t*>(&v3.x), 3 * sizeof(float)});
        });
    binder->RegisterFunction(
        "lull.Render.SetSubmeshUniform4f",
        [this](Entity entity, int submesh, const std::string& name,
               float x, float y, float z, float w) {
          const mathfu::vec4 v4(x, y, z, w);
          SetUniform({entity, DrawableIndex(submesh)}, name, ShaderDataType_Float4,
                     {reinterpret_cast<const uint8_t*>(&v4.x), 4 * sizeof(float)});
        });
    binder->RegisterFunction(
        "lull.Render.SetUniforms",
        [this](Entity entity, Optional<HashValue> pass, Optional<int> index,
               std::string name, int type, const VariantArray& data,
               int count) {
          // TODO support primitive arrays in jni_convert.
          std::vector<float> floats;
          floats.reserve(data.size());
          for (const auto& variant : data) {
            floats.emplace_back(variant.ValueOr(0.f));
          }
          auto bytes = reinterpret_cast<const uint8_t*>(floats.data());
          const size_t num_bytes = floats.size() * sizeof(float);
          SetUniform({entity, pass, index}, name,
                     static_cast<ShaderDataType>(type), {bytes, num_bytes},
                     count);
        });
    binder->RegisterMethod("lull.Render.SetColor",
                           &lull::RenderSystem::SetColor);
    binder->RegisterFunction(
        "lull.Render.CreateExternalTexture",
        [this](Entity entity, Optional<HashValue> pass, Optional<int> index,
               int unit, const mathfu::vec2& size) -> int {
          auto* texture_factory = registry_->Get<TextureFactory>();
          if (!texture_factory) {
            LOG(DFATAL) << "CreateExternalTexture without TextureFactory.";
            return 0;
          }
          const auto texture_ptr =
              texture_factory->CreateExternalTexture(mathfu::vec2i(size));
          if (!texture_ptr) {
            LOG(DFATAL) << "CreateExternalTexture no texture.";
            return 0;
          }
          SetTexture({entity, pass, index}, unit, texture_ptr);
          return GetTextureGlHandle(texture_ptr).value_or(0);
        });
  }

  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->Connect(this,
                        [this](const HideEvent& event) { Hide(event.entity); });
    dispatcher->Connect(this,
                        [this](const ShowEvent& event) { Show(event.entity); });
    dispatcher->Connect(this, [this](const SetRenderGroupIdEvent& event) {
      SetGroupId(event.entity, event.group_id);
    });
    dispatcher->Connect(this, [this](const SetRenderGroupParamsEvent& event) {
      SetGroupParams(event.group_id, GroupParams(event.sort_order_offset));
    });
    dispatcher->Connect(this, [this](const SetTextureIdEvent& e) {
      const int texture_unit = 0;
      SetTextureId(e.entity, texture_unit,
                   static_cast<uint32_t>(e.texture_target),
                   static_cast<uint32_t>(e.texture_id));
    });
    dispatcher->Connect(this, [this](const SetImageEvent& e) {
      if (e.image) {
        const int texture_unit = 0;
        auto texture = CreateTexture(*e.image, e.create_mips);
        SetTexture(e.entity, texture_unit, texture);
      }
    });
    dispatcher->Connect(this, [this](const SetSortOffsetEvent& e) {
      SetSortOrderOffset(e.entity, e.sort_offset);
    });
    dispatcher->Connect(this, [this](const SetRenderPassEvent& e) {
      SetRenderPass(e.entity, static_cast<RenderPass>(e.render_pass));
    });
    dispatcher->Connect(this, [this](const SetTextureEvent& e) {
      SetTexture(e.entity, 0, e.filename);
    });
    dispatcher->Connect(this, [this](const SetColorEvent& e) {
      const auto color = e.int_argb
                             ? Color4ub::ToVec4(Color4ub::FromARGB(e.int_argb))
                             : e.color;
      SetUniform(e.entity, "color", &color[0], 4);
    });
    dispatcher->Connect(this, [this](const SetDefaultColorEvent& e) {
      const auto color = e.int_argb
                             ? Color4ub::ToVec4(Color4ub::FromARGB(e.int_argb))
                             : e.color;
      SetDefaultColor(e.entity, color);
    });
  }
}

RenderSystem::~RenderSystem() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Render.BeginFrame");
    binder->UnregisterFunction("lull.Render.EndFrame");
    binder->UnregisterFunction("lull.Render.BeginRendering");
    binder->UnregisterFunction("lull.Render.EndRendering");
    binder->UnregisterFunction("lull.Render.SubmitRenderData");
    binder->UnregisterFunction("lull.Render.ProcessTasks");
    binder->UnregisterFunction("lull.Render.Show");
    binder->UnregisterFunction("lull.Render.ShowSubmesh");
    binder->UnregisterFunction("lull.Render.Hide");
    binder->UnregisterFunction("lull.Render.HideSubmesh");
    binder->UnregisterFunction("lull.Render.RenderPassesWithView");
    binder->UnregisterFunction("lull.Render.SetClearParams");
    binder->UnregisterFunction("lull.Render.SetQuad");
    binder->UnregisterFunction("lull.Render.CornerMask.None");
    binder->UnregisterFunction("lull.Render.CornerMask.TopRight");
    binder->UnregisterFunction("lull.Render.CornerMask.BottomRight");
    binder->UnregisterFunction("lull.Render.CornerMask.BottomLeft");
    binder->UnregisterFunction("lull.Render.CornerMask.TopLeft");
    binder->UnregisterFunction("lull.Render.CornerMask.All");
    binder->UnregisterFunction("lull.Render.SetUniform1f");
    binder->UnregisterFunction("lull.Render.SetUniform2f");
    binder->UnregisterFunction("lull.Render.SetUniform3f");
    binder->UnregisterFunction("lull.Render.SetUniform4f");
    binder->UnregisterFunction("lull.Render.SetSubmeshUniform1f");
    binder->UnregisterFunction("lull.Render.SetSubmeshUniform2f");
    binder->UnregisterFunction("lull.Render.SetSubmeshUniform3f");
    binder->UnregisterFunction("lull.Render.SetSubmeshUniform4f");
    binder->UnregisterFunction("lull.Render.SetUniforms");
    binder->UnregisterFunction("lull.Render.SetColor");
    binder->UnregisterFunction("lull.Render.CreateExternalTexture");
  }
  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->DisconnectAll(this);
  }
}

void RenderSystem::Initialize() { impl_->Initialize(); }

void RenderSystem::SubmitRenderData() { impl_->SubmitRenderData(); }

void RenderSystem::SetStereoMultiviewEnabled(bool enabled) {
  impl_->SetStereoMultiviewEnabled(enabled);
}

void RenderSystem::PreloadFont(const char* name) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->PreloadFont(name);
#else
  LOG(DFATAL) << "This function is deprecated.";
#endif
}

TexturePtr RenderSystem::GetWhiteTexture() const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->GetWhiteTexture();
#else
  return registry_->Get<TextureFactory>()->GetWhiteTexture();
#endif
}

TexturePtr RenderSystem::GetInvalidTexture() const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->GetInvalidTexture();
#else
  return registry_->Get<TextureFactory>()->GetInvalidTexture();
#endif
}

TexturePtr RenderSystem::GetTexture(HashValue texture_hash) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->GetTexture(texture_hash);
#else
  return registry_->Get<TextureFactory>()->GetTexture(texture_hash);
#endif
}

TexturePtr RenderSystem::LoadTexture(const std::string& filename) {
  const bool create_mips = false;
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->LoadTexture(filename, create_mips);
#else
  return LoadTexture(filename, create_mips);
#endif
}

TexturePtr RenderSystem::LoadTexture(const std::string& filename,
                                     bool create_mips) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->LoadTexture(filename, create_mips);
#else
  TextureParams params;
  params.generate_mipmaps = create_mips;
  return registry_->Get<TextureFactory>()->LoadTexture(filename, params);
#endif
}

void RenderSystem::LoadTextureAtlas(const std::string& filename) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->LoadTextureAtlas(filename);
#else
  TextureParams params;
  registry_->Get<TextureFactory>()->LoadAtlas(filename, params);
#endif
}

MeshPtr RenderSystem::LoadMesh(const std::string& filename) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->LoadMesh(filename);
#else
  LOG(DFATAL) << "This function is deprecated.";
  return nullptr;
#endif
}

TexturePtr RenderSystem::CreateTexture(const ImageData& image) {
  const bool create_mips = false;
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->CreateTexture(image, create_mips);
#else
  return CreateTexture(image, create_mips);
#endif
}

TexturePtr RenderSystem::CreateTexture(const ImageData& image,
                                       bool create_mips) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->CreateTexture(image, create_mips);
#else
  TextureParams params;
  params.generate_mipmaps = create_mips;
  return registry_->Get<TextureFactory>()->CreateTextureDeprecated(&image,
                                                                   params);
#endif
}

ShaderPtr RenderSystem::LoadShader(const std::string& filename) {
  return impl_->LoadShader(filename);
}

void RenderSystem::Create(Entity entity, HashValue type, const Def* def) {
  impl_->Create(entity, type, def);
}

void RenderSystem::Create(Entity entity, HashValue pass) {
  impl_->Create(entity, pass);
}

void RenderSystem::PostCreateInit(Entity entity, HashValue type,
                                  const Def* def) {
  impl_->PostCreateInit(entity, type, def);
}

void RenderSystem::Destroy(Entity entity) { impl_->Destroy(entity); }

void RenderSystem::Destroy(Entity entity, HashValue pass) {
  impl_->Destroy(entity, pass);
}

void RenderSystem::ProcessTasks() {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->ProcessTasks();
#else
  // Do nothing.
#endif
}

void RenderSystem::WaitForAssetsToLoad() {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->ProcessTasks();
#else
  while (registry_->Get<AssetLoader>()->Finalize()) {}
#endif
}

HashValue RenderSystem::GetRenderPass(Entity entity) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return static_cast<HashValue>(impl_->GetRenderPass(entity));
#else
  LOG(DFATAL) << "This function is deprecated.";
  return 0;
#endif
}

std::vector<HashValue> RenderSystem::GetRenderPasses(Entity entity) const {
  return impl_->GetRenderPasses(entity);
}

const mathfu::vec4& RenderSystem::GetDefaultColor(Entity entity) const {
  return impl_->GetDefaultColor(entity);
}

void RenderSystem::SetDefaultColor(Entity entity, const mathfu::vec4& color) {
  impl_->SetDefaultColor(entity, color);
}

bool RenderSystem::GetColor(Entity entity, mathfu::vec4* color) const {
  return impl_->GetColor(entity, color);
}

void RenderSystem::SetColor(Entity entity, const mathfu::vec4& color) {
  impl_->SetColor(entity, color);
}

template <typename T>
static Span<uint8_t> ArrayToSpan(const T* data, int dimension, int count) {
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
  const size_t len = static_cast<size_t>(dimension * count * sizeof(T));
  return {bytes, len};
}

void RenderSystem::SetUniform(const Drawable& drawable, string_view name,
                              ShaderDataType type, Span<uint8_t> data,
                              int count) {
#if LULLABY_RENDER_BACKEND_DEPRECATED || defined(LULLABY_RENDER_BACKEND_MOCK)
  impl_->SetUniform(drawable.entity, drawable.pass, drawable.index, name, type,
                    data, count);
#else
  impl_->SetUniform(drawable, name, type, data, count);
#endif
}

void RenderSystem::SetUniform(Entity entity, string_view name,
                              ShaderDataType type, Span<uint8_t> data,
                              int count) {
  Drawable drawable(entity);
  SetUniform(drawable, name, type, data, count);
}

void RenderSystem::SetUniform(Entity entity, string_view name, Span<float> data,
                              int count) {
  const int dimensions = static_cast<int>(data.size() / count);
  ShaderDataType type = FloatDimensionsToUniformType(dimensions);
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data());
  const size_t num_bytes = data.size() * sizeof(float);
  SetUniform(entity, name, type, {bytes, num_bytes}, count);
}

void RenderSystem::SetUniform(Entity entity, string_view name, Span<int> data,
                              int count) {
  const int dimensions = static_cast<int>(data.size() / count);
  ShaderDataType type = IntDimensionsToUniformType(dimensions);
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data.data());
  const size_t num_bytes = data.size() * sizeof(int);
  SetUniform(entity, name, type, {bytes, num_bytes}, count);
}

void RenderSystem::SetUniform(Entity entity, const char* name,
                              const float* data, int dimension) {
  SetUniform(entity, name, data, dimension, 1);
}

void RenderSystem::SetUniform(Entity entity, const char* name,
                              const float* data, int dimension, int count) {
#if LULLABY_RENDER_BACKEND_DEPRECATED || defined(LULLABY_RENDER_BACKEND_MOCK)
  impl_->SetUniform(entity, name, data, dimension, count);
#else
  auto bytes = ArrayToSpan(data, dimension, count);
  auto type = FloatDimensionsToUniformType(dimension);
  SetUniform(entity, name, type, bytes, count);
#endif
}

void RenderSystem::SetUniform(Entity entity, HashValue pass, const char* name,
                              const float* data, int dimension, int count) {
#if LULLABY_RENDER_BACKEND_DEPRECATED || defined(LULLABY_RENDER_BACKEND_MOCK)
  impl_->SetUniform(entity, pass, name, data, dimension, count);
#else
  auto bytes = ArrayToSpan(data, dimension, count);
  auto type = FloatDimensionsToUniformType(dimension);
  SetUniform({entity, pass}, name, type, bytes, count);
#endif
}

bool RenderSystem::GetUniform(const Drawable& drawable, string_view name,
                              size_t length, uint8_t* data_out) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED || defined(LULLABY_RENDER_BACKEND_MOCK)
  return impl_->GetUniform(drawable.entity, drawable.pass, drawable.index, name,
                           length, data_out);
#else
  return impl_->GetUniform(drawable, name, length, data_out);
#endif
}

bool RenderSystem::GetUniform(Entity entity, string_view name, size_t length,
                              uint8_t* data_out) const {
  Drawable drawable(entity);
  return GetUniform(drawable, name, length, data_out);
}

bool RenderSystem::GetUniform(Entity entity, HashValue pass, const char* name,
                              size_t length, float* data_out) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED || defined(LULLABY_RENDER_BACKEND_MOCK)
  return impl_->GetUniform(entity, pass, name, length, data_out);
#else
  return GetUniform({entity, pass}, name, length * sizeof(float),
                    reinterpret_cast<uint8_t*>(data_out));
#endif
}

bool RenderSystem::GetUniform(Entity entity, const char* name, size_t length,
                              float* data_out) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED || defined(LULLABY_RENDER_BACKEND_MOCK)
  return impl_->GetUniform(entity, name, length, data_out);
#else
  return GetUniform(entity, name, length * sizeof(float),
                    reinterpret_cast<uint8_t*>(data_out));
#endif
}

void RenderSystem::CopyUniforms(Entity entity, Entity source) {
  impl_->CopyUniforms(entity, source);
}

void RenderSystem::SetUniformChangedCallback(Entity entity, HashValue pass,
                                             UniformChangedCallback callback) {
  impl_->SetUniformChangedCallback(entity, pass, std::move(callback));
}

const uint8_t* RenderSystem::GetBoneParents(Entity entity, int* num) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->GetBoneParents(entity, num);
#else
  LOG(DFATAL) << "This function is deprecated.";
  return nullptr;
#endif
}

const std::string* RenderSystem::GetBoneNames(Entity entity, int* num) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->GetBoneNames(entity, num);
#else
  LOG(DFATAL) << "This function is deprecated.";
  return nullptr;
#endif
}

const mathfu::AffineTransform* RenderSystem::GetDefaultBoneTransformInverses(
    Entity entity, int* num) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->GetDefaultBoneTransformInverses(entity, num);
#else
  LOG(DFATAL) << "This function is deprecated.";
  return nullptr;
#endif
}

void RenderSystem::SetBoneTransforms(Entity entity,
                                     const mathfu::AffineTransform* transforms,
                                     int num_transforms) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->SetBoneTransforms(entity, transforms, num_transforms);
#else
  LOG(DFATAL) << "This function is deprecated.";
#endif
}

void RenderSystem::SetTexture(const Drawable& drawable, int unit,
                              const TexturePtr& texture) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  DCHECK(!drawable.index);
  if (drawable.pass) {
    impl_->SetTexture(drawable.entity, *drawable.pass, unit, texture);
  } else {
    impl_->SetTexture(drawable.entity, unit, texture);
  }
#else
  const TextureUsageInfo usage(unit);
  impl_->SetTexture(drawable, usage, texture);
#endif
}

void RenderSystem::SetTexture(const Drawable& drawable, int unit,
                              const std::string& file) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  DCHECK(!drawable.pass);
  DCHECK(!drawable.index);
  SetTexture(drawable.entity, unit, LoadTexture(file));
#else
  SetTexture(drawable, unit, LoadTexture(file));
#endif
}

void RenderSystem::SetTextureId(const Drawable& drawable, int unit,
                                uint32_t texture_target, uint32_t texture_id) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  DCHECK(!drawable.index);
  if (drawable.pass) {
    impl_->SetTextureId(drawable.entity, *drawable.pass, unit, texture_target,
                        texture_id);
  } else {
    impl_->SetTextureId(drawable.entity, unit, texture_target, texture_id);
  }
#else
  const TextureUsageInfo usage(unit);
  impl_->SetTextureId(drawable, usage, texture_target, texture_id);
#endif
}

void RenderSystem::SetTextureExternal(const Drawable& drawable, int unit) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  DCHECK(drawable.pass);
  DCHECK(!drawable.index);
  impl_->SetTextureExternal(drawable.entity, *drawable.pass, unit);
#else
  const TextureUsageInfo usage(unit);
  impl_->SetTextureExternal(drawable, usage);
#endif
}

TexturePtr RenderSystem::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    TextureProcessor processor) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->CreateProcessedTexture(source_texture, create_mips, processor);
#else
  LOG(DFATAL) << "This function is deprecated.";
  return nullptr;
#endif
}

TexturePtr RenderSystem::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    TextureProcessor processor, const mathfu::vec2i& output_dimensions) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->CreateProcessedTexture(source_texture, create_mips, processor,
                                       output_dimensions);
#else
  LOG(DFATAL) << "This function is deprecated.";
  return nullptr;
#endif
}

TexturePtr RenderSystem::GetTexture(const Drawable& drawable, int unit) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  DCHECK(!drawable.pass);
  DCHECK(!drawable.index);
  return impl_->GetTexture(drawable.entity, unit);
#else
  const TextureUsageInfo usage(unit);
  return impl_->GetTexture(drawable, usage);
#endif
}


void RenderSystem::SetText(Entity entity, const std::string& text) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->SetText(entity, text);
#else
  LOG(DFATAL) << "This function is deprecated.";
#endif
}

bool RenderSystem::GetQuad(Entity entity, RenderQuad* quad) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->GetQuad(entity, quad);
#else
  LOG(DFATAL) << "This function is deprecated.";
  return false;
#endif
}

void RenderSystem::SetQuad(Entity entity, const RenderQuad& quad) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->SetQuad(entity, quad);
#else
  LOG(DFATAL) << "This function is deprecated.";
#endif
}

MeshPtr RenderSystem::GetMesh(const Drawable& drawable) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  if (drawable.pass) {
    return impl_->GetMesh(drawable.entity, *drawable.pass);
  } else {
    return impl_->GetMesh(drawable.entity, RenderPass_Opaque);
  }
#else
  return impl_->GetMesh(drawable);
#endif
}

void RenderSystem::SetMesh(const Drawable& drawable, const MeshPtr& mesh) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  if (drawable.pass) {
    impl_->SetMesh(drawable.entity, *drawable.pass, mesh);
  } else {
    impl_->SetMesh(drawable.entity, RenderPass_Opaque, mesh);
  }
#else
  impl_->SetMesh(drawable, mesh);
#endif
}

void RenderSystem::SetMesh(const Drawable& drawable, const MeshData& mesh) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  if (drawable.pass) {
    impl_->SetMesh(drawable.entity, *drawable.pass, mesh);
  } else {
    impl_->SetMesh(drawable.entity, mesh);
  }
#else
  impl_->SetMesh(drawable, mesh);
#endif
}

void RenderSystem::SetAndDeformMesh(Entity entity, const MeshData& mesh) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->SetAndDeformMesh(entity, mesh);
#else
  LOG_ONCE(WARNING) << "Deformations no longer supported. Setting mesh as-is.";
  impl_->SetMesh(entity, mesh);
#endif
}

void RenderSystem::SetMesh(Entity entity, const std::string& file) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->SetMesh(entity, file);
#else
  LOG(DFATAL) << "This function is deprecated.";
#endif
}

ShaderPtr RenderSystem::GetShader(Entity entity) const {
  return impl_->GetShader(entity);
}

ShaderPtr RenderSystem::GetShader(Entity entity, HashValue pass) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->GetShader(entity, pass);
#else
  return impl_->GetShader({entity, pass});
#endif
}

void RenderSystem::SetShader(Entity entity, const ShaderPtr& shader) {
  impl_->SetShader(entity, shader);
}

void RenderSystem::SetShader(Entity entity, HashValue pass,
                             const ShaderPtr& shader) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->SetShader(entity, pass, shader);
#else
  impl_->SetShader({entity, pass}, shader);
#endif
}

void RenderSystem::SetShader(Entity entity, const std::string& file) {
  SetShader(entity, LoadShader(file));
}

void RenderSystem::SetMaterial(const Drawable& drawable,
                               const MaterialInfo& info) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  LOG(FATAL) << "Unimplemented.";
#else
  impl_->SetMaterial(drawable, info);
#endif
}

bool RenderSystem::IsShaderFeatureRequested(const Drawable& drawable,
                                            HashValue feature) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  LOG(FATAL) << "Unimplemented.";
  return false;
#else
  return impl_->IsShaderFeatureRequested(drawable, feature);
#endif
}

void RenderSystem::RequestShaderFeature(const Drawable& drawable,
                                        HashValue feature) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  LOG(FATAL) << "Unimplemented.";
#else
  impl_->RequestShaderFeature(drawable, feature);
#endif
}

void RenderSystem::ClearShaderFeature(const Drawable& drawable,
                                      HashValue feature) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  LOG(FATAL) << "Unimplemented.";
#else
  impl_->ClearShaderFeature(drawable, feature);
#endif
}

RenderSystem::SortOrder RenderSystem::GetSortOrder(Entity e) const {
  return impl_->GetSortOrder(e);
}

RenderSortOrderOffset RenderSystem::GetSortOrderOffset(Entity entity) const {
  return impl_->GetSortOrderOffset(entity);
}

void RenderSystem::SetSortOrderOffset(Entity entity,
                                      RenderSortOrderOffset sort_order_offset) {
  impl_->SetSortOrderOffset(entity, sort_order_offset);
}

void RenderSystem::SetSortOrderOffset(Entity entity, HashValue pass,
                                      RenderSortOrderOffset sort_order_offset) {
  impl_->SetSortOrderOffset(entity, pass, sort_order_offset);
}

void RenderSystem::SetStencilMode(Entity entity, RenderStencilMode mode,
                                  int value) {
  impl_->SetStencilMode(entity, mode, value);
}

void RenderSystem::SetStencilMode(Entity entity, HashValue pass,
                                  RenderStencilMode mode, int value) {
  impl_->SetStencilMode(entity, pass, mode, value);
}

bool RenderSystem::IsTextureSet(const Drawable& drawable, int unit) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  DCHECK(!drawable.pass);
  DCHECK(!drawable.index);
  return impl_->IsTextureSet(drawable.entity, unit);
#else
  LOG(DFATAL) << "This function is deprecated.";
  return false;
#endif
}

bool RenderSystem::IsTextureLoaded(Entity entity, int unit) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->IsTextureLoaded(entity, unit);
#else
  LOG(DFATAL) << "This function is deprecated.";
  return false;
#endif
}

bool RenderSystem::IsTextureLoaded(const TexturePtr& texture) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->IsTextureLoaded(texture);
#else
  LOG(DFATAL) << "This function is deprecated.";
  return false;
#endif
}

bool RenderSystem::IsReadyToRender(const Drawable& drawable) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  if (drawable.pass) {
    return impl_->IsReadyToRender(drawable.entity, *drawable.pass);
  } else {
    return impl_->IsReadyToRender(drawable.entity);
  }
#else
  return impl_->IsReadyToRender(drawable);
#endif
}

void RenderSystem::OnReadyToRender(const Drawable& drawable,
                                   const std::function<void()>& fn) const {
  if (IsReadyToRender(drawable.entity)) {
    fn();
  } else {
    auto* dispatcher_system = registry_->Get<DispatcherSystem>();
    if (!dispatcher_system) {
      LOG(DFATAL) << "OnReadyToRender requires the DispatcherSystem.";
      return;
    }
    auto connection = std::make_shared<Dispatcher::ScopedConnection>();
    *connection = dispatcher_system->Connect(
        drawable.entity, [=](const ReadyToRenderEvent& event) mutable {
          DCHECK(event.entity == drawable.entity);
          if (event.entity == drawable.entity && event.pass == *drawable.pass) {
            connection.reset();
            fn();
          }
        });
  }
}

void RenderSystem::SetDeformationFunction(Entity entity,
                                          const DeformationFn& deform) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->SetDeformationFunction(entity, deform);
#else
  LOG(DFATAL) << "This function is deprecated.";
#endif
}

bool RenderSystem::IsHidden(const Drawable& drawable) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->IsHidden(drawable.entity, drawable.pass, drawable.index);
#else
  return impl_->IsHidden(drawable);
#endif
}

void RenderSystem::Hide(const Drawable& drawable) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->Hide(drawable.entity, drawable.pass, drawable.index);
#else
  impl_->Hide(drawable);
#endif
}

void RenderSystem::Show(const Drawable& drawable) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->Show(drawable.entity, drawable.pass, drawable.index);
#else
  impl_->Show(drawable);
#endif
}

void RenderSystem::SetRenderPass(Entity entity, HashValue pass) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->SetRenderPass(entity, pass);
#else
  LOG(DFATAL) << "This function is deprecated.";
#endif
}

void RenderSystem::SetDefaultRenderPass(HashValue pass) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  LOG(DFATAL) << "This feature is not available.";
#else
  impl_->SetDefaultRenderPass(pass);
#endif
}

HashValue RenderSystem::GetDefaultRenderPass() const {
#if LULLABY_RENDER_BACKEND_DEPRECATED && !LULLABY_RENDER_BACKEND_MOCK
  LOG(WARNING) << "This feature is not available.";
  return 0;
#else
  return impl_->GetDefaultRenderPass();
#endif
}

void RenderSystem::SetRenderState(HashValue pass,
                                  const fplbase::RenderState& render_state) {
  impl_->SetRenderState(pass, render_state);
}

void RenderSystem::SetClearParams(HashValue pass,
                                  const RenderClearParams& clear_params) {
  impl_->SetClearParams(pass, clear_params);
}

void RenderSystem::SetSortMode(HashValue pass, SortMode mode) {
  impl_->SetSortMode(pass, mode);
}

void RenderSystem::SetSortVector(HashValue pass, const mathfu::vec3& vector) {
  impl_->SetSortVector(pass, vector);
}

void RenderSystem::SetCullMode(HashValue pass, RenderCullMode mode) {
  impl_->SetCullMode(pass, mode);
}

void RenderSystem::SetDefaultFrontFace(RenderFrontFace face) {
  impl_->SetDefaultFrontFace(face);
}

void RenderSystem::SetRenderTarget(HashValue pass,
                                   HashValue render_target_name) {
  impl_->SetRenderTarget(pass, render_target_name);
}

ImageData RenderSystem::GetRenderTargetData(HashValue render_target_name) {
  return impl_->GetRenderTargetData(render_target_name);
}

void RenderSystem::SetDepthTest(const bool enabled) {
  impl_->SetDepthTest(enabled);
}

void RenderSystem::SetDepthWrite(const bool enabled) {
  impl_->SetDepthWrite(enabled);
}

void RenderSystem::SetBlendMode(const fplbase::BlendMode blend_mode) {
  impl_->SetBlendMode(blend_mode);
}

void RenderSystem::SetViewport(const RenderView& view) {
  impl_->SetViewport(view);
}

mathfu::vec4 RenderSystem::GetClearColor() const {
  return impl_->GetClearColor();
}

void RenderSystem::SetClearColor(float r, float g, float b, float a) {
  impl_->SetClearColor(r, g, b, a);
}

void RenderSystem::Render(const RenderView* views, size_t num_views) {
  impl_->Render(views, num_views);
}

void RenderSystem::Render(const RenderView* views, size_t num_views,
                          HashValue pass) {
  impl_->Render(views, num_views, pass);
}

RenderSystemImpl* RenderSystem::GetImpl() { return impl_.get(); }

void RenderSystem::BeginFrame() {
  RenderStats* render_stats = registry_->Get<RenderStats>();
  if (render_stats) {
    render_stats->BeginFrame();
  }

#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->BeginFrame();
#endif
}

void RenderSystem::EndFrame() {
  RenderStats* render_stats = registry_->Get<RenderStats>();
  if (render_stats) {
    render_stats->EndFrame();
  }

#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->EndFrame();
#endif
}

const fplbase::RenderState& RenderSystem::GetCachedRenderState() const {
  return impl_->GetCachedRenderState();
}

void RenderSystem::UpdateCachedRenderState(
    const fplbase::RenderState& render_state) {
  impl_->UpdateCachedRenderState(render_state);
}

void RenderSystem::BeginRendering() { impl_->BeginRendering(); }

void RenderSystem::EndRendering() { impl_->EndRendering(); }

void RenderSystem::UpdateDynamicMesh(
    Entity entity, MeshData::PrimitiveType primitive_type,
    const VertexFormat& vertex_format, const size_t max_vertices,
    const size_t max_indices,
    const std::function<void(MeshData*)>& update_mesh) {
  UpdateDynamicMesh(entity, primitive_type, vertex_format, max_vertices,
                    max_indices, MeshData::kIndexU16, 0, update_mesh);
}

void RenderSystem::UpdateDynamicMesh(
    Entity entity, MeshData::PrimitiveType primitive_type,
    const VertexFormat& vertex_format, const size_t max_vertices,
    const size_t max_indices, MeshData::IndexType index_type,
    const size_t max_ranges,
    const std::function<void(MeshData*)>& update_mesh) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->UpdateDynamicMesh(entity, primitive_type, vertex_format, max_vertices,
                           max_indices, index_type, max_ranges, update_mesh);
#else
  MeshPtr mesh;
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

    MeshFactory* mesh_factory = registry_->Get<MeshFactory>();
    mesh = mesh_factory->CreateMesh(std::move(data));
  }
  impl_->SetMesh(entity, mesh);
#endif
}

void RenderSystem::BindShader(const ShaderPtr& shader) {
  impl_->BindShader(shader);
}

void RenderSystem::BindTexture(int unit, const TexturePtr& texture) {
  impl_->BindTexture(unit, texture);
}

void RenderSystem::BindUniform(const char* name, const float* data,
                               int dimension) {
  impl_->BindUniform(name, data, dimension);
}

void RenderSystem::DrawMesh(const MeshData& mesh,
                            Optional<mathfu::mat4> clip_from_model) {
  impl_->DrawMesh(mesh, clip_from_model);
}

void RenderSystem::CreateRenderTarget(
    HashValue render_target_name,
    const RenderTargetCreateParams& create_params) {
  impl_->CreateRenderTarget(render_target_name, create_params);
}

Optional<HashValue> RenderSystem::GetGroupId(Entity entity) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->GetGroupId(entity);
#else
  LOG_ONCE(ERROR) << "This function is deprecated.";
  return NullOpt;
#endif
}

void RenderSystem::SetGroupId(Entity entity,
                              const Optional<HashValue>& group_id) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->SetGroupId(entity, group_id);
#else
  LOG_ONCE(ERROR) << "This function is deprecated.";
#endif
}

const RenderSystem::GroupParams* RenderSystem::GetGroupParams(
    HashValue group_id) const {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  return impl_->GetGroupParams(group_id);
#else
  LOG_ONCE(ERROR) << "This function is deprecated.";
  return nullptr;
#endif
}

void RenderSystem::SetGroupParams(HashValue group_id,
                                  const GroupParams& group_params) {
#if LULLABY_RENDER_BACKEND_DEPRECATED
  impl_->SetGroupParams(group_id, group_params);
#else
  LOG_ONCE(ERROR) << "This function is deprecated.";
#endif
}

std::string RenderSystem::GetShaderString(Entity entity, HashValue pass,
                                          int submesh_index,
                                          ShaderStageType stage) const {
  return impl_->GetShaderString(entity, pass, submesh_index, stage);
}

ShaderPtr RenderSystem::CompileShaderString(
    const std::string& vertex_string, const std::string& fragment_string) {
  return impl_->CompileShaderString(vertex_string, fragment_string);
}

}  // namespace lull
