#include "lullaby/systems/render/render_system.h"

#include "lullaby/events/render_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/render/render_stats.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

RenderSystem::RenderSystem(Registry* registry, const InitParams& init_params)
    : System(registry), impl_(new RenderSystemImpl(registry, init_params)) {
  RegisterDef(this, Hash("RenderDef"));

  RegisterDependency<TransformSystem>(this);

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
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
  }
  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->Connect(this,
                        [this](const HideEvent& event) { Hide(event.entity); });
    dispatcher->Connect(this,
                        [this](const ShowEvent& event) { Show(event.entity); });
  }
}

RenderSystem::~RenderSystem() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Render.SetQuad");
    binder->UnregisterFunction("lull.Render.CornerMask.None");
    binder->UnregisterFunction("lull.Render.CornerMask.TopRight");
    binder->UnregisterFunction("lull.Render.CornerMask.BottomRight");
    binder->UnregisterFunction("lull.Render.CornerMask.BottomLeft");
    binder->UnregisterFunction("lull.Render.CornerMask.TopLeft");
    binder->UnregisterFunction("lull.Render.CornerMask.All");
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

void RenderSystem::PreloadFont(const char* name) { impl_->PreloadFont(name); }

FontPtr RenderSystem::LoadFonts(const std::vector<std::string>& names) {
  return impl_->LoadFonts(names);
}

const TexturePtr& RenderSystem::GetWhiteTexture() const {
  return impl_->GetWhiteTexture();
}

const TexturePtr& RenderSystem::GetInvalidTexture() const {
  return impl_->GetInvalidTexture();
}

TexturePtr RenderSystem::GetTexture(HashValue texture_hash) const {
  return impl_->GetTexture(texture_hash);
}

TexturePtr RenderSystem::LoadTexture(const std::string& filename) {
  const bool create_mips = false;
  return impl_->LoadTexture(filename, create_mips);
}

TexturePtr RenderSystem::LoadTexture(const std::string& filename,
                                     bool create_mips) {
  return impl_->LoadTexture(filename, create_mips);
}

void RenderSystem::LoadTextureAtlas(const std::string& filename) {
  impl_->LoadTextureAtlas(filename);
}

MeshPtr RenderSystem::LoadMesh(const std::string& filename) {
  return impl_->LoadMesh(filename);
}

TexturePtr RenderSystem::CreateTexture(const ImageData& image) {
  const bool create_mips = false;
  return impl_->CreateTexture(image, create_mips);
}

TexturePtr RenderSystem::CreateTexture(const ImageData& image,
                                       bool create_mips) {
  return impl_->CreateTexture(image, create_mips);
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

void RenderSystem::Create(Entity entity, HashValue pass, HashValue pass_enum) {
  impl_->Create(entity, pass, pass_enum);
}

void RenderSystem::PostCreateInit(Entity entity, HashValue type,
                                  const Def* def) {
  impl_->PostCreateInit(entity, type, def);
}

void RenderSystem::Destroy(Entity entity) { impl_->Destroy(entity); }

void RenderSystem::Destroy(Entity entity, HashValue pass) {
  impl_->Destroy(entity, pass);
}

void RenderSystem::ProcessTasks() { impl_->ProcessTasks(); }

void RenderSystem::WaitForAssetsToLoad() { impl_->WaitForAssetsToLoad(); }

HashValue RenderSystem::GetRenderPass(Entity entity) const {
  return static_cast<HashValue>(impl_->GetRenderPass(entity));
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

void RenderSystem::SetUniform(Entity entity, const char* name,
                              const float* data, int dimension) {
  impl_->SetUniform(entity, name, data, dimension, 1);
}

void RenderSystem::SetUniform(Entity entity, const char* name,
                              const float* data, int dimension, int count) {
  impl_->SetUniform(entity, name, data, dimension, count);
}

void RenderSystem::SetUniform(Entity entity, HashValue pass, const char* name,
                              const float* data, int dimension, int count) {
  impl_->SetUniform(entity, pass, name, data, dimension, count);
}

bool RenderSystem::GetUniform(Entity entity, HashValue pass, const char* name,
                              size_t length, float* data_out) const {
  return impl_->GetUniform(entity, pass, name, length, data_out);
}

bool RenderSystem::GetUniform(Entity entity, const char* name, size_t length,
                              float* data_out) const {
  return impl_->GetUniform(entity, name, length, data_out);
}

void RenderSystem::CopyUniforms(Entity entity, Entity source) {
  impl_->CopyUniforms(entity, source);
}

int RenderSystem::GetNumBones(Entity entity) const {
  return impl_->GetNumBones(entity);
}

const uint8_t* RenderSystem::GetBoneParents(Entity entity, int* num) const {
  return impl_->GetBoneParents(entity, num);
}

const std::string* RenderSystem::GetBoneNames(Entity entity, int* num) const {
  return impl_->GetBoneNames(entity, num);
}

const mathfu::AffineTransform* RenderSystem::GetDefaultBoneTransformInverses(
    Entity entity, int* num) const {
  return impl_->GetDefaultBoneTransformInverses(entity, num);
}

void RenderSystem::SetBoneTransforms(Entity entity,
                                     const mathfu::AffineTransform* transforms,
                                     int num_transforms) {
  impl_->SetBoneTransforms(entity, transforms, num_transforms);
}

void RenderSystem::SetTexture(Entity entity, int unit,
                              const TexturePtr& texture) {
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  impl_->SetTexture(entity, unit, texture);
}

void RenderSystem::SetTexture(Entity entity, HashValue pass, int unit,
                              const TexturePtr& texture) {
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  impl_->SetTexture(entity, pass, unit, texture);
}

void RenderSystem::SetTexture(Entity entity, int unit,
                              const std::string& file) {
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  SetTexture(entity, unit, LoadTexture(file));
}

void RenderSystem::SetTextureId(Entity entity, HashValue pass, int unit,
                                uint32_t texture_target, uint32_t texture_id) {
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  impl_->SetTextureId(entity, pass, unit, texture_target, texture_id);
}

void RenderSystem::SetTextureId(Entity entity, int unit,
                                uint32_t texture_target, uint32_t texture_id) {
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  impl_->SetTextureId(entity, unit, texture_target, texture_id);
}

TexturePtr RenderSystem::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    TextureProcessor processor) {
  return impl_->CreateProcessedTexture(source_texture, create_mips, processor);
}

TexturePtr RenderSystem::CreateProcessedTexture(
    const TexturePtr& source_texture, bool create_mips,
    TextureProcessor processor, const mathfu::vec2i& output_dimensions) {
  return impl_->CreateProcessedTexture(source_texture, create_mips, processor,
                                       output_dimensions);
}

TexturePtr RenderSystem::GetTexture(Entity entity, int unit) const {
  return impl_->GetTexture(entity, unit);
}


void RenderSystem::SetText(Entity entity, const std::string& text) {
  impl_->SetText(entity, text);
}

bool RenderSystem::GetQuad(Entity entity, RenderQuad* quad) const {
  return impl_->GetQuad(entity, quad);
}

void RenderSystem::SetQuad(Entity entity, const RenderQuad& quad) {
  impl_->SetQuad(entity, quad);
}

void RenderSystem::SetQuad(Entity entity, HashValue pass,
                           const RenderQuad& quad) {
  impl_->SetQuad(entity, pass, quad);
}

MeshPtr RenderSystem::GetMesh(Entity entity, HashValue pass) {
  return impl_->GetMesh(entity, pass);
}

void RenderSystem::SetMesh(Entity entity, HashValue pass, const MeshPtr& mesh) {
  impl_->SetMesh(entity, pass, mesh);
}

void RenderSystem::SetMesh(Entity entity, const MeshData& mesh) {
  impl_->SetMesh(entity, mesh);
}

void RenderSystem::SetAndDeformMesh(Entity entity, const MeshData& mesh) {
  impl_->SetAndDeformMesh(entity, mesh);
}

void RenderSystem::SetMesh(Entity entity, const std::string& file) {
  impl_->SetMesh(entity, file);
}

ShaderPtr RenderSystem::GetShader(Entity entity) const {
  return impl_->GetShader(entity);
}

ShaderPtr RenderSystem::GetShader(Entity entity, HashValue pass) const {
  return impl_->GetShader(entity, pass);
}

void RenderSystem::SetShader(Entity entity, const ShaderPtr& shader) {
  impl_->SetShader(entity, shader);
}

void RenderSystem::SetShader(Entity entity, HashValue pass,
                             const ShaderPtr& shader) {
  impl_->SetShader(entity, pass, shader);
}

void RenderSystem::SetShader(Entity entity, const std::string& file) {
  SetShader(entity, LoadShader(file));
}

void RenderSystem::SetMaterial(Entity entity, int submesh_index,
                               const MaterialInfo& info) {
  impl_->SetMaterial(entity, submesh_index, info);
}

void RenderSystem::SetFont(Entity entity, const FontPtr& font) {
  impl_->SetFont(entity, font);
}

void RenderSystem::SetTextSize(Entity entity, int size) {
  impl_->SetTextSize(entity, size);
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

bool RenderSystem::IsTextureSet(Entity entity, int unit) const {
  return impl_->IsTextureSet(entity, unit);
}

bool RenderSystem::IsTextureLoaded(Entity entity, int unit) const {
  return impl_->IsTextureLoaded(entity, unit);
}

bool RenderSystem::IsTextureLoaded(const TexturePtr& texture) const {
  return impl_->IsTextureLoaded(texture);
}

bool RenderSystem::IsReadyToRender(Entity entity) const {
  return impl_->IsReadyToRender(entity);
}

bool RenderSystem::IsHidden(Entity entity) const {
  return impl_->IsHidden(entity);
}

void RenderSystem::SetDeformationFunction(Entity entity,
                                          const DeformationFn& deform) {
  impl_->SetDeformationFunction(entity, deform);
}

void RenderSystem::Hide(Entity entity) { impl_->Hide(entity); }

void RenderSystem::Show(Entity entity) { impl_->Show(entity); }

void RenderSystem::SetRenderPass(Entity entity, HashValue pass) {
  impl_->SetRenderPass(entity, pass);
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

void RenderSystem::SetClipFromModelMatrix(const mathfu::mat4& mvp) {
  impl_->SetClipFromModelMatrix(mvp);
}

void RenderSystem::SetClipFromModelMatrixFunction(
    const ClipFromModelMatrixFn& func) {
  impl_->SetClipFromModelMatrixFunction(func);
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

  impl_->BeginFrame();
}

void RenderSystem::EndFrame() {
  RenderStats* render_stats = registry_->Get<RenderStats>();
  if (render_stats) {
    render_stats->EndFrame();
  }

  impl_->EndFrame();
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
  impl_->UpdateDynamicMesh(entity, primitive_type, vertex_format, max_vertices,
                           max_indices, update_mesh);
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

void RenderSystem::DrawPrimitives(MeshData::PrimitiveType primitive_type,
                                  const VertexFormat& vertex_format,
                                  const void* vertex_data,
                                  size_t num_vertices) {
  DataContainer vertex_wrapper = DataContainer::WrapDataAsReadOnly(
      reinterpret_cast<const uint8_t*>(vertex_data),
      num_vertices * vertex_format.GetVertexSize());
  MeshData mesh(primitive_type, vertex_format, std::move(vertex_wrapper));
  impl_->DrawMesh(mesh);
}

void RenderSystem::DrawIndexedPrimitives(MeshData::PrimitiveType primitive_type,
                                         const VertexFormat& vertex_format,
                                         const void* vertex_data,
                                         size_t num_vertices,
                                         const uint16_t* indices,
                                         size_t num_indices) {
  DataContainer vertex_wrapper = DataContainer::WrapDataAsReadOnly(
      reinterpret_cast<const uint8_t*>(vertex_data),
      num_vertices * vertex_format.GetVertexSize());
  DataContainer index_wrapper = DataContainer::WrapDataAsReadOnly(
      reinterpret_cast<const uint8_t*>(indices),
      num_indices * sizeof(indices[0]));
  MeshData mesh(primitive_type, vertex_format, std::move(vertex_wrapper),
                MeshData::kIndexU16, std::move(index_wrapper));
  impl_->DrawMesh(mesh);
}

void RenderSystem::DrawMesh(const MeshData& mesh) { impl_->DrawMesh(mesh); }

void RenderSystem::CreateRenderTarget(
    HashValue render_target_name,
    const RenderTargetCreateParams& create_params) {
  impl_->CreateRenderTarget(render_target_name, create_params);
}

void RenderSystem::BindUniform(const Uniform& uniform) {
  impl_->BindUniform(uniform);
}

}  // namespace lull
