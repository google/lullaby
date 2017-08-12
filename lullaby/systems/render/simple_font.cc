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

#include "lullaby/systems/render/simple_font.h"

#include "lullaby/util/logging.h"

namespace lull {

// TODO(b/28612528) Improve/cleanup glyph rendering.
static const int kZeroChar = ' ';
static const int kMaxChar = '`';
static const int kNumCols = 13;
static const float kTextureGlyphWidth = 1.0f / kNumCols - .5f / 256.0f;
static const float kTextureLineHeight = 43.0f / 256.0f;


SimpleFont::SimpleFont(ShaderPtr shader, TexturePtr texture)
    : shader_(shader), texture_(texture), size_(16) {}

void SimpleFont::SetSize(float size) { size_ = size; }

void SimpleFont::AddStringToMesh(const char* str, TriangleMesh<VertexPT>* mesh,
                                 mathfu::vec3* cursor_pos) const {
  if (!str) {
    LOG(DFATAL) << "Must provide a string to add!";
    return;
  }

  mathfu::vec3 pos = *cursor_pos;

  for (const char* ptr = str; *ptr; ++ptr) {
    char c = *ptr;

    if (c == '\n') {
      pos.x = cursor_pos->x;
      pos.y -= size_;
      continue;
    }
    if (c >= 'a' && c <= 'z') {
      c = static_cast<char>(c + 'A' - 'a');
    }
    if (c < kZeroChar || c > kMaxChar) {
      continue;
    }

    const int index = c - kZeroChar;
    const int row = index / kNumCols;
    const int col = index % kNumCols;
    const float dx = size_;
    const float dy = size_;
    const float du = kTextureGlyphWidth;
    const float dv = kTextureLineHeight;

    VertexPT verts[4];
    SetPosition(&verts[0], pos);
    SetUv0(&verts[0], static_cast<float>(col) * du,
           static_cast<float>(row + 1) * dv);

    SetPosition(&verts[1], verts[0].x, verts[0].y + dy, verts[0].z);
    SetUv0(&verts[1], verts[0].u0, verts[0].v0 - dv);

    SetPosition(&verts[2], verts[0].x + dx, verts[0].y, verts[0].z);
    SetUv0(&verts[2], verts[0].u0 + du, verts[0].v0);

    SetPosition(&verts[3], verts[2].x, verts[1].y, verts[0].z);
    SetUv0(&verts[3], verts[2].u0, verts[1].v0);

    const TriangleMesh<VertexPT>::Index idx[4] = {
        mesh->AddVertex(verts[0]), mesh->AddVertex(verts[1]),
        mesh->AddVertex(verts[2]), mesh->AddVertex(verts[3])};

    mesh->AddTriangle(idx[0], idx[2], idx[1]);
    mesh->AddTriangle(idx[1], idx[2], idx[3]);

    pos.x += dx;
  }

  *cursor_pos = pos;
}

TriangleMesh<VertexPT> SimpleFont::GetMeshForString(
    const char* str, const mathfu::vec3& initial_pos) const {
  mathfu::vec3 pos = initial_pos;
  TriangleMesh<VertexPT> mesh;
  AddStringToMesh(str, &mesh, &pos);
  return mesh;
}

TriangleMesh<VertexPT> SimpleFont::GetMeshForString(const char* str) const {
  return GetMeshForString(str, mathfu::kZeros3f);
}

SimpleFontRenderer::SimpleFontRenderer(SimpleFont* font)
    : font_(font), cursor_(0, 0, 0) {}

void SimpleFontRenderer::Print(const char* str) {
  font_->AddStringToMesh(str, &mesh_, &cursor_);
}

}  // namespace lull
