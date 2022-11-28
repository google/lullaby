/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_ENGINES_TEXT_INTERNAL_TEXT_LAYOUT_H_
#define REDUX_ENGINES_TEXT_INTERNAL_TEXT_LAYOUT_H_

#include "redux/engines/text/text_engine.h"
#include "redux/modules/graphics/mesh_data.h"

namespace redux {

class TextLayout {
 public:
  explicit TextLayout(const TextParams& params, float rasterization_size);

  MeshData GenerateMesh(std::string_view text, const GlyphSequence& sequence);

 private:
  void StartNewLine();
  void EndLine();
  void SplitLines(std::string_view text, const GlyphSequence& sequence);
  void WrapText(std::string_view text, const GlyphSequence& sequence);

  void PlaceGlyphsOnCurrentLine(size_t start, size_t end,
                                const GlyphSequence& sequence);
  void PlaceWrappedWord(size_t start, size_t end,
                        const GlyphSequence& sequence);
  MeshData BuildMesh(const GlyphSequence& sequence) const;

  struct Line {
    Line() = default;
    explicit Line(size_t start_index) : start_index(start_index) {}

    // Index of the first glyph on this line (from Layout::glyphs).
    size_t start_index = 0;
    // Number of glyphs on this line.
    size_t num_glyphs = 0;
    // Width of this line.
    float width = 0.0f;
  };

  const TextParams& params_;
  std::vector<Line> lines_;
  std::vector<Bounds2f> bounds_;
  Line* current_line_ = nullptr;
  vec2 cursor_ = {0, 0};
  size_t placed_glyphs_ = 0;
  float rasterization_size_ = 0.0f;
};

}  // namespace redux

#endif  // REDUX_ENGINES_TEXT_INTERNAL_TEXT_LAYOUT_H_
