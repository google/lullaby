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

#include "lullaby/systems/text/modular/gumbo/html.h"

#include "gtest/gtest.h"

namespace lull {
namespace {

std::vector<HtmlParser::Section> ParseHtml(string_view text,
                                           TextHtmlMode mode) {
  GumboHtmlParser parser;
  parser.SetMode(mode);
  return parser.ParseText(text);
}

TEST(ParseHtml, BasicTextStringHasTagsStripped) {
  constexpr const char kSource[] =
      "<b>Hello,</b> my name is <i>Elder</i> Price.";
  constexpr const char kExpected[] = "Hello, my name is Elder Price.";
  std::vector<HtmlParser::Section> sections =
      ParseHtml(kSource, TextHtmlMode_RemoveTags);
  EXPECT_EQ(sections.size(), 1U);
  EXPECT_TRUE(sections[0].url.empty());
  EXPECT_EQ(sections[0].text, kExpected);
}

TEST(ParseHtml, WhitespacesAreConvertedToSpacesAndExtrasAreRemoved) {
  constexpr const char kSource[] =
      "\n\n\n  A \t word\r\n if\tyou <b></b> please";
  constexpr const char kExpected[] = "A word if you please";
  std::vector<HtmlParser::Section> sections =
      ParseHtml(kSource, TextHtmlMode_RemoveTags);
  EXPECT_EQ(sections.size(), 1U);
  EXPECT_TRUE(sections[0].url.empty());
  EXPECT_EQ(sections[0].text, kExpected);
}

TEST(ParseHtml, LinksAreTreatedAsPlainTextWhenNotExtracted) {
  constexpr const char kSource[] =
      "This is a <a href=\"link\">link</a> with some <a "
      "href=\"text\">te</a>xt after it";
  constexpr const char kExpected[] = "This is a link with some text after it";
  std::vector<HtmlParser::Section> sections =
      ParseHtml(kSource, TextHtmlMode_RemoveTags);
  EXPECT_EQ(sections.size(), 1U);
  EXPECT_TRUE(sections[0].url.empty());
  EXPECT_EQ(sections[0].text, kExpected);
}

TEST(ParseHtml, LinksAreExtracted) {
  constexpr const char kSource[] =
      "This is a <a href=\"link\">link</a> with some <a "
      "href=\"text\">te</a>xt after it";
  constexpr size_t kNumExpectedSections = 5;
  constexpr const char* kExpectedText[kNumExpectedSections] = {
      "This is a ", "link", " with some ", "te", "xt after it"};
  constexpr const char* kExpectedLink[kNumExpectedSections] = {
      "", "link", "", "text", ""};
  std::vector<HtmlParser::Section> sections =
      ParseHtml(kSource, TextHtmlMode_ExtractLinks);
  EXPECT_EQ(sections.size(), kNumExpectedSections);
  for (size_t i = 0; i < sections.size(); ++i) {
    EXPECT_EQ(sections[i].text, kExpectedText[i]);
    EXPECT_EQ(sections[i].url, kExpectedLink[i]);
  }
}

}  // namespace
}  // namespace lull
