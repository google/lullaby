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

#ifndef REDUX_ENGINES_TEXT_INTERNAL_GLYPH_H_
#define REDUX_ENGINES_TEXT_INTERNAL_GLYPH_H_

#include <cstddef>

#include "redux/engines/text/text_enums.h"
#include "redux/modules/graphics/image_atlaser.h"
#include "redux/modules/math/vector.h"

namespace redux {

// The ID of a glyph within a font face.
using TextGlyphId = std::uint32_t;

// The rasterized image of a glyph.
struct GlyphImage {
  ImageData bitmap;
  vec2i size = {0, 0};
  vec2i offset = {0, 0};
  float advance = 0.f;
};

// A sequence of glyphs that were generated from a string.
struct GlyphSequence {
  struct Element {
    // The ID of the glyph.
    TextGlyphId id = 0;

    // Index of the character/codepoint in the string. For single-byte character
    // sequences, this will be the same as the index of the character.
    int character_index = 0;
  };

  std::vector<Element> elements;
  std::vector<TextCharacterBreakType> breaks;
};

// Responsible for rasterizing a glyph into an image.
struct GlyphRasterizer {
  virtual ~GlyphRasterizer() = default;
  virtual int GetUnitsPerEm() const = 0;
  virtual GlyphImage Rasterize(TextGlyphId id, unsigned int size_in_pixels,
                               int sdf_padding) = 0;
};

// Responsible for arranging glyphs in the correct order for a given string.
struct GlyphSequencer {
  virtual ~GlyphSequencer() = default;
  virtual float GetAscender() const = 0;
  virtual float GetDescender() const = 0;
  virtual GlyphSequence GetGlyphSequence(std::string_view text,
                                         std::string_view language_iso_639,
                                         TextDirection direction) = 0;
};

std::unique_ptr<GlyphRasterizer> CreateGlyphRasterizer(
    const DataContainer& data);

std::unique_ptr<GlyphSequencer> CreateGlyphSequencer(const DataContainer& data,
                                                     int units_per_em);

}  // namespace redux

#endif  // REDUX_ENGINES_TEXT_INTERNAL_GLYPH_H_
