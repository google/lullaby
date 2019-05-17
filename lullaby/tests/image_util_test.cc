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

#include "lullaby/modules/render/image_util.h"

#include "gtest/gtest.h"
#include "mathfu/constants.h"

namespace lull {
namespace {

TEST(ConvertRgb888ToRgba8888, NullGuard) {
#ifdef DEBUG  // These checks are DFATALs.
  const char* error_regex = "";
  uint8_t unused_data[4];
  EXPECT_DEATH_IF_SUPPORTED(
      ConvertRgb888ToRgba8888(nullptr, mathfu::kZeros2i, unused_data),
      error_regex);
  EXPECT_DEATH_IF_SUPPORTED(
      ConvertRgb888ToRgba8888(unused_data, mathfu::kZeros2i, nullptr),
      error_regex);
#endif
}

TEST(ConvertRgb888ToRgba8888, ExpectedValues) {
  constexpr int kWidth = 4;
  constexpr int kHeight = 4;
  constexpr int kNumPixels = kWidth * kHeight;

  uint8_t rgb_data[3 * kNumPixels];
  for (int i = 0; i < 3 * kNumPixels; ++i) {
    rgb_data[i] = static_cast<uint8_t>(i);
  }

  uint8_t rgba_data[4 * kNumPixels];
  ConvertRgb888ToRgba8888(rgb_data, mathfu::vec2i(kWidth, kHeight), rgba_data);

  for (int i = 0; i < kNumPixels; ++i) {
    EXPECT_EQ(rgba_data[4 * i + 0], rgb_data[3 * i + 0]);
    EXPECT_EQ(rgba_data[4 * i + 1], rgb_data[3 * i + 1]);
    EXPECT_EQ(rgba_data[4 * i + 2], rgb_data[3 * i + 2]);
    EXPECT_EQ(rgba_data[4 * i + 3], 255);
  }
}

}  // namespace
}  // namespace lull
