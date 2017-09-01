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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "mathfu/io.h"
#include "lullaby/util/math.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using testing::NearMathfu;

const float kEpsilon = kDefaultEpsilon;
const float M_PI_float = static_cast<float>(M_PI);
const float kSqrt2 = static_cast<float>(sqrt(2));

void TestMatrixEquality(const mathfu::mat4& mat1, const mathfu::mat4& mat2) {
  EXPECT_NEAR(mat1(0, 0), mat2(0, 0), kEpsilon);
  EXPECT_NEAR(mat1(0, 1), mat2(0, 1), kEpsilon);
  EXPECT_NEAR(mat1(0, 2), mat2(0, 2), kEpsilon);
  EXPECT_NEAR(mat1(0, 3), mat2(0, 3), kEpsilon);

  EXPECT_NEAR(mat1(1, 0), mat2(1, 0), kEpsilon);
  EXPECT_NEAR(mat1(1, 1), mat2(1, 1), kEpsilon);
  EXPECT_NEAR(mat1(1, 2), mat2(1, 2), kEpsilon);
  EXPECT_NEAR(mat1(1, 3), mat2(1, 3), kEpsilon);

  EXPECT_NEAR(mat1(2, 0), mat2(2, 0), kEpsilon);
  EXPECT_NEAR(mat1(2, 1), mat2(2, 1), kEpsilon);
  EXPECT_NEAR(mat1(2, 2), mat2(2, 2), kEpsilon);
  EXPECT_NEAR(mat1(2, 3), mat2(2, 3), kEpsilon);

  EXPECT_NEAR(mat1(3, 0), mat2(3, 0), kEpsilon);
  EXPECT_NEAR(mat1(3, 1), mat2(3, 1), kEpsilon);
  EXPECT_NEAR(mat1(3, 2), mat2(3, 2), kEpsilon);
  EXPECT_NEAR(mat1(3, 3), mat2(3, 3), kEpsilon);
}

TEST(CalculateTransformMatrix, Simple) {
  const mathfu::vec3 position(0.f, 1.f, 2.f);
  const mathfu::vec3 angles(0.f, M_PI_float, 0.f);
  const mathfu::vec3 scale(1.f, 2.f, 3.f);

  auto mat4 = CalculateTransformMatrix(
      position, mathfu::quat::FromEulerAngles(angles), scale);
  const mathfu::mat4 expected(-1.f, 0.f, 0.f, 0.f, 0.f, 2.f, 0.f, 0.f, 0.f, 0.f,
                              -3.f, 0.f, 0.f, 1.f, 2.f, 1.f);
  TestMatrixEquality(expected, mat4);
}

TEST(CheckRayTriangleCollision, Hit) {
  Ray ray(mathfu::kZeros3f, mathfu::kAxisZ3f);
  Triangle triangle(mathfu::vec3(0.f, 1.f, 1.f), mathfu::vec3(-1.f, -1.f, 1.f),
                    mathfu::vec3(1.f, -1.f, 1.f));
  const float dist = CheckRayTriangleCollision(ray, triangle);
  EXPECT_NEAR(dist, 1.f, kEpsilon);
}

TEST(CheckRayTriangleCollision, Parallel) {
  Ray ray(mathfu::kZeros3f, mathfu::kAxisZ3f);
  Triangle triangle(mathfu::vec3(0.f, 1.f, 0.f), mathfu::vec3(-1.f, -1.f, 0.f),
                    mathfu::vec3(1.f, -1.f, 0.f));
  const float dist = CheckRayTriangleCollision(ray, triangle);
  EXPECT_EQ(dist, kNoHitDistance);
}

TEST(CheckRayTriangleCollision, Outside) {
  Triangle triangle(mathfu::vec3(0.f, 1.f, 1.f), mathfu::vec3(-1.f, -1.f, 1.f),
                    mathfu::vec3(1.f, -1.f, 1.f));

  Ray outside_right_edge(mathfu::vec3(0.1f, -1.f, 1.f), mathfu::kAxisZ3f);
  float dist = CheckRayTriangleCollision(outside_right_edge, triangle);
  EXPECT_EQ(dist, kNoHitDistance);

  Ray outside_left_edge(mathfu::vec3(-0.1f, -1.f, 1.f), mathfu::kAxisZ3f);
  dist = CheckRayTriangleCollision(outside_left_edge, triangle);
  EXPECT_EQ(dist, kNoHitDistance);

  Ray outside_bottom_edge(mathfu::vec3(0.f, -1.1f, 1.f), mathfu::kAxisZ3f);
  dist = CheckRayTriangleCollision(outside_bottom_edge, triangle);
  EXPECT_EQ(dist, kNoHitDistance);
}

void TestAxisRays(const mathfu::mat4& world_mat, const Aabb& aabb,
                  float result[], bool collision_on_exit) {
  // These are inward facing rays, which should all hit.
  result[0] =
      CheckRayOBBCollision(Ray(mathfu::vec3(2, 0, 0), mathfu::vec3(-1, 0, 0)),
                           world_mat, aabb, collision_on_exit);
  result[1] =
      CheckRayOBBCollision(Ray(mathfu::vec3(-2, 0, 0), mathfu::vec3(1, 0, 0)),
                           world_mat, aabb, collision_on_exit);
  result[2] =
      CheckRayOBBCollision(Ray(mathfu::vec3(0, 2, 0), mathfu::vec3(0, -1, 0)),
                           world_mat, aabb, collision_on_exit);
  result[3] =
      CheckRayOBBCollision(Ray(mathfu::vec3(0, -2, 0), mathfu::vec3(0, 1, 0)),
                           world_mat, aabb, collision_on_exit);
  result[4] =
      CheckRayOBBCollision(Ray(mathfu::vec3(0, 0, 2), mathfu::vec3(0, 0, -1)),
                           world_mat, aabb, collision_on_exit);
  result[5] =
      CheckRayOBBCollision(Ray(mathfu::vec3(0, 0, -2), mathfu::vec3(0, 0, 1)),
                           world_mat, aabb, collision_on_exit);

  // These are outward facing rays, which should all miss.
  result[6] =
      CheckRayOBBCollision(Ray(mathfu::vec3(2, 0, 0), mathfu::vec3(1, 0, 0)),
                           world_mat, aabb, collision_on_exit);
  result[7] =
      CheckRayOBBCollision(Ray(mathfu::vec3(-2, 0, 0), mathfu::vec3(-1, 0, 0)),
                           world_mat, aabb, collision_on_exit);
  result[8] =
      CheckRayOBBCollision(Ray(mathfu::vec3(0, 2, 0), mathfu::vec3(0, 1, 0)),
                           world_mat, aabb, collision_on_exit);
  result[9] =
      CheckRayOBBCollision(Ray(mathfu::vec3(0, -2, 0), mathfu::vec3(0, -1, 0)),
                           world_mat, aabb, collision_on_exit);
  result[10] =
      CheckRayOBBCollision(Ray(mathfu::vec3(0, 0, 2), mathfu::vec3(0, 0, 1)),
                           world_mat, aabb, collision_on_exit);
  result[11] =
      CheckRayOBBCollision(Ray(mathfu::vec3(0, 0, -2), mathfu::vec3(0, 0, -1)),
                           world_mat, aabb, collision_on_exit);
}

TEST(CheckRayOBBCollision, Hit) {
  Ray ray(mathfu::kZeros3f, mathfu::kAxisZ3f);
  Sqt sqt(mathfu::kZeros3f, mathfu::quat::identity, mathfu::kOnes3f);
  auto mat4 =
      CalculateTransformMatrix(sqt.translation, sqt.rotation, sqt.scale);
  Aabb aabb(-1.f * mathfu::kOnes3f, mathfu::kOnes3f);
  float dist = CheckRayOBBCollision(ray, mat4, aabb, false);
  EXPECT_NEAR(dist, 1.f, kEpsilon);
  dist = CheckRayOBBCollision(ray, mat4, aabb, true);
  EXPECT_NEAR(dist, 1.f, kEpsilon);
}

TEST(CheckRayOBBCollision, Thorough) {
  const float kDegreesToRadians = M_PI_float / 180.0f;

  // Setup.
  Aabb aabb(-1.f * mathfu::kOnes3f, mathfu::kOnes3f);

  auto mat4 = CalculateTransformMatrix(mathfu::kZeros3f, mathfu::quat::identity,
                                       mathfu::kOnes3f);

  // Test axis aligned rays from each direction.
  float results[12];
  TestAxisRays(mat4, aabb, results, false);
  EXPECT_NEAR(results[0], 1, kEpsilon);
  EXPECT_NEAR(results[1], 1, kEpsilon);
  EXPECT_NEAR(results[2], 1, kEpsilon);
  EXPECT_NEAR(results[3], 1, kEpsilon);
  EXPECT_NEAR(results[4], 1, kEpsilon);
  EXPECT_NEAR(results[5], 1, kEpsilon);
  EXPECT_NEAR(results[6], -1, kEpsilon);
  EXPECT_NEAR(results[7], -1, kEpsilon);
  EXPECT_NEAR(results[8], -1, kEpsilon);
  EXPECT_NEAR(results[9], -1, kEpsilon);
  EXPECT_NEAR(results[10], -1, kEpsilon);
  EXPECT_NEAR(results[11], -1, kEpsilon);

  TestAxisRays(mat4, aabb, results, true);
  EXPECT_NEAR(results[0], 3, kEpsilon);
  EXPECT_NEAR(results[1], 3, kEpsilon);
  EXPECT_NEAR(results[2], 3, kEpsilon);
  EXPECT_NEAR(results[3], 3, kEpsilon);
  EXPECT_NEAR(results[4], 3, kEpsilon);
  EXPECT_NEAR(results[5], 3, kEpsilon);
  EXPECT_NEAR(results[6], -1, kEpsilon);
  EXPECT_NEAR(results[7], -1, kEpsilon);
  EXPECT_NEAR(results[8], -1, kEpsilon);
  EXPECT_NEAR(results[9], -1, kEpsilon);
  EXPECT_NEAR(results[10], -1, kEpsilon);
  EXPECT_NEAR(results[11], -1, kEpsilon);

  // Rotate the cube by 45 degrees an check that the numbers are still correct
  const float kYRot = 45 * kDegreesToRadians;
  auto rotation = mathfu::quat::FromEulerAngles(mathfu::vec3(0, kYRot, 0));

  mat4 = CalculateTransformMatrix(mathfu::kZeros3f, rotation, mathfu::kOnes3f);
  float expected = 2 - kSqrt2;
  TestAxisRays(mat4, aabb, results, false);
  EXPECT_NEAR(results[0], expected, kEpsilon);
  EXPECT_NEAR(results[1], expected, kEpsilon);
  EXPECT_NEAR(results[2], 1, kEpsilon);
  EXPECT_NEAR(results[3], 1, kEpsilon);
  EXPECT_NEAR(results[4], expected, kEpsilon);
  EXPECT_NEAR(results[5], expected, kEpsilon);
  EXPECT_NEAR(results[6], -1, kEpsilon);
  EXPECT_NEAR(results[7], -1, kEpsilon);
  EXPECT_NEAR(results[8], -1, kEpsilon);
  EXPECT_NEAR(results[9], -1, kEpsilon);
  EXPECT_NEAR(results[10], -1, kEpsilon);
  EXPECT_NEAR(results[11], -1, kEpsilon);

  float expected_exit = 2 + kSqrt2;
  TestAxisRays(mat4, aabb, results, true);
  EXPECT_NEAR(results[0], expected_exit, kEpsilon);
  EXPECT_NEAR(results[1], expected_exit, kEpsilon);
  EXPECT_NEAR(results[2], 3, kEpsilon);
  EXPECT_NEAR(results[3], 3, kEpsilon);
  EXPECT_NEAR(results[4], expected_exit, kEpsilon);
  EXPECT_NEAR(results[5], expected_exit, kEpsilon);
  EXPECT_NEAR(results[6], -1, kEpsilon);
  EXPECT_NEAR(results[7], -1, kEpsilon);
  EXPECT_NEAR(results[8], -1, kEpsilon);
  EXPECT_NEAR(results[9], -1, kEpsilon);
  EXPECT_NEAR(results[10], -1, kEpsilon);
  EXPECT_NEAR(results[11], -1, kEpsilon);

  // Add a translation.
  auto position = mathfu::vec3(0.1f, 0.2f, 0.3f);
  mat4 = CalculateTransformMatrix(position, rotation, mathfu::kOnes3f);
  TestAxisRays(mat4, aabb, results, false);
  EXPECT_NEAR(results[0], expected - position.x + position.z, kEpsilon);
  EXPECT_NEAR(results[1], expected + position.x + position.z, kEpsilon);
  EXPECT_NEAR(results[2], 1 - position.y, kEpsilon);
  EXPECT_NEAR(results[3], 1 + position.y, kEpsilon);
  EXPECT_NEAR(results[4], expected - position.z + position.x, kEpsilon);
  EXPECT_NEAR(results[5], expected + position.z + position.x, kEpsilon);
  EXPECT_NEAR(results[6], -1, kEpsilon);
  EXPECT_NEAR(results[7], -1, kEpsilon);
  EXPECT_NEAR(results[8], -1, kEpsilon);
  EXPECT_NEAR(results[9], -1, kEpsilon);
  EXPECT_NEAR(results[10], -1, kEpsilon);
  EXPECT_NEAR(results[11], -1, kEpsilon);

  TestAxisRays(mat4, aabb, results, true);
  EXPECT_NEAR(results[0], expected_exit - position.x - position.z, kEpsilon);
  EXPECT_NEAR(results[1], expected_exit + position.x - position.z, kEpsilon);
  EXPECT_NEAR(results[2], 3 - position.y, kEpsilon);
  EXPECT_NEAR(results[3], 3 + position.y, kEpsilon);
  EXPECT_NEAR(results[4], expected_exit - position.z - position.x, kEpsilon);
  EXPECT_NEAR(results[5], expected_exit + position.z - position.x, kEpsilon);
  EXPECT_NEAR(results[6], -1, kEpsilon);
  EXPECT_NEAR(results[7], -1, kEpsilon);
  EXPECT_NEAR(results[8], -1, kEpsilon);
  EXPECT_NEAR(results[9], -1, kEpsilon);
  EXPECT_NEAR(results[10], -1, kEpsilon);
  EXPECT_NEAR(results[11], -1, kEpsilon);

  // Add a bit of scale.
  const float kScale = 0.5;
  auto scale = mathfu::vec3(kScale, kScale, kScale);
  mat4 = CalculateTransformMatrix(position, rotation, scale);
  TestAxisRays(mat4, aabb, results, false);
  expected = 2 - kSqrt2 * kScale;
  float expectedY = 2 - 1 * kScale;
  EXPECT_NEAR(results[0], expected - position.x + position.z, kEpsilon);
  EXPECT_NEAR(results[1], expected + position.x + position.z, kEpsilon);
  EXPECT_NEAR(results[2], expectedY - position.y, kEpsilon);
  EXPECT_NEAR(results[3], expectedY + position.y, kEpsilon);
  EXPECT_NEAR(results[4], expected - position.z + position.x, kEpsilon);
  EXPECT_NEAR(results[5], expected + position.z + position.x, kEpsilon);
  EXPECT_NEAR(results[6], -1, kEpsilon);
  EXPECT_NEAR(results[7], -1, kEpsilon);
  EXPECT_NEAR(results[8], -1, kEpsilon);
  EXPECT_NEAR(results[9], -1, kEpsilon);
  EXPECT_NEAR(results[10], -1, kEpsilon);
  EXPECT_NEAR(results[11], -1, kEpsilon);

  TestAxisRays(mat4, aabb, results, true);
  expected_exit = 2 + kSqrt2 * kScale;
  float expectedY_exit = 2 + 1 * kScale;
  EXPECT_NEAR(results[0], expected_exit - position.x - position.z, kEpsilon);
  EXPECT_NEAR(results[1], expected_exit + position.x - position.z, kEpsilon);
  EXPECT_NEAR(results[2], expectedY_exit - position.y, kEpsilon);
  EXPECT_NEAR(results[3], expectedY_exit + position.y, kEpsilon);
  EXPECT_NEAR(results[4], expected_exit - position.z - position.x, kEpsilon);
  EXPECT_NEAR(results[5], expected_exit + position.z - position.x, kEpsilon);
  EXPECT_NEAR(results[6], -1, kEpsilon);
  EXPECT_NEAR(results[7], -1, kEpsilon);
  EXPECT_NEAR(results[8], -1, kEpsilon);
  EXPECT_NEAR(results[9], -1, kEpsilon);
  EXPECT_NEAR(results[10], -1, kEpsilon);
  EXPECT_NEAR(results[11], -1, kEpsilon);
}

TEST(CheckRayOBBCollision, Scaled) {
  const float kPosValue = 2.0f;
  const float boxSize = 1.0f;
  const float root2 = 1.41421356f;
  // Looking forward and 45 degrees up
  Ray ray(mathfu::kZeros3f,
          (-mathfu::kAxisZ3f - mathfu::kAxisY3f).Normalized());
  Aabb aabb(-boxSize * mathfu::kOnes3f, boxSize * mathfu::kOnes3f);

  for (float scale = 0.1f; scale <= 2.0f; scale += 0.1f) {
    auto mat4 = CalculateTransformMatrix(
        mathfu::vec3(0, -kPosValue, -kPosValue), mathfu::quat::identity,
        mathfu::vec3(scale, scale, scale));
    float dist = CheckRayOBBCollision(ray, mat4, aabb, false);
    EXPECT_NEAR(dist, kPosValue * root2 - boxSize * scale * root2, kEpsilon);
    dist = CheckRayOBBCollision(ray, mat4, aabb, true);
    EXPECT_NEAR(dist, kPosValue * root2 + boxSize * scale * root2, kEpsilon);
  }
}

TEST(CheckRayOBBCollision, NonUniformScaled) {
  const float boxSize = 1.0f;
  const float root2 = 1.41421356f;
  // Looking forward and 45 degrees up
  Ray ray(mathfu::kZeros3f,
          (-mathfu::kAxisZ3f - mathfu::kAxisY3f).Normalized());

  for (float scale = 0.5f; scale <= 2.0f; scale += 0.1f) {
    auto boxVector = mathfu::vec3(1.0f, scale, 1.0f);
    Aabb aabb(-boxSize * boxVector, boxSize * mathfu::kOnes3f);
    auto mat4 =
        CalculateTransformMatrix(mathfu::kZeros3f, mathfu::quat::identity,
                                 mathfu::vec3(1.0f, 1 / scale, 1.0f));
    float dist = CheckRayOBBCollision(ray, mat4, aabb, true);
    EXPECT_NEAR(dist, root2, kEpsilon);
  }
}

TEST(CheckRayOBBCollision, Translated) {
  // Create a bounding box translated in the Z direction and a transformation
  // matrix that undoes this translation.
  const mathfu::vec3 trans(mathfu::kAxisZ3f);
  const Aabb aabb(trans - mathfu::kOnes3f, trans + mathfu::kOnes3f);
  const mathfu::mat4 mat4(mathfu::mat4::FromTranslationVector(-trans));

  // Create a ray pointing in the Z direction.
  const Ray ray(mathfu::kZeros3f, mathfu::kAxisZ3f);

  float dist = CheckRayOBBCollision(ray, mat4, aabb, false);
  EXPECT_NEAR(dist, 1.0f, kEpsilon);
  dist = CheckRayOBBCollision(ray, mat4, aabb, true);
  EXPECT_NEAR(dist, 1.0f, kEpsilon);
}

TEST(CheckPointOBBCollision, HitInside) {
  auto mat4 = CalculateTransformMatrix(mathfu::kZeros3f, mathfu::quat::identity,
                                       mathfu::kOnes3f);
  Aabb aabb(-1.f * mathfu::kOnes3f, 3.f * mathfu::kOnes3f);

  // Check when point is well inside of aabb.
  const float d = 0.5f;
  EXPECT_EQ(CheckPointOBBCollision(mathfu::kZeros3f, mat4, aabb), true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, 0.0f, 0.0f), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, d, 0.0f), mat4, aabb), true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, d, d), mat4, aabb), true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, d, 0.0f), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, d, d), mat4, aabb), true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, 0.0f, d), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, 0.0f, d), mat4, aabb), true);

  const float e = 2.5f;
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(-d, 0.0f, e), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(-d, -d, e), mat4, aabb), true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(-d, e, e), mat4, aabb), true);
}

TEST(CheckPointOBBCollision, HitExtents) {
  auto mat4 = CalculateTransformMatrix(mathfu::kZeros3f, mathfu::quat::identity,
                                       mathfu::kOnes3f);
  Aabb aabb(-1.f * mathfu::kOnes3f, mathfu::kOnes3f);

  // Check when point is at extents of aabb.
  const float d = 1.0f;
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, 0.0f, 0.0f), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, d, 0.0f), mat4, aabb), true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, d, d), mat4, aabb), true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, d, 0.0f), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, d, d), mat4, aabb), true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, 0.0f, d), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, 0.0f, d), mat4, aabb), true);
}

TEST(CheckPointOBBCollision, NoHit) {
  auto mat4 = CalculateTransformMatrix(mathfu::kZeros3f, mathfu::quat::identity,
                                       mathfu::kOnes3f);
  Aabb aabb(-1.f * mathfu::kOnes3f, mathfu::kOnes3f);

  // Check when point is well outside of aabb.
  const float d = 2.0f;
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, 0.0f, 0.0f), mat4, aabb),
            false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, d, 0.0f), mat4, aabb),
            false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, d, d), mat4, aabb), false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, d, 0.0f), mat4, aabb),
            false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, d, d), mat4, aabb),
            false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, 0.0f, d), mat4, aabb),
            false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, 0.0f, d), mat4, aabb),
            false);
}

TEST(CheckPointOBBCollision, NoHitExtents) {
  auto mat4 = CalculateTransformMatrix(mathfu::kZeros3f, mathfu::quat::identity,
                                       mathfu::kOnes3f);
  Aabb aabb(-1.f * mathfu::kOnes3f, mathfu::kOnes3f);

  // Check when point is just outside of aabb.
  const float d = 1.01f;
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, 0.0f, 0.0f), mat4, aabb),
            false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, d, 0.0f), mat4, aabb),
            false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, d, d), mat4, aabb), false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, d, 0.0f), mat4, aabb),
            false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, d, d), mat4, aabb),
            false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(0.0f, 0.0f, d), mat4, aabb),
            false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(d, 0.0f, d), mat4, aabb),
            false);
}

TEST(CheckPointOBBCollision, Scaled) {
  auto mat4 = CalculateTransformMatrix(mathfu::kZeros3f, mathfu::quat::identity,
                                       mathfu::vec3(2.0f, 1.0f, 3.0f));
  Aabb aabb(-1.f * mathfu::kOnes3f, mathfu::kOnes3f);

  EXPECT_EQ(CheckPointOBBCollision(mathfu::kZeros3f, mat4, aabb), true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(-1.0f, 0.5f, 2.5f), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(2.0f, 1.0f, 3.0f), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(-2.5f, 0.0f, 0.0f), mat4, aabb),
            false);
  EXPECT_EQ(
      CheckPointOBBCollision(mathfu::vec3(2.0f, 1.0f, -3.01f), mat4, aabb),
      false);
}

TEST(CheckPointOBBCollision, Translated) {
  auto mat4 = CalculateTransformMatrix(mathfu::vec3(2.0f, 1.0f, 3.0f),
                                       mathfu::quat::identity, mathfu::kOnes3f);
  Aabb aabb(-1.f * mathfu::kOnes3f, mathfu::kOnes3f);

  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(2.0f, 1.0f, 3.0f), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(1.0f, 1.0f, 3.0f), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(1.0f, 1.5f, 2.01f), mat4, aabb),
            true);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::kZeros3f, mat4, aabb), false);
  EXPECT_EQ(CheckPointOBBCollision(mathfu::vec3(1.0f, 1.5f, 4.01f), mat4, aabb),
            false);
}

TEST(PlaneConstructor, WithOrigin) {
  const mathfu::vec3 normal = mathfu::vec3(3.0f, 4.0f, 0.0f).Normalized();
  const mathfu::vec3 position(4.0f, 0.0f, 0.0f);
  const float expected_dist = 3.0f * 4.0f / 5.0f;
  Plane p(position, normal);
  EXPECT_NEAR(p.distance, expected_dist, kEpsilon);
  EXPECT_TRUE(IsNearlyZero((p.Origin() - normal * expected_dist).Length()));
}

TEST(ProjectPointOntoPlane, PointOnPlane) {
  const mathfu::vec3 point(1.0f, 2.0f, 1.0f);
  const Plane plane(1.0f, mathfu::kAxisZ3f);
  const mathfu::vec3 expected_point(1.0f, 2.0f, 1.0f);
  const mathfu::vec3 out = ProjectPointOntoPlane(plane, point);
  EXPECT_TRUE(IsNearlyZero((out - expected_point).Length()));
}

TEST(ProjectPointOntoPlane, PointAbovePlane) {
  const mathfu::vec3 point(1.0f, 2.0f, 3.0f);
  const Plane plane(1.0f, mathfu::kAxisZ3f);
  const mathfu::vec3 expected_point(1.0f, 2.0f, 1.0f);
  const mathfu::vec3 out = ProjectPointOntoPlane(plane, point);
  EXPECT_TRUE(IsNearlyZero((out - expected_point).Length()));
}

TEST(ProjectPointOntoPlane, PointBelowPlane) {
  const mathfu::vec3 point(1.0f, 2.0f, -3.0f);
  const Plane plane(1.0f, mathfu::kAxisZ3f);
  const mathfu::vec3 expected_point(1.0f, 2.0f, 1.0f);
  const mathfu::vec3 out = ProjectPointOntoPlane(plane, point);
  EXPECT_TRUE(IsNearlyZero((out - expected_point).Length()));
}

TEST(ComputeRayPlaneCollision, HitFront) {
  Plane p(mathfu::vec3(0.0f, 0.0f, 0.0f), mathfu::vec3(0.0f, 1.0f, 0.0f));
  Ray r(mathfu::vec3(0.0f, 4.0f, 0.0f),
        mathfu::vec3(3.0f, -4.0f, 0.0f).Normalized());
  mathfu::vec3 out;
  EXPECT_TRUE(ComputeRayPlaneCollision(r, p, &out));
  mathfu::vec3 expected_out(3.0f, 0.0f, 0.0f);
  EXPECT_TRUE(IsNearlyZero((out - expected_out).Length()));
}

TEST(ComputeRayPlaneCollision, HitBack) {
  Plane p(mathfu::vec3(0.0f, 0.0f, 0.0f), mathfu::vec3(0.0f, -1.0f, 0.0f));
  Ray r(mathfu::vec3(0.0f, 4.0f, 0.0f),
        mathfu::vec3(3.0f, -4.0f, 0.0f).Normalized());
  mathfu::vec3 out;
  EXPECT_TRUE(ComputeRayPlaneCollision(r, p, &out));
  mathfu::vec3 expected_out(3.0f, 0.0f, 0.0f);
  EXPECT_TRUE(IsNearlyZero((out - expected_out).Length()));
}

TEST(ComputeRayPlaneCollision, NoHit) {
  Plane p(mathfu::vec3(0.0f, 0.0f, 0.0f), mathfu::vec3(0.0f, -1.0f, 0.0f));
  Ray r(mathfu::vec3(0.0f, 4.0f, 0.0f),
        mathfu::vec3(3.0f, 4.0f, 0.0f).Normalized());
  mathfu::vec3 out;
  EXPECT_FALSE(ComputeRayPlaneCollision(r, p, &out));
}

TEST(ComputeRayPlaneCollision, RayOriginOnPlane) {
  Plane p(mathfu::vec3(0.0f, 0.0f, 0.0f), mathfu::vec3(0.0f, -1.0f, 0.0f));
  Ray r(mathfu::vec3(0.0f, 0.0f, 0.0f),
        mathfu::vec3(3.0f, 4.0f, 0.0f).Normalized());
  mathfu::vec3 out;
  EXPECT_TRUE(ComputeRayPlaneCollision(r, p, &out));
  mathfu::vec3 expected_out(0.0f, 0.0f, 0.0f);
  EXPECT_TRUE(IsNearlyZero((out - expected_out).Length()));
}

TEST(ComputeRayPlaneCollision, Parallel) {
  Plane p(mathfu::vec3(0.0f, 0.0f, 0.0f), mathfu::vec3(0.0f, -1.0f, 0.0f));
  Ray r(mathfu::vec3(0.0f, 4.0f, 0.0f),
        mathfu::vec3(1.0f, 0.0f, 0.0f));
  mathfu::vec3 out;
  EXPECT_FALSE(ComputeRayPlaneCollision(r, p, &out));
}

TEST(ProjectPointOntoLine, OnLine) {
  const Line line(mathfu::vec3(0.0f, 1.0f, 0.0f), mathfu::kAxisX3f);
  const mathfu::vec3 point(2.0f, 1.0f, 0.0f);
  const mathfu::vec3 projected_point = ProjectPointOntoLine(line, point);
  EXPECT_TRUE(IsNearlyZero((projected_point - point).Length()));
}

TEST(ProjectPointOntoLine, OffLine) {
  const Line line(mathfu::vec3(0.0f, 1.0f, 0.0f), mathfu::kAxisX3f);
  const mathfu::vec3 point(2.0f, 0.0f, 1.0f);
  const mathfu::vec3 point_expected(2.0f, 1.0f, 0.0f);
  const mathfu::vec3 projected_point = ProjectPointOntoLine(line, point);
  EXPECT_TRUE(IsNearlyZero((projected_point - point_expected).Length()));
}

TEST(ComputeClosestPointBetweenLines, Parallel) {
  const Line line_a(mathfu::vec3(0.0f, 0.0f, 0.0f), mathfu::kAxisX3f);
  const Line line_b(mathfu::vec3(0.0f, 1.0f, 0.0f), mathfu::kAxisX3f);
  EXPECT_FALSE(
      ComputeClosestPointBetweenLines(line_a, line_b, nullptr, nullptr));
}

TEST(ComputeClosestPointBetweenLines, Intersecting) {
  const Line line_a(mathfu::vec3(0.0f, 0.0f, 0.0f), mathfu::kAxisX3f);
  const Line line_b(mathfu::vec3(1.0f, 0.0f, 3.0f), mathfu::kAxisZ3f);
  mathfu::vec3 out_a = mathfu::kZeros3f;
  mathfu::vec3 out_b = mathfu::kZeros3f;
  EXPECT_TRUE(ComputeClosestPointBetweenLines(line_a, line_b, &out_a, &out_b));
  const mathfu::vec3 expected_point(1.0f, 0.0f, 0.0f);
  EXPECT_TRUE(IsNearlyZero((out_a - expected_point).Length()));
  EXPECT_TRUE(IsNearlyZero((out_b - expected_point).Length()));
}

TEST(ComputeClosestPointBetweenLines, NonIntersecting) {
  const Line line_a(mathfu::vec3(0.0f, 0.0f, 0.0f), mathfu::kAxisX3f);
  const Line line_b(mathfu::vec3(1.0f, 2.0f, 3.0f), mathfu::kAxisZ3f);
  mathfu::vec3 out_a = mathfu::kZeros3f;
  mathfu::vec3 out_b = mathfu::kZeros3f;
  EXPECT_TRUE(ComputeClosestPointBetweenLines(line_a, line_b, &out_a, &out_b));
  const mathfu::vec3 expected_a(1.0f, 0.0f, 0.0f);
  const mathfu::vec3 expected_b(1.0f, 2.0f, 0.0f);
  EXPECT_TRUE(IsNearlyZero((out_a - expected_a).Length()));
  EXPECT_TRUE(IsNearlyZero((out_b - expected_b).Length()));
}

TEST(EvalPointUvFromAabb, Normal) {
  const mathfu::vec3 min(-1.f, -2.f, -3.f);
  const mathfu::vec3 max(5.f, 6.f, 7.f);
  Aabb aabb(min, max);

  mathfu::vec2 uv;

  uv = EvalPointUvFromAabb(aabb, 2.f, 2.f);
  EXPECT_NEAR(0.5f, uv.x, kEpsilon);
  EXPECT_NEAR(0.5f, uv.y, kEpsilon);

  uv = EvalPointUvFromAabb(aabb, 3.f, 4.f);
  EXPECT_NEAR(0.66666f, uv.x, kEpsilon);
  EXPECT_NEAR(0.75f, uv.y, kEpsilon);

  uv = EvalPointUvFromAabb(aabb, -1.f, 6.f);
  EXPECT_NEAR(0.f, uv.x, kEpsilon);
  EXPECT_NEAR(1.f, uv.y, kEpsilon);

  uv = EvalPointUvFromAabb(aabb, -4.f, 8.f);
  EXPECT_NEAR(0.f, uv.x, kEpsilon);
  EXPECT_NEAR(1.f, uv.y, kEpsilon);

  uv = EvalPointUvFromAabb(aabb, 6.f, -3.f);
  EXPECT_NEAR(1.f, uv.x, kEpsilon);
  EXPECT_NEAR(0.f, uv.y, kEpsilon);
}

TEST(EvalPointUvFromAabb, ZeroWidth) {
  const mathfu::vec3 min(-1.f, -2.f, -3.f);
  const mathfu::vec3 max(-1.f, 6.f, 7.f);
  Aabb aabb(min, max);
  const mathfu::vec2 uv = EvalPointUvFromAabb(aabb, 2.f, 2.f);
  EXPECT_NEAR(0.f, uv.x, kEpsilon);
  EXPECT_NEAR(0.f, uv.y, kEpsilon);
}

TEST(EvalPointUvFromAabb, ZeroHeight) {
  const mathfu::vec3 min(-1.f, 6.f, -3.f);
  const mathfu::vec3 max(1.f, 6.f, 7.f);
  Aabb aabb(min, max);
  const mathfu::vec2 uv = EvalPointUvFromAabb(aabb, 2.f, 2.f);
  EXPECT_NEAR(0.f, uv.x, kEpsilon);
  EXPECT_NEAR(0.f, uv.y, kEpsilon);
}

TEST(CalculateSqtFromMatrix, Simple) {
  mathfu::vec3 start_pos = mathfu::vec3(1, 2, 3);
  mathfu::vec3 start_eulers = mathfu::vec3(30, 45, 90) * kDegreesToRadians;
  mathfu::vec3 start_scale = mathfu::vec3(2, 1, 3);

  mathfu::quat start_quat = mathfu::quat::FromEulerAngles(start_eulers);
  mathfu::mat3 rot_mat = start_quat.ToMatrix();
  mathfu::mat4 trans_mat = mathfu::mat4::FromTranslationVector(start_pos) *
                           mathfu::mat4::FromRotationMatrix(rot_mat) *
                           mathfu::mat4::FromScaleVector(start_scale);
  Sqt sqt = CalculateSqtFromMatrix(trans_mat);
  mathfu::vec3 end_eulers = sqt.rotation.ToEulerAngles();

  EXPECT_NEAR(start_pos[0], sqt.translation[0], kEpsilon);
  EXPECT_NEAR(start_pos[1], sqt.translation[1], kEpsilon);
  EXPECT_NEAR(start_pos[2], sqt.translation[2], kEpsilon);

  EXPECT_NEAR(start_eulers[0], end_eulers[0], kEpsilon);
  EXPECT_NEAR(start_eulers[1], end_eulers[1], kEpsilon);
  EXPECT_NEAR(start_eulers[2], end_eulers[2], kEpsilon);

  EXPECT_NEAR(start_scale[0], sqt.scale[0], kEpsilon);
  EXPECT_NEAR(start_scale[1], sqt.scale[1], kEpsilon);
  EXPECT_NEAR(start_scale[2], sqt.scale[2], kEpsilon);

  const mathfu::mat4 new_trans_mat = CalculateTransformMatrix(sqt);
  TestMatrixEquality(trans_mat, new_trans_mat);
}

TEST(CalculateRelativeMatrix, Simple) {
  mathfu::vec3 a_pos = mathfu::vec3(1, 2, 3);
  mathfu::vec3 a_eulers = mathfu::vec3(30, 45, 90) * kDegreesToRadians;
  mathfu::vec3 a_scale = mathfu::vec3(2, 2, 2);
  mathfu::quat a_quat = mathfu::quat::FromEulerAngles(a_eulers);
  mathfu::mat3 a_rot_mat = a_quat.ToMatrix();
  mathfu::mat4 world_to_a_mat = mathfu::mat4::FromTranslationVector(a_pos) *
                                mathfu::mat4::FromRotationMatrix(a_rot_mat) *
                                mathfu::mat4::FromScaleVector(a_scale);

  mathfu::vec3 b_pos = mathfu::vec3(-1, -1, 2);
  mathfu::vec3 b_eulers = mathfu::vec3(30, 0, 0) * kDegreesToRadians;
  mathfu::vec3 b_scale = mathfu::vec3(1, 1, 1);
  mathfu::quat b_quat = mathfu::quat::FromEulerAngles(b_eulers);
  mathfu::mat3 b_rot_mat = b_quat.ToMatrix();
  mathfu::mat4 a_to_b_mat = mathfu::mat4::FromTranslationVector(b_pos) *
                            mathfu::mat4::FromRotationMatrix(b_rot_mat) *
                            mathfu::mat4::FromScaleVector(b_scale);

  mathfu::mat4 world_to_b_mat = world_to_a_mat * a_to_b_mat;

  mathfu::mat4 a_to_b_mat_2 =
      CalculateRelativeMatrix(world_to_a_mat, world_to_b_mat);

  for (int i = 0; i < 16; ++i) {
    EXPECT_NEAR(a_to_b_mat[i], a_to_b_mat_2[i], kEpsilon);
  }
}

TEST(CalculateCylinderDeformedTransformMatrix, Simple) {
  // TODO(b/29100730) Remove this function
  const float kRadius = 2.0f;
  const float kParentRadius = 2.0f;

  Sqt sqt(mathfu::kZeros3f, mathfu::quat::identity, mathfu::kOnes3f);

  // Check no wrapping case.
  auto mat =
      CalculateCylinderDeformedTransformMatrix(sqt, kParentRadius, kRadius);
  EXPECT_NEAR(mat(0, 3), 0, kEpsilon);
  EXPECT_NEAR(mat(2, 3), 0, kEpsilon);

  // Check that a 1/4 wrap around will be handled.
  sqt.translation = mathfu::vec3(0.5f * M_PI_float * kRadius, 0, 0);
  mat = CalculateCylinderDeformedTransformMatrix(sqt, kParentRadius, kRadius);
  EXPECT_NEAR(mat(0, 3), kRadius, kEpsilon);
  EXPECT_NEAR(mat(2, 3), kRadius, kEpsilon);

  // Check that a 1/2 wrap around will be handled.
  sqt.translation = mathfu::vec3(1 * M_PI_float * kRadius, 0, 0);
  mat = CalculateCylinderDeformedTransformMatrix(sqt, kParentRadius, kRadius);
  EXPECT_NEAR(mat(0, 3), 0, kEpsilon);
  EXPECT_NEAR(mat(2, 3), 2 * kRadius, kEpsilon);

  // Check that a -1/4 wrap around will be handled.
  sqt.translation = mathfu::vec3(-0.5f * M_PI_float * kRadius, 0, 0);
  mat = CalculateCylinderDeformedTransformMatrix(sqt, kParentRadius, kRadius);
  EXPECT_NEAR(mat(0, 3), -kRadius, kEpsilon);
  EXPECT_NEAR(mat(2, 3), kRadius, kEpsilon);

  // Check that a full wrap around will be handled.
  sqt.translation = mathfu::vec3(2 * M_PI_float * kRadius, 0, 0);
  mat = CalculateCylinderDeformedTransformMatrix(sqt, kParentRadius, kRadius);
  EXPECT_NEAR(mat(0, 3), 0, kEpsilon);
  EXPECT_NEAR(mat(2, 3), 0, kEpsilon);

  // Check that radius a is correctly inherited.
  sqt.translation = mathfu::vec3(0.5f * M_PI_float * kRadius, 0, 0.25);
  mat = CalculateCylinderDeformedTransformMatrix(sqt, kParentRadius, kRadius);
  EXPECT_NEAR(mat(0, 3), kRadius - 0.25, kEpsilon);
  EXPECT_NEAR(mat(2, 3), kRadius, kEpsilon);
}

TEST(CalculateCylinderDeformedTransformMatrix, Identity) {
  // Check that a 1/4 wrap via translation will be handled.
  const float kRadius = 2.0f;
  mathfu::mat4 undeformed_mat = mathfu::mat4::Identity();

  // Should be no change for an identity matrix.
  mathfu::mat4 deformed_mat =
      CalculateCylinderDeformedTransformMatrix(undeformed_mat, kRadius);
  TestMatrixEquality(mathfu::mat4::Identity(), deformed_mat);
}

TEST(CalculateCylinderDeformedTransformMatrix, Full_Wrap) {
  // Check that a 1/4 wrap via translation will be handled.
  const float kAngle = 2.0f * M_PI_float;
  const float kRadius = 2.0f;
  mathfu::mat4 undeformed_mat =
      mathfu::mat4::FromTranslationVector(mathfu::vec3(kAngle * kRadius, 0, 0));

  mathfu::mat4 deformed_mat =
      CalculateCylinderDeformedTransformMatrix(undeformed_mat, kRadius);
  TestMatrixEquality(mathfu::mat4::Identity(), deformed_mat);
}

TEST(CalculateCylinderDeformedTransformMatrix, Translation) {
  // Check that a 1/4 wrap via translation will be handled.
  const float kAngle = 0.5f * M_PI_float;
  const float kRadius = 2.0f;

  mathfu::mat4 undeformed_mat =
      mathfu::mat4::FromTranslationVector(mathfu::vec3(kAngle * kRadius, 0, 0));

  mathfu::mat4 deformed_mat =
      CalculateCylinderDeformedTransformMatrix(undeformed_mat, kRadius);

  mathfu::quat rot = mathfu::quat::FromAngleAxis(-kAngle, mathfu::kAxisY3f);
  mathfu::mat4 expected_mat = rot.ToMatrix4();
  expected_mat(0, 3) = kRadius * std::sin(kAngle);
  expected_mat(1, 3) = 0;
  expected_mat(2, 3) = kRadius - kRadius * std::cos(kAngle);
  TestMatrixEquality(expected_mat, deformed_mat);
}
TEST(CalculateCylinderDeformedTransformMatrix, Negative_Translation) {
  // Check that a -1/4 wrap around will be handled.
  const float kAngle = -0.5f * M_PI_float;
  const float kRadius = 2.0f;

  mathfu::mat4 undeformed_mat =
      mathfu::mat4::FromTranslationVector(mathfu::vec3(kAngle * kRadius, 0, 0));

  mathfu::mat4 deformed_mat =
      CalculateCylinderDeformedTransformMatrix(undeformed_mat, kRadius);

  mathfu::quat rot = mathfu::quat::FromAngleAxis(-kAngle, mathfu::kAxisY3f);
  mathfu::mat4 expected_mat = rot.ToMatrix4();
  expected_mat(0, 3) = kRadius * std::sin(kAngle);
  expected_mat(1, 3) = 0;
  expected_mat(2, 3) = kRadius - kRadius * std::cos(kAngle);
  TestMatrixEquality(expected_mat, deformed_mat);
}

TEST(CalculateCylinderDeformedTransformMatrix, Translation_Scale) {
  // Check that a 1/2 wrap via translation and scale will be handled.
  const float kAngle = 0.5f * M_PI_float;
  const float kScale = 2;
  const float kRadius = 2.0f;

  mathfu::mat4 undeformed_mat =
      mathfu::mat4::FromScaleVector(mathfu::vec3(kScale, 1, 1)) *
      mathfu::mat4::FromTranslationVector(mathfu::vec3(kAngle * kRadius, 0, 0));

  mathfu::mat4 deformed_mat =
      CalculateCylinderDeformedTransformMatrix(undeformed_mat, kRadius);

  mathfu::quat rot =
      mathfu::quat::FromAngleAxis(-kAngle * kScale, mathfu::kAxisY3f);
  mathfu::mat4 expected_mat = rot.ToMatrix4() * mathfu::mat4::FromScaleVector(
                                                    mathfu::vec3(kScale, 1, 1));
  expected_mat(0, 3) = kRadius * std::sin(kAngle * kScale);
  expected_mat(1, 3) = 0;
  expected_mat(2, 3) = kRadius - kRadius * std::cos(kAngle * kScale);
  TestMatrixEquality(expected_mat, deformed_mat);
}

TEST(CalculateCylinderDeformedTransformMatrix, Translation_Scale_Rotate) {
  // Check that a -1/4 wrap via translation, scale, and a z rotation will be
  // handled.
  const float kAngle = 0.5f * M_PI_float;
  const float kScale = 1;
  const float kZAngle = M_PI_float;
  const float kRadius = 2.0f;

  mathfu::quat z_rot = mathfu::quat::FromAngleAxis(kZAngle, mathfu::kAxisZ3f);
  mathfu::mat4 undeformed_mat =
      z_rot.ToMatrix4() *
      mathfu::mat4::FromScaleVector(mathfu::vec3(kScale, 1, 1)) *
      mathfu::mat4::FromTranslationVector(mathfu::vec3(kAngle * kRadius, 0, 0));

  mathfu::mat4 deformed_mat =
      CalculateCylinderDeformedTransformMatrix(undeformed_mat, kRadius);

  mathfu::quat rot =
      mathfu::quat::FromAngleAxis(-kAngle * kScale, mathfu::kAxisY3f);
  mathfu::mat4 expected_mat =
      z_rot.ToMatrix4() * rot.ToMatrix4() *
      mathfu::mat4::FromScaleVector(mathfu::vec3(kScale, 1, 1));
  expected_mat(0, 3) = -kRadius * std::sin(kAngle * kScale);
  expected_mat(1, 3) = 0;
  expected_mat(2, 3) = kRadius - kRadius * std::cos(kAngle * kScale);
  TestMatrixEquality(expected_mat, deformed_mat);
}

TEST(CalculateRotateAroundMatrix, Simple) {
  const float kAngle = M_PI_float / 2.f;

  // Rotate (1,0,0) by PI / 2 around the Y-axis, centered on the origin.
  mathfu::mat4 origin_rotation_matrix =
      CalculateRotateAroundMatrix(mathfu::kZeros3f, mathfu::kAxisY3f, kAngle);
  EXPECT_THAT(origin_rotation_matrix * mathfu::kAxisX3f,
              NearMathfu(-mathfu::kAxisZ3f, kEpsilon));

  // Rotate (1,0,0) by PI / 2 around the Y-axis, centered on (1,1,1).
  mathfu::mat4 general_rotation_matrix =
      CalculateRotateAroundMatrix(mathfu::kOnes3f, mathfu::kAxisY3f, kAngle);
  EXPECT_THAT(general_rotation_matrix * mathfu::kAxisX3f,
              NearMathfu(mathfu::kAxisZ3f, kEpsilon));
}

TEST(CalculateLookAtMatrixFromDir, Simple) {
  const mathfu::vec3 eye = mathfu::kZeros3f;
  const mathfu::vec3 dir(0, 0.5f, 0.5f);
  const mathfu::vec3 up = mathfu::kAxisY3f;

  mathfu::mat4 look_at_matrix = CalculateLookAtMatrixFromDir(eye, dir, up);
  mathfu::mat4 expected_matrix(-1, 0, 0, 0, 0, kSqrt2 / 2, -(kSqrt2 / 2), 0, 0,
                               -(kSqrt2 / 2), -(kSqrt2 / 2), 0, 0, 0, 0, 1);

  TestMatrixEquality(expected_matrix, look_at_matrix);
}

TEST(CalculateLookAtMatrixFromDir, ZeroDirection) {
  const mathfu::vec3 eye = mathfu::kZeros3f;
  // The direction should never be the zero vector.
  const mathfu::vec3 dir = mathfu::kZeros3f;
  const mathfu::vec3 up = mathfu::kZeros3f;

  mathfu::mat4 loot_at_matrix = CalculateLookAtMatrixFromDir(eye, dir, up);
  // When dealing with invalid values, the return value defaults to the identity
  // matrix.
  mathfu::mat4 expected_matrix = mathfu::mat4::Identity();

  TestMatrixEquality(expected_matrix, loot_at_matrix);
}

TEST(CalculatePerspectiveMatrixFromFrustum, Simple) {
  const float x_left = 0;
  const float x_right = 1;
  const float y_bottom = 0;
  const float y_top = 1;
  const float z_near = 1;
  const float z_far = 2;

  mathfu::mat4 perspective_matrix = CalculatePerspectiveMatrixFromFrustum(
      x_left, x_right, y_bottom, y_top, z_near, z_far);
  mathfu::mat4 expected_matrix(2, 0, 0, 0, 0, 2, 0, 0, 1, 1, -3, -1, 0, 0, -4,
                               0);

  TestMatrixEquality(expected_matrix, perspective_matrix);
}

TEST(CalculatePerspectiveMatrixFromFrustum, ZeroLength) {
  // The dimensions of the frustum should be non-zero in length
  // - otherwise we have an invalid frustum.
  const float x_left = 2;
  const float x_right = 2;
  const float y_bottom = -1;
  const float y_top = 1;
  const float z_near = 1;
  const float z_far = 10;

  mathfu::mat4 perspective_matrix = CalculatePerspectiveMatrixFromFrustum(
      x_left, x_right, y_bottom, y_top, z_near, z_far);
  mathfu::mat4 expected_matrix = mathfu::mat4::Identity();

  TestMatrixEquality(expected_matrix, perspective_matrix);
}

TEST(CalculatePerspectiveMatrixFromView, Simple) {
  const float fovy = 90 * lull::kDegreesToRadians;
  const float aspect = 1.f;
  const float z_near = 1.f;
  const float z_far = 2.f;

  mathfu::mat4 perspective_matrix =
      CalculatePerspectiveMatrixFromView(fovy, aspect, z_near, z_far);
  mathfu::mat4 expected_matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -3, -1, 0, 0, -4,
                               0);

  TestMatrixEquality(expected_matrix, perspective_matrix);
}

TEST(CalculatePerspectiveMatrixFromView, NegativeAngle) {
  // The angle should not be negative.
  const float fovy = -20 * lull::kDegreesToRadians;
  const float aspect = 1.f;
  const float z_near = 1.f;
  const float z_far = 2.f;

  mathfu::mat4 perspective_matrix =
      CalculatePerspectiveMatrixFromView(fovy, aspect, z_near, z_far);
  mathfu::mat4 expected_matrix = mathfu::mat4::Identity();

  TestMatrixEquality(expected_matrix, perspective_matrix);
}

TEST(CalculatePerspectiveMatrixFromView, Rect) {
  const mathfu::rectf fov =
      mathfu::rectf(0.785398f, 0.785398f, 0.785398f, 0.785398f);
  const float z_near = 1.f;
  const float z_far = 2.f;

  mathfu::mat4 perspective_matrix =
      CalculatePerspectiveMatrixFromView(fov, z_near, z_far);
  mathfu::mat4 expected_matrix(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -3, -1, 0, 0, -4,
                               0);

  TestMatrixEquality(expected_matrix, perspective_matrix);
}

TEST(DeformPoint, Simple) {
  const float kRadius = 2;

  // Everything along the xy-plane should map to the y-axis. The value along the
  // y-axis should remain unchanged.
  for (int i = -3; i <= 3; ++i) {
    float x = static_cast<float>(i);
    EXPECT_THAT(DeformPoint(mathfu::vec3(x, 2 * x, 0.0f), kRadius),
                NearMathfu(mathfu::vec3(0.0f, 2 * x, 0.0f), kEpsilon));
  }

  // Everything along the yz-plane axis should map to itself.
  for (int i = -3; i <= 3; ++i) {
    float x = static_cast<float>(i);
    EXPECT_THAT(DeformPoint(mathfu::vec3(0.0f, x, 2 * x), kRadius),
                NearMathfu(mathfu::vec3(0.0f, x, 2 * x), kEpsilon));
  }

  // Everything on the plane at x = +/- radius * pi / 2 should be mapped to the
  // xy-plane at a distance equal to the distance the point was down the z-axis.
  for (int i = -3; i <= 3; ++i) {
    float x = static_cast<float>(i);
    EXPECT_THAT(
        DeformPoint(mathfu::vec3(kRadius * M_PI_float / 2, x, 2 * x), kRadius),
        NearMathfu(mathfu::vec3(-2 * x, x, 0), kEpsilon));
    EXPECT_THAT(
        DeformPoint(mathfu::vec3(-kRadius * M_PI_float / 2, x, 2 * x), kRadius),
        NearMathfu(mathfu::vec3(2 * x, x, 0), kEpsilon));
  }
}

TEST(UndeformPoint, Simple) {
  const float kRadius = 2;
  for (int i = -3; i <= 3; ++i) {
    float x = static_cast<float>(i);
    for (int j = 1; j <= 4; ++j) {
      float z = static_cast<float>(j);
      const mathfu::vec3 point(x, 2 * x, -z);
      EXPECT_THAT(UndeformPoint(DeformPoint(point, kRadius), kRadius),
                  NearMathfu(point, kEpsilon));
    }
  }
}

TEST(AreNearlyEqual, Simple) {
  EXPECT_TRUE(AreNearlyEqual(1.0f, .5f + .49999999f));
  EXPECT_TRUE(AreNearlyEqual(1.0f, 1.0000001f, .000001f));
  EXPECT_FALSE(AreNearlyEqual(1.0f, 1.1f, 0.01f));
  EXPECT_FALSE(AreNearlyEqual(1.0f, -1.0f));
}

TEST(IsNearlyZero, Simple) {
  EXPECT_TRUE(IsNearlyZero(1.0f - .99999999f));
  EXPECT_TRUE(IsNearlyZero(-0.0f));
  EXPECT_TRUE(IsNearlyZero(.00001f, .0001f));
  EXPECT_FALSE(IsNearlyZero(.1f));
  EXPECT_FALSE(IsNearlyZero(.0001f, .00001f));
}

TEST(AreNearlyEqual, Quaternion) {
  const mathfu::quat positive = mathfu::quat::FromEulerAngles(mathfu::kOnes3f);
  EXPECT_TRUE(AreNearlyEqual(positive, positive));
  EXPECT_FALSE(AreNearlyEqual(positive, mathfu::quat::identity));

  const mathfu::quat negative =
      mathfu::quat(-positive.scalar(), -1.f * positive.vector());
  EXPECT_TRUE(AreNearlyEqual(positive, negative));

  const mathfu::quat offset =
      mathfu::quat(positive.scalar() + 0.1f, positive.vector()).Normalized();
  EXPECT_FALSE(AreNearlyEqual(positive, offset));
}

TEST(GetMatrixColumn3D, Simple) {
  const mathfu::mat4 m = mathfu::mat4::Identity();

  mathfu::vec3 col = GetMatrixColumn3D(m, 0);
  EXPECT_NEAR(col[0], 1, kEpsilon);
  EXPECT_NEAR(col[1], 0, kEpsilon);
  EXPECT_NEAR(col[2], 0, kEpsilon);

  col = GetMatrixColumn3D(m, 1);
  EXPECT_NEAR(col[0], 0, kEpsilon);
  EXPECT_NEAR(col[1], 1, kEpsilon);
  EXPECT_NEAR(col[2], 0, kEpsilon);

  col = GetMatrixColumn3D(m, 2);
  EXPECT_NEAR(col[0], 0, kEpsilon);
  EXPECT_NEAR(col[1], 0, kEpsilon);
  EXPECT_NEAR(col[2], 1, kEpsilon);

  col = GetMatrixColumn3D(m, 3);
  EXPECT_NEAR(col[0], 0, kEpsilon);
  EXPECT_NEAR(col[1], 0, kEpsilon);
  EXPECT_NEAR(col[2], 0, kEpsilon);
}

TEST(GetTransformedBoxCorners, Simple) {
  const Aabb aabb(mathfu::vec3(-2.5f, -1.3f, .7f),
                  mathfu::vec3(2.8f, -.8f, 9.2f));
  const mathfu::vec4 offset(45.f, 28.1f, 71.12f, 0);
  mathfu::mat4 m(mathfu::vec4(1, 0, 0, 0), mathfu::vec4(0, 1, 0, 0),
                 mathfu::vec4(0, 0, 1, 0), offset);

  mathfu::vec3 corners[8];
  GetTransformedBoxCorners(aabb, m, corners);

  mathfu::vec3 min = corners[0];
  mathfu::vec3 max = corners[0];
  for (int i = 1; i < 8; ++i) {
    min = mathfu::vec3::Min(min, corners[i]);
    max = mathfu::vec3::Max(max, corners[i]);
  }

  EXPECT_NEAR(min.x, aabb.min.x + offset.x, kEpsilon);
  EXPECT_NEAR(min.y, aabb.min.y + offset.y, kEpsilon);
  EXPECT_NEAR(min.z, aabb.min.z + offset.z, kEpsilon);

  EXPECT_NEAR(max.x, aabb.max.x + offset.x, kEpsilon);
  EXPECT_NEAR(max.y, aabb.max.y + offset.y, kEpsilon);
  EXPECT_NEAR(max.z, aabb.max.z + offset.z, kEpsilon);
}

TEST(TransformAabb, Simple) {
  const Aabb aabb(mathfu::vec3(-2.5f, -1.3f, .7f),
                  mathfu::vec3(2.8f, -.8f, 9.2f));
  const mathfu::vec3 offset(45.f, 28.1f, 71.12f);

  Sqt sqt;
  sqt.translation = offset;
  const Aabb transformed = TransformAabb(sqt, aabb);

  EXPECT_NEAR(transformed.min.x, aabb.min.x + offset.x, kEpsilon);
  EXPECT_NEAR(transformed.min.y, aabb.min.y + offset.y, kEpsilon);
  EXPECT_NEAR(transformed.min.z, aabb.min.z + offset.z, kEpsilon);

  EXPECT_NEAR(transformed.max.x, aabb.max.x + offset.x, kEpsilon);
  EXPECT_NEAR(transformed.max.y, aabb.max.y + offset.y, kEpsilon);
  EXPECT_NEAR(transformed.max.z, aabb.max.z + offset.z, kEpsilon);
}

TEST(MergeAabbs, Simple) {
  const Aabb aabb1(mathfu::vec3(-2.5f, -1.3f, .7f),
                   mathfu::vec3(2.8f, -.8f, 9.2f));
  const Aabb aabb2(mathfu::vec3(-3.7f, -1.0f, 1.7f),
                   mathfu::vec3(2.8f, -5.0f, 11.2f));
  const Aabb merged = MergeAabbs(aabb1, aabb2);
  EXPECT_EQ(merged.min.x, aabb2.min.x);
  EXPECT_EQ(merged.min.y, aabb1.min.y);
  EXPECT_EQ(merged.min.z, aabb1.min.z);

  EXPECT_EQ(merged.max.x, aabb1.max.x);
  EXPECT_EQ(merged.max.y, aabb1.max.y);
  EXPECT_EQ(merged.max.z, aabb2.max.z);
}

TEST(GetBoundingBoxDeathTest, NoDataVec3) {
  PORT_EXPECT_DEATH(GetBoundingBox(nullptr, /* num_points = */ 1), "");
  PORT_EXPECT_DEATH(GetBoundingBox(&mathfu::kZeros3f, /* num_points = */ 0),
                    "");
}

TEST(GetBoundingBox, SimpleVec3) {
  const mathfu::vec3 points[] = {
      mathfu::vec3(0, 0, 5),   mathfu::vec3(1, 2, 0),    mathfu::vec3(0, 8, 2),
      mathfu::vec3(-4, 3, -1), mathfu::vec3(2, -9, -13),
  };
  const int num_points = sizeof(points) / sizeof(points[0]);

  const Aabb box = GetBoundingBox(points, num_points);

  EXPECT_NEAR(box.min.x, -4, kEpsilon);
  EXPECT_NEAR(box.min.y, -9, kEpsilon);
  EXPECT_NEAR(box.min.z, -13, kEpsilon);
  EXPECT_NEAR(box.max.x, 2, kEpsilon);
  EXPECT_NEAR(box.max.y, 8, kEpsilon);
  EXPECT_NEAR(box.max.z, 5, kEpsilon);
}

TEST(GetBoundingBoxDeathTest, NoDataFloats) {
  const float kData[] = {0};
  PORT_EXPECT_DEATH(GetBoundingBox(nullptr, /* len = */ 3, /* stride = */ 3),
                    "");
  PORT_EXPECT_DEATH(GetBoundingBox(kData, /* len = */ 4, /* stride = */ 3),
                    "array size must be a multiple of stride");
  PORT_EXPECT_DEATH(GetBoundingBox(kData, /* len = */ 3, /* stride = */ 1), "");
}

TEST(GetBoundingBox, NotEnoughDataFloats) {
  const float kData[] = {0};
  const Aabb box = GetBoundingBox(kData, /* len = */ 1, /* stride = */ 3);
  EXPECT_EQ(box.min, mathfu::kZeros3f);
  EXPECT_EQ(box.max, mathfu::kZeros3f);
}

TEST(GetBoundingBox, SimpleFloats) {
  const size_t kStride = 5;
  const float kData[] = {0,    0,    5, 100, 200, 1,    2,   0, 300,
                         400,  0,    8, 2,   500, 600,  -4,  3, -1,
                         -100, -200, 2, -9,  -13, -300, -400};
  const size_t kLen = sizeof(kData) / sizeof(kData[0]);

  const Aabb box = GetBoundingBox(kData, kLen, kStride);

  EXPECT_NEAR(box.min.x, -4, kEpsilon);
  EXPECT_NEAR(box.min.y, -9, kEpsilon);
  EXPECT_NEAR(box.min.z, -13, kEpsilon);
  EXPECT_NEAR(box.max.x, 2, kEpsilon);
  EXPECT_NEAR(box.max.y, 8, kEpsilon);
  EXPECT_NEAR(box.max.z, 5, kEpsilon);
}

TEST(CalculateDeterminant3x3, Simple) {
  const float kValues[] = {0, 7, 1, 8, 2, 6, 3, 9, 4, 0, 5, 1, 13, 5, 17, 11};
  const int kNumValues = sizeof(kValues) / sizeof(kValues[0]);
  const int kMatrixDimension = 3;
  const int kMatrixSize = kMatrixDimension * kMatrixDimension;
  mathfu::mat4 mathfu_matrix = mathfu::mat4::Identity();
  const int determinants[3] = {-32, -425, -51};
  for (int i = 0; i < kNumValues; ++i) {
    for (int k = 0; k < kMatrixSize; ++k) {
      const float value = kValues[(i + k) % kMatrixSize];
      const int row = k / kMatrixDimension;
      const int col = k % kMatrixDimension;
      mathfu_matrix(row, col) = value;
    }
    EXPECT_NEAR(determinants[i % 3], CalculateDeterminant3x3(mathfu_matrix),
                kEpsilon);
  }
}

TEST(ProjectHomogeneous, Simple) {
  mathfu::vec4 input(5.0f, 4.0f, 3.0f, 2.0f);
  mathfu::vec3 val = ProjectHomogeneous(input);
  EXPECT_NEAR(val.x, 2.5f, kEpsilon);
  EXPECT_NEAR(val.y, 2.0f, kEpsilon);
  EXPECT_NEAR(val.z, 1.5f, kEpsilon);
}

TEST(ProjectHomogeneous, DivByZero) {
  mathfu::vec4 input(5.0f, 4.0f, 3.0f, 0.0f);
  mathfu::vec3 val = ProjectHomogeneous(input);
  EXPECT_TRUE(std::isinf(val.x));
  EXPECT_TRUE(std::isinf(val.y));
  EXPECT_TRUE(std::isinf(val.z));
}

TEST(Distance, Vec2) {
  const mathfu::vec2 a(0, 10);
  const mathfu::vec2 b(15, 12);
  EXPECT_NEAR(DistanceBetween(a, b), sqrtf(15 * 15 + 2 * 2), kEpsilon);
}

TEST(Distance, Vec3) {
  const mathfu::vec3 a(0, 10, 3);
  const mathfu::vec3 b(15, 12, -4);
  EXPECT_NEAR(DistanceBetween(a, b), sqrtf(15 * 15 + 2 * 2 + 7 * 7), kEpsilon);
}

TEST(GetPitchRadians, Simple) {
  // asinf is pretty low accuracy, so we need to use a larger epsilon here.
  float epsilon = 0.001f;
  EXPECT_NEAR(0.0, GetPitchRadians(mathfu::quat::identity), epsilon);

  const mathfu::quat rotation_60 =
      mathfu::quat::FromAngleAxis(M_PI_float / 3, mathfu::kAxisX3f);
  EXPECT_NEAR(M_PI_float / 3, GetPitchRadians(rotation_60), epsilon);

  const mathfu::quat rotation_neg90 =
      mathfu::quat::FromAngleAxis(-0.5f * M_PI_float, mathfu::kAxisX3f);
  EXPECT_NEAR(-0.5f * M_PI_float, GetPitchRadians(rotation_neg90), epsilon);

  // Do a more complex rotation of turning left 90 degrees and looking up.
  const mathfu::quat rotation_90 =
      mathfu::quat::RotateFromTo(-mathfu::kAxisZ3f, mathfu::vec3(-1, 1, 0));
  EXPECT_NEAR(0.25f * M_PI_float, GetPitchRadians(rotation_90), epsilon);
}

TEST(GetHeadingRadians, Simple) {
  EXPECT_NEAR(0.0, GetHeadingRadians(mathfu::quat::identity), kEpsilon);

  const mathfu::quat rotation_60 =
      mathfu::quat::FromAngleAxis(M_PI_float / 3, mathfu::kAxisY3f);
  EXPECT_NEAR(M_PI_float / 3, GetHeadingRadians(rotation_60), kEpsilon);

  const mathfu::quat rotation_neg90 =
      mathfu::quat::FromAngleAxis(-0.5f * M_PI_float, mathfu::kAxisY3f);
  EXPECT_NEAR(-0.5f * M_PI_float, GetHeadingRadians(rotation_neg90), kEpsilon);

  // Do a more complex rotation of turning left 90 degrees and looking up.
  const mathfu::quat rotation_90 =
      mathfu::quat::RotateFromTo(-mathfu::kAxisZ3f, mathfu::vec3(-1, 1, 0));
  EXPECT_NEAR(0.5f * M_PI_float, GetHeadingRadians(rotation_90), kEpsilon);
}

TEST(GetHeadingRadians, Singularities) {
  // Rotate the gaze up by nearly 90 degrees. Heading should still be 0.
  const mathfu::quat almost_up =
      mathfu::quat::FromAngleAxis(0.4999f * M_PI_float, mathfu::kAxisX3f);
  EXPECT_NEAR(0.0, GetHeadingRadians(almost_up), kEpsilon);

  // Rotate the gaze down by nearly 90 degrees. Heading should still be 0.
  const mathfu::quat almost_down =
      mathfu::quat::FromAngleAxis(-0.4999f * M_PI_float, mathfu::kAxisX3f);
  EXPECT_NEAR(0.0, GetHeadingRadians(almost_down), kEpsilon);
}

TEST(GetHeading, Simple) {
  const mathfu::quat rotation_90 =
      mathfu::quat::RotateFromTo(-mathfu::kAxisZ3f, mathfu::vec3(-1, 1, 0));
  const Sqt heading_sqt =
      GetHeading(Sqt(mathfu::kZeros3f, rotation_90, mathfu::kOnes3f));
  float angle;
  mathfu::vec3 axis;
  heading_sqt.rotation.ToAngleAxis(&angle, &axis);
  EXPECT_NEAR(0.0, DistanceBetween(axis, mathfu::kAxisY3f), kEpsilon);
  EXPECT_NEAR(0.5f * M_PI_float, angle, kEpsilon);
}

TEST(ProjectPositionToVicinity, Inside) {
  const mathfu::vec3 pos(1.f, 2.f, 3.f);
  const mathfu::vec3 target(2.f, 4.f, 6.f);
  const float max_offset = 4.f;
  const mathfu::vec3 res = ProjectPositionToVicinity(pos, target, max_offset);
  EXPECT_NEAR(pos[0], res[0], kEpsilon);
  EXPECT_NEAR(pos[1], res[1], kEpsilon);
  EXPECT_NEAR(pos[2], res[2], kEpsilon);
}

TEST(ProjectPositionToVicinity, Outside) {
  const mathfu::vec3 pos(1.f, 2.f, 3.f);
  const mathfu::vec3 target(2.f, 4.f, 5.f);
  const float max_offset = 1.f;
  const mathfu::vec3 res = ProjectPositionToVicinity(pos, target, max_offset);
  EXPECT_NEAR(1.6666666f, res[0], kEpsilon);
  EXPECT_NEAR(3.3333333f, res[1], kEpsilon);
  EXPECT_NEAR(4.3333333f, res[2], kEpsilon);
}

TEST(ProjectPositionToVicinity, ZeroOffset) {
  const mathfu::vec3 pos(1.f, 2.f, 3.f);
  const mathfu::vec3 target(2.f, 4.f, 6.f);
  const float max_offset = 0.0f;
  const mathfu::vec3 res = ProjectPositionToVicinity(pos, target, max_offset);
  EXPECT_EQ(res.x, target.x);
  EXPECT_EQ(res.y, target.y);
  EXPECT_EQ(res.z, target.z);
}

TEST(ProjectRotationToVicinity, Inside) {
  const mathfu::quat rot =
      mathfu::quat::FromAngleAxis(10.f * kDegreesToRadians, mathfu::kAxisZ3f);
  const mathfu::quat target =
      rot *
      mathfu::quat::FromAngleAxis(60.f * kDegreesToRadians, mathfu::kAxisY3f);

  const float max_offset_rad = 75.f * kDegreesToRadians;
  const mathfu::quat res =
      ProjectRotationToVicinity(rot, target, max_offset_rad);
  const mathfu::vec3 res_angles_deg = res.ToEulerAngles() * kRadiansToDegrees;
  EXPECT_NEAR(0.f, res_angles_deg[0], 1e-4);
  EXPECT_NEAR(0.f, res_angles_deg[1], 1e-4);
  EXPECT_NEAR(10.f, res_angles_deg[2], 1e-4);
}

TEST(ProjectRotationToVicinity, Outside) {
  const mathfu::quat rot =
      mathfu::quat::FromAngleAxis(10.f * kDegreesToRadians, mathfu::kAxisZ3f);
  const mathfu::quat target =
      rot *
      mathfu::quat::FromAngleAxis(60.f * kDegreesToRadians, mathfu::kAxisY3f);

  const float max_offset_rad = 15.f * kDegreesToRadians;
  const mathfu::quat res =
      ProjectRotationToVicinity(rot, target, max_offset_rad);
  const mathfu::vec3 res_angles_deg = res.ToEulerAngles() * kRadiansToDegrees;
  EXPECT_NEAR(0.f, res_angles_deg[0], 1e-4);
  EXPECT_NEAR(45.f, res_angles_deg[1], 1e-4);
  EXPECT_NEAR(10.f, res_angles_deg[2], 1e-4);
}

TEST(ProjectRotationToVicinity, ZeroOffset) {
  const mathfu::quat rot =
      mathfu::quat::FromAngleAxis(10.f * kDegreesToRadians, mathfu::kAxisZ3f);
  const mathfu::quat target =
      rot *
      mathfu::quat::FromAngleAxis(60.f * kDegreesToRadians, mathfu::kAxisY3f);

  const float max_offset = 0.0f;
  const mathfu::quat res = ProjectRotationToVicinity(rot, target, max_offset);
  EXPECT_EQ(res[0], target[0]);
  EXPECT_EQ(res[1], target[1]);
  EXPECT_EQ(res[2], target[2]);
  EXPECT_EQ(res[3], target[3]);
}

TEST(DampedDriveEase, Simple) {
  EXPECT_EQ(0, DampedDriveEase(-0.1f));
  EXPECT_EQ(1, DampedDriveEase(1.1f));
  EXPECT_NEAR(0.601893, DampedDriveEase(0.1f), kEpsilon);
  EXPECT_NEAR(0.99, DampedDriveEase(0.5f), kEpsilon);
  EXPECT_NEAR(0.999749, DampedDriveEase(0.9f), kEpsilon);
}

TEST(Streams, Simple) {
  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    os << mathfu::vec2(1, 2);
    EXPECT_EQ("(1, 2)", buffer.str());
  }

  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    os << mathfu::vec3(1, 2, 3);
    EXPECT_EQ("(1, 2, 3)", buffer.str());
  }

  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    os << mathfu::vec4(1, 2, 3, 4);
    EXPECT_EQ("(1, 2, 3, 4)", buffer.str());
  }

  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    os << mathfu::vec2i(1, 2);
    EXPECT_EQ("(1, 2)", buffer.str());
  }

  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    os << mathfu::vec3i(1, 2, 3);
    EXPECT_EQ("(1, 2, 3)", buffer.str());
  }

  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    os << mathfu::vec4i(1, 2, 3, 4);
    EXPECT_EQ("(1, 2, 3, 4)", buffer.str());
  }
  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    mathfu::vec3 a_eulers = mathfu::vec3(30, 45, 90) * kDegreesToRadians;
    os << mathfu::quat::FromEulerAngles(a_eulers).ToEulerAngles();
    EXPECT_EQ("(0.523599, 0.785398, 1.5708)", buffer.str());
  }

  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    os << mathfu::mat4(0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32,
                       33);
    EXPECT_EQ("(0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 30, 31, 32, 33)",
              buffer.str());
  }

  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    os << Ray(mathfu::vec3(1, 2, 3), mathfu::vec3(4, 5, 6));
    EXPECT_EQ("Ray: dir(4, 5, 6) orig(1, 2, 3)", buffer.str());
  }

  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    os << Sqt(mathfu::vec3(1, 2, 3), mathfu::quat::identity,
              mathfu::vec3(4, 5, 6));
    EXPECT_EQ("Sqt: S(4, 5, 6) Q(0, -0, 0) T(1, 2, 3)", buffer.str());
  }

  {
    std::stringbuf buffer;
    std::ostream os(&buffer);
    os << Aabb(mathfu::vec3(1, 2, 3), mathfu::vec3(4, 5, 6));
    EXPECT_EQ("Aabb: min(1, 2, 3) max(4, 5, 6)", buffer.str());
  }
}

TEST(IsPowerOf2, Simple) {
  EXPECT_FALSE(IsPowerOf2(0));
  EXPECT_TRUE(IsPowerOf2(1));
  EXPECT_TRUE(IsPowerOf2(2));

  for (uint32_t i = 2; i < 32; ++i) {
    const uint32_t n = 1 << i;
    EXPECT_TRUE(IsPowerOf2(n));
    EXPECT_FALSE(IsPowerOf2(n + 1));
    EXPECT_FALSE(IsPowerOf2(n - 1));
  }
}

TEST(AlignToPowerOf2, Simple) {
  // Step up through some powers of 2 and perform tests around them.
  const uint32_t kMaxExponent = 8;
  for (uint32_t a = 0; a < kMaxExponent; ++a) {
    const uint32_t lower_pow2 = 1 << a;
    EXPECT_EQ(AlignToPowerOf2(lower_pow2, lower_pow2), lower_pow2);
    if (lower_pow2 != 1) {
      EXPECT_EQ(AlignToPowerOf2(lower_pow2 - 1, lower_pow2), lower_pow2);
    }
    EXPECT_EQ(AlignToPowerOf2(lower_pow2 + 1, lower_pow2), 2 * lower_pow2);

    for (uint32_t b = a + 1; b < kMaxExponent; ++b) {
      const uint32_t higher_pow2 = 1 << b;

      EXPECT_EQ(AlignToPowerOf2(lower_pow2, higher_pow2), higher_pow2);
      EXPECT_EQ(AlignToPowerOf2(higher_pow2, lower_pow2), higher_pow2);
    }
  }
}

TEST(NegativeZAxisRay, Simple) {
  Sqt identity;
  const Ray identity_ray = NegativeZAxisRay(identity);
  EXPECT_NEAR(identity_ray.origin.x, 0, kEpsilon);
  EXPECT_NEAR(identity_ray.origin.y, 0, kEpsilon);
  EXPECT_NEAR(identity_ray.origin.z, 0, kEpsilon);

  EXPECT_NEAR(identity_ray.direction.x, 0, kEpsilon);
  EXPECT_NEAR(identity_ray.direction.y, 0, kEpsilon);
  EXPECT_NEAR(identity_ray.direction.z, -1, kEpsilon);
}

TEST(CosAngleFromRay, Simple) {
  Ray test_ray(mathfu::kZeros3f, mathfu::kAxisZ3f);

  // Orthogonal should give 0.
  EXPECT_NEAR(CosAngleFromRay(test_ray, mathfu::kAxisZ3f), 1, kEpsilon)
      << "In front of the ray should give 1.";
  EXPECT_NEAR(CosAngleFromRay(test_ray, 3.2f * mathfu::kAxisZ3f), 1, kEpsilon)
      << "Point scale shouldn't matter.";
  EXPECT_NEAR(CosAngleFromRay(test_ray, mathfu::kAxisX3f), 0, kEpsilon)
      << "Orthogonal to the ray should give 0.";
  EXPECT_NEAR(CosAngleFromRay(test_ray, -mathfu::kAxisZ3f), -1, kEpsilon)
      << "Behind the ray should give -1.";
}

TEST(ProjectPointOntoRay, Simple) {
  Ray z_ray(mathfu::kZeros3f, mathfu::kAxisZ3f);

  mathfu::vec3 test_point(1.f, 2.f, 3.f);
  EXPECT_NEAR(ProjectPointOntoRay(z_ray, test_point), 3.f, kEpsilon)
      << "Projecting onto z-axis should give z coordinate";

  Ray xy_ray(mathfu::kZeros3f, mathfu::vec3(1.f, 1.f, 0.f));
  EXPECT_NEAR(ProjectPointOntoRay(xy_ray, test_point), 3.f / kSqrt2, kEpsilon)
      << "Expected to get the sum of the x&y coordinates divided by the length "
         "of the direction vector";
}

TEST(CalculateViewFrustum, Simple) {
  const float fovy = .5f * kPi;
  const float aspect = 1.0f;
  const float z_near = 1.0f;
  const float z_far = 10.0f;
  const mathfu::mat4 clip_from_world_matrix =
      CalculatePerspectiveMatrixFromView(fovy, aspect, z_near, z_far);

  mathfu::vec4 planes[kNumFrustumPlanes];
  CalculateViewFrustum(clip_from_world_matrix, planes);

  const float half_sqrt_2 = .5f * std::sqrt(2.f);
  EXPECT_THAT(
      planes[kRightFrustumPlane].xyz(),
      NearMathfu(mathfu::vec3(-half_sqrt_2, 0, -half_sqrt_2), kEpsilon));
  EXPECT_THAT(planes[kLeftFrustumPlane].xyz(),
              NearMathfu(mathfu::vec3(half_sqrt_2, 0, -half_sqrt_2), kEpsilon));
  EXPECT_THAT(planes[kBottomFrustumPlane].xyz(),
              NearMathfu(mathfu::vec3(0, half_sqrt_2, -half_sqrt_2), kEpsilon));
  EXPECT_THAT(
      planes[kTopFrustumPlane].xyz(),
      NearMathfu(mathfu::vec3(0, -half_sqrt_2, -half_sqrt_2), kEpsilon));
  EXPECT_THAT(planes[kFarFrustumPlane].xyz(),
              NearMathfu(mathfu::vec3(0, 0, 1.f), kEpsilon));
  EXPECT_THAT(planes[kNearFrustumPlane].xyz(),
              NearMathfu(mathfu::vec3(0, 0, -1.f), kEpsilon));

  EXPECT_NEAR(planes[kRightFrustumPlane].w, 0.f, kEpsilon);
  EXPECT_NEAR(planes[kLeftFrustumPlane].w, 0.f, kEpsilon);
  EXPECT_NEAR(planes[kBottomFrustumPlane].w, 0.f, kEpsilon);
  EXPECT_NEAR(planes[kTopFrustumPlane].w, 0.f, kEpsilon);
  EXPECT_NEAR(planes[kFarFrustumPlane].w, z_far, kEpsilon);
  EXPECT_NEAR(planes[kNearFrustumPlane].w, -z_near, kEpsilon);
}

TEST(CheckSphereInFrustum, Inside) {
  const float fovy = .5f * kPi;
  const float aspect = 1.0f;
  const float z_near = 1.0f;
  const float z_far = 10.0f;
  const mathfu::mat4 clip_from_world_matrix =
      CalculatePerspectiveMatrixFromView(fovy, aspect, z_near, z_far);

  mathfu::vec4 planes[kNumFrustumPlanes];
  CalculateViewFrustum(clip_from_world_matrix, planes);

  const bool contained =
      CheckSphereInFrustum(mathfu::vec3(0, 0, -5.f), 1.0f, planes);
  EXPECT_TRUE(contained);

  const bool containing =
      CheckSphereInFrustum(mathfu::kZeros3f, 100.0f, planes);
  EXPECT_TRUE(containing);

  const bool right_side =
      CheckSphereInFrustum(mathfu::vec3(5.5f, 0, -5.f), 1.0f, planes);
  EXPECT_TRUE(right_side);

  const bool left_side =
      CheckSphereInFrustum(mathfu::vec3(-5.5f, 0, -5.f), 1.0f, planes);
  EXPECT_TRUE(left_side);

  const bool bottom_side =
      CheckSphereInFrustum(mathfu::vec3(0, -5.5f, -5.f), 1.0f, planes);
  EXPECT_TRUE(bottom_side);

  const bool top_side =
      CheckSphereInFrustum(mathfu::vec3(0, 5.5f, -5.f), 1.0f, planes);
  EXPECT_TRUE(top_side);

  const bool far_side =
      CheckSphereInFrustum(mathfu::vec3(0, 0, -10.5f), 1.0f, planes);
  EXPECT_TRUE(far_side);

  const bool near_side =
      CheckSphereInFrustum(mathfu::vec3(0, 0, -0.95f), 0.06f, planes);
  EXPECT_TRUE(near_side);
}

TEST(CheckSphereInFrustum, Outside) {
  const float fovy = .5f * kPi;
  const float aspect = 1.0f;
  const float z_near = 1.0f;
  const float z_far = 10.0f;
  const mathfu::mat4 clip_from_world_matrix =
      CalculatePerspectiveMatrixFromView(fovy, aspect, z_near, z_far);

  mathfu::vec4 planes[kNumFrustumPlanes];
  CalculateViewFrustum(clip_from_world_matrix, planes);

  const bool right_side =
      CheckSphereInFrustum(mathfu::vec3(6.5f, 0, -5.f), 1.0f, planes);
  EXPECT_FALSE(right_side);

  const bool left_side =
      CheckSphereInFrustum(mathfu::vec3(-6.5f, 0, -5.f), 1.0f, planes);
  EXPECT_FALSE(left_side);

  const bool bottom_side =
      CheckSphereInFrustum(mathfu::vec3(0, -6.5f, -5.f), 1.0f, planes);
  EXPECT_FALSE(bottom_side);

  const bool top_side =
      CheckSphereInFrustum(mathfu::vec3(0, 6.5f, -5.f), 1.0f, planes);
  EXPECT_FALSE(top_side);

  const bool far_side =
      CheckSphereInFrustum(mathfu::vec3(0, 0, -11.5f), 1.0f, planes);
  EXPECT_FALSE(far_side);

  const bool near_side =
      CheckSphereInFrustum(mathfu::vec3(0, 0, -0.9f), 0.06f, planes);
  EXPECT_FALSE(near_side);
}

TEST(FindPositionBetweenPoints, Edges) {
  const std::vector<float> points{-2, -1, 0, 1, 2};
  size_t min_index;
  size_t max_index;
  float match_percent;

  // Should be clamped to the lowest point when below the min
  FindPositionBetweenPoints(-100.f, points, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 0);
  EXPECT_EQ((int)max_index, 0);
  EXPECT_NEAR(match_percent, 1.0f, kEpsilon);

  // Should be clamped to the highest point when above the max
  FindPositionBetweenPoints(100.f, points, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 4);
  EXPECT_EQ((int)max_index, 4);
  EXPECT_NEAR(match_percent, 1.0f, kEpsilon);

  // Exact edges work as expected
  FindPositionBetweenPoints(-2.0f, points, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 0);
  EXPECT_EQ((int)max_index, 0);
  EXPECT_NEAR(match_percent, 1.0f, kEpsilon);
  FindPositionBetweenPoints(2.0f, points, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 3);
  EXPECT_EQ((int)max_index, 4);
  EXPECT_NEAR(match_percent, 1.0f, kEpsilon);
}

TEST(FindPositionBetweenPoints, Simple) {
  const std::vector<float> points{-2, -1, 0, 1, 2};
  size_t min_index;
  size_t max_index;
  float match_percent;

  // Test positions found inbetween defined points
  FindPositionBetweenPoints(0.5f, points, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 2);
  EXPECT_EQ((int)max_index, 3);
  EXPECT_NEAR(match_percent, 0.5f, kEpsilon);
  FindPositionBetweenPoints(0.75f, points, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 2);
  EXPECT_EQ((int)max_index, 3);
  EXPECT_NEAR(match_percent, 0.75f, kEpsilon);
  FindPositionBetweenPoints(0.789245f, points, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 2);
  EXPECT_EQ((int)max_index, 3);
  EXPECT_NEAR(match_percent, 0.789245f, kEpsilon);

  // Test exact matches
  FindPositionBetweenPoints(-1, points, &min_index, &max_index, &match_percent);
  EXPECT_EQ((int)min_index, 0);
  EXPECT_EQ((int)max_index, 1);
  EXPECT_NEAR(match_percent, 1.f, kEpsilon);
  FindPositionBetweenPoints(1, points, &min_index, &max_index, &match_percent);
  EXPECT_EQ((int)min_index, 2);
  EXPECT_EQ((int)max_index, 3);
  EXPECT_NEAR(match_percent, 1.f, kEpsilon);
}

TEST(FindPositionBetweenPoints, Overlapping) {
  const std::vector<float> points_1{-2, -1, -1, 1, 2};
  const std::vector<float> points_2{-2, -2, 0, 1, 2};
  size_t min_index;
  size_t max_index;
  float match_percent;

  // Test that overlapping points doesn't crash
  FindPositionBetweenPoints(-1, points_1, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 0);
  EXPECT_EQ((int)max_index, 1);
  EXPECT_NEAR(match_percent, 1.f, kEpsilon);
  FindPositionBetweenPoints(-2, points_2, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 0);
  EXPECT_EQ((int)max_index, 0);
  EXPECT_NEAR(match_percent, 1.f, kEpsilon);
}

TEST(FindPositionBetweenPoints, Single) {
  const std::vector<float> points{1};
  size_t min_index;
  size_t max_index;
  float match_percent;

  // Test that only having 1 point doesn't crash
  FindPositionBetweenPoints(-1, points, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 0);
  EXPECT_EQ((int)max_index, 0);
  EXPECT_NEAR(match_percent, 1.f, kEpsilon);
  FindPositionBetweenPoints(1, points, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 0);
  EXPECT_EQ((int)max_index, 0);
  EXPECT_NEAR(match_percent, 1.f, kEpsilon);
  FindPositionBetweenPoints(2, points, &min_index, &max_index,
                            &match_percent);
  EXPECT_EQ((int)min_index, 0);
  EXPECT_EQ((int)max_index, 0);
  EXPECT_NEAR(match_percent, 1.f, kEpsilon);
}

TEST(CheckPercentageOfLineClosestToPoint, Middle) {
  const mathfu::vec3 start_vector(0.0f, 0.0f, 0.0f);
  const mathfu::vec3 end_vector(0.0f, 0.0f, 1.0f);
  const mathfu::vec3 test_vector(0.0f, 1.0f, 0.5f);
  const float percentage =
      GetPercentageOfLineClosestToPoint(start_vector, end_vector, test_vector);
  EXPECT_NEAR(percentage, 0.5f, kEpsilon);
}

TEST(CheckPercentageOfLineClosestToPoint, ThreeAxis) {
  const mathfu::vec3 start_vector(0.0f, 1.0f / std::sqrt(2.0f), 0.0f);
  const mathfu::vec3 end_vector(1.0f / std::sqrt(2.0f), 0.0f, 0.0f);
  const mathfu::vec3 test_vector(0.0f, 0.0f, 0.0f);
  const float percentage =
      GetPercentageOfLineClosestToPoint(start_vector, end_vector, test_vector);
  EXPECT_NEAR(percentage, 0.5f, kEpsilon);
}

TEST(CheckPercentageOfLineClosestToPoint, BeforeStart) {
  const mathfu::vec3 start_vector(0.0f, 0.0f, 0.0f);
  const mathfu::vec3 end_vector(0.0f, 0.0f, 2.0f);
  const mathfu::vec3 test_vector(1.0f, 1.0f, -0.5f);
  const float percentage =
      GetPercentageOfLineClosestToPoint(start_vector, end_vector, test_vector);
  std::cout << "percentage " << percentage << std::endl;
  EXPECT_NEAR(percentage, -0.25f, kEpsilon);
}

TEST(CheckPercentageOfLineClosestToPoint, AfterStart) {
  const mathfu::vec3 start_vector(0.0f, 0.0f, 0.0f);
  const mathfu::vec3 end_vector(0.0f, 0.0f, 2.0f);
  const mathfu::vec3 test_vector(1.0f, 1.0f, 2.5f);
  const float percentage =
      GetPercentageOfLineClosestToPoint(start_vector, end_vector, test_vector);
  std::cout << "percentage " << percentage << std::endl;
  EXPECT_NEAR(percentage, 1.25f, kEpsilon);
}

TEST(ComputeNormalMatrix, SimpleIdentity) {
  const mathfu::vec3 z_vector(0.0f, 0.0f, 1.0f);

  const mathfu::mat4 mat_identity = mathfu::mat4::Identity();
  const mathfu::mat3 mat_normal = ComputeNormalMatrix(mat_identity);

  EXPECT_THAT(mat_normal * z_vector, NearMathfu(z_vector, kEpsilon));
}

TEST(ComputeNormalMatrix, SimpleRotation) {
  const mathfu::vec3 z_vector(0.0f, 0.0f, 1.0f);

  const mathfu::mat4 mat_rotation_x =
      mathfu::mat4::FromRotationMatrix(mathfu::mat3::RotationX(M_PI_float));
  const mathfu::mat3 mat_normal_x = ComputeNormalMatrix(mat_rotation_x);
  EXPECT_THAT(mat_normal_x * z_vector, NearMathfu(-z_vector, kEpsilon));

  const mathfu::mat4 mat_rotation_y =
      mathfu::mat4::FromRotationMatrix(mathfu::mat3::RotationY(M_PI_float));
  const mathfu::mat3 mat_normal_y = ComputeNormalMatrix(mat_rotation_y);
  EXPECT_THAT(mat_normal_y * z_vector, NearMathfu(-z_vector, kEpsilon));

  const mathfu::mat4 mat_rotation_z =
      mathfu::mat4::FromRotationMatrix(mathfu::mat3::RotationZ(M_PI_float));
  const mathfu::mat3 mat_normal_z = ComputeNormalMatrix(mat_rotation_z);
  EXPECT_THAT(mat_normal_z * z_vector, NearMathfu(z_vector, kEpsilon));
}

TEST(ComputeNormalMatrix, UniformScaledRotation) {
  const mathfu::vec3 z_vector(0.0f, 0.0f, 1.0f);

  const mathfu::mat4 mat_rotation_x =
      mathfu::mat4::FromRotationMatrix(mathfu::mat3::RotationX(M_PI_float)) *
      mathfu::mat4::FromScaleVector(mathfu::vec3(2.0f, 2.0f, 2.0f));
  const mathfu::mat3 mat_normal_x = ComputeNormalMatrix(mat_rotation_x);
  EXPECT_THAT((mat_normal_x * z_vector).Normalized(),
              NearMathfu(-z_vector, kEpsilon));

  const mathfu::mat4 mat_rotation_y =
      mathfu::mat4::FromRotationMatrix(mathfu::mat3::RotationY(M_PI_float)) *
      mathfu::mat4::FromScaleVector(mathfu::vec3(3.5f, 3.5f, 3.5f));
  const mathfu::mat3 mat_normal_y = ComputeNormalMatrix(mat_rotation_y);
  EXPECT_THAT((mat_normal_y * z_vector).Normalized(),
              NearMathfu(-z_vector, kEpsilon));

  const mathfu::mat4 mat_rotation_z =
      mathfu::mat4::FromRotationMatrix(mathfu::mat3::RotationZ(M_PI_float)) *
      mathfu::mat4::FromScaleVector(mathfu::vec3(25.3f, 25.3f, 25.3f));
  const mathfu::mat3 mat_normal_z = ComputeNormalMatrix(mat_rotation_z);
  EXPECT_THAT((mat_normal_z * z_vector).Normalized(),
              NearMathfu(z_vector, kEpsilon));
}

TEST(ComputeNormalMatrix, NonUniformScaledRotation) {
  const mathfu::vec3 z_vector(0.0f, 0.0f, 1.0f);

  const mathfu::mat4 mat_rotation_x =
      mathfu::mat4::FromRotationMatrix(mathfu::mat3::RotationX(M_PI_float)) *
      mathfu::mat4::FromScaleVector(mathfu::vec3(2.0f, 5.0f, 2.0f));
  const mathfu::mat3 mat_normal_x = ComputeNormalMatrix(mat_rotation_x);
  EXPECT_THAT((mat_normal_x * z_vector).Normalized(),
              NearMathfu(-z_vector, kEpsilon));

  const mathfu::mat4 mat_rotation_y =
      mathfu::mat4::FromRotationMatrix(mathfu::mat3::RotationY(M_PI_float)) *
      mathfu::mat4::FromScaleVector(mathfu::vec3(13.5f, 3.5f, 3.5f));
  const mathfu::mat3 mat_normal_y = ComputeNormalMatrix(mat_rotation_y);
  EXPECT_THAT((mat_normal_y * z_vector).Normalized(),
              NearMathfu(-z_vector, kEpsilon));

  const mathfu::mat4 mat_rotation_z =
      mathfu::mat4::FromRotationMatrix(mathfu::mat3::RotationZ(M_PI_float)) *
      mathfu::mat4::FromScaleVector(mathfu::vec3(25.3f, 25.3f, 5.3f));
  const mathfu::mat3 mat_normal_z = ComputeNormalMatrix(mat_rotation_z);
  EXPECT_THAT((mat_normal_z * z_vector).Normalized(),
              NearMathfu(z_vector, kEpsilon));
}

TEST(CalculateCameraDirection, Simple) {
  EXPECT_THAT(CalculateCameraDirection(mathfu::mat4::Identity()),
              NearMathfu(mathfu::vec3(0.0f, 0.0f, -1.0f), kEpsilon));

  const mathfu::mat4 mat_rotation_y =
      mathfu::mat4::FromRotationMatrix(mathfu::mat3::RotationY(M_PI_float));
  EXPECT_THAT(CalculateCameraDirection(mat_rotation_y),
              NearMathfu(mathfu::vec3(0.0f, 0.0f, 1.0f), kEpsilon));
}

TEST(CalculateCameraDirection, LookAtSimple) {
  const mathfu::mat4 mat_eye_0 = mathfu::mat4::LookAt(
      mathfu::vec3(0.0f, 0.0f, 1.0f), mathfu::vec3(0.0f, 0.0f, 0.0f),
      mathfu::kAxisY3f, 1.0f);
  EXPECT_THAT(CalculateCameraDirection(mat_eye_0),
              NearMathfu(mathfu::vec3(0.0f, 0.0f, 1.0f), kEpsilon));

  const mathfu::mat4 mat_eye_1 = mathfu::mat4::LookAt(
      mathfu::vec3(0.0f, 0.0f, -1.0f), mathfu::vec3(0.0f, 0.0f, 0.0f),
      mathfu::kAxisY3f, 1.0f);
  EXPECT_THAT(CalculateCameraDirection(mat_eye_1),
              NearMathfu(mathfu::vec3(0.0f, 0.0f, -1.0f), kEpsilon));

  const mathfu::mat4 mat_eye_2 = mathfu::mat4::LookAt(
      mathfu::vec3(1.0f, 0.0f, 0.0f), mathfu::vec3(0.0f, 0.0f, 0.0f),
      mathfu::kAxisY3f, 1.0f);
  EXPECT_THAT(CalculateCameraDirection(mat_eye_2),
              NearMathfu(mathfu::vec3(-1.0f, 0.0f, 0.0f), kEpsilon));
}

TEST(CalculateCameraDirection, LookAt) {
  const mathfu::mat4 mat_eye_0 = mathfu::mat4::LookAt(
      mathfu::vec3(5.0f, 0.0f, 5.0f), mathfu::vec3(2.0f, 0.0f, 2.0f),
      mathfu::kAxisY3f, 1.0f);
  EXPECT_THAT(CalculateCameraDirection(mat_eye_0),
              NearMathfu(mathfu::vec3(-0.707107f, 0.0f, 0.707107f), kEpsilon));
}

}  // namespace
}  // namespace lull
