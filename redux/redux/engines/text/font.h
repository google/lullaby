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

#ifndef REDUX_ENGINES_TEXT_FONT_H_
#define REDUX_ENGINES_TEXT_FONT_H_

#include <memory>

#include "redux/engines/text/internal/glyph.h"
#include "redux/engines/text/text_enums.h"
#include "redux/modules/base/data_container.h"
#include "redux/modules/graphics/image_atlaser.h"

namespace redux {

// Rasterizes and stores the glyphs for a given Font object (e.g. a true-type
// font binary loaded from disk).
class Font {
 public:
  // Constructs the font of a given name using binary data (e.g. a ttf file).
  Font(HashValue name, DataContainer data);

  // Returns the name of the font.
  HashValue GetName() const;

  // Returns the image atlas containing all the glyphs.
  const ImageAtlaser& GetGlyphAtlas() const;

  // Returns information about a specific glyph. If the specified glyph hasn't
  // been rasterized, will return zero-sized values.
  Bounds2f GetGlyphBounds(TextGlyphId id) const;
  Bounds2f GetGlyphSubBounds(TextGlyphId id) const;
  Bounds2f GetGlyphUvBounds(TextGlyphId id) const;
  float GetGlyphAdvance(TextGlyphId id) const;

  // Returns information about the font.
  float GetAscender() const;
  float GetDescender() const;

  // Generates the sequence of Glyphs that represents the given `text`. This
  // function will also update the font's internal glyph map/texture atlas that
  // stores the rasterized images for each glyph.
  //
  // This function can be slow when generating new glyphs, so use with caution.
  GlyphSequence GenerateGlyphSequence(std::string_view text,
                                      std::string_view language_iso_639,
                                      float font_size, TextDirection direction);

 private:
  struct GlyphData {
    Bounds2i bounds = {vec2i::Zero(), vec2i::Zero()};
    Bounds2i bitmap_bounds = {vec2i::Zero(), vec2i::Zero()};
    float advance = 0.f;
  };

  HashValue name_;
  DataContainer data_;
  std::unique_ptr<GlyphRasterizer> rasterizer_;
  std::unique_ptr<GlyphSequencer> sequencer_;
  std::unique_ptr<ImageAtlaser> atlas_;
  absl::flat_hash_map<TextGlyphId, GlyphData> glyphs_;
  int sdf_padding_ = 4;
};

using FontPtr = std::shared_ptr<Font>;

}  // namespace redux

#endif  // REDUX_ENGINES_TEXT_FONT_H_
