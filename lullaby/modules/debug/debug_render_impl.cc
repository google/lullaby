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

#include "lullaby/modules/debug/debug_render_impl.h"


#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/mesh_util.h"

namespace lull {

constexpr const char* kShapeShader = "shaders/vertex_color.fplshader";
constexpr const char* kTextureShader = "shaders/texture.fplshader";
constexpr const char* kTexture2DShader = "shaders/texture_2d.fplshader";
constexpr const char* kTexture2DExternalOesShader =
    "shaders/texture_2d_external_oes.fplshader";
constexpr const char* kFontShader = "shaders/texture.fplshader";
constexpr const char* kFontTexture = "textures/debug_font.webp";
constexpr float kUVBounds[4] = {0.0f, 0.0f, 1.0f, 1.0f};
constexpr float kFontSize = 0.12f;

struct NormalizedCoordinates {
  mathfu::vec3 pos0;
  mathfu::vec3 pos1;
};

// Convert from screen coordinates to normalized device coordinates [-1.0, 1.0].
static NormalizedCoordinates NormalizeScreenCoordinates(
    const mathfu::vec2& pixel_pos0, const mathfu::vec2& pixel_pos1,
    const mathfu::vec2i& dimensions) {
  // Flip Y axis from Y-down to Y-up, preserving quad winding.
  const float sx = 2.0f / dimensions.x;
  const float sy = 2.0f / dimensions.y;
  const float x0 = sx * pixel_pos0.x - 1.0f;
  const float y0 = 1.0f - sy * pixel_pos0.y;
  const float x1 = sx * pixel_pos1.x - 1.0f;
  const float y1 = 1.0f - sy * pixel_pos1.y;
  const float z = -1.0f;
  const NormalizedCoordinates coords = {
    mathfu::vec3(x0, y1, z),
    mathfu::vec3(x1, y0, z)
  };
  return coords;
}

static fplbase::RenderState GetRenderState() {
  fplbase::RenderState state;
  state.blend_state.enabled = true;
  state.blend_state.src_alpha = fplbase::BlendState::kSrcAlpha;
  state.blend_state.src_color = fplbase::BlendState::kSrcAlpha;
  state.blend_state.dst_alpha = fplbase::BlendState::kOneMinusSrcAlpha;
  state.blend_state.dst_color = fplbase::BlendState::kOneMinusSrcAlpha;
  state.depth_state.test_enabled = false;
  state.depth_state.write_enabled = false;
  state.cull_state.enabled = false;
  return state;
}

// By default, shaders are loaded from the assets directory. If a different path
// is used, include a prefix for that path.
DebugRenderImpl::DebugRenderImpl(Registry* registry, const std::string& prefix)
    : registry_(registry), views_(nullptr), num_views_(0), asset_prefix_(prefix), is_initialized_(false) {
  render_system_ = registry_->Get<RenderSystem>();
}

void DebugRenderImpl::Initialize() {
  // In some environments (like tests), the below load shader calls may cause a
  // crash.  To avoid this, only load the shaders when this something will
  // actually be drawn.
  if (is_initialized_) {
    // Already Initialized
    return;
  }
  is_initialized_ = true;
  shape_shader_ = render_system_->LoadShader(asset_prefix_ + kShapeShader);
  texture_shader_ = render_system_->LoadShader(asset_prefix_ + kTextureShader);
  texture_2d_shader_ =
      render_system_->LoadShader(asset_prefix_ + kTexture2DShader);
  texture_2d_external_oes_shader_ =
      render_system_->LoadShader(asset_prefix_ + kTexture2DExternalOesShader);
  font_shader_ = render_system_->LoadShader(asset_prefix_ + kFontShader);
  quad_mesh_ = CreateQuadMesh<VertexPT>(1.0f, 1.0f, 2, 2, 0.0f, 0);

  // Prevent crash loading texture when WebP decoder is disabled.
  // TODO: Use a trusted texture format so this is not necessary.
#if !LULLABY_DISABLE_WEBP_LOADER
  font_texture_ = render_system_->LoadTexture(asset_prefix_ + kFontTexture);
  font_.reset(new SimpleFont(font_shader_, font_texture_));
#endif  // !LULLABY_DISABLE_WEBP_LOADER
}

void DebugRenderImpl::SetState(State state) const {
  bool depth_test = false;
  bool depth_write = false;
  switch (state) {
    case kState2D:
      break;
    case kState3DTransparent:
      depth_test = true;
      break;
    case kState3DOpaque:
      depth_test = true;
      depth_write = true;
      break;
  }
  render_system_->SetDepthTest(depth_test);
  render_system_->SetDepthWrite(depth_write);
}

DebugRenderImpl::State DebugRenderImpl::Choose3DState(Color4ub color) {
  return color.a == 255 ? kState3DOpaque : kState3DTransparent;
}

DebugRenderImpl::~DebugRenderImpl() {}

void DebugRenderImpl::Begin(const RenderSystem::View* views, size_t num_views) {
  views_ = views;
  num_views_ = num_views;
  render_system_->UpdateCachedRenderState(GetRenderState());
}

void DebugRenderImpl::End() {
}

void DebugRenderImpl::DrawLine(const mathfu::vec3& start_point,
                               const mathfu::vec3& end_point,
                               const Color4ub color) {
  Initialize();
  SetState(Choose3DState(color));
  verts_.clear();
  verts_.emplace_back(start_point.x, start_point.y, start_point.z, color);
  verts_.emplace_back(end_point.x, end_point.y, end_point.z, color);
  MeshData mesh(
      MeshData::PrimitiveType::kLines, VertexPC::kFormat,
      DataContainer::WrapDataAsReadOnly(verts_.data(),
        VertexPC::kFormat.GetVertexSize() * verts_.size()));

  for (size_t i = 0; i < num_views_; ++i) {
    render_system_->SetViewport(views_[i]);
    render_system_->BindShader(shape_shader_);
    render_system_->DrawMesh(mesh, views_[i].clip_from_world_matrix);
  }
}

void DebugRenderImpl::DrawText3D(const mathfu::vec3& pos, const Color4ub color,
                                 const char* text) {
  Initialize();
  SetState(Choose3DState(color));
  CHECK(font_);
  font_->SetSize(kFontSize);
  const mathfu::vec4 cv = Color4ub::ToVec4(color);
  const float cf[4] = {cv.x, cv.y, cv.z, cv.w};
  for (size_t i = 0; i < num_views_; ++i) {
    render_system_->SetViewport(views_[i]);
    mathfu::vec3 eye_space_pos =
        views_[i].world_from_eye_matrix.Inverse() * pos;
    MeshData mesh = font_->CreateMeshForString(text, eye_space_pos);
    render_system_->BindShader(font_->GetShader());
    render_system_->BindTexture(0, font_->GetTexture());
    render_system_->BindUniform("uv_bounds", kUVBounds, 4);
    render_system_->BindUniform("color", cf, 4);
    render_system_->DrawMesh(mesh, views_[i].clip_from_eye_matrix);
  }
}

void DebugRenderImpl::DrawText2D(const Color4ub color, const char* text) {
  const float kTopOfTextScreenScale = 0.40f;
  const float kFontScreenScale = .075f;
  Initialize();
  SetState(kState2D);
  const float z = -1.0f;
  const float tan_half_fov = 1.0f / views_[0].clip_from_eye_matrix[5];
  const float font_size = .5f * kFontScreenScale * -z * tan_half_fov;
  CHECK(font_);
  font_->SetSize(font_size);
  const mathfu::vec3 start_pos =
      mathfu::vec3(-.5f, kTopOfTextScreenScale * -z * tan_half_fov, z);
  const mathfu::vec4 cv = Color4ub::ToVec4(color);
  const float cf[4] = {cv.x, cv.y, cv.z, cv.w};
  for (size_t i = 0; i < num_views_; ++i) {
    render_system_->SetViewport(views_[i]);
    mathfu::vec3 eye_space_pos =
        views_[i].world_from_eye_matrix.Inverse() *
        (views_[0].world_from_eye_matrix * start_pos);
    MeshData mesh = font_->CreateMeshForString(text, eye_space_pos);
    render_system_->BindShader(font_->GetShader());
    render_system_->BindTexture(0, font_->GetTexture());
    render_system_->BindUniform("uv_bounds", kUVBounds, 4);
    render_system_->BindUniform("color", cf, 4);
    render_system_->DrawMesh(mesh, views_[i].clip_from_eye_matrix);
  }
}

void DebugRenderImpl::DrawBox3D(const mathfu::mat4& world_from_object_matrix,
                                const Aabb& box, Color4ub color) {
  constexpr int kNumCorners = 8;
  mathfu::vec3 corners[kNumCorners];
  Initialize();
  SetState(Choose3DState(color));
  GetTransformedBoxCorners(box, world_from_object_matrix, corners);

  verts_.clear();
  verts_.reserve(kNumCorners);
  for (int i = 0; i < kNumCorners; ++i) {
    verts_.emplace_back(corners[i], color);
  }

  constexpr int kNumIndices = 6 * 2 * 3;  // 6 faces * 2 triangles * 3 indices
  constexpr uint16_t indices[kNumIndices] = {
      // -x face
      0, 1, 3, 0, 3, 2,
      // -y face
      0, 4, 5, 0, 5, 1,
      // -z face
      0, 2, 6, 0, 6, 4,
      // +x face
      4, 6, 7, 4, 7, 5,
      // +y face
      2, 3, 7, 2, 7, 6,
      // +z face
      1, 5, 7, 1, 7, 3,
  };

  MeshData mesh(MeshData::PrimitiveType::kTriangles, VertexPC::kFormat,
                DataContainer::WrapDataAsReadOnly(verts_.data(),
                  VertexPC::kFormat.GetVertexSize() * verts_.size()),
                MeshData::kIndexU16,
                DataContainer::WrapDataAsReadOnly(indices,
                  kNumIndices * sizeof(uint16_t)));
  for (size_t i = 0; i < num_views_; ++i) {
    render_system_->SetViewport(views_[i]);
    render_system_->BindShader(shape_shader_);
    render_system_->DrawMesh(mesh, views_[i].clip_from_world_matrix);
  }
}

void DebugRenderImpl::DrawQuad2D(const Color4ub color, float x, float y,
                                 float w, float h, const TexturePtr& texture) {
  const float z = -1.0f;
  Initialize();
  SetState(kState2D);
  const mathfu::vec3 pos0(x - w, y - h, z);
  const mathfu::vec3 pos1(x + w, y + h, z);
  const mathfu::vec4 cv = Color4ub::ToVec4(color);
  for (size_t i = 0; i < num_views_; ++i) {
    const RenderSystem::View& view = views_[i];
    render_system_->SetViewport(view);
    SubmitQuad2D(cv, pos0, mathfu::kZeros2f, pos1, mathfu::kOnes2f, texture);
  }
}

void DebugRenderImpl::DrawQuad2DAbsolute(
    const mathfu::vec4& color,
    const mathfu::vec2& pixel_pos0, const mathfu::vec2& uv0,
    const mathfu::vec2& pixel_pos1, const mathfu::vec2& uv1,
    const TexturePtr& texture) {
  Initialize();
  SetState(kState2D);
  for (size_t i = 0; i < num_views_; ++i) {
    const RenderSystem::View& view = views_[i];
    render_system_->SetViewport(view);
    const NormalizedCoordinates coords = NormalizeScreenCoordinates(
        pixel_pos0, pixel_pos1, view.dimensions);
    SubmitQuad2D(color, coords.pos0, uv0, coords.pos1, uv1, texture);
  }
}

void DebugRenderImpl::SubmitQuad2D(
    const mathfu::vec4& color,
    const mathfu::vec3& pos0, const mathfu::vec2& uv0,
    const mathfu::vec3& pos1, const mathfu::vec2& uv1,
    const TexturePtr& texture) const {
  const mathfu::vec3 center = 0.5f * (pos0 + pos1);
  const mathfu::vec3 dpos = pos1 - pos0;
  const mathfu::vec2 duv = uv1 - uv0;
  const float uv_bounds[] = {uv0.x, uv0.y, duv.x, duv.y};
  const float position_scale[] = {dpos.x, dpos.y, dpos.z, 0.0f};
  const float position_offset[] = {center.x, center.y, center.z, 1.0f};

  const ShaderPtr& shader = IsTextureExternalOes(texture)
                                ? texture_2d_external_oes_shader_
                                : texture_2d_shader_;
  CHECK(shader);
  render_system_->BindShader(shader);
  render_system_->BindTexture(0, texture);
  render_system_->BindUniform("uv_bounds", uv_bounds, 4);
  render_system_->BindUniform("position_offset", position_offset, 4);
  render_system_->BindUniform("position_scale", position_scale, 4);
  render_system_->BindUniform("color", &color.x, 4);
  render_system_->DrawMesh(quad_mesh_);
}

}  // namespace lull
