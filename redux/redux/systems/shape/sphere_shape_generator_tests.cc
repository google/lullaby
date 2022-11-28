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

TEST(ShapeGeneratorsTest, SphereVertexAndIndexCount) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  SphereShapeDef def;

  def.radius = 1.0f;
  def.num_parallels = 1;
  def.num_meridians = 3;
  builder.Build(def);
  EXPECT_THAT(builder.Vertices().size(), Eq(6));
  EXPECT_THAT(builder.Indices().size(), Eq(3 * 6));

  def.radius = 1.0f;
  def.num_parallels = 1;
  def.num_meridians = 7;
  builder.Build(def);
  EXPECT_THAT(builder.Vertices().size(), Eq(10));
  EXPECT_THAT(builder.Indices().size(), Eq(3 * 14));

  def.radius = 1.0f;
  def.num_parallels = 5;
  def.num_meridians = 3;
  builder.Build(def);
  EXPECT_THAT(builder.Vertices().size(), Eq(22));
  EXPECT_THAT(builder.Indices().size(), Eq(3 * (6 + 24)));
}

TEST(ShapeGeneratorsTest, InvalidParallels) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  SphereShapeDef def;

  def.radius = 1.0f;
  def.num_parallels = 0;
  def.num_meridians = 3;
  EXPECT_DEATH(builder.Build(def), "");
}

TEST(ShapeGeneratorsTest, InvalidMeridians) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  SphereShapeDef def;

  def.radius = 1.0f;
  def.num_parallels = 1;
  def.num_meridians = 2;
  EXPECT_DEATH(builder.Build(def), "");
}

TEST(ShapeGeneratorsTest, PositionsHaveRadiusLength) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  SphereShapeDef def;

  def.radius = 2.5f;
  def.num_parallels = 3;
  def.num_meridians = 5;
  builder.Build(def);

  for (auto& v : builder.Vertices()) {
    const float len = Length(AsVec(v.Position()));
    EXPECT_THAT(len, FloatNear(def.radius, kDefaultEpsilon));
  }

  def.radius = 8.3f;
  def.num_parallels = 3;
  def.num_meridians = 5;
  builder.Build(def);
  for (auto& v : builder.Vertices()) {
    const float len = Length(AsVec(v.Position()));
    EXPECT_THAT(len, FloatNear(def.radius, kDefaultEpsilon));
  }
}

TEST(ShapeGeneratorsTest, ExternallyFacingTriangles) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  SphereShapeDef def;

  def.radius = 1.0f;
  def.num_parallels = 1;
  def.num_meridians = 3;
  builder.Build(def);

  auto indices = builder.Indices();
  auto vertices = builder.Vertices();
  for (size_t i = 0; i < indices.size(); i += 3) {
    const vec3 p0 = AsVec(vertices[indices[i + 0]].Position());
    const vec3 p1 = AsVec(vertices[indices[i + 1]].Position());
    const vec3 p2 = AsVec(vertices[indices[i + 2]].Position());
    const vec3 d1 = p1 - p0;
    const vec3 d2 = p2 - p0;
    const vec3 normal = Cross(d1, d2).Normalized();
    EXPECT_THAT(Dot(p0, normal), Gt(0.0f));
    EXPECT_THAT(Dot(p1, normal), Gt(0.0f));
    EXPECT_THAT(Dot(p2, normal), Gt(0.0f));
  }
}

TEST(ShapeGeneratorsTest, GeneratesUniqueVerticesExceptForWhenUWraps) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  SphereShapeDef def;

  def.radius = 2.5f;
  def.num_parallels = 3;
  def.num_meridians = 5;
  builder.Build(def);

  float min_wrap_v = 1.0f;
  float max_wrap_v = 0.0f;

  auto vertices = builder.Vertices();
  for (size_t i = 0; i < vertices.size(); ++i) {
    const vec3 p1 = AsVec(vertices[i].Position());
    const vec2 uv1 = AsVec(vertices[i].TexCoord0());
    for (size_t k = i + 1; k < vertices.size(); ++k) {
      const vec3 p2 = AsVec(vertices[k].Position());
      const vec2 uv2 = AsVec(vertices[k].TexCoord0());

      const vec3 pos_delta = p1 - p2;
      if (pos_delta.Length() < kDefaultEpsilon) {
        EXPECT_THAT(uv1.y, Eq(uv2.y));
        EXPECT_TRUE((uv1.x == 0.0f && uv2.x == 1.0f) ||
                    (uv1.x == 1.0f && uv2.x == 0.0f));
        max_wrap_v = std::max(uv1.y, max_wrap_v);
        min_wrap_v = std::min(uv1.y, min_wrap_v);
      } else {
        EXPECT_TRUE(uv1.x != uv2.x || uv1.y != uv2.y);
      }
    }
  }

  EXPECT_THAT(min_wrap_v, Lt(max_wrap_v));
}

TEST(ShapeGeneratorsTest, GeneratesUvsAccordingToLatLonRegardlessOfFacing) {
  ShapeBuilder<ShapeVertex, ShapeIndex> builder;
  SphereShapeDef def;

  def.radius = 1.0f;
  def.num_parallels = 3;
  def.num_meridians = 5;
  builder.Build(def);

  // const float kEpsilon = 0.001f;
  for (auto& v : builder.Vertices()) {
    const vec3 pos = AsVec(v.Position());
    const vec2 uv = AsVec(v.TexCoord0());

    const float lat_angle = std::acos(pos.y / pos.Length());
    EXPECT_THAT(uv.y, FloatEq(lat_angle / kPi));

    // Pole U values are expected to be .5, otherwise they should follow
    // longitude except for the seam vertices which have u=1.0 at lon=0.
    if (pos.x == 0.0f && pos.z == 0.0f) {
      EXPECT_THAT(uv.x, Eq(0.5f));
    } else if (uv.x == 0.0f || uv.x == 1.0f) {
      EXPECT_THAT(pos.z, Eq(0.0f));
    } else {
      float lon_angle = std::atan2(pos.z, pos.x);
      if (lon_angle < 0.0f) {
        lon_angle += kTwoPi;
      }
      EXPECT_THAT(uv.x, FloatEq(1.0f - (lon_angle / kTwoPi)));
    }
  }
}

}  // namespace
}  // namespace redux
