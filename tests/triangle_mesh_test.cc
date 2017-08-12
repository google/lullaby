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

#include "lullaby/modules/render/triangle_mesh.h"
#include "gtest/gtest.h"
#include "lullaby/util/math.h"

namespace lull {
namespace {

static const float kEpsilon = kDefaultEpsilon;

TEST(TriangleMesh, StartsEmpty) {
  const TriangleMesh<VertexP> mesh;
  EXPECT_TRUE(mesh.IsEmpty());
  EXPECT_EQ(mesh.GetVertices().size(), size_t(0));
  EXPECT_EQ(mesh.GetIndices().size(), size_t(0));
}

TEST(TriangleMesh, Clear) {
  TriangleMesh<VertexP> mesh;

  mesh.AddVertex(mathfu::kZeros3f);
  mesh.AddVertex(mathfu::kZeros3f);
  mesh.AddVertex(mathfu::kZeros3f);
  EXPECT_EQ(mesh.GetVertices().size(), size_t(3));

  mesh.AddTriangle(0, 1, 2);
  EXPECT_EQ(mesh.GetIndices().size(), size_t(3));

  mesh.Clear();
  EXPECT_EQ(mesh.GetVertices().size(), size_t(0));
  EXPECT_EQ(mesh.GetIndices().size(), size_t(0));
}

TEST(TriangleMesh, BasicTriangle) {
  TriangleMesh<VertexP> mesh;

  // Add some vertices, and make sure they get copied ok.
  const mathfu::vec3 v0(0, 0, 0);
  const mathfu::vec3 v1(1, 0, 0);
  const mathfu::vec3 v2(0, 1, 0);

  const auto i0 = mesh.AddVertex(v0);
  const auto i1 = mesh.AddVertex(v1);
  const auto i2 = mesh.AddVertex(v2);

  EXPECT_EQ(mesh.GetVertices().size(), size_t(3));
  EXPECT_NEAR(DistanceBetween(v0, GetPosition(mesh.GetVertices()[i0])), 0.0f,
              kEpsilon);
  EXPECT_NEAR(DistanceBetween(v1, GetPosition(mesh.GetVertices()[i1])), 0.0f,
              kEpsilon);
  EXPECT_NEAR(DistanceBetween(v2, GetPosition(mesh.GetVertices()[i2])), 0.0f,
              kEpsilon);

  // Add and verify a triangle.
  mesh.AddTriangle(i0, i1, i2);
  EXPECT_EQ(mesh.GetIndices().size(), size_t(3));
  EXPECT_EQ(mesh.GetIndices()[0], i0);
  EXPECT_EQ(mesh.GetIndices()[1], i1);
  EXPECT_EQ(mesh.GetIndices()[2], i2);
}

TEST(TriangleMesh, Aabb) {
  TriangleMesh<VertexP> mesh;

  // First add a bunch of points to the mesh.
  const mathfu::vec3 points[] = {
      mathfu::vec3(0, 9, 2), mathfu::vec3(1, 4, -3), mathfu::vec3(-7, -2, 5),
  };
  const size_t num_points = sizeof(points) / sizeof(points[0]);

  for (size_t i = 0; i < num_points; ++i) {
    mesh.AddVertex(points[i]);
  }

  // Compare mesh's aabb against our own.
  const Aabb a = GetBoundingBox(points, num_points);
  const Aabb b = mesh.GetAabb();

  EXPECT_NEAR(DistanceBetween(a.min, b.min), 0.0f, kEpsilon);
  EXPECT_NEAR(DistanceBetween(a.max, b.max), 0.0f, kEpsilon);
}

}  // namespace
}  // namespace lull
