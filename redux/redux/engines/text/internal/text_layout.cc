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

#include "redux/engines/text/internal/text_layout.h"

#include "redux/engines/text/internal/glyph.h"
#include "redux/engines/text/internal/locale.h"
#include "redux/modules/base/data_builder.h"
#include "redux/modules/base/logging.h"

namespace redux {

TextLayout::TextLayout(const TextParams& params, float rasterization_size)
    : params_(params), rasterization_size_(rasterization_size) {
  CHECK(params.font);
  CHECK_GT(params.font_size, 0.0f);
  CHECK_GT(params.line_height, 0.0f);
}

MeshData TextLayout::GenerateMesh(std::string_view text,
                                  const GlyphSequence& sequence) {
  bounds_.reserve(sequence.elements.size());
  for (size_t i = 0; i < sequence.elements.size(); ++i) {
    const Bounds2f r = params_.font->GetGlyphBounds(sequence.elements[i].id);
    bounds_.push_back(r);
  }

  StartNewLine();
  if (params_.wrap == TextWrapMode::kNone) {
    // Even if we're not wrapping, we still need pay attention to newlines in
    // the text itself.
    SplitLines(text, sequence);
  } else {
    WrapText(text, sequence);
  }
  // End the current line to finish any positioning.
  EndLine();

  return BuildMesh(sequence);
}

void TextLayout::StartNewLine() {
  size_t end = 0;
  if (current_line_) {
    end = current_line_->start_index + current_line_->num_glyphs + 1;
  }
  EndLine();
  lines_.emplace_back();
  current_line_ = &lines_.back();
  current_line_->start_index = end;
}

void TextLayout::EndLine() {
  if (current_line_ == nullptr) {
    return;
  }

  // Move the line into the bounds rect and apply alignments.
  vec2 offset = params_.bounds.min;
  vec2 size = params_.bounds.Size();
  const float line_width = current_line_->width;
  const float bounds_width = size.x;
  const float bounds_height = size.y;
  const float ascender = params_.font->GetAscender();
  const float descender = params_.font->GetDescender();

  TextDirection direction = params_.text_direction;
  if (direction == TextDirection::kLanguageDefault) {
    direction = GetDefaultTextDirection(params_.language_iso_639.data());
  }

  switch (params_.horizontal_alignment) {
    case HorizontalTextAlignment::kLeft:
      if (direction == TextDirection::kRightToLeft) {
        offset.x -= bounds_width - line_width;
      }
      break;
    case HorizontalTextAlignment::kCenter:
      if (direction == TextDirection::kLeftToRight) {
        offset.x += (bounds_width - line_width) * 0.5f;
      } else {
        offset.x -= (bounds_width - line_width) * 0.5f;
      }
      break;
    case HorizontalTextAlignment::kRight:
      if (direction == TextDirection::kLeftToRight) {
        offset.x += bounds_width - line_width;
      }
      break;
    default:
      LOG(FATAL) << "Invalid horizontal alignment "
                 << static_cast<int>(params_.horizontal_alignment);
      break;
  }

  switch (params_.vertical_alignment) {
    case VerticalTextAlignment::kTop:
      offset.y += bounds_height - ascender;
      break;
    case VerticalTextAlignment::kCenter:
      offset.y += 0.5f * (bounds_height - ascender - descender);
      break;
    case VerticalTextAlignment::kBaseline:
      // Empty.
      break;
    case VerticalTextAlignment::kBottom:
      offset.y -= descender;
      break;
    default:
      LOG(FATAL) << "Invalid vertical alignment "
                 << static_cast<int>(params_.vertical_alignment);
      break;
  }

  // Adjust all glyphs on this line by the calculated alignment offset.
  const size_t start = current_line_->start_index;
  const size_t end = start + current_line_->num_glyphs;
  for (size_t i = start; i < end; ++i) {
    bounds_[i].min += offset;
    bounds_[i].max += offset;
  }

  // Move the cursor the start of the next line to end the current line.
  cursor_.x = 0.0f;
  cursor_.y -= params_.line_height;
  current_line_ = nullptr;
}

void TextLayout::PlaceGlyphsOnCurrentLine(size_t start, size_t end,
                                          const GlyphSequence& sequence) {
  if (current_line_ == nullptr) {
    StartNewLine();
  }

  TextDirection direction = params_.text_direction;
  if (direction == TextDirection::kLanguageDefault) {
    direction = GetDefaultTextDirection(params_.language_iso_639.data());
  }

  for (size_t i = start; i < end; ++i) {
    CHECK(i < bounds_.size());
    const float x = params_.font->GetGlyphAdvance(sequence.elements[i].id);

    if (direction == TextDirection::kRightToLeft) {
      cursor_.x -= x;
    }
    const auto size = bounds_[i].Size();
    bounds_[i].min = cursor_;
    bounds_[i].max = bounds_[i].min + size;
    placed_glyphs_++;
    if (direction == TextDirection::kLeftToRight) {
      cursor_.x += x;
    }
  }

  current_line_->num_glyphs += (end - start);
  current_line_->width = std::abs(cursor_.x);
}

static bool IsNewLine(char c) { return c == '\n'; }

void TextLayout::SplitLines(std::string_view text,
                            const GlyphSequence& sequence) {
  size_t line_start = 0;

  const size_t num_glyphs = bounds_.size();
  for (size_t i = 0; i < num_glyphs; ++i) {
    const auto& glyph = sequence.elements[i];
    if (IsNewLine(text[glyph.character_index])) {
      if (i > line_start) {
        PlaceGlyphsOnCurrentLine(line_start, i, sequence);
      }
      StartNewLine();
      line_start = i + 1;
    }
  }

  if (line_start < bounds_.size()) {
    PlaceGlyphsOnCurrentLine(line_start, num_glyphs, sequence);
  }
}

void TextLayout::WrapText(std::string_view text,
                          const GlyphSequence& sequence) {
  size_t word_start = 0;
  for (size_t i = 0; i < sequence.elements.size(); ++i) {
    const uint32_t break_index = sequence.elements[i].character_index;
    const TextCharacterBreakType break_type = sequence.breaks[break_index];
    if (break_type == TextCharacterBreakType::kMustBreakNewLine) {
      StartNewLine();

      if (break_index == sequence.breaks.size() - 1) {
        // Skip other glyphs which come from the same character.
        while (i < sequence.elements.size() &&
               sequence.elements[i].character_index == break_index) {
          ++i;
        }
      }

      PlaceWrappedWord(word_start, i, sequence);
      word_start = i + 1;
    } else if (break_type == TextCharacterBreakType::kCanBreakBetweenWords) {
      PlaceWrappedWord(word_start, i, sequence);
      // TODO(b/72092385) Skip spaces at beginning of line.
      word_start = i;
    }
  }

  DCHECK_GE(word_start, sequence.elements.size());
}

void TextLayout::PlaceWrappedWord(size_t start, size_t end,
                                  const GlyphSequence& sequence) {
  if (current_line_) {
    float word_width = 0;
    for (size_t i = start; i < end; ++i) {
      word_width += bounds_[i].Size().x;
    }

    const float line_width = current_line_->width;
    const float max_width = params_.bounds.Size().x;
    if (line_width + word_width > max_width) {
      StartNewLine();
    }
  }

  return PlaceGlyphsOnCurrentLine(start, end, sequence);
}

static void AddVertex(DataBuilder* vertices, const vec2& pos, const vec2& uv) {
  vertices->Append<float>({pos.x, pos.y, 0.f, 1.f, 1.f, 1.f, 1.f, uv.x, uv.y});
}

MeshData TextLayout::BuildMesh(const GlyphSequence& sequence) const {
  const VertexFormat format({
      {VertexUsage::Position, VertexType::Vec3f},
      {VertexUsage::Color0, VertexType::Vec4f},
      {VertexUsage::TexCoord0, VertexType::Vec2f},
  });

  const size_t num_vertices = bounds_.size() * 6;
  DataBuilder vertices(num_vertices * format.GetVertexSize());
  const float scale = (params_.font_size / rasterization_size_);

  Box bounds;
  for (size_t i = 0; i < sequence.elements.size(); ++i) {
    const TextGlyphId id = sequence.elements[i].id;
    const Bounds2f uvs = params_.font->GetGlyphUvBounds(id);
    const vec2 pos =
        (bounds_[i].min + params_.font->GetGlyphSubBounds(id).min) * scale;
    const vec2 d = bounds_[i].Size() * scale;
    const float d_u = uvs.Size().x;
    const float d_v = uvs.Size().y;

    AddVertex(&vertices, pos + vec2(0.f, 0.f), uvs.min + vec2(0.f, d_v));
    AddVertex(&vertices, pos + vec2(0.f, d.y), uvs.min + vec2(0.f, 0.f));
    AddVertex(&vertices, pos + vec2(d.x, 0.f), uvs.min + vec2(d_u, d_v));
    AddVertex(&vertices, pos + vec2(d.x, 0.f), uvs.min + vec2(d_u, d_v));
    AddVertex(&vertices, pos + vec2(0.f, d.y), uvs.min + vec2(0.f, 0.f));
    AddVertex(&vertices, pos + vec2(d.x, d.y), uvs.min + vec2(d_u, 0.f));

    bounds.min = Min(bounds.min, vec3(pos.x, pos.y, 0.f));
    bounds.max = Min(bounds.max, vec3(pos.x + d.x, pos.y + d.y, 0.f));
  }

  MeshData::PartData part;
  part.primitive_type = MeshPrimitiveType::Triangles;
  part.start = 0;
  part.end = num_vertices;
  part.box = bounds;
  DataBuilder parts(sizeof(MeshData::PartData));
  parts.Append(part);

  MeshData mesh_data;
  mesh_data.SetVertexData(format, vertices.Release(), bounds);
  mesh_data.SetParts(parts.Release());
  return mesh_data;
}
}  // namespace redux
