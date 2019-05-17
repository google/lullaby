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

#include "lullaby/modules/render/tangent_generation.h"
#include "gtest/gtest.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/tests/portable_test_macros.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {
namespace {

constexpr float kEpsilon = 1.0E-5f;

TEST(ComputeTangents, IndexedQuad) {
  // vec3 position, vec3 normal, vec2 texcoord
  float vertices[] = {
      0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0,
      0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1,
  };

  int indices[] = {
      0, 1, 2, 2, 1, 3,
  };

  // vec4 tangent, vec3 bitangent
  float tan_bitans[4 * (4 + 3)];

  size_t vertex_stride = sizeof(float) * (3 + 3 + 2);
  size_t tan_bitan_stride = sizeof(float) * (4 + 3);
  lull::ComputeTangentsWithIndexedTriangles(
      reinterpret_cast<const uint8_t*>(vertices), vertex_stride,
      reinterpret_cast<const uint8_t*>(vertices + 3), vertex_stride,
      reinterpret_cast<const uint8_t*>(vertices + 3 + 3), vertex_stride, 4,
      reinterpret_cast<const uint8_t*>(indices), sizeof(indices[0]), 2,
      reinterpret_cast<uint8_t*>(tan_bitans), tan_bitan_stride,
      reinterpret_cast<uint8_t*>(tan_bitans + 4), tan_bitan_stride);

  EXPECT_NEAR(tan_bitans[0], 1.f, kEpsilon);
  EXPECT_NEAR(tan_bitans[1], 0.f, kEpsilon);
  EXPECT_NEAR(tan_bitans[2], 0.f, kEpsilon);

  EXPECT_NEAR(tan_bitans[3], -1.f, kEpsilon);
  EXPECT_NEAR(tan_bitans[4], 0.f, kEpsilon);
  EXPECT_NEAR(tan_bitans[5], -1.f, kEpsilon);
}

TEST(ComputeTangents, NonindexedQuad) {
  // vec3 position, vec3 normal, vec2 texcoord
  float vertices[] = {
      0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 1,
      0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 1, 1,
  };

  // vec4 tangent, vec3 bitangent
  float tan_bitans[6 * (4 + 3)];

  size_t vertex_stride = sizeof(float) * (3 + 3 + 2);
  size_t tan_bitan_stride = sizeof(float) * (4 + 3);
  lull::ComputeTangentsWithTriangles(
      reinterpret_cast<const uint8_t*>(vertices), vertex_stride,
      reinterpret_cast<const uint8_t*>(vertices + 3), vertex_stride,
      reinterpret_cast<const uint8_t*>(vertices + 3 + 3), vertex_stride, 4, 2,
      reinterpret_cast<uint8_t*>(tan_bitans), tan_bitan_stride,
      reinterpret_cast<uint8_t*>(tan_bitans + 4), tan_bitan_stride);

  EXPECT_NEAR(tan_bitans[0], 1.f, kEpsilon);
  EXPECT_NEAR(tan_bitans[1], 0.f, kEpsilon);
  EXPECT_NEAR(tan_bitans[2], 0.f, kEpsilon);

  EXPECT_NEAR(tan_bitans[3], -1.f, kEpsilon);
  EXPECT_NEAR(tan_bitans[4], 0.f, kEpsilon);
  EXPECT_NEAR(tan_bitans[5], -1.f, kEpsilon);
}

}  // namespace
}  // namespace lull
