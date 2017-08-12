/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#include "lullaby/util/intersections.h"

#include "gtest/gtest.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using testing::EqualsMathfu;

TEST(IntersectionsTest, Basic) {
  mathfu::vec3 intersection_position;
  EXPECT_TRUE(
      IntersectRayPlane(/* plane_normal= */ mathfu::vec3(0.0f, 0.0f, 1.0f),
                        /* plane_offset= */ 2.0f,
                        /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 1.0f),
                        /* ray_direction= */ mathfu::vec3(0.0f, 0.0f, 1.0f),
                        /* intersection_position= */ &intersection_position));
  EXPECT_THAT(intersection_position,
              EqualsMathfu(mathfu::vec3(0.0f, 0.0f, 2.0f)));

  EXPECT_TRUE(
      IntersectRayPlane(/* plane_normal= */ mathfu::vec3(1.0f, 0.0f, 0.0f),
                        /* plane_offset= */ 5.0f,
                        /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 1.0f),
                        /* ray_direction= */ mathfu::vec3(1.0f, 0.0f, 0.0f),
                        /* intersection_position= */ &intersection_position));
  EXPECT_THAT(intersection_position,
              EqualsMathfu(mathfu::vec3(5.0f, 0.0f, 1.0f)));
}

TEST(IntersectionsTest, RayInFrontOfPlane) {
  EXPECT_FALSE(
      IntersectRayPlane(/* plane_normal= */ mathfu::vec3(0.0f, 0.0f, 1.0f),
                        /* plane_offset= */ 2.0f,
                        /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 3.0f),
                        /* ray_direction= */ mathfu::vec3(0.0f, 0.0f, 1.0f)));
}

TEST(IntersectionsTest, RayBehindPlane) {
  EXPECT_FALSE(
      IntersectRayPlane(/* plane_normal= */ mathfu::vec3(0.0f, 0.0f, 1.0f),
                        /* plane_offset= */ 2.0f,
                        /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 1.0f),
                        /* ray_direction= */ mathfu::vec3(0.0f, 0.0f, -1.0f)));

  EXPECT_FALSE(
      IntersectRayPlane(/* plane_normal= */ mathfu::vec3(0.0f, 0.0f, 1.0f),
                        /* plane_offset= */ -1.0f,
                        /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 1.0f),
                        /* ray_direction= */ mathfu::vec3(0.0f, 0.0f, 1.0f)));
}

TEST(IntersectionsTest, RayParallelToPlane) {
  EXPECT_FALSE(
      IntersectRayPlane(/* plane_normal= */ mathfu::vec3(0.0f, 0.0f, 1.0f),
                        /* plane_offset= */ 1.0f,
                        /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 0.0f),
                        /* ray_direction= */ mathfu::vec3(1.0f, 0.0f, 0.0f)));

  EXPECT_FALSE(
      IntersectRayPlane(/* plane_normal= */ mathfu::vec3(0.0f, -1.0f, 0.0f),
                        /* plane_offset= */ 1.0f,
                        /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 0.0f),
                        /* ray_direction= */ mathfu::vec3(0.0f, 0.0f, 1.0f)));

  EXPECT_FALSE(
      IntersectRayPlane(/* plane_normal= */ mathfu::vec3(1.0f, 0.0f, 0.0f),
                        /* plane_offset= */ 1.0f,
                        /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 0.0f),
                        /* ray_direction= */ mathfu::vec3(0.0f, 1.0f, 0.0f)));
}

TEST(IntersectionsTest, QuadRotated45) {
  mathfu::vec3 intersection_position;
  EXPECT_TRUE(IntersectRayPlane(
      /* plane_normal= */ mathfu::vec3(0.707106781f, 0.0f, 0.707106781f),
      /* plane_offset= */ 0.707106781f,
      /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 0.0f),
      /* ray_direction= */ mathfu::vec3(1.0f, 0.0f, 0.0f),
      /* intersection_position= */ &intersection_position));
  EXPECT_THAT(intersection_position,
              EqualsMathfu(mathfu::vec3(1.0f, 0.0f, 0.0f)));
}

TEST(IntersectionsDeathTest, UnnormalizedVectors) {
  PORT_EXPECT_DEBUG_DEATH(
      IntersectRayPlane(
          /* plane_normal= */ mathfu::vec3(1.707106781f, 0.0f, 0.707106781f),
          /* plane_offset= */ 0.707106781f,
          /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 0.0f),
          /* ray_direction= */ mathfu::vec3(1.0f, 0.0f, 0.0f)),
      "");

  PORT_EXPECT_DEBUG_DEATH(
      IntersectRayPlane(
          /* plane_normal= */ mathfu::vec3(0.707106781f, 0.0f, 0.707106781f),
          /* plane_offset= */ 0.707106781f,
          /* ray_position= */ mathfu::vec3(0.0f, 0.0f, 0.0f),
          /* ray_direction= */ mathfu::vec3(5.0f, 0.0f, 0.0f)),
      "");
}

}  // namespace
}  // namespace lull
