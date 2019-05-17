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

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/render/material_info.h"
#include "lullaby/util/hash.h"
#include "lullaby/tests/portable_test_macros.h"

using ::testing::Eq;
using ::testing::Not;

namespace lull {

TEST(MaterialInfoTest, TextureUsageInfoConstructors) {
  TextureUsageInfo single_usage_info(MaterialTextureUsage_BaseColor);
  EXPECT_THAT(single_usage_info.GetChannelUsage(0),
              Eq(MaterialTextureUsage_BaseColor));
  EXPECT_THAT(single_usage_info.GetChannelUsage(1),
              Eq(MaterialTextureUsage_Unused));
  EXPECT_THAT(single_usage_info.GetChannelUsage(2),
              Eq(MaterialTextureUsage_Unused));
  EXPECT_THAT(single_usage_info.GetChannelUsage(3),
              Eq(MaterialTextureUsage_Unused));

  std::vector<MaterialTextureUsage> usages = {MaterialTextureUsage_Occlusion,
                                              MaterialTextureUsage_Roughness};
  TextureUsageInfo multi_usage_info(usages);
  EXPECT_THAT(multi_usage_info.GetChannelUsage(0),
              Eq(MaterialTextureUsage_Occlusion));
  EXPECT_THAT(multi_usage_info.GetChannelUsage(1),
              Eq(MaterialTextureUsage_Roughness));
  EXPECT_THAT(multi_usage_info.GetChannelUsage(2),
              Eq(MaterialTextureUsage_Unused));
  EXPECT_THAT(multi_usage_info.GetChannelUsage(3),
              Eq(MaterialTextureUsage_Unused));
}

TEST(MaterialInfoTest, TextureUsageInfoEquality) {
  std::vector<MaterialTextureUsage> unused_roughness_metallic = {
      MaterialTextureUsage_Unused, MaterialTextureUsage_Roughness,
      MaterialTextureUsage_Metallic};
  TextureUsageInfo usage_info_a(unused_roughness_metallic);
  TextureUsageInfo usage_info_b(unused_roughness_metallic);
  EXPECT_THAT(usage_info_a, Eq(usage_info_b));

  std::vector<MaterialTextureUsage> occlusion_roughness_metallic = {
      MaterialTextureUsage_Occlusion, MaterialTextureUsage_Roughness,
      MaterialTextureUsage_Metallic};
  TextureUsageInfo usage_info_c(occlusion_roughness_metallic);
  EXPECT_THAT(usage_info_a, Not(Eq(usage_info_c)));
}

TEST(MaterialInfoTest, TextureUsageInfoToHash) {
  std::vector<MaterialTextureUsage> unused_roughness_metallic = {
      MaterialTextureUsage_Unused, MaterialTextureUsage_Roughness,
      MaterialTextureUsage_Metallic};
  TextureUsageInfo usage_info(unused_roughness_metallic);
  EXPECT_THAT(usage_info.GetHash(),
              Eq(Hash("Texture_UnusedRoughnessMetallic")));
}

TEST(MaterialInfoTest, TextureUsageInfoHasher) {
  TextureUsageInfo::Hasher hasher;

  std::vector<MaterialTextureUsage> unused_roughness_metallic = {
      MaterialTextureUsage_Unused, MaterialTextureUsage_Roughness,
      MaterialTextureUsage_Metallic};
  TextureUsageInfo usage_info_a(unused_roughness_metallic);
  TextureUsageInfo usage_info_b(unused_roughness_metallic);
  const size_t hash_a = hasher(usage_info_a);
  const size_t hash_b = hasher(usage_info_b);
  EXPECT_THAT(hash_a, Eq(hash_b));

  std::vector<MaterialTextureUsage> occlusion_roughness_metallic = {
      MaterialTextureUsage_Occlusion, MaterialTextureUsage_Roughness,
      MaterialTextureUsage_Metallic};
  TextureUsageInfo usage_info_c(occlusion_roughness_metallic);
  const size_t hash_c = hasher(usage_info_c);
  EXPECT_THAT(hash_a, Not(Eq(hash_c)));
}

}  // namespace lull
