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

#include "gtest/gtest.h"

#include "mathfu/constants.h"
#include "lullaby/util/vertex.h"

namespace lull {
namespace {

TEST(VertexP, ctor) {
  const mathfu::vec3 pos(1, 2, 3);

  const VertexP v1(pos.x, pos.y, pos.z);
  EXPECT_EQ(GetPosition(v1), pos);

  const VertexP v2(pos);
  EXPECT_EQ(GetPosition(v2), pos);
}

TEST(VertexPC, ctor) {
  const mathfu::vec3 pos(1, 2, 3);
  const Color4ub color(255, 128, 64, 192);

  VertexPC v1(pos, color);
  EXPECT_EQ(GetPosition(v1), pos);
  EXPECT_EQ(v1.color, color);

  VertexPC v2(pos.x, pos.y, pos.z, color);
  EXPECT_EQ(GetPosition(v2), pos);
  EXPECT_EQ(v2.color, color);
}

TEST(VertexPN, ctor) {
  const mathfu::vec3 pos(1, 2, 3);
  const mathfu::vec3 normal(0, 1, 0);

  VertexPN v1(pos, normal);
  EXPECT_EQ(GetPosition(v1), pos);
  EXPECT_EQ(GetNormal(v1), normal);

  VertexPN v2(pos.x, pos.y, pos.z, normal.x, normal.y, normal.z);
  EXPECT_EQ(GetPosition(v2), pos);
  EXPECT_EQ(GetNormal(v2), normal);
}

TEST(VertexPT, ctor) {
  const mathfu::vec3 pos(1, 2, 3);
  const mathfu::vec2 uv(4, 5);

  VertexPT v1(pos, uv);
  EXPECT_EQ(GetPosition(v1), pos);
  EXPECT_EQ(GetUv0(v1), uv);

  VertexPT v2(pos.x, pos.y, pos.z, uv.x, uv.y);
  EXPECT_EQ(GetPosition(v2), pos);
  EXPECT_EQ(GetUv0(v2), uv);
}

TEST(VertexPTT, VectorCtor) {
  const mathfu::vec3 pos(1, 2, 3);
  const mathfu::vec2 uv0(4, 5);
  const mathfu::vec2 uv1(6, 7);

  VertexPTT v1(pos, uv0, uv1);
  EXPECT_EQ(GetPosition(v1), pos);
  EXPECT_EQ(GetUv0(v1), uv0);
  EXPECT_EQ(GetUv1(v1), uv1);
}

TEST(VertexPTT, ComponentCtor) {
  const mathfu::vec3 pos(1, 2, 3);
  const mathfu::vec2 uv0(4, 5);
  const mathfu::vec2 uv1(6, 7);

  VertexPTT v2(pos.x, pos.y, pos.z, uv0.x, uv0.y, uv1.x, uv1.y);
  EXPECT_EQ(GetPosition(v2), pos);
  EXPECT_EQ(GetUv0(v2), uv0);
  EXPECT_EQ(GetUv1(v2), uv1);
}

TEST(VertexPTC, ctor) {
  const mathfu::vec3 pos(1, 2, 3);
  const mathfu::vec2 uv(4, 5);
  const Color4ub color(255, 128, 64, 192);

  VertexPTC v1(pos, uv, color);
  EXPECT_EQ(GetPosition(v1), pos);
  EXPECT_EQ(GetUv0(v1), uv);
  EXPECT_EQ(v1.color, color);

  VertexPTC v2(pos.x, pos.y, pos.z, uv.x, uv.y, color);
  EXPECT_EQ(GetPosition(v2), pos);
  EXPECT_EQ(GetUv0(v2), uv);
  EXPECT_EQ(v2.color, color);
}

TEST(VertexPTI, ctor) {
  const mathfu::vec3 pos(1, 2, 3);
  const mathfu::vec2 uv(4, 5);
  const uint8_t indices[4] = { 6, 7, 8, 9 };

  VertexPTI v1(pos, uv, indices);
  EXPECT_EQ(GetPosition(v1), pos);
  EXPECT_EQ(GetUv0(v1), uv);
  EXPECT_EQ(memcmp(v1.indices, indices, sizeof(indices)), 0);

  VertexPTI v2(pos.x, pos.y, pos.z, uv.x, uv.y, indices);
  EXPECT_EQ(GetPosition(v2), pos);
  EXPECT_EQ(GetUv0(v2), uv);
  EXPECT_EQ(memcmp(v2.indices, indices, sizeof(indices)), 0);
}

TEST(VertexPTN, ctor) {
  const mathfu::vec3 pos(1, 2, 3);
  const mathfu::vec2 uv(4, 5);
  const mathfu::vec3 normal(0, 1, 0);

  VertexPTN v1(pos, uv, normal);
  EXPECT_EQ(GetPosition(v1), pos);
  EXPECT_EQ(GetUv0(v1), uv);
  EXPECT_EQ(GetNormal(v1), normal);

  VertexPTN v2(pos.x, pos.y, pos.z, uv.x, uv.y, normal.x, normal.y, normal.z);
  EXPECT_EQ(GetPosition(v2), pos);
  EXPECT_EQ(GetUv0(v2), uv);
  EXPECT_EQ(GetNormal(v2), normal);
}

TEST(VertexPTTI, ctor) {
  const mathfu::vec3 pos(1, 2, 3);
  const mathfu::vec2 uv0(4, 5);
  const mathfu::vec2 uv1(6, 7);
  const uint8_t indices[4] = {8, 9, 10, 11};

  VertexPTTI v1(pos, uv0, uv1, indices);
  EXPECT_EQ(GetPosition(v1), pos);
  EXPECT_EQ(GetUv0(v1), uv0);
  EXPECT_EQ(GetUv1(v1), uv1);
  EXPECT_EQ(memcmp(v1.indices, indices, sizeof(indices)), 0);

  VertexPTTI v2(pos.x, pos.y, pos.z, uv0.x, uv0.y, uv1.x, uv1.y, indices);
  EXPECT_EQ(GetPosition(v2), pos);
  EXPECT_EQ(GetUv0(v2), uv0);
  EXPECT_EQ(GetUv1(v2), uv1);
  EXPECT_EQ(memcmp(v2.indices, indices, sizeof(indices)), 0);
}

TEST(Vertex, SetPosition) {
  VertexP v(0, 0, 0);

  const mathfu::vec3 p1(1, 2, 3);
  SetPosition(&v, p1);
  EXPECT_EQ(GetPosition(v), p1);

  const mathfu::vec3 p2(4, 5, 6);
  SetPosition(&v, p2.x, p2.y, p2.z);
  EXPECT_EQ(GetPosition(v), p2);
}

TEST(Vertex, SetColor) {
  const Color4ub color(255, 128, 64, 192);

  VertexPC v(mathfu::kZeros3f, Color4ub());
  EXPECT_NE(v.color, color);
  SetColor(&v, color);
  EXPECT_EQ(v.color, color);
}

TEST(Vertex, SetNormal) {
  const mathfu::vec3 normal(1, 0, 0);

  VertexPN v(mathfu::kZeros3f, mathfu::kZeros3f);
  EXPECT_NE(GetNormal(v), normal);
  SetNormal(&v, normal);
  EXPECT_EQ(GetNormal(v), normal);

  SetNormal(&v, mathfu::kZeros3f);
  SetNormal(&v, normal.x, normal.y, normal.z);
  EXPECT_EQ(GetNormal(v), normal);
}

TEST(Vertex, SetUv0) {
  const mathfu::vec2 uv(1, 2);

  VertexPT v(mathfu::kZeros3f, mathfu::kZeros2f);
  EXPECT_NE(GetUv0(v), uv);
  SetUv0(&v, uv);
  EXPECT_EQ(GetUv0(v), uv);

  SetUv0(&v, mathfu::kZeros2f);
  SetUv0(&v, uv.x, uv.y);
  EXPECT_EQ(GetUv0(v), uv);
}

TEST(Vertex, VectorSetUv1) {
  const mathfu::vec2 uv(1, 2);

  VertexPTT v(mathfu::kZeros3f, mathfu::kZeros2f, mathfu::kZeros2f);
  EXPECT_NE(GetUv1(v), uv);
  SetUv1(&v, uv);
  EXPECT_EQ(GetUv1(v), uv);
}

TEST(Vertex, ComponentSetUv1) {
  const mathfu::vec2 uv(1, 2);

  VertexPTT v(mathfu::kZeros3f, mathfu::kZeros2f, mathfu::kZeros2f);
  SetUv1(&v, mathfu::kZeros2f);
  SetUv1(&v, uv.x, uv.y);
  EXPECT_EQ(GetUv1(v), uv);
}

TEST(Vertex, ForEachVertexPosition) {
  VertexPTN vertices[] = {
      VertexPTN(mathfu::vec3(1, 2, 3), mathfu::kZeros2f, mathfu::kZeros3f),
      VertexPTN(mathfu::vec3(4, 5, 6), mathfu::kZeros2f, mathfu::kZeros3f),
      VertexPTN(mathfu::vec3(7, 8, 9), mathfu::kZeros2f, mathfu::kZeros3f),
  };

  const size_t vertex_count = sizeof(vertices) / sizeof(vertices[0]);

  std::vector<mathfu::vec3> positions;
  ForEachVertexPosition(
      reinterpret_cast<uint8_t*>(vertices), vertex_count, VertexPTN::kFormat,
      [&positions](const mathfu::vec3& p) { positions.push_back(p); });

  ASSERT_EQ(vertex_count, positions.size());
  for (size_t index = 0; index < positions.size(); ++index) {
    EXPECT_EQ(GetPosition(vertices[index]), positions[index]);
  }
}

}  // namespace
}  // namespace lull
