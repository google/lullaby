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

#ifndef LULLABY_SYSTEMS_TEXT_FLATUI_TEXT_BUFFER_H_
#define LULLABY_SYSTEMS_TEXT_FLATUI_TEXT_BUFFER_H_

#include <memory>
#include <string>
#include <vector>

#include "flatui/font_manager.h"
#include "lullaby/modules/render/triangle_mesh.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/systems/text/flatui/font.h"
#include "lullaby/systems/text/html_tags.h"
#include "lullaby/generated/text_def_generated.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Options for generating a text buffer.
struct TextBufferParams {
  std::string ellipsis;
  mathfu::vec2 bounds = mathfu::kZeros2f;
  float font_size = 0;
  float line_height_scale = 1.2f;
  float kerning_scale = 1.f;
  HorizontalAlignment horizontal_align = HorizontalAlignment_Center;
  VerticalAlignment vertical_align = VerticalAlignment_Baseline;
  TextDirection direction = TextDirection_LeftToRight;
  TextHtmlMode html_mode = TextHtmlMode_Ignore;
  TextWrapMode wrap_mode = TextWrapMode_None;
};

// A TextBuffer holds the data necessary to render a text string: vertices,
// indices, textures.
class TextBuffer {
 public:
  static std::shared_ptr<TextBuffer> Create(flatui::FontManager* manager,
                                            const std::string& text,
                                            const TextBufferParams& params);

  ~TextBuffer();

  bool IsReady() const;

  // TODO(b/33705855) Remove Finalize.
  void Finalize();

  size_t GetNumSlices() const;

  std::vector<VertexPT>& GetVertices() { return vertices_; }

  const std::vector<VertexPT>& GetVertices() const { return vertices_; }

  const std::vector<VertexPT>& GetUnderlineVertices() const {
    return underline_vertices_;
  }

  fplbase::Texture* GetSliceTexture(size_t i) const;
  bool IsLinkSlice(size_t i) const;

  TriangleMesh<VertexPT> BuildSliceMesh(size_t i) const;

  TriangleMesh<VertexPT> BuildUnderlineMesh() const;

  const Aabb& GetAabb() { return aabb_; }

  void SetAabb(const Aabb& aabb) { aabb_ = aabb; }

  const std::vector<mathfu::vec3>& GetCaretPositions() {
    return caret_positions_;
  }

  const std::vector<LinkTag>& GetLinks() { return links_; }

 private:
  TextBuffer(flatui::FontManager* manager, flatui::FontBuffer* buffer,
             const TextBufferParams& params);

  flatui::FontManager* font_manager_;
  flatui::FontBuffer* font_buffer_;
  std::vector<VertexPT> vertices_;
  TextBufferParams params_;
  Aabb aabb_;
  std::vector<mathfu::vec3> caret_positions_;
  std::vector<LinkTag> links_;
  std::vector<VertexPT> underline_vertices_;
};

using TextBufferPtr = std::shared_ptr<TextBuffer>;

}  // namespace lull

#endif  // LULLABY_SYSTEMS_TEXT_FLATUI_TEXT_BUFFER_H_
