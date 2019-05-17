/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/systems/text/modular/harfbuzz/shaping.h"

#include "gtest/gtest.h"
#include "lullaby/util/logging.h"

#include "lullaby/tests/DroidTV-Regular.inc"

namespace lull {
namespace {

constexpr float kFontSize = 1.0f;

void TestThatCharacterIndexDoesntDecrease(Span<TextShaper::Glyph> glyphs) {
  uint32_t last_index = 0;
  for (const auto& glyph : glyphs) {
    EXPECT_GE(glyph.character_index, last_index);
    last_index = glyph.character_index;
  }
}

TEST(ShapeText, CharacterIndexDoesntDecrease) {
  HarfBuzzTextShaper shaper;
  std::unique_ptr<ShapingFont> font = shaper.CreateTestFont(
      third_party_webfonts_apache_DroidTV_Regular_ttf,
      third_party_webfonts_apache_DroidTV_Regular_ttf_len);

  constexpr const char kLtrText[] =
      "Latin Text.  中文文本.  кириллический текст.";
  std::vector<TextShaper::Glyph> ltr_glyphs =
      shaper.ShapeText(*font, kFontSize, kLtrText);
  TestThatCharacterIndexDoesntDecrease(ltr_glyphs);

  constexpr const char kRtlText[] = "مرحبا بالعالم";
  shaper.SetDirection(TextDirection_RightToLeft);
  std::vector<TextShaper::Glyph> rtl_glyphs =
      shaper.ShapeText(*font, kFontSize, kRtlText);
  TestThatCharacterIndexDoesntDecrease(rtl_glyphs);
}

}  // namespace
}  // namespace lull
