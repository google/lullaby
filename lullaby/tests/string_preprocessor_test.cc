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

#include "lullaby/util/string_preprocessor.h"

#include "gtest/gtest.h"

namespace lull {
namespace {

class StringLocalizer : public StringPreprocessor {
 public:
  std::string ProcessString(const std::string& input) override { return input; }
};

// NOTE: StringPreprocessor is not actually functional by itself.  This file
// just tests the prefix processing.
TEST(SringPreprocessorTest, Localized) {
  StringPreprocessor::ProcessStringRequest result =
      StringLocalizer::CheckPrefix("@FeaturedTab");
  EXPECT_EQ("FeaturedTab", result.text);
  EXPECT_EQ(StringPreprocessor::kLocalize, result.mode);

  result = StringLocalizer::CheckPrefix("@SavedTab");
  EXPECT_EQ("SavedTab", result.text);
  EXPECT_EQ(StringPreprocessor::kLocalize, result.mode);

  result = StringLocalizer::CheckPrefix("@ProfileTab");
  EXPECT_EQ("ProfileTab", result.text);
  EXPECT_EQ(StringPreprocessor::kLocalize, result.mode);
}

TEST(SringPreprocessorTest, Literal) {
  StringPreprocessor::ProcessStringRequest result =
      StringLocalizer::CheckPrefix("A");
  EXPECT_EQ("A", result.text);
  EXPECT_EQ(StringPreprocessor::kNoPrefix, result.mode);

  result = StringLocalizer::CheckPrefix("Banana");
  EXPECT_EQ("Banana", result.text);
  EXPECT_EQ(StringPreprocessor::kNoPrefix, result.mode);
}

TEST(SringPreprocessorTest, Escaped) {
  StringPreprocessor::ProcessStringRequest result =
      StringLocalizer::CheckPrefix("'@email.com");
  EXPECT_EQ("@email.com", result.text);
  EXPECT_EQ(StringPreprocessor::kLiteral, result.mode);
}

TEST(SringPreprocessorTest, UpperCase) {
  StringPreprocessor::ProcessStringRequest result =
      StringLocalizer::CheckPrefix("^testTest");
  EXPECT_EQ("testTest", result.text);
  EXPECT_EQ(StringPreprocessor::kLocalizeToUpperCase, result.mode);
}

TEST(SringPreprocessorTest, Empty) {
  StringPreprocessor::ProcessStringRequest result =
      StringLocalizer::CheckPrefix("");
  EXPECT_EQ("", result.text);
  EXPECT_EQ(StringPreprocessor::kNoPrefix, result.mode);
}

}  // namespace
}  // namespace lull
