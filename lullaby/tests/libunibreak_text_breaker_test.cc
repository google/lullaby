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

#include "lullaby/systems/text/modular/libunibreak/breaking.h"

#include "gtest/gtest.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace {

using CharacterBreakType = TextBreaker::CharacterBreakType;

TEST(BreakText, LengthMatchesInputLength) {
  LibunibreakTextBreaker breaker;

  constexpr const char kLatinText[] = "Latin text";
  std::vector<CharacterBreakType> breaks = breaker.IdentifyBreaks(kLatinText);
  EXPECT_EQ(breaks.size(), string_view(kLatinText).length());

  constexpr const char kUtf8Text[] = "中文文本";
  breaks = breaker.IdentifyBreaks(kUtf8Text);
  EXPECT_EQ(breaks.size(), string_view(kUtf8Text).length());
}

TEST(BreakText, AlwaysEndsInLineBreak) {
  LibunibreakTextBreaker breaker;

  constexpr const char kLatinText[] = "Latin text";
  std::vector<CharacterBreakType> breaks = breaker.IdentifyBreaks(kLatinText);
  EXPECT_EQ(breaks[breaks.size() - 1], TextBreaker::kLineBreak);

  constexpr const char kUtf8Text[] = "中文文本";
  breaks = breaker.IdentifyBreaks(kUtf8Text);
  EXPECT_EQ(breaks[breaks.size() - 1], TextBreaker::kLineBreak);
}

static void TestThatCharactersMatchBreak(
    string_view text, char matching_char,
    const std::vector<CharacterBreakType>& breaks,
    CharacterBreakType matching_break) {
  for (size_t i = 0; i < text.length(); ++i) {
    if (text[i] == matching_char) {
      EXPECT_EQ(breaks[i], matching_break);
    }
  }
}

TEST(BreakText, SpacesAreWordBreaks) {
  LibunibreakTextBreaker breaker;

  constexpr const char kLatinText[] = "Latin text";
  std::vector<CharacterBreakType> breaks = breaker.IdentifyBreaks(kLatinText);
  TestThatCharactersMatchBreak(kLatinText, ' ', breaks,
                               TextBreaker::kWordBreak);

  constexpr const char kUtf8Text[] = "中文 文本";
  breaks = breaker.IdentifyBreaks(kUtf8Text);
  TestThatCharactersMatchBreak(kUtf8Text, ' ', breaks, TextBreaker::kWordBreak);
}

TEST(BreakText, NewlinesAreLineBreaks) {
  LibunibreakTextBreaker breaker;

  constexpr const char kLatinText[] = "Latin\ntext";
  std::vector<CharacterBreakType> breaks = breaker.IdentifyBreaks(kLatinText);
  TestThatCharactersMatchBreak(kLatinText, '\n', breaks,
                               TextBreaker::kLineBreak);

  constexpr const char kUtf8Text[] = "中文\n文本";
  breaks = breaker.IdentifyBreaks(kUtf8Text);
  TestThatCharactersMatchBreak(kUtf8Text, '\n', breaks,
                               TextBreaker::kLineBreak);
}

}  // namespace
}  // namespace lull
