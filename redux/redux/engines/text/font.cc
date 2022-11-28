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

#include "redux/engines/text/font.h"

#include <utility>

namespace redux {

Font::Font(HashValue name, DataContainer data)
    : name_(name), data_(std::move(data)) {
  rasterizer_ = CreateGlyphRasterizer(data_);
  CHECK(rasterizer_);
  sequencer_ = CreateGlyphSequencer(data_, rasterizer_->GetUnitsPerEm());
  CHECK(sequencer_);

  const vec2i initial_size(512, 512);
  atlas_ = std::make_unique<ImageAtlaser>(ImageFormat::Alpha8, initial_size);
}

HashValue Font::GetName() const { return name_; }

float Font::GetAscender() const { return sequencer_->GetAscender(); }

float Font::GetDescender() const { return sequencer_->GetDescender(); }

const ImageAtlaser& Font::GetGlyphAtlas() const { return *atlas_; }

Bounds2f Font::GetGlyphBounds(TextGlyphId id) const {
  auto iter = glyphs_.find(id);
  if (iter != glyphs_.end()) {
    const Bounds2i& r = iter->second.bounds;
    return Bounds2f(vec2(r.min), vec2(r.max));
  } else {
    return Bounds2f(vec2::Zero(), vec2::Zero());
  }
}

Bounds2f Font::GetGlyphSubBounds(TextGlyphId id) const {
  auto iter = glyphs_.find(id);
  if (iter != glyphs_.end()) {
    const Bounds2i& r = iter->second.bitmap_bounds;
    return Bounds2f(vec2(r.min), vec2(r.max));
  } else {
    return Bounds2f(vec2::Zero(), vec2::Zero());
  }
}

float Font::GetGlyphAdvance(TextGlyphId id) const {
  auto iter = glyphs_.find(id);
  return iter != glyphs_.end() ? iter->second.advance : 0.f;
}

Bounds2f Font::GetGlyphUvBounds(TextGlyphId id) const {
  const Bounds2f bounds = atlas_->GetUvBounds(HashValue(id));
  const vec2 offset =
      static_cast<float>(sdf_padding_) / vec2(atlas_->GetSize());
  const vec2 pos = bounds.min + offset;
  const vec2 size = bounds.Size() - (2.f * offset);
  return Bounds2f(pos, pos + size);
}

GlyphSequence Font::GenerateGlyphSequence(std::string_view text,
                                          std::string_view language_iso_639,
                                          float font_size,
                                          TextDirection direction) {
  GlyphSequence sequence =
      sequencer_->GetGlyphSequence(text, language_iso_639, direction);

  // Rasterize all the glyphs and store them in the glyph map/texture atlas.
  for (size_t i = 0; i < sequence.elements.size(); ++i) {
    const TextGlyphId id = sequence.elements[i].id;
    if (id == 0 || glyphs_.contains(id)) {
      // The glyph has already been rasterized into the atlas.
      continue;
    }

    // TODO: Generating the glyph directly into the atlas would be faster.
    const GlyphImage image =
        rasterizer_->Rasterize(id, static_cast<int>(font_size), sdf_padding_);
    const auto res = atlas_->Add(HashValue(id), image.bitmap);
    CHECK(res != ImageAtlaser::kNoMoreSpace);

    auto& gd = glyphs_[id];
    gd.bounds.min = vec2i::Zero();
    gd.bounds.max = image.size;
    gd.advance = image.advance;
    gd.bitmap_bounds.min = image.offset;
    gd.bitmap_bounds.max = image.offset + image.bitmap.GetSize();
  }
  return sequence;
}
}  // namespace redux
