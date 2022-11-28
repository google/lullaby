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

#include <string_view>

#include "hb-ot.h"
#include "redux/engines/text/internal/glyph.h"
#include "redux/engines/text/internal/locale.h"
#include "redux/engines/text/text_engine.h"

namespace redux {

class HarfBuzzGlyphSequencer : public GlyphSequencer {
 public:
  HarfBuzzGlyphSequencer(const DataContainer& data, int units_per_em);
  ~HarfBuzzGlyphSequencer() override;

  GlyphSequence GetGlyphSequence(std::string_view text,
                                 std::string_view language_iso_639,
                                 TextDirection direction) override;

  float GetAscender() const override { return ascender_; }
  float GetDescender() const override { return descender_; }

 private:
  hb_face_t* hb_face_ = nullptr;
  hb_font_t* hb_font_ = nullptr;
  hb_buffer_t* buffer_ = nullptr;
  float ascender_ = 0.f;
  float descender_ = 0.f;
  vec2 scale_ = {0.f, 0.f};
};

static hb_direction_t HarfBuzzTextDirection(TextDirection direction,
                                            std::string_view language_iso_639) {
  if (direction == TextDirection::kLanguageDefault) {
    direction = GetDefaultTextDirection(language_iso_639.data());
  }

  switch (direction) {
    case TextDirection::kLeftToRight:
      return HB_DIRECTION_LTR;
    case TextDirection::kRightToLeft:
      return HB_DIRECTION_RTL;
    // case TextDirection::kTopToBottom:
    //   return HB_DIRECTION_TTB;
    default:
      LOG(FATAL) << "Invalid text direction " << static_cast<int>(direction);
  }
}

static hb_language_t HarfBuzzLanguage(std::string_view language_iso_639) {
  hb_language_t hb_lang = nullptr;
  if (language_iso_639.empty()) {
    constexpr const char kDefaultLanguage[] = "en-US";
    hb_lang =
        hb_language_from_string(kDefaultLanguage, sizeof(kDefaultLanguage));
  } else {
    const char* str = language_iso_639.data();
    const int len = static_cast<int>(language_iso_639.length());
    hb_lang = hb_language_from_string(str, len);
  }
  return hb_lang;
}

static hb_script_t HarfBuzzScript(std::string_view language_iso_639) {
  const char* lang = language_iso_639.data();
  if (language_iso_639.empty()) {
    lang = "en-US";
  }

  const char* script = GetTextScriptIso15924(lang);
  uint32_t s = *reinterpret_cast<const uint32_t*>(script);
  return hb_script_t(s >> 24 | (s & 0xff0000) >> 8 | (s & 0xff00) << 8 |
                     s << 24);
}

HarfBuzzGlyphSequencer::HarfBuzzGlyphSequencer(const DataContainer& data,
                                               int units_per_em) {
  hb_blob_t* blob =
      hb_blob_create(reinterpret_cast<const char*>(data.GetBytes()),
                     static_cast<unsigned int>(data.GetNumBytes()),
                     HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  CHECK(blob);
  hb_blob_make_immutable(blob);

  constexpr int ttc_index = 0;
  hb_face_ = hb_face_create(blob, ttc_index);
  CHECK(hb_face_);
  hb_blob_destroy(blob);

  hb_face_set_index(hb_face_, ttc_index);
  hb_face_set_upem(hb_face_, units_per_em);
  hb_face_make_immutable(hb_face_);

  hb_font_ = hb_font_create(hb_face_);
  hb_ot_font_set_funcs(hb_font_);

  hb_font_extents_t extents;
  hb_font_get_h_extents(hb_font_, &extents);
  ascender_ = static_cast<float>(extents.ascender);
  descender_ = static_cast<float>(extents.descender);

  int x_scale = 0;
  int y_scale = 0;
  hb_font_get_scale(hb_font_, &x_scale, &y_scale);
  scale_.x = 1.f / static_cast<float>(x_scale);
  scale_.y = 1.f / static_cast<float>(y_scale);

  buffer_ = hb_buffer_create();
}

HarfBuzzGlyphSequencer::~HarfBuzzGlyphSequencer() {
  hb_buffer_destroy(buffer_);
  hb_font_destroy(hb_font_);
  hb_face_destroy(hb_face_);
}

GlyphSequence HarfBuzzGlyphSequencer::GetGlyphSequence(
    std::string_view text, std::string_view language_iso_639,
    TextDirection direction) {
  hb_buffer_set_language(buffer_, HarfBuzzLanguage(language_iso_639));
  hb_buffer_set_script(buffer_, HarfBuzzScript(language_iso_639));
  hb_buffer_set_direction(buffer_,
                          HarfBuzzTextDirection(direction, language_iso_639));

  const auto length = static_cast<int>(text.size());
  hb_buffer_add_utf8(buffer_, text.data(), length, 0, length);
  hb_buffer_guess_segment_properties(buffer_);

  hb_shape(hb_font_, buffer_, nullptr, 0);

  if (direction == TextDirection::kRightToLeft) {
    hb_buffer_reverse(buffer_);
  }

  const size_t num_glyphs = hb_buffer_get_length(buffer_);

  GlyphSequence sequence;
  sequence.elements.resize(num_glyphs);
  if (num_glyphs > 0) {
    const hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer_, nullptr);
    for (size_t i = 0; i < num_glyphs; ++i) {
      auto& element = sequence.elements[i];
      element.id = infos[i].codepoint;
      element.character_index = infos[i].cluster;
    }
  }

  hb_buffer_clear_contents(buffer_);
  return sequence;
}

std::unique_ptr<GlyphSequencer> CreateGlyphSequencer(const DataContainer& data,
                                                     int units_per_em) {
  auto ptr = new HarfBuzzGlyphSequencer(data, units_per_em);
  return std::unique_ptr<GlyphSequencer>(ptr);
}

}  // namespace redux
