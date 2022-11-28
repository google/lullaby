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

#include "freetype/freetype.h"
#include "ft2build.h"
#include "redux/engines/text/internal/glyph.h"
#include "redux/engines/text/internal/sdf_computer.h"
#include "redux/engines/text/text_engine.h"
#include "redux/modules/graphics/enums.h"
#include "redux/modules/graphics/image_data.h"

namespace redux {

// GlyphRasterizer that uses the freetype library.
class FreeTypeGlyphRasterizer : public GlyphRasterizer {
 public:
  explicit FreeTypeGlyphRasterizer(const DataContainer& data);
  ~FreeTypeGlyphRasterizer() override;

  int GetUnitsPerEm() const override {
    return 2048;  // TODO(b/72713908): Query this value from freetype.
  }

  GlyphImage Rasterize(TextGlyphId id, uint32_t size_in_pixels,
                       int sdf_padding) override;

 private:
  FT_Library ft_lib_;
  FT_Face ft_face_;
  SdfComputer sdf_computer_;
};

static float ToPixels(FT_Pos v26_6) {
  static constexpr float kToPixels = 1.0f / 64.0f;
  return kToPixels * static_cast<float>(v26_6);
}

FreeTypeGlyphRasterizer::FreeTypeGlyphRasterizer(const DataContainer& data) {
  FT_Init_FreeType(&ft_lib_);
  FT_New_Memory_Face(ft_lib_, reinterpret_cast<const FT_Byte*>(data.GetBytes()),
                     static_cast<FT_Long>(data.GetNumBytes()), 0, &ft_face_);
  CHECK(ft_face_);
}

FreeTypeGlyphRasterizer::~FreeTypeGlyphRasterizer() {
  FT_Done_FreeType(ft_lib_);
}

GlyphImage FreeTypeGlyphRasterizer::Rasterize(TextGlyphId id,
                                              uint32_t size_in_pixels,
                                              int sdf_padding) {
  if (FT_IS_SCALABLE(ft_face_)) {
    FT_Set_Pixel_Sizes(ft_face_, size_in_pixels, size_in_pixels);
  } else {
    CHECK(ft_face_->num_fixed_sizes);
    int closest_face = 0;  // Index of bitmap-strike closest to size_in_pixels.
    size_t closest_size = 0xFFFFFFFF;
    for (int i = 0; i < ft_face_->num_fixed_sizes; ++i) {
      FT_Select_Size(ft_face_, i);
      const size_t size_difference =
          abs(static_cast<int>(ft_face_->size->metrics.y_ppem) -
              static_cast<int>(size_in_pixels));
      if (size_difference < closest_size) {
        closest_size = size_difference;
        closest_face = i;
      }
    }

    FT_Select_Size(ft_face_, closest_face);
  }

  FT_Int32 flags = FT_LOAD_RENDER;
  FT_Error err = FT_Load_Glyph(ft_face_, id, flags);
  CHECK_EQ(err, 0);
  const FT_GlyphSlot ft_glyph = ft_face_->glyph;
  CHECK(ft_glyph);

  const int width = ft_glyph->bitmap.width;
  const int height = ft_glyph->bitmap.rows;
  const vec2i bitmap_size(width, height);
  const std::byte* bitmap_pixels =
      reinterpret_cast<const std::byte*>(ft_glyph->bitmap.buffer);

  GlyphImage image;
  image.bitmap = sdf_computer_.Compute(bitmap_pixels, bitmap_size, sdf_padding);
  image.size.x = ToPixels(ft_glyph->metrics.width);
  image.size.y = ToPixels(ft_glyph->metrics.height);
  image.advance = ToPixels(ft_glyph->advance.x);
  image.offset.x = ft_glyph->bitmap_left;
  image.offset.y = ft_glyph->bitmap_top - ft_glyph->bitmap.rows;
  return image;
}

std::unique_ptr<GlyphRasterizer> CreateGlyphRasterizer(
    const DataContainer& data) {
  auto ptr = new FreeTypeGlyphRasterizer(data);
  return std::unique_ptr<GlyphRasterizer>(ptr);
}

}  // namespace redux
