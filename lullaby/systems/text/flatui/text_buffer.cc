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

#include "lullaby/systems/text/flatui/text_buffer.h"

#include "flatui/font_util.h"

namespace lull {
namespace {

constexpr float kPixelsFromMetersScale = 1000.0f;
constexpr float kMetersFromPixelsScale = 1.0f / kPixelsFromMetersScale;

Aabb GetBoundingBox(const std::vector<VertexPT>& vertices) {
  Aabb aabb;
  if (vertices.empty()) {
    return aabb;
  }
  aabb.min.x = vertices[0].x;
  aabb.min.y = vertices[0].y;
  aabb.min.z = vertices[0].z;
  aabb.max.x = vertices[0].x;
  aabb.max.y = vertices[0].y;
  aabb.max.z = vertices[0].z;
  for (const VertexPT& v : vertices) {
    if (aabb.min.x > v.x) {
      aabb.min.x = v.x;
    }
    if (aabb.min.y > v.y) {
      aabb.min.y = v.y;
    }
    if (aabb.min.z > v.z) {
      aabb.min.z = v.z;
    }
    if (aabb.max.x < v.x) {
      aabb.max.x = v.x;
    }
    if (aabb.max.y < v.y) {
      aabb.max.y = v.y;
    }
    if (aabb.max.z < v.z) {
      aabb.max.z = v.z;
    }
  }
  return aabb;
}

Aabb GetBoundingBox(const mathfu::vec4& rect) {
  static const int x_verts = 3;
  static const int y_verts = 3;
  std::vector<VertexPT> vertices;
  float x = rect[0];
  float y = rect[1];
  float diff_x = rect[2];
  float diff_y = rect[3];
  for (int i = 0; i < x_verts; ++i) {
    for (int j = 0; j < y_verts; ++j) {
      vertices.emplace_back(x + static_cast<float>(i) * diff_x / (x_verts - 1),
                            y + static_cast<float>(j) * diff_y / (y_verts - 1),
                            0.0f, 0.0f, 0.0f);
    }
  }

  return GetBoundingBox(vertices);
}

}  // namespace

TextBuffer::TextBuffer(flatui::FontManager* manager,
                       flatui::FontBuffer* font_buffer,
                       const TextBufferParams& params)
    : font_manager_(manager), font_buffer_(font_buffer), params_(params) {
  font_manager_->StartLayoutPass();

  // TODO(b/30064717): Don't make copy once flatui support creating font
  // buffer without cache.
  const std::vector<flatui::FontVertex> glyph_vertices =
      font_buffer_->get_vertices();
  vertices_.reserve(glyph_vertices.size());
  for (const auto& v : glyph_vertices) {
    vertices_.emplace_back(mathfu::vec3(v.position_), mathfu::vec2(v.uv_));
  }

  if (params.html_mode == TextHtmlMode_ExtractLinks) {
    for (const auto& link : font_buffer_->get_links()) {
      std::vector<Aabb> aabbs;
      std::vector<mathfu::vec4> boxes = font_buffer_->CalculateBounds(
          link.start_glyph_index, link.end_glyph_index);
      for (const auto& box : boxes) {
        // Convert from top-left, bottom-right to bottom-left, top-right
        aabbs.emplace_back(mathfu::vec3(box.x, box.w, 0.0f),
                           mathfu::vec3(box.z, box.y, 0.0f));
      }
      links_.push_back({link.link, aabbs});
    }
    const bool is_rtl = params.direction == TextDirection_RightToLeft;
    std::vector<mathfu::vec3_packed> underline_vertices =
        flatui::GenerateUnderlineVertices(*font_buffer_, mathfu::kZeros2f,
            is_rtl);
    underline_vertices_.reserve(underline_vertices.size());
    for (const mathfu::vec3_packed& position : underline_vertices) {
      underline_vertices_.emplace_back(mathfu::vec3(position),
                                       mathfu::kZeros2f);
    }
  }

  if (font_buffer_->HasCaretPositions()) {
    for (const mathfu::vec2i& pos : font_buffer_->GetCaretPositions()) {
      caret_positions_.emplace_back(pos[0], pos[1], 0);
    }
  }
}

TextBuffer::~TextBuffer() { font_manager_->ReleaseBuffer(font_buffer_); }

bool TextBuffer::IsReady() const {
  return font_manager_->GetFontBufferStatus(*font_buffer_) ==
         flatui::kFontBufferStatusReady;
}

size_t TextBuffer::GetNumSlices() const {
  return static_cast<const flatui::FontBuffer*>(font_buffer_)
      ->get_slices()
      .size();
}

fplbase::Texture* TextBuffer::GetSliceTexture(size_t i) const {
  const int slice_index = static_cast<const flatui::FontBuffer*>(font_buffer_)
                              ->get_slices()
                              .at(i)
                              .get_slice_index();
  return font_manager_->GetAtlasTexture(slice_index);
}

bool TextBuffer::IsLinkSlice(size_t i) const {
  const auto& attributes =
      static_cast<const flatui::FontBuffer*>(font_buffer_)->get_slices()[i];
  return attributes.get_underline();
}

void TextBuffer::Finalize() {
  // Font metrics definition from FlatUi:
  // ascender + descender = font size. Text is always aligned along baseline.
  // Baseline's y value is 0 + ascender. Font size usually equals to ascender
  // + descender. Height of specified rect is used by FlatUi to truncate the
  // text if ellipses string is specified and caret is disabled. Internal
  // leading is additional space above ascender. External leading is additinal
  // space below descender. Note that internal leading's definition is
  // different from standard one where internal leading is inside ascender.

  const float ascender = static_cast<float>(font_buffer_->metrics().ascender());

  // Scale it to match 1 px == 1 mm
  const float kScaleFactor = kMetersFromPixelsScale;

  // Get the size of the flatui text.  It will be exactly the size that wraps
  // the generated text, potentially smaller than the requested rect.
  const float x_size = static_cast<float>(font_buffer_->get_size().x);
  const float y_size = static_cast<float>(font_buffer_->get_size().y);

  // Lullaby text is aligned relative to the origin *and* within whichever
  // params.bounds were set.
  const float rect_width = params_.bounds.x * kPixelsFromMetersScale;
  // Without ellipsis, flatui will draw as many lines of text as requested,
  // ignoring rect_height even if set. To properly vertically align and wrap the
  // aabb, use the larger of rect_height or the generated height.
  const float rect_height = params_.bounds.y > 0 ?
      std::max(params_.bounds.y * kPixelsFromMetersScale, y_size) : 0;

  // The origin of flatui text is always exactly (0, 0), with the text aligned
  // to the top left and the positive y-axis pointing down.
  // First we must offset that to lullaby's desired origin alignment.

  float y_off = 0;
  switch (params_.vertical_align) {
    case VerticalAlignment_Top:
      // Flatui produces top aligned text, so keep same offset.
      break;
    case VerticalAlignment_Center:
      y_off = y_size / 2;
      break;
    case VerticalAlignment_Baseline:
      // Align to the baseline of the first line of the text. For this reason
      // baseline alignment is only meaningful to single line text.
      y_off = ascender;
      break;
    case VerticalAlignment_Bottom:
      y_off = y_size;
      break;
  }

  // Flatui does support horizontal alignment, so use the offset produced with
  // rect_width instead if it was set.
  float x_off = 0;
  float bounding_box_x = 0;  // origin of the box that wraps the text exactly.
  switch (params_.horizontal_align) {
    case HorizontalAlignment_Left:
      // Offset same as flatui since it is already left aligned.
      break;
    case HorizontalAlignment_Center:
      bounding_box_x = x_size / 2;
      x_off = rect_width != 0 ? rect_width / 2 : bounding_box_x;
      break;
    case HorizontalAlignment_Right:
      bounding_box_x = x_size;
      x_off = rect_width != 0 ? rect_width : bounding_box_x;
      break;
  }

  // Apply the offsets. Note that flatui positive y-axis points down, whereas
  // lullaby positive y-axis points up.
  for (mathfu::vec3& pos : caret_positions_) {
    pos.x = (pos.x - x_off) * kScaleFactor;
    // Flatui returns ascender (baseline) as caret y coordinate. We expect
    // caret position at half ascender.
    pos.y = (pos.y - y_off - ascender / 2) * -kScaleFactor;
  }

  for (VertexPT& v : vertices_) {
    v.x = kScaleFactor * (v.x - x_off);
    v.y = -kScaleFactor * (v.y - y_off);
  }

  for (VertexPT& v : underline_vertices_) {
    v.x = kScaleFactor * (v.x - x_off);
    v.y = -kScaleFactor * (v.y - y_off);
  }

  // Flatui doesn't add vertical alignment, so we need to offset our rect y
  // to produce the vertical alignment.
  float rect_y = 0;
  switch (params_.vertical_align) {
    case VerticalAlignment_Top:
    case VerticalAlignment_Baseline:
      // Top and baseline are the same as default flatui functionality.
      rect_y = y_off;
      break;
    case VerticalAlignment_Center:
      rect_y = rect_height / 2;
      break;
    case VerticalAlignment_Bottom:
      rect_y = rect_height;
      break;
  }

  // bounding_box_aabb is the size that wraps the text exactly.
  // rect_aabb is the size of the requested param's bounds.
  const mathfu::vec4 bounding_box(-bounding_box_x, y_off, x_size, -y_size);
  const Aabb bounding_box_aabb = GetBoundingBox(kScaleFactor * bounding_box);
  const mathfu::vec4 rect(-x_off, rect_y, rect_width, -rect_height);
  const Aabb rect_aabb = GetBoundingBox(kScaleFactor * rect);

  Aabb aabb;
  if (params_.wrap_mode != TextWrapMode_None) {
    // We want wrapping, so use the exact size.
    aabb = bounding_box_aabb;
  } else {
    // If rect width or height were not specified, use the exact size.
    // Otherwise use the rect value.
    aabb.min.x = rect_width == 0 ? bounding_box_aabb.min.x : rect_aabb.min.x;
    aabb.min.y = rect_height == 0 ? bounding_box_aabb.min.y : rect_aabb.min.y;
    aabb.min.z = bounding_box_aabb.min.z;
    aabb.max.x = rect_width == 0 ? bounding_box_aabb.max.x : rect_aabb.max.x;
    aabb.max.y = rect_height == 0 ? bounding_box_aabb.max.y : rect_aabb.max.y;
    aabb.max.z = bounding_box_aabb.max.z;
  }

  for (LinkTag& link : links_) {
    for (Aabb& link_aabb : link.aabbs) {
      link_aabb.min.x = link_aabb.min.x - x_off;
      link_aabb.min.y = y_off - link_aabb.min.y;
      link_aabb.max.x = link_aabb.max.x - x_off;
      link_aabb.max.y = y_off - link_aabb.max.y;

      mathfu::vec4 link_rect = mathfu::vec4(
          link_aabb.min.xy(), link_aabb.max.xy() - link_aabb.min.xy());
      link_aabb = GetBoundingBox(kScaleFactor * link_rect);
    }
  }

  SetAabb(aabb);
}

TriangleMesh<VertexPT> TextBuffer::BuildSliceMesh(size_t slice) const {
  const std::vector<uint16_t>& indices =
      static_cast<const flatui::FontBuffer*>(font_buffer_)
          ->get_indices(static_cast<int>(slice));
  TriangleMesh<VertexPT> mesh;
  mesh.GetIndices().reserve(indices.size());

  // Copy vertices and remap indices.
  for (const auto& index : indices) {
    const uint16_t new_index = mesh.AddVertex(vertices_[index]);
    mesh.GetIndices().emplace_back(new_index);
  }

  return mesh;
}

TriangleMesh<VertexPT> TextBuffer::BuildUnderlineMesh() const {
  TriangleMesh<VertexPT> mesh;
  if (underline_vertices_.size() < 3) {
    return mesh;
  }

  mesh.GetVertices() = underline_vertices_;

  // Underline vertices are arranged in triangle strips.  Convert them into
  // a triangle list.
  const size_t num_triangles = underline_vertices_.size() - 2;
  mesh.GetIndices().reserve(3 * num_triangles);

  for (size_t i = 2; i < underline_vertices_.size(); ++i) {
    const uint16_t index = static_cast<uint16_t>(i);
    if ((i % 2) == 0) {
      mesh.AddTriangle(static_cast<uint16_t>(index - 2),
                       static_cast<uint16_t>(index - 1), index);
    } else {
      mesh.AddTriangle(static_cast<uint16_t>(index - 2), index,
                       static_cast<uint16_t>(index - 1));
    }
  }

  return mesh;
}

TextBufferPtr TextBuffer::Create(flatui::FontManager* manager,
                                 const std::string& text,
                                 const TextBufferParams& params) {
  CHECK_GT(params.font_size, 0);  // Otherwise this leads to a crash in flatui.

  manager->SetTextEllipsis(params.ellipsis.c_str());

  // Set the text layout direction based on system locale. This is required
  // for RTL text to render correctly. Ideally, the system would be able to
  // render RTL text correctly even on LTR locales (e.g., Arabic account names
  // on an en-us phone), but this allows translated UI elements to render
  // correctly.
  flatui::TextLayoutDirection layout_direction =
      flatui::kTextLayoutDirectionLTR;
  if (params.direction == TextDirection_RightToLeft) {
    layout_direction = flatui::kTextLayoutDirectionRTL;
  }
  manager->SetLayoutDirection(layout_direction);

  flatui::TextAlignment halign;
  switch (params.horizontal_align) {
    case HorizontalAlignment_Left:
      if (layout_direction == flatui::kTextLayoutDirectionRTL) {
        // FlatUI: "Note that in RTL layout direction mode, the setting is
        // flipped."
        halign = flatui::TextAlignment::kTextAlignmentRight;
      } else {
        halign = flatui::TextAlignment::kTextAlignmentLeft;
      }
      break;
    case HorizontalAlignment_Center:
      halign = flatui::TextAlignment::kTextAlignmentCenter;
      break;
    case HorizontalAlignment_Right:
      if (layout_direction == flatui::kTextLayoutDirectionRTL) {
        halign = flatui::TextAlignment::kTextAlignmentLeft;
      } else {
        halign = flatui::TextAlignment::kTextAlignmentRight;
      }
      break;
  }

  // To enable the ellipsis, both rect_height and ellipsis have to be specified,
  // and caret_info has to be set to false so flatui wouldn't generate font
  // buffer to the end. rect_height is assumed to be always set at this point,
  // so it's not explicitly checked.
  const bool ellipsis_enabled = !params.ellipsis.empty();
  const bool caret_info = !ellipsis_enabled;
  const bool ref_count = true;
  const bool rtl_layout = false;
  const bool enable_hyphenation = params.wrap_mode == TextWrapMode_Hyphenate;
  const mathfu::vec2 size = params.bounds[0] == 0 && params.bounds[1] == 0
                                ? mathfu::vec2(0, params.font_size)
                                : params.bounds;
  mathfu::vec2i flatui_size(
      static_cast<int>(kPixelsFromMetersScale * size.x),
      static_cast<int>(kPixelsFromMetersScale * size.y));
  // If bounds is non-zero, then the intent is to have bounds, so don't allow
  // to round to 0, minimum 1.
  if (params.bounds[0] != 0) {
    flatui_size.x = std::max(flatui_size.x, 1);
  }
  if (params.bounds[1] != 0) {
    flatui_size.y = std::max(flatui_size.y, 1);
  }

  flatui::FontBufferParameters flatui_params(
      manager->GetCurrentFont()->GetFontId(), flatui::HashId(text.c_str()),
      // TODO(b/33921724) Don't base font size on world size.
      kPixelsFromMetersScale * params.font_size, flatui_size, halign,
      flatui::GlyphFlags::kGlyphFlagsOuterSDF |
          flatui::GlyphFlags::kGlyphFlagsInnerSDF,
      caret_info, ref_count, enable_hyphenation, rtl_layout,
      params.kerning_scale, params.line_height_scale);

  flatui::FontBuffer* font_buffer;
  if (params.html_mode != TextHtmlMode_Ignore) {
    font_buffer = manager->GetHtmlBuffer(text.c_str(), flatui_params);
  } else {
    font_buffer = manager->GetBuffer(text.c_str(), text.size(), flatui_params);
  }

  if (!font_buffer) {
    LOG(ERROR) << "Failed to create text buffer for '" << text << "'";
    return TextBufferPtr();
  }

  return TextBufferPtr(new TextBuffer(manager, font_buffer, params));
}

}  // namespace lull
