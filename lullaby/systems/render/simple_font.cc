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

#include "lullaby/systems/render/simple_font.h"

#include "lullaby/util/logging.h"

namespace lull {

// TODO Improve/cleanup glyph rendering.
static constexpr int kZeroChar = ' ';
static constexpr int kMaxChar = '`';
static constexpr int kNumCols = 13;
static constexpr float kTextureGlyphWidth = 1.0f / kNumCols - .5f / 256.0f;
static constexpr float kTextureLineHeight = 43.0f / 256.0f;

template <typename T>
DataContainer WrapVectorAsReadOnly(const std::vector<T>& v) {
  const size_t size_in_bytes = v.size() * sizeof(T);
  return DataContainer::WrapDataAsReadOnly(
      reinterpret_cast<const uint8_t*>(v.data()), size_in_bytes);
}

MeshData SimpleFontMesh::GetMesh() const {
  return MeshData(MeshData::kTriangles, VertexPT::kFormat,
                  WrapVectorAsReadOnly(vertices_), MeshData::kIndexU16,
                  WrapVectorAsReadOnly(indices_));
}

MeshData SimpleFontMesh::CreateHeapCopyMesh() const {
  return MeshData(MeshData::kTriangles, VertexPT::kFormat,
                  WrapVectorAsReadOnly(vertices_).CreateHeapCopy(),
                  MeshData::kIndexU16,
                  WrapVectorAsReadOnly(indices_).CreateHeapCopy());
}

mathfu::vec3 SimpleFontMesh::AddGlyph(char c, const mathfu::vec3& pos,
                                      float size) {
  if (c >= 'a' && c <= 'z') {
    c = static_cast<char>(c + 'A' - 'a');
  }
  if (c < kZeroChar || c > kMaxChar) {
    return pos;
  }

  const int glyph_index = c - kZeroChar;
  const int row = glyph_index / kNumCols;
  const int col = glyph_index % kNumCols;
  const float dx = size;
  const float dy = size;
  const float du = kTextureGlyphWidth;
  const float dv = kTextureLineHeight;
  const float u0 = static_cast<float>(col) * du;
  const float v0 = static_cast<float>(row) * dv;

  const uint16_t start_index = static_cast<uint16_t>(vertices_.size());
  vertices_.emplace_back(pos.x, pos.y, pos.z, u0, v0 + dv);
  vertices_.emplace_back(pos.x, pos.y + dy, pos.z, u0, v0);
  vertices_.emplace_back(pos.x + dx, pos.y, pos.z, u0 + du, v0 + dv);
  vertices_.emplace_back(pos.x + dx, pos.y + dy, pos.z, u0 + du, v0);

  indices_.emplace_back(start_index);
  indices_.emplace_back(static_cast<uint16_t>(start_index + 2));
  indices_.emplace_back(static_cast<uint16_t>(start_index + 1));
  indices_.emplace_back(static_cast<uint16_t>(start_index + 1));
  indices_.emplace_back(static_cast<uint16_t>(start_index + 2));
  indices_.emplace_back(static_cast<uint16_t>(start_index + 3));

  return mathfu::vec3(pos.x + dx, pos.y, pos.z);
}

SimpleFont::SimpleFont(ShaderPtr shader, TexturePtr texture)
    : shader_(shader), texture_(texture), size_(16) {}

void SimpleFont::SetSize(float size) { size_ = size; }

void SimpleFont::AddStringToMesh(const char* str, SimpleFontMesh* mesh,
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
    } else {
      pos = mesh->AddGlyph(c, pos, size_);
    }
  }

  *cursor_pos = pos;
}

MeshData SimpleFont::CreateMeshForString(
    const char* str, const mathfu::vec3& initial_pos) const {
  mathfu::vec3 pos = initial_pos;
  SimpleFontMesh mesh;
  AddStringToMesh(str, &mesh, &pos);
  // The result needs its data to live beyond the current scope, so create an
  // independent mesh.
  return mesh.CreateHeapCopyMesh();
}

MeshData SimpleFont::CreateMeshForString(const char* str) const {
  return CreateMeshForString(str, mathfu::kZeros3f);
}

SimpleFontRenderer::SimpleFontRenderer(SimpleFont* font)
    : font_(font), cursor_(0, 0, 0) {}

void SimpleFontRenderer::Print(const char* str) {
  font_->AddStringToMesh(str, &mesh_, &cursor_);
}

}  // namespace lull
