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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/base/data_builder.h"
#include "redux/modules/graphics/image_atlaser.h"
#include "redux/modules/math/math.h"

namespace redux {
namespace {

using ::testing::Eq;

ImageData MakeImage(const vec2i& size) {
  DataBuilder data(size.x * size.y);
  data.Advance(size.x * size.y);
  return ImageData(ImageFormat::Alpha8, size, data.Release(), 0);
}

TEST(ImageAtlaserTest, Empty) {
  ImageAtlaser atlas(ImageFormat::Alpha8, vec2i(10, 10));
  EXPECT_THAT(atlas.GetNumSubimages(), Eq(0));
}

TEST(ImageAtlaserTest, FullImage) {
  ImageAtlaser atlas(ImageFormat::Alpha8, vec2i(10, 10));

  HashValue key(1);
  ImageData image = MakeImage(vec2i(10, 10));
  const auto result = atlas.Add(key, image);

  EXPECT_THAT(result, Eq(ImageAtlaser::kAddSuccessful));
  EXPECT_THAT(atlas.GetNumSubimages(), Eq(1));
  EXPECT_TRUE(atlas.Contains(key));

  auto bounds = atlas.GetUvBounds(key);
  EXPECT_THAT(bounds.min, Eq(vec2::Zero()));
  EXPECT_THAT(bounds.max, Eq(vec2::One()));
}

TEST(ImageAtlaserTest, MultipleImages) {
  ImageAtlaser atlas(ImageFormat::Alpha8, vec2i(10, 10));

  ImageData image = MakeImage(vec2i(5, 5));

  HashValue keys[] = {HashValue(1), HashValue(2), HashValue(3), HashValue(4)};
  for (int i = 0; i < 4; ++i) {
    atlas.Add(keys[i], image);
  }

  Bounds2f total_bounds = Bounds2f::Empty();

  EXPECT_THAT(atlas.GetNumSubimages(), Eq(4));
  for (int i = 0; i < 4; ++i) {
    EXPECT_TRUE(atlas.Contains(keys[i]));
    const auto bounds = atlas.GetUvBounds(keys[i]);
    total_bounds = total_bounds.Included(bounds.min);
    total_bounds = total_bounds.Included(bounds.max);
  }
  EXPECT_THAT(total_bounds.min, Eq(vec2::Zero()));
  EXPECT_THAT(total_bounds.max, Eq(vec2::One()));
}

TEST(ImageAtlaserTest, DuplicateKey) {
  ImageAtlaser atlas(ImageFormat::Alpha8, vec2i(10, 10));

  HashValue key(1);
  ImageData image = MakeImage(vec2i(10, 10));
  const auto result1 = atlas.Add(key, image);
  EXPECT_THAT(result1, Eq(ImageAtlaser::kAddSuccessful));

  const auto result2 = atlas.Add(key, image);
  EXPECT_THAT(result2, Eq(ImageAtlaser::kAlreadyExists));
}

TEST(ImageAtlaserTest, RejectTooLarge) {
  ImageAtlaser atlas(ImageFormat::Alpha8, vec2i(10, 10));

  HashValue key(1);
  ImageData image = MakeImage(vec2i(20, 20));
  EXPECT_THAT(atlas.Add(key, image), Eq(ImageAtlaser::kNoMoreSpace));
}
}  // namespace
}  // namespace redux
