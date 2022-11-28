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
#include "redux/systems/shape/shape_builder.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::FloatEq;
using ::testing::FloatNear;
using ::testing::Gt;
using ::testing::Lt;

using VertexPosition3f =
    VertexElement<VertexType::Vec3f, VertexUsage::Position>;
using VertexNormal3f = VertexElement<VertexType::Vec3f, VertexUsage::Normal>;
using VertexUv2f = VertexElement<VertexType::Vec2f, VertexUsage::TexCoord0>;
using ShapeVertex = Vertex<VertexPosition3f, VertexNormal3f, VertexUv2f>;
using ShapeIndex = uint16_t;

vec2 AsVec(VertexUv2f v) { return vec2(v.x, v.y); }
vec3 AsVec(VertexPosition3f v) { return vec3(v.x, v.y, v.z); }
vec3 AsVec(VertexNormal3f v) { return vec3(v.x, v.y, v.z); }

TEST(ShapeGeneratorsTest, BoxVertexAndIndexCount) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  BoxShapeDef def;

  def.half_extents = vec3(1, 1, 1);

  builder.Build(def);
  EXPECT_THAT(builder.Vertices().size(), Eq(24));
  EXPECT_THAT(builder.Indices().size(), Eq(36));
}

TEST(ShapeGeneratorsTest, BoxSize) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  BoxShapeDef def;

  def.half_extents = vec3(2, 3, 4);
  builder.Build(def);

  Box bounds = Box::Empty();
  for (ShapeVertex& v : builder.Vertices()) {
    bounds = bounds.Included(AsVec(v.Position()));
  }

  EXPECT_THAT(bounds.min.x, Eq(-2));
  EXPECT_THAT(bounds.min.y, Eq(-3));
  EXPECT_THAT(bounds.min.z, Eq(-4));
  EXPECT_THAT(bounds.max.x, Eq(2));
  EXPECT_THAT(bounds.max.y, Eq(3));
  EXPECT_THAT(bounds.max.z, Eq(4));
}

TEST(ShapeGeneratorsTest, BoxNormalsFacingOutwards) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  BoxShapeDef def;

  def.half_extents = vec3(1, 1, 1);

  builder.Build(def);
  for (ShapeVertex& v : builder.Vertices()) {
    auto position = AsVec(v.Position());
    auto normal = AsVec(v.Normal());
    if (normal.x == 0 && normal.y == 0) {
      if (position.z > 0) {
        EXPECT_THAT(normal.z, Eq(1));
      } else {
        EXPECT_THAT(normal.z, Eq(-1));
      }
    } else if (normal.x == 0 && normal.z == 0) {
      if (position.y > 0) {
        EXPECT_THAT(normal.y, Eq(1));
      } else {
        EXPECT_THAT(normal.y, Eq(-1));
      }
    } else if (normal.y == 0 && normal.z == 0) {
      if (position.x > 0) {
        EXPECT_THAT(normal.x, Eq(1));
      } else {
        EXPECT_THAT(normal.x, Eq(-1));
      }
    } else {
      EXPECT_TRUE(false);
    }
  }
}

TEST(ShapeGeneratorsTest, BoxTexturesFacingUpwards) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  BoxShapeDef def;

  def.half_extents = vec3(1, 1, 1);

  builder.Build(def);
  for (ShapeVertex& v : builder.Vertices()) {
    auto position = AsVec(v.Position());
    auto uv = AsVec(v.TexCoord0());
    auto normal = AsVec(v.Normal());
    if (normal.y == 0.0f) {
      if (position.y < 0) {
        EXPECT_THAT(uv.y, Eq(1));
      } else {
        EXPECT_THAT(uv.y, Eq(0));
      }
    }
  }
}

}  // namespace
}  // namespace redux
