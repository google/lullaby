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

#include <array>
#include <thread>
#include <random>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/debug/log_tag.h"

namespace lull {
namespace debug {
namespace {

using ::testing::Eq;

TEST(LogTagTest, SplitTag) {
  std::array<Tag, 6> sub_tags;
  const size_t num =
      SplitTag("lull.Transform.Set_Sqt", sub_tags.data(), sub_tags.size());

  EXPECT_EQ((size_t)3, num);
  EXPECT_EQ("lull", sub_tags[0].name);
  EXPECT_EQ("Transform", sub_tags[1].name);
  EXPECT_EQ("Set_Sqt", sub_tags[2].name);
}

TEST(LogTagTest, SplitTagEmpty) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag("", sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)0, num);
}

TEST(LogTagTest, SplitTagNoDot) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag("ok", sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)1, num);
  EXPECT_EQ("ok", sub_tags[0].name);
}

TEST(LogTagTest, SplitTagDot) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag(".", sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)0, num);
}

TEST(LogTagTest, SplitTagInvalidChars) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag(" . ", sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)0, num);
}

TEST(LogTagTest, SplitTagDots) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag(".....", sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)0, num);
}

TEST(LogTagTest, SplitTagDotStart) {
  std::array<Tag, 6> sub_tags;
  const size_t num =
      SplitTag(".lull.Transform.Set_Sqt", sub_tags.data(), sub_tags.size());

  EXPECT_EQ((size_t)3, num);
  EXPECT_EQ("lull", sub_tags[0].name);
  EXPECT_EQ("Transform", sub_tags[1].name);
  EXPECT_EQ("Set_Sqt", sub_tags[2].name);
}

TEST(LogTagTest, SplitTagDotEnd) {
  std::array<Tag, 6> sub_tags;
  const size_t num =
      SplitTag("lull.Transform.Set_Sqt.", sub_tags.data(), sub_tags.size());

  EXPECT_EQ((size_t)3, num);
  EXPECT_EQ("lull", sub_tags[0].name);
  EXPECT_EQ("Transform", sub_tags[1].name);
  EXPECT_EQ("Set_Sqt", sub_tags[2].name);
}

TEST(LogTagTest, SplitTagDotMid) {
  std::array<Tag, 6> sub_tags;
  const size_t num =
      SplitTag("lull...Transform...Set_Sqt", sub_tags.data(), sub_tags.size());

  EXPECT_EQ((size_t)3, num);
  EXPECT_EQ("lull", sub_tags[0].name);
  EXPECT_EQ("Transform", sub_tags[1].name);
  EXPECT_EQ("Set_Sqt", sub_tags[2].name);
}

TEST(LogTagTest, SplitTagInvalidCharsStart) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag("*no", sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)0, num);
}

TEST(LogTagTest, SplitTagInvalidCharsEnd) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag("no*", sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)0, num);
}

TEST(LogTagTest, SplitTagInvalidCharsStartEnd) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag("*no.no*", sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)0, num);
}

TEST(LogTagTest, SplitTagInvalidCharsInBetween) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag("no.*&.no", sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)0, num);
}

TEST(LogTagTest, SplitTagStringOverflow) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag(
      "iIylymePwipi1xsssy6QNOGclsulVmVvArQSaoeJKszhLYAMxGClvixQL3cHo9cZA3SKr7uc"
      "z4uk357ALIVg2t8MwkuUsGU3IMNaSimrmRMoWSZHBVXCo6pVpTPRAKUiwHtp",
      sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)0, num);
}

TEST(LogTagTest, SplitTagBufferOverflow) {
  std::array<Tag, 6> sub_tags;
  const size_t num = SplitTag("tag.tag.tag.tag.tag.tag.tag.tag.tag.tag",
                              sub_tags.data(), sub_tags.size());
  EXPECT_EQ((size_t)6, num);
}

TEST(LogTagTest, Enable) {
  InitializeLogTag();
  Enable("lull.Transform.SetSqt");
  const bool tag_1 = IsEnabled("lull");
  EXPECT_THAT(tag_1, Eq(true));
  const bool tag_2 = IsEnabled("lull.Transform");
  EXPECT_THAT(tag_2, Eq(true));
  const bool tag_3 = IsEnabled("lull.Transform.SetSqt");
  EXPECT_THAT(tag_3, Eq(true));
  ShutdownLogTag();
}

TEST(LogTagTest, Disable) {
  InitializeLogTag();
  Disable("lull.Transform.Rotation");
  const bool tag_1 = IsEnabled("lull");
  EXPECT_THAT(tag_1, Eq(false));
  const bool tag_2 = IsEnabled("lull.Transform");
  EXPECT_THAT(tag_2, Eq(false));
  const bool tag_3 = IsEnabled("lull.Transform.Rotation");
  EXPECT_THAT(tag_3, Eq(false));
  ShutdownLogTag();
}

TEST(LogTagTest, EnableOverwrite) {
  InitializeLogTag();
  Disable("lull.Transform.Rotation");
  Enable("lull.Transform.SetSqt");
  const bool tag_1 = IsEnabled("lull");
  EXPECT_THAT(tag_1, Eq(true));
  const bool tag_2 = IsEnabled("lull.Transform");
  EXPECT_THAT(tag_2, Eq(true));
  const bool tag_3 = IsEnabled("lull.Transform.SetSqt");
  EXPECT_THAT(tag_3, Eq(true));
  const bool tag_4 = IsEnabled("lull.Transform.Rotation");
  EXPECT_THAT(tag_4, Eq(false));
  ShutdownLogTag();
}

TEST(LogTagTest, DisableOverwrite) {
  InitializeLogTag();
  Enable("lull.Transform.SetSqt");
  Disable("lull.Transform.Rotation");
  const bool tag_1 = IsEnabled("lull");
  EXPECT_THAT(tag_1, Eq(true));
  const bool tag_2 = IsEnabled("lull.Transform");
  EXPECT_THAT(tag_2, Eq(true));
  const bool tag_3 = IsEnabled("lull.Transform.SetSqt");
  EXPECT_THAT(tag_3, Eq(true));
  const bool tag_4 = IsEnabled("lull.Transform.Rotation");
  EXPECT_THAT(tag_4, Eq(false));
  ShutdownLogTag();
}

TEST(LogTagTest, EnableBranch) {
  InitializeLogTag();
  Disable("lull.Transform.Linestrip");
  Disable("lull.Transform.Line");
  Disable("lull.Text.Pos");
  EnableBranch("lull.Transform");
  const bool tag_1 = IsEnabled("lull");
  EXPECT_THAT(tag_1, Eq(true));
  const bool tag_2 = IsEnabled("lull.Transform");
  EXPECT_THAT(tag_2, Eq(true));
  const bool tag_3 = IsEnabled("lull.Transform.Linestrip");
  EXPECT_THAT(tag_3, Eq(true));
  const bool tag_4 = IsEnabled("lull.Transform.Line");
  EXPECT_THAT(tag_4, Eq(true));
  const bool tag_5 = IsEnabled("lull.Text");
  EXPECT_THAT(tag_5, Eq(false));
  const bool tag_6 = IsEnabled("lull.Text.Pos");
  EXPECT_THAT(tag_6, Eq(false));
  ShutdownLogTag();
}

TEST(LogTagTest, DisableBranch) {
  InitializeLogTag();
  Enable("lull.Transform.Linestrip");
  Enable("lull.Transform.Line");
  Enable("lull.Text.Pos");
  DisableBranch("lull.Transform");
  const bool tag_1 = IsEnabled("lull");
  EXPECT_THAT(tag_1, Eq(true));
  const bool tag_2 = IsEnabled("lull.Transform");
  EXPECT_THAT(tag_2, Eq(false));
  const bool tag_3 = IsEnabled("lull.Transform.Linestrip");
  EXPECT_THAT(tag_3, Eq(false));
  const bool tag_4 = IsEnabled("lull.Transform.Line");
  EXPECT_THAT(tag_4, Eq(false));
  const bool tag_5 = IsEnabled("lull.Text");
  EXPECT_THAT(tag_5, Eq(true));
  const bool tag_6 = IsEnabled("lull.Text.Pos");
  EXPECT_THAT(tag_6, Eq(true));
  ShutdownLogTag();
}

TEST(LogTagTest, IsEnabled) {
  InitializeLogTag();
  const bool enabled = IsEnabled("lull.Audio.Sound");
  EXPECT_THAT(enabled, Eq(false));
  ShutdownLogTag();
}

TEST(LogTagTest, DisabledParent) {
  InitializeLogTag();
  Enable("lull.Transform.SetSqt");
  Disable("lull.Transform.Rotation");
  Disable("lull.Transform");
  const bool transform = IsEnabled("lull.Transform");
  const bool rotation = IsEnabled("lull.Transform.Rotation");
  const bool sqt = IsEnabled("lull.Transform.SetSqt");
  EXPECT_THAT(transform, Eq(false));
  EXPECT_THAT(rotation, Eq(false));
  EXPECT_THAT(sqt, Eq(false));
  ShutdownLogTag();
}

TEST(LogTagTest, InsentiveCase) {
  InitializeLogTag();
  Enable("LULL.Transform.Set_Sqt");
  const bool tag_1 = IsEnabled("lull");
  EXPECT_THAT(tag_1, Eq(true));
  const bool tag_2 = IsEnabled("lull.transform");
  EXPECT_THAT(tag_2, Eq(true));
  const bool tag_3 = IsEnabled("lull.transform.set_sqt");
  EXPECT_THAT(tag_3, Eq(true));
}

TEST(LogTagTest, ThreadSafety) {
  const char* const a_to_z = "abcdefghijklmnopqrstuvwxyz";
  InitializeLogTag();
  static const int kNumProducers = 100;
  std::vector<std::thread> producers;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(1, 25);
  for (int i = 0; i < kNumProducers; ++i) {
    producers.emplace_back([&]() {
      for (int j = 0; j < 100; ++j) {
        Enable(string_view(&a_to_z[dis(gen)], 1));
        Disable(string_view(&a_to_z[dis(gen)], 1));
        IsEnabled(string_view(&a_to_z[j], 1));
      }
    });
  }
  for (auto& thread : producers) {
    thread.join();
  }
  ShutdownLogTag();
}

}  // namespace
}  // namespace debug
}  // namespace lull
