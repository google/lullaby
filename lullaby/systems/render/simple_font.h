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

#ifndef LULLABY_SYSTEMS_RENDER_SIMPLE_FONT_H_
#define LULLABY_SYSTEMS_RENDER_SIMPLE_FONT_H_

#include "lullaby/systems/render/shader.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/modules/render/triangle_mesh.h"
#include "lullaby/modules/render/vertex.h"

namespace lull {

// SimpleFont provides a very simple, limited ASCII font solution.  Its texture
// is expected to a grid of monospace ascii characters from 32 ' ' to 96 '`'.
// Line wrapping and i18n are not supported.
class SimpleFont {
 public:
  SimpleFont(ShaderPtr shader, TexturePtr texture);

  // Returns the shader used to draw the font.
  const ShaderPtr& GetShader() const { return shader_; }

  // Returns the texture containing the font's glyphs.
  const TexturePtr& GetTexture() const { return texture_; }

  // Returns the vertical size of a character.
  float GetSize() const { return size_; }

  // Sets the vertical size of a character to |size|.
  void SetSize(float size);

  // Adds the geometry to render |str| to |mesh| at |pos|.  Out-of-range
  // characters are ignored.  |pos| is updated as glyphs are drawn.
  void AddStringToMesh(const char* str, TriangleMesh<VertexPT>* mesh,
                       mathfu::vec3* pos) const;

  // Creates and returns a mesh for the string |str|.  Out-of-range characters
  // are ignored.
  TriangleMesh<VertexPT> GetMeshForString(const char* str) const;
  TriangleMesh<VertexPT> GetMeshForString(const char* str,
                                          const mathfu::vec3& pos) const;

 private:
  ShaderPtr shader_;
  TexturePtr texture_;
  float size_;
};

// SimpleFontRenderer provides a simple way to combine multiple strings into a
// single mesh.
class SimpleFontRenderer {
 public:
  explicit SimpleFontRenderer(SimpleFont* font);

  const TriangleMesh<VertexPT>& GetMesh() const { return mesh_; }

  void SetCursor(const mathfu::vec3& pos) { cursor_ = pos; }

  void SetSize(float size) { font_->SetSize(size); }

  // Adds |str|'s glyphs to the mesh and updates the cursor.
  void Print(const char* str);

 private:
  SimpleFont* font_;
  TriangleMesh<VertexPT> mesh_;
  mathfu::vec3 cursor_;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_SIMPLE_FONT_H_

