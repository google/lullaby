#include "lullaby/systems/render/render_system.h"

#include "lullaby/events/render_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/render/render_stats.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

RenderSystem::RenderSystem(Registry* registry)
    : System(registry), impl_(new RenderSystemImpl(registry)) {
  RegisterDef(this, Hash("RenderDef"));

  RegisterDependency<TransformSystem>(this);

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterFunction("lull.Render.SetQuad", [this](
        Entity e,
        const mathfu::vec2& size,
        const mathfu::vec2i& verts,
        float corner_radius,
        int corner_verts,
        bool has_uv,
        int corner_mask) {
      Quad quad;
      quad.size = size;
      quad.verts = verts;
      quad.corner_radius = corner_radius;
      quad.corner_verts = corner_verts;
      quad.has_uv = has_uv;
      quad.corner_mask = static_cast<CornerMask>(corner_mask);
      SetQuad(e, quad);
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
    dispatcher->Connect(this, [this](const HideEvent& event) {
      Hide(event.entity);
    });
    dispatcher->Connect(this, [this](const ShowEvent& event) {
      Show(event.entity);
    });
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


ShaderPtr RenderSystem::LoadShader(const std::string& filename) {
  return impl_->LoadShader(filename);
}

void RenderSystem::Create(Entity e, HashValue type, const Def* def) {
  impl_->Create(e, type, def);
}

void RenderSystem::Create(Entity e, RenderPass pass) { impl_->Create(e, pass); }

void RenderSystem::Create(Entity e, HashValue component_id, RenderPass pass) {
  impl_->Create(e, component_id, pass);
}

void RenderSystem::PostCreateInit(Entity e, HashValue type, const Def* def) {
  impl_->PostCreateInit(e, type, def);
}

void RenderSystem::Destroy(Entity e) { impl_->Destroy(e); }

void RenderSystem::Destroy(Entity e, HashValue component_id) {
  impl_->Destroy(e, component_id);
}

void RenderSystem::ProcessTasks() { impl_->ProcessTasks(); }

void RenderSystem::WaitForAssetsToLoad() { impl_->WaitForAssetsToLoad(); }

RenderPass RenderSystem::GetRenderPass(Entity entity) const {
  return (RenderPass)impl_->GetRenderPass(entity);
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

void RenderSystem::SetUniform(Entity e, const char* name, const float* data,
                              int dimension) {
  impl_->SetUniform(e, name, data, dimension, 1);
}

void RenderSystem::SetUniform(Entity e, const char* name, const float* data,
                              int dimension, int count) {
  impl_->SetUniform(e, name, data, dimension, count);
}

void RenderSystem::SetUniform(Entity e, HashValue component_id,
                              const char* name, const float* data,
                              int dimension, int count) {
  impl_->SetUniform(e, component_id, name, data, dimension, count);
}

bool RenderSystem::GetUniform(Entity e, HashValue component_id,
                              const char* name, size_t length,
                              float* data_out) const {
  return impl_->GetUniform(e, component_id, name, length, data_out);
}

bool RenderSystem::GetUniform(Entity e, const char* name, size_t length,
                              float* data_out) const {
  return impl_->GetUniform(e, name, length, data_out);
}

void RenderSystem::CopyUniforms(Entity entity, Entity source) {
  impl_->CopyUniforms(entity, source);
}

int RenderSystem::GetNumBones(Entity entity) const {
  return impl_->GetNumBones(entity);
}

const uint8_t* RenderSystem::GetBoneParents(Entity e, int* num) const {
  return impl_->GetBoneParents(e, num);
}

const std::string* RenderSystem::GetBoneNames(Entity e, int* num) const {
  return impl_->GetBoneNames(e, num);
}

const mathfu::AffineTransform* RenderSystem::GetDefaultBoneTransformInverses(
    Entity e, int* num) const {
  return impl_->GetDefaultBoneTransformInverses(e, num);
}

void RenderSystem::SetBoneTransforms(Entity entity,
                         const mathfu::AffineTransform* transforms,
                         int num_transforms) {
  impl_->SetBoneTransforms(entity, transforms, num_transforms);
}

void RenderSystem::SetTexture(Entity e, int unit, const TexturePtr& texture) {
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  impl_->SetTexture(e, unit, texture);
}

void RenderSystem::SetTexture(Entity e, HashValue component_id, int unit,
                              const TexturePtr& texture) {
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  impl_->SetTexture(e, component_id, unit, texture);
}

void RenderSystem::SetTexture(Entity e, int unit, const std::string& file) {
// Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  SetTexture(e, unit, LoadTexture(file));
}

void RenderSystem::SetTextureId(Entity e, HashValue component_id, int unit,
                                uint32_t texture_target, uint32_t texture_id) {
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  impl_->SetTextureId(e, component_id, unit, texture_target, texture_id);
}

void RenderSystem::SetTextureId(Entity e, int unit, uint32_t texture_target,
                                uint32_t texture_id) {
  // Texture unit is an int, but must be positive and below the number of
  // samplers. Currently we aren't checking how many samplers are available,
  // therefore we at best ensure the value is below 256.
  DCHECK_GE(unit, 0);
  DCHECK_LE(unit, 255);
  impl_->SetTextureId(e, unit, texture_target, texture_id);
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


void RenderSystem::SetText(Entity e, const std::string& text) {
  impl_->SetText(e, text);
}

const std::vector<LinkTag>* RenderSystem::GetLinkTags(Entity e) const {
  return impl_->GetLinkTags(e);
}

bool RenderSystem::GetQuad(Entity e, Quad* quad) const {
  return impl_->GetQuad(e, quad);
}

void RenderSystem::SetQuad(Entity e, const Quad& quad) {
  impl_->SetQuad(e, quad);
}

void RenderSystem::SetQuad(Entity e, HashValue component_id, const Quad& quad) {
  impl_->SetQuad(e, component_id, quad);
}

MeshPtr RenderSystem::GetMesh(Entity e, HashValue component_id) {
  return impl_->GetMesh(e, component_id);
}

void RenderSystem::SetMesh(Entity e, HashValue component_id,
                           const MeshPtr& mesh) {
  impl_->SetMesh(e, component_id, mesh);
}

void RenderSystem::SetMesh(Entity e, const TriangleMesh<VertexPT>& mesh) {
  impl_->SetMesh(e, mesh);
}

void RenderSystem::SetMesh(Entity e, HashValue component_id,
                           const TriangleMesh<VertexPT>& mesh) {
  impl_->SetMesh(e, component_id, mesh);
}

void RenderSystem::SetAndDeformMesh(Entity entity,
    const TriangleMesh<VertexPT>& mesh) {
  impl_->SetAndDeformMesh(entity, mesh);
}

void RenderSystem::SetAndDeformMesh(Entity entity, HashValue component_id,
                                    const TriangleMesh<VertexPT>& mesh) {
  impl_->SetAndDeformMesh(entity, component_id, mesh);
}

void RenderSystem::SetMesh(Entity e, const MeshData& mesh) {
  impl_->SetMesh(e, mesh);
}

void RenderSystem::SetMesh(Entity e, const std::string& file) {
  impl_->SetMesh(e, file);
}

ShaderPtr RenderSystem::GetShader(Entity entity) const {
  return impl_->GetShader(entity);
}

ShaderPtr RenderSystem::GetShader(Entity entity, HashValue component_id) const {
  return impl_->GetShader(entity, component_id);
}

void RenderSystem::SetShader(Entity entity, const ShaderPtr& shader) {
  impl_->SetShader(entity, shader);
}

void RenderSystem::SetShader(Entity entity, HashValue component_id,
                             const ShaderPtr& shader) {
  impl_->SetShader(entity, component_id, shader);
}

void RenderSystem::SetShader(Entity entity, const std::string& file) {
  SetShader(entity, LoadShader(file));
}

void RenderSystem::SetFont(Entity entity, const FontPtr& font) {
  impl_->SetFont(entity, font);
}

void RenderSystem::SetTextSize(Entity entity, int size) {
  impl_->SetTextSize(entity, size);
}

RenderSystem::SortOrderOffset RenderSystem::GetSortOrderOffset(Entity e) const {
  return impl_->GetSortOrderOffset(e);
}

void RenderSystem::SetSortOrderOffset(Entity e,
                                      SortOrderOffset sort_order_offset) {
  impl_->SetSortOrderOffset(e, sort_order_offset);
}

void RenderSystem::SetSortOrderOffset(Entity e, HashValue component_id,
                                      SortOrderOffset sort_order_offset) {
  impl_->SetSortOrderOffset(e, component_id, sort_order_offset);
}

void RenderSystem::SetStencilMode(Entity e, StencilMode mode, int value) {
  impl_->SetStencilMode(e, mode, value);
}

bool RenderSystem::IsTextureSet(Entity e, int unit) const {
  return impl_->IsTextureSet(e, unit);
}

bool RenderSystem::IsTextureLoaded(Entity e, int unit) const {
  return impl_->IsTextureLoaded(e, unit);
}

bool RenderSystem::IsTextureLoaded(const TexturePtr& texture) const {
  return impl_->IsTextureLoaded(texture);
}

bool RenderSystem::IsReadyToRender(Entity entity) const {
  return impl_->IsReadyToRender(entity);
}

bool RenderSystem::IsHidden(Entity e) const { return impl_->IsHidden(e); }

void RenderSystem::SetDeformationFunction(Entity e, const Deformation& deform) {
  impl_->SetDeformationFunction(e, deform);
}

void RenderSystem::Hide(Entity e) { impl_->Hide(e); }

void RenderSystem::Show(Entity e) { impl_->Show(e); }

void RenderSystem::SetRenderPass(Entity e, RenderPass pass) {
  impl_->SetRenderPass(e, pass);
}

void RenderSystem::SetSortMode(RenderPass pass, SortMode mode) {
  impl_->SetSortMode(pass, mode);
}

void RenderSystem::SetCullMode(RenderPass pass, CullMode mode) {
  impl_->SetCullMode(pass, mode);
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

void RenderSystem::SetViewport(const View& view) {
  impl_->SetViewport(view);
}

void RenderSystem::SetClipFromModelMatrix(const mathfu::mat4& mvp) {
  impl_->SetClipFromModelMatrix(mvp);
}

mathfu::vec4 RenderSystem::GetClearColor() const {
  return impl_->GetClearColor();
}

void RenderSystem::SetClearColor(float r, float g, float b, float a) {
  impl_->SetClearColor(r, g, b, a);
}

void RenderSystem::Render(const View* views, size_t num_views) {
  impl_->Render(views, num_views);
}

void RenderSystem::Render(const View* views, size_t num_views,
                          RenderPass pass) {
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

void RenderSystem::BeginRendering() { impl_->BeginRendering(); }

void RenderSystem::EndRendering() { impl_->EndRendering(); }

void RenderSystem::UpdateDynamicMesh(
    Entity entity, PrimitiveType primitive_type,
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

void RenderSystem::BindUniform(const char* name,
    const float* data, int dimension) {
 impl_->BindUniform(name, data, dimension);
}

void RenderSystem::DrawPrimitives(PrimitiveType primitive_type,
                    const VertexFormat& vertex_format,
                    const void* vertex_data, size_t num_vertices) {
  impl_->DrawPrimitives(primitive_type, vertex_format, vertex_data,
                        num_vertices);
}

void RenderSystem::DrawIndexedPrimitives(PrimitiveType primitive_type,
                           const VertexFormat& vertex_format,
                           const void* vertex_data, size_t num_vertices,
                           const uint16_t* indices, size_t num_indices) {
  impl_->DrawIndexedPrimitives(primitive_type, vertex_format, vertex_data,
                               num_vertices, indices, num_indices);
}

const std::vector<mathfu::vec3>* RenderSystem::GetCaretPositions(
    Entity e) const {
  return impl_->GetCaretPositions(e);
}

void RenderSystem::CreateRenderTarget(HashValue render_target_name,
                        const mathfu::vec2i& dimensions,
                        TextureFormat texture_format,
                        DepthStencilFormat depth_stencil_format) {
  impl_->CreateRenderTarget(render_target_name, dimensions, texture_format,
                            depth_stencil_format);
}

void RenderSystem::BindUniform(const Uniform& uniform) {
  impl_->BindUniform(uniform);
}

}  // namespace lull
