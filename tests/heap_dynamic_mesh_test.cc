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

#include "lullaby/util/heap_dynamic_mesh.h"
#include "lullaby/util/vertex.h"
#include "lullaby/generated/tests/portable_test_macros.h"

namespace lull {
namespace {

using TestVertex = VertexP;

class TestHeapDynamicMesh : public HeapDynamicMesh {
 public:
  TestHeapDynamicMesh(size_t max_verts, size_t max_indices)
      : HeapDynamicMesh(HeapDynamicMesh::PrimitiveType::kTriangles,
                        TestVertex::kFormat, max_verts, max_indices) {}
};

TEST(HeapDynamicMesh, CreatedEmpty) {
  TestHeapDynamicMesh mesh(16, 16);
  EXPECT_EQ(mesh.GetNumVertices(), 0U);
  EXPECT_EQ(mesh.GetNumIndices(), 0U);
}

TEST(HeapDynamicMesh, CorrectFormat) {
  TestHeapDynamicMesh mesh(16, 16);
  TestVertex v;
  mesh.AddVertex(v);
  mesh.AddVertex<TestVertex>(1.f, 2.f, 2.5f);
  EXPECT_EQ(mesh.GetNumVertices(), 2U);
}

TEST(HeapDynamicMesh, AddVertices) {
  std::vector<TestVertex> list;
  list.emplace_back(1.f, 2.f, 2.5f);
  list.emplace_back(-1.f, -2.f, -2.5f);

  TestHeapDynamicMesh mesh1(16, 16);
  mesh1.AddVertices(list.data(), list.size());
  EXPECT_EQ(mesh1.GetNumVertices(), 2U);

  TestHeapDynamicMesh mesh2(16, 16);
  mesh2.AddVertices(reinterpret_cast<const uint8_t*>(list.data()), list.size(),
                    sizeof(TestVertex));
  EXPECT_EQ(mesh2.GetNumVertices(), mesh1.GetNumVertices());
}

TEST(HeapDynamicMesh, AddIndices) {
  const TestVertex vertex(0, 0, 0);
  const int kNumVertices = 8;

  TestHeapDynamicMesh mesh(kNumVertices, kNumVertices);

  for (int i = 0; i < kNumVertices; ++i) {
    mesh.AddVertex(vertex);
  }

  mesh.AddIndex(0);
  EXPECT_EQ(mesh.GetNumIndices(), 1U);

  mesh.AddIndices({1, 2, 3});
  EXPECT_EQ(mesh.GetNumIndices(), 4U);

  const HeapDynamicMesh::Index list[] = {4, 5, 6};
  mesh.AddIndices(list, 3);
  EXPECT_EQ(mesh.GetNumIndices(), 7U);
}

TEST(HeapDynamicMeshDeathTest, WrongFormat) {
  TestHeapDynamicMesh mesh(16, 16);
  PORT_EXPECT_DEBUG_DEATH(mesh.AddVertex(VertexPT()), "");
  PORT_EXPECT_DEBUG_DEATH(mesh.AddVertex<VertexPT>(), "");
}

TEST(HeapDynamicMeshDeathTest, BadIndexDetected) {
  TestHeapDynamicMesh mesh(16, 16);
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndex(0), "");
}

TEST(HeapDynamicMeshDeathTest, OverflowDetected) {
  const size_t kSize = 16;
  TestHeapDynamicMesh mesh(kSize, kSize);

  for (size_t i = 0; i < kSize; ++i) {
    mesh.AddVertex(TestVertex());
    mesh.AddIndex(0);
  }

  EXPECT_EQ(mesh.GetNumVertices(), kSize);
  EXPECT_EQ(mesh.GetNumIndices(), kSize);

  PORT_EXPECT_DEBUG_DEATH(mesh.AddVertex(TestVertex()), "");

  const TestVertex vertex(0, 0, 0);
  PORT_EXPECT_DEBUG_DEATH(mesh.AddVertices(&vertex, 1), "");
  PORT_EXPECT_DEBUG_DEATH(
      mesh.AddVertices(reinterpret_cast<const uint8_t*>(&vertex), 1,
                       sizeof(vertex)),
      "");

  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndex(0), "");
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndices({0, 1}), "");

  const HeapDynamicMesh::Index index = 0;
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndices(&index, 1), "");
}

}  // namespace
}  // namespace lull
