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

#include "lullaby/modules/debug/debug_render_impl.h"

#include "lullaby/modules/render/mesh_data.h"

namespace lull {

constexpr const char* kShapeShader = "shaders/vertex_color.fplshader";
constexpr const char* kFontShader = "shaders/texture.fplshader";
constexpr const char* kFontTexture = "textures/debug_font.webp";
constexpr float kUVBounds[4] = {0.0f, 0.0f, 1.0f, 1.0f};
constexpr float kFontSize = 0.12f;

DebugRenderImpl::DebugRenderImpl(Registry* registry) : registry_(registry) {
  render_system_ = registry_->Get<RenderSystem>();
  shape_shader_ = render_system_->LoadShader(kShapeShader);
  font_shader_ = render_system_->LoadShader(kFontShader);
  font_texture_ = render_system_->LoadTexture(kFontTexture);
  font_.reset(new SimpleFont(font_shader_, font_texture_));
}

DebugRenderImpl::~DebugRenderImpl() {}

void DebugRenderImpl::Begin(const RenderSystem::View* views, size_t num_views) {
  views_ = views;
  num_views_ = num_views;
  render_system_->SetDepthTest(false);
  render_system_->SetDepthWrite(false);
}

void DebugRenderImpl::End() {
  // TODO(b/66690010) Reset depth test and depth write to original state.
  render_system_->SetDepthTest(true);
  render_system_->SetDepthWrite(true);
}

void DebugRenderImpl::DrawLine(const mathfu::vec3& start_point,
                               const mathfu::vec3& end_point,
                               const Color4ub color) {
  verts_.clear();
  verts_.emplace_back(start_point.x, start_point.y, start_point.z, color);
  verts_.emplace_back(end_point.x, end_point.y, end_point.z, color);

  for (size_t i = 0; i < num_views_; ++i) {
    render_system_->SetViewport(views_[i]);
    render_system_->SetClipFromModelMatrix(views_[i].clip_from_world_matrix);
    render_system_->BindShader(shape_shader_);
    render_system_->DrawPrimitives(lull::RenderSystem::PrimitiveType::kLines,
                                   verts_);
  }
}

void DebugRenderImpl::DrawText3D(const mathfu::vec3& pos, const Color4ub color,
                                 const char* text) {
  font_->SetSize(kFontSize);
  const mathfu::vec4 cv = Color4ub::ToVec4(color);
  const float cf[4] = {cv.x, cv.y, cv.z, cv.w};
  for (size_t i = 0; i < num_views_; ++i) {
    TriangleMesh<VertexPT> mesh;
    render_system_->SetViewport(views_[i]);
    render_system_->SetClipFromModelMatrix(views_[i].clip_from_eye_matrix);
    mathfu::vec3 eye_space_pos =
        views_[i].world_from_eye_matrix.Inverse() * pos;
    font_->AddStringToMesh(text, &mesh, &eye_space_pos);
    render_system_->BindShader(font_->GetShader());
    render_system_->BindTexture(0, font_->GetTexture());
    render_system_->BindUniform("uv_bounds", kUVBounds, 4);
    render_system_->BindUniform("color", cf, 4);
    registry_->Get<lull::RenderSystem>()->DrawMesh(mesh);
  }
}

void DebugRenderImpl::DrawText2D(const Color4ub color, const char* text) {
  const float kTopOfTextScreenScale = 0.40f;
  const float kFontScreenScale = .075f;
  const float z = -1.0f;
  const float tan_half_fov = 1.0f / views_[0].clip_from_eye_matrix[5];
  const float font_size = .5f * kFontScreenScale * -z * tan_half_fov;
  font_->SetSize(font_size);
  const mathfu::vec3 start_pos =
      mathfu::vec3(-.5f, kTopOfTextScreenScale * -z * tan_half_fov, z);
  const mathfu::vec4 cv = Color4ub::ToVec4(color);
  const float cf[4] = {cv.x, cv.y, cv.z, cv.w};
  for (size_t i = 0; i < num_views_; ++i) {
    TriangleMesh<VertexPT> mesh;
    render_system_->SetViewport(views_[i]);
    render_system_->SetClipFromModelMatrix(views_[i].clip_from_eye_matrix);
    mathfu::vec3 eye_space_pos =
        views_[i].world_from_eye_matrix.Inverse() *
        (views_[0].world_from_eye_matrix * start_pos);
    font_->AddStringToMesh(text, &mesh, &eye_space_pos);
    render_system_->BindShader(font_->GetShader());
    render_system_->BindTexture(0, font_->GetTexture());
    render_system_->BindUniform("uv_bounds", kUVBounds, 4);
    render_system_->BindUniform("color", cf, 4);
    registry_->Get<lull::RenderSystem>()->DrawMesh(mesh);
  }
}

void DebugRenderImpl::DrawBox3D(const mathfu::mat4& world_from_object_matrix,
                                const Aabb& box, Color4ub color) {
  constexpr int kNumCorners = 8;
  mathfu::vec3 corners[kNumCorners];
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

  for (size_t i = 0; i < num_views_; ++i) {
    render_system_->SetViewport(views_[i]);
    render_system_->SetClipFromModelMatrix(views_[i].clip_from_world_matrix);
    render_system_->BindShader(shape_shader_);
    render_system_->DrawIndexedPrimitives(MeshData::PrimitiveType::kTriangles,
                                          verts_.data(), verts_.size(), indices,
                                          kNumIndices);
  }
}

}  // namespace lull
