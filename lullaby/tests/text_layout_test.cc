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

#include "lullaby/systems/text/modular/layout.h"

#include "lullaby/systems/text/modular/gumbo/html.h"
#include "lullaby/systems/text/modular/harfbuzz/shaping.h"
#include "lullaby/systems/text/modular/libunibreak/breaking.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/make_unique.h"

#include "lullaby/tests/DroidTV-Regular.inc"

namespace lull {
namespace {

using ::testing::AnyOf;
using ::testing::FloatNear;

constexpr float kDefaultEpsilon = .001f;
constexpr float kFontSize = 1.0f;

std::unique_ptr<TextLayout> CreateTestLayout() {
  auto layout = MakeUnique<TextLayout>(MakeUnique<HarfBuzzTextShaper>(),
                                       MakeUnique<LibunibreakTextBreaker>(),
                                       MakeUnique<GumboHtmlParser>());

  std::unique_ptr<FormingFont> forming_font;  // Unused.
  std::unique_ptr<ShapingFont> shaping_font =
      layout->GetShaper().CreateTestFont(
          third_party_webfonts_apache_DroidTV_Regular_ttf,
          third_party_webfonts_apache_DroidTV_Regular_ttf_len);
  FontPtr font =
      std::make_shared<Font>(std::move(forming_font), std::move(shaping_font));

  layout->SetFont(font, kFontSize);
  layout->SetLineHeight(kFontSize);

  return layout;
}

TEST(TextLayout, CursorAdvancesLTR) {
  constexpr const char kText[] = "this is some text";
  auto layout = CreateTestLayout();
  layout->SetLineWrapping(TextWrapMode_None);
  layout->AddText(kText);

  mathfu::vec2 cursor = layout->GetGlyphs()[0].position;
  for (const auto& glyph : layout->GetGlyphs()) {
    EXPECT_GE(glyph.position.x, cursor.x);
    cursor.x = glyph.position.x;
  }
}

TEST(TextLayout, CursorAdancesRTL) {
  constexpr const char kText[] = "مرحبا بالعالم";
  auto layout = CreateTestLayout();
  layout->SetLineWrapping(TextWrapMode_None);
  layout->SetDirection(TextDirection_RightToLeft);
  layout->AddText(kText);

  mathfu::vec2 cursor = layout->GetGlyphs()[0].position;
  for (const auto& glyph : layout->GetGlyphs()) {
    EXPECT_LE(glyph.position.x, cursor.x);
    cursor.x = glyph.position.x;
  }
}

TEST(TextLayout, NewLinesAdvanceCursor) {
  constexpr const char kText[] = "this\nis\nsome\ntext";
  auto layout = CreateTestLayout();
  layout->SetLineWrapping(TextWrapMode_None);
  layout->AddText(kText);

  mathfu::vec2 cursor = {0, 0};
  for (const auto& glyph : layout->GetGlyphs()) {
    const auto on_same_or_new_line =
        AnyOf(FloatNear(cursor.y, kDefaultEpsilon),
              FloatNear(cursor.y - kFontSize, kDefaultEpsilon));
    EXPECT_THAT(glyph.position.y, on_same_or_new_line);
    cursor.y = glyph.position.y;
  }
}

TEST(TextLayout, LinesWrapBetweenWords) {
  constexpr const char kText[] = "this is some text with spaces between words";
  auto layout = CreateTestLayout();
  layout->SetBounds({0, 0, 10, 10});
  layout->SetLineWrapping(TextWrapMode_BetweenWords);
  layout->AddText(kText);

  const Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), sizeof(kText) - 1);

  mathfu::vec2 cursor = glyphs[0].position;
  int num_lines = 1;
  for (size_t i = 0; i < glyphs.size(); ++i) {
    const auto& glyph = glyphs[i];
    const bool is_new_line =
        glyph.position.y <= cursor.y - kFontSize + kDefaultEpsilon;
    if (is_new_line) {
      EXPECT_EQ(kText[i], ' ');
      ++num_lines;
    }

    cursor = glyph.position;
  }
  EXPECT_EQ(num_lines, 3);
}

TEST(TextLayout, LongLineDoesntWrapWhenWrappingDisabled) {
  constexpr const char kText[] = "this is a long line that would wrap if it were enabled";
  auto layout = CreateTestLayout();
  const mathfu::rectf bounds(0, 0, 10, 10);
  layout->SetBounds(bounds);
  layout->SetLineWrapping(TextWrapMode_None);
  layout->AddText(kText);

  const Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), sizeof(kText) - 1);

  mathfu::vec2 cursor = glyphs[0].position;
  for (size_t i = 0; i < glyphs.size(); ++i) {
    const auto& glyph = glyphs[i];
    const bool is_new_line =
        glyph.position.y <= cursor.y - kFontSize + kDefaultEpsilon;
    EXPECT_FALSE(is_new_line);

    cursor = glyph.position;
  }
  EXPECT_GE(cursor.x, bounds.size.x);
}

TEST(TextLayout, HorizontalAlignmentLeftIsCorrectlyApplied) {
  constexpr const char kText[] = "A";
  auto layout = CreateTestLayout();
  const mathfu::rectf bounds(13, 0, 10, 10);
  layout->SetBounds(bounds);
  layout->SetLineWrapping(TextWrapMode_None);
  layout->SetHorizontalAlignment(HorizontalAlignment_Left);
  layout->AddText(kText);

  Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), 1U);
  EXPECT_GE(glyphs[0].position.x, bounds.pos.x);
  EXPECT_LT(glyphs[0].position.x, bounds.pos.x + kFontSize);
}

TEST(TextLayout, HorizontalAlignmentCenterIsCorrectlyApplied) {
  constexpr const char kText[] = "A";
  auto layout = CreateTestLayout();
  const mathfu::rectf bounds(13, 0, 10, 10);
  layout->SetBounds(bounds);
  layout->SetLineWrapping(TextWrapMode_None);
  layout->SetHorizontalAlignment(HorizontalAlignment_Center);
  layout->AddText(kText);

  Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), 1U);

  const float center_x = bounds.pos.x + .5f * bounds.size.x;
  EXPECT_GE(glyphs[0].position.x, center_x - kFontSize);
  EXPECT_LT(glyphs[0].position.x, center_x + kFontSize);
}

TEST(TextLayout, HorizontalAlignmentRightIsCorrectlyApplied) {
  constexpr const char kText[] = "A";
  auto layout = CreateTestLayout();
  const mathfu::rectf bounds(13, 0, 10, 10);
  layout->SetBounds(bounds);
  layout->SetLineWrapping(TextWrapMode_None);
  layout->SetHorizontalAlignment(HorizontalAlignment_Right);
  layout->AddText(kText);

  Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), 1U);

  const float right_x = bounds.pos.x + bounds.size.x;
  EXPECT_GE(glyphs[0].position.x, right_x - kFontSize);
  EXPECT_LT(glyphs[0].position.x, right_x);
}

TEST(TextLayout, VerticalAlignmentBaselineIsCorrectlyApplied) {
  constexpr const char kText[] = "A";
  auto layout = CreateTestLayout();
  const mathfu::rectf bounds(0, 11, 10, 10);
  layout->SetBounds(bounds);
  layout->SetLineWrapping(TextWrapMode_None);
  layout->SetVerticalAlignment(VerticalAlignment_Baseline);
  layout->AddText(kText);

  Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), 1U);

  EXPECT_EQ(glyphs[0].position.y, bounds.pos.y);
}

TEST(TextLayout, VerticalAlignmentTopIsCorrectlyApplied) {
  constexpr const char kText[] = "A";
  auto layout = CreateTestLayout();
  const mathfu::rectf bounds(0, 11, 10, 10);
  layout->SetBounds(bounds);
  layout->SetLineWrapping(TextWrapMode_None);
  layout->SetVerticalAlignment(VerticalAlignment_Top);
  layout->AddText(kText);

  Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), 1U);

  const float top_y = bounds.pos.y + bounds.size.y;
  EXPECT_LT(glyphs[0].position.y, top_y);
  EXPECT_GE(glyphs[0].position.y, top_y - kFontSize);
}

TEST(TextLayout, VerticalAlignmentCenterIsCorrectlyApplied) {
  constexpr const char kText[] = "A";
  auto layout = CreateTestLayout();
  const mathfu::rectf bounds(0, 11, 10, 10);
  layout->SetBounds(bounds);
  layout->SetLineWrapping(TextWrapMode_None);
  layout->SetVerticalAlignment(VerticalAlignment_Center);
  layout->AddText(kText);

  Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), 1U);

  const float center_y = bounds.pos.y + .5f * bounds.size.y;
  EXPECT_GE(glyphs[0].position.y, center_y - kFontSize);
  EXPECT_LE(glyphs[0].position.y, center_y + kFontSize);
}

TEST(TextLayout, VerticalAlignmentBottomIsCorrectlyApplied) {
  constexpr const char kText[] = "A";
  auto layout = CreateTestLayout();
  const mathfu::rectf bounds(0, 11, 10, 10);
  layout->SetBounds(bounds);
  layout->SetLineWrapping(TextWrapMode_None);
  layout->SetVerticalAlignment(VerticalAlignment_Bottom);
  layout->AddText(kText);

  Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), 1U);

  const float bottom_y = bounds.pos.y;
  EXPECT_GE(glyphs[0].position.y, bottom_y);
  EXPECT_LT(glyphs[0].position.y, bottom_y + kFontSize);
}

TEST(TextLayout, HtmlLinksAreIgnoredWhenHtmlModeIsDisabled) {
  constexpr const char kText[] = "<a href=\"link.com\">link</a>";
  auto layout = CreateTestLayout();
  layout->SetHtmlMode(TextHtmlMode_Ignore);
  layout->AddText(kText);

  EXPECT_TRUE(layout->GetLinks().empty());

  Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), sizeof(kText) - 1);

  for (const auto& glyph : glyphs) {
    EXPECT_EQ(glyph.link_index, -1);
  }
}

TEST(TextLayout, HtmlLinksAreExtractedWhenHtmlModeIsExtract) {
  constexpr const char kText[] = "<a href=\"link.com\">link</a>";
  auto layout = CreateTestLayout();
  layout->SetHtmlMode(TextHtmlMode_ExtractLinks);
  layout->AddText(kText);

  ASSERT_FALSE(layout->GetLinks().empty());
  EXPECT_EQ(layout->GetLinks()[0].href, "link.com");

  Span<TextLayout::Glyph> glyphs = layout->GetGlyphs();
  EXPECT_EQ(glyphs.size(), 4U);
  for (const auto& glyph : glyphs) {
    EXPECT_EQ(glyph.link_index, 0);
  }
}

TEST(TextLayout, GlyphsLinksAndCaretsAreClearedAsExpected) {
  constexpr const char kText[] = "<a href=\"link.com\">link</a>";
  auto layout = CreateTestLayout();
  layout->SetHtmlMode(TextHtmlMode_ExtractLinks);
  layout->AddText(kText);

  EXPECT_FALSE(layout->GetGlyphs().empty());
  EXPECT_FALSE(layout->GetCaretPositions().empty());
  EXPECT_FALSE(layout->GetLinks().empty());

  layout->Clear();

  EXPECT_TRUE(layout->GetGlyphs().empty());
  EXPECT_TRUE(layout->GetCaretPositions().empty());
  EXPECT_TRUE(layout->GetLinks().empty());
}

}  // namespace
}  // namespace lull
