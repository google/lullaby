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
#include "redux/modules/math/bounds.h"

namespace redux {
namespace {

using ::testing::Eq;

using BoundsTestTypes = ::testing::Types<Bounds1i, Bounds2i, Bounds3i, Bounds1f,
                                         Bounds2f, Bounds3f>;

template <typename T>
class BoundsTest : public ::testing::Test {};

TYPED_TEST_SUITE(BoundsTest, BoundsTestTypes);

template <typename Scalar>
auto RandScalar() {
  auto rand_value = (rand() / static_cast<Scalar>(RAND_MAX) * 100.0);
  return Scalar(1 + rand_value);
}

void RandInit(int& v) { v = RandScalar<int>(); }

void RandInit(vec2i& v) {
  v.x = RandScalar<int>();
  v.y = RandScalar<int>();
}

void RandInit(vec3i& v) {
  v.x = RandScalar<int>();
  v.y = RandScalar<int>();
  v.z = RandScalar<int>();
}

void RandInit(float& v) { v = RandScalar<float>(); }

void RandInit(vec2& v) {
  v.x = RandScalar<float>();
  v.y = RandScalar<float>();
}

void RandInit(vec3& v) {
  v.x = RandScalar<float>();
  v.y = RandScalar<float>();
  v.z = RandScalar<float>();
}

template <typename T>
auto RandPoint() {
  T t;
  RandInit(t);
  return t;
}

TYPED_TEST(BoundsTest, FromOnePoint) {
  using Point = typename TypeParam::Point;
  const Point p = RandPoint<Point>();
  const TypeParam bounds(p);
  EXPECT_THAT(bounds.min, Eq(p));
  EXPECT_THAT(bounds.max, Eq(p));
}

TYPED_TEST(BoundsTest, FromMinMax) {
  using Point = typename TypeParam::Point;
  const Point p1 = RandPoint<Point>();
  const Point p2 = p1 + Point(10);
  const TypeParam bounds(p1, p2);
  EXPECT_THAT(bounds.min, Eq(p1));
  EXPECT_THAT(bounds.max, Eq(p2));
}

TYPED_TEST(BoundsTest, FromManyPoints) {
  using Point = typename TypeParam::Point;
  const Point p1 = RandPoint<Point>();
  const Point points[] = {p1 + Point(1), p1, p1 + Point(3), p1 + Point(2)};
  const TypeParam bounds(points);
  EXPECT_THAT(bounds.min, Eq(points[1]));
  EXPECT_THAT(bounds.max, Eq(points[2]));
}

TYPED_TEST(BoundsTest, Size) {
  using Point = typename TypeParam::Point;
  const Point size(10);
  const Point p1 = RandPoint<Point>();
  const Point p2 = p1 + size;
  const TypeParam bounds(p1, p2);
  EXPECT_TRUE(AreNearlyEqual(bounds.Size(), size));
}

TYPED_TEST(BoundsTest, Center) {
  using Point = typename TypeParam::Point;
  using Scalar = typename TypeParam::Scalar;
  const Point size(10);
  const Point p1 = RandPoint<Point>();
  const Point p2 = p1 + size;
  const TypeParam bounds1(p1, p2);
  EXPECT_THAT(bounds1.Center(), Eq((p1 + p2) / Scalar(2)));

  const TypeParam bounds2(p1, -p1);
  EXPECT_THAT(bounds2.Center(), Eq(Point(Scalar(0))));
}

TYPED_TEST(BoundsTest, Contains) {
  using Point = typename TypeParam::Point;
  const Point p1 = RandPoint<Point>();
  const Point p2 = p1 + Point(10);
  const TypeParam bounds(p1, p2);
  EXPECT_TRUE(bounds.Contains(p1 + Point(5)));
  EXPECT_FALSE(bounds.Contains(p1 + Point(15)));
}

TYPED_TEST(BoundsTest, Scaled) {
  // Bounds Scaled(Scalar percent) const;
}

TYPED_TEST(BoundsTest, Included) {
  using Point = typename TypeParam::Point;
  const Point p1 = RandPoint<Point>();
  TypeParam bounds(p1 + Point(5), p1 + Point(10));
  EXPECT_FALSE(bounds.Contains(p1));
  EXPECT_FALSE(bounds.Contains(p1 + Point(15)));
  bounds = bounds.Included(p1 + Point(20));
  EXPECT_FALSE(bounds.Contains(p1));
  EXPECT_TRUE(bounds.Contains(p1 + Point(15)));
  bounds = bounds.Included(p1 - Point(5));
  EXPECT_TRUE(bounds.Contains(p1));
  EXPECT_TRUE(bounds.Contains(p1 + Point(15)));
}

TYPED_TEST(BoundsTest, Empty) {
  using Point = typename TypeParam::Point;
  const Point p1 = RandPoint<Point>();
  TypeParam bounds = TypeParam::Empty();
  bounds = bounds.Included(p1);
  EXPECT_THAT(bounds.min, Eq(p1));
  EXPECT_THAT(bounds.max, Eq(p1));
}

TYPED_TEST(BoundsTest, Compare) {
  using Point = typename TypeParam::Point;
  const Point p1 = RandPoint<Point>();
  const Point p2 = p1 + Point(10);
  const Point p3 = p1 + Point(11);
  const Point p4 = p1 + Point(12);
  const TypeParam bounds1(p1, p2);
  const TypeParam bounds2(p1, p2);
  const TypeParam bounds3(p1, p3);
  const TypeParam bounds4(p1, p4);
  const TypeParam bounds5(p2, p4);

  EXPECT_TRUE(bounds1 == bounds2);
  EXPECT_TRUE(bounds1 != bounds3);
  EXPECT_TRUE(bounds4 != bounds5);
}

}  // namespace
}  // namespace redux
