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

#include <array>
#include <vector>

#include "gtest/gtest.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/vertex.h"
#include "lullaby/tests/portable_test_macros.h"
#include "lullaby/tests/test_data_container.h"

namespace lull {
namespace {

using PrimitiveType = MeshData::PrimitiveType;
using testing::CreateReadDataContainer;
using testing::CreateWriteDataContainer;

constexpr float kEpsilon = 0.0001f;

TEST(MeshData, GetPrimitiveType) {
  MeshData points_mesh(PrimitiveType::kPoints, VertexPT::kFormat,
                       CreateReadDataContainer(0U));
  MeshData lines_mesh(PrimitiveType::kLines, VertexPT::kFormat,
                      CreateReadDataContainer(0U));
  MeshData tri_mesh(PrimitiveType::kTriangles, VertexPT::kFormat,
                    CreateReadDataContainer(0U));
  MeshData tri_strip_mesh(PrimitiveType::kTriangleStrip, VertexPT::kFormat,
                          CreateReadDataContainer(0U));
  MeshData tri_fan_mesh(PrimitiveType::kTriangleFan, VertexPT::kFormat,
                        CreateReadDataContainer(0U));

  EXPECT_EQ(points_mesh.GetPrimitiveType(), PrimitiveType::kPoints);
  EXPECT_EQ(lines_mesh.GetPrimitiveType(), PrimitiveType::kLines);
  EXPECT_EQ(tri_mesh.GetPrimitiveType(), PrimitiveType::kTriangles);
  EXPECT_EQ(tri_strip_mesh.GetPrimitiveType(), PrimitiveType::kTriangleStrip);
  EXPECT_EQ(tri_fan_mesh.GetPrimitiveType(), PrimitiveType::kTriangleFan);
}

TEST(MeshData, GetVertexFormat) {
  MeshData p_mesh(PrimitiveType::kPoints, VertexP::kFormat,
                  CreateReadDataContainer(0U));
  MeshData pt_mesh(PrimitiveType::kPoints, VertexPT::kFormat,
                   CreateReadDataContainer(0U));
  MeshData pn_mesh(PrimitiveType::kPoints, VertexPN::kFormat,
                   CreateReadDataContainer(0U));
  MeshData pc_mesh(PrimitiveType::kPoints, VertexPC::kFormat,
                   CreateReadDataContainer(0U));
  MeshData ptc_mesh(PrimitiveType::kPoints, VertexPTC::kFormat,
                    CreateReadDataContainer(0U));
  MeshData ptn_mesh(PrimitiveType::kPoints, VertexPTN::kFormat,
                    CreateReadDataContainer(0U));
  MeshData pti_mesh(PrimitiveType::kPoints, VertexPTI::kFormat,
                    CreateReadDataContainer(0U));

  EXPECT_EQ(p_mesh.GetVertexFormat(), VertexP::kFormat);
  EXPECT_EQ(pt_mesh.GetVertexFormat(), VertexPT::kFormat);
  EXPECT_EQ(pn_mesh.GetVertexFormat(), VertexPN::kFormat);
  EXPECT_EQ(pc_mesh.GetVertexFormat(), VertexPC::kFormat);
  EXPECT_EQ(ptc_mesh.GetVertexFormat(), VertexPTC::kFormat);
  EXPECT_EQ(ptn_mesh.GetVertexFormat(), VertexPTN::kFormat);
  EXPECT_EQ(pti_mesh.GetVertexFormat(), VertexPTI::kFormat);
}

TEST(MeshData, GetVertexBytes) {
  // Dump some bytes into the MeshData, and make sure we get back those same
  // bytes when we request them.
  std::vector<uint8_t> vertex_buffer;
  for (std::vector<uint8_t>::value_type i = 0U; i < 9U; i++) {
    vertex_buffer.push_back(static_cast<uint8_t>(i + 1U));
  }

  // We don't use CreateReadDataContainer here because that function sets up a
  // DataContainer that deletes its data, whereas here our data is managed by
  // the std::vector.
  DataContainer vertex_data(
      DataContainer::DataPtr(vertex_buffer.data(), [](const uint8_t* ptr) {}),
      9U, DataContainer::AccessFlags::kRead);

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                std::move(vertex_data));

  const uint8_t* vertex_bytes = mesh.GetVertexBytes();
  ASSERT_NE(vertex_bytes, nullptr);

  for (uint8_t i = 0; i < 9; i++) {
    EXPECT_EQ(vertex_bytes[i], i + static_cast<uint8_t>(1U));
  }
}

TEST(MeshData, GetVertexBytesFailsWithoutReadAccess) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateWriteDataContainer(1U * sizeof(VertexP)));
  const uint8_t* vertex_bytes = mesh.GetVertexBytes();
  EXPECT_EQ(vertex_bytes, nullptr);
}

TEST(MeshData, GetVertexBytesEmpty) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateReadDataContainer(0U));
  const uint8_t* vertex_bytes = mesh.GetVertexBytes();
  EXPECT_EQ(vertex_bytes, nullptr);
}

TEST(MeshData, GetVertexData) {
  VertexP* vertex_buffer = new VertexP[3];
  vertex_buffer[0] = VertexP(1.f, 2.f, 3.f);
  vertex_buffer[1] = VertexP(4.f, 5.f, 6.f);
  vertex_buffer[2] = VertexP(7.f, 8.f, 9.f);
  DataContainer vertex_data = CreateReadDataContainer(
      reinterpret_cast<uint8_t*>(vertex_buffer), 3U * sizeof(VertexP));

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                std::move(vertex_data));

  const VertexP* vertex_read_ptr = mesh.GetVertexData<VertexP>();
  ASSERT_NE(vertex_read_ptr, nullptr);
  EXPECT_NEAR(vertex_read_ptr[0].x, 1.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[0].y, 2.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[0].z, 3.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[1].x, 4.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[1].y, 5.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[1].z, 6.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[2].x, 7.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[2].y, 8.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[2].z, 9.f, kEpsilon);
}

TEST(MeshData, GetVertexDataFailsWithoutReadAccess) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateWriteDataContainer(1U * sizeof(VertexP)));
  const VertexP* vertex_read_ptr = mesh.GetVertexData<VertexP>();
  EXPECT_EQ(vertex_read_ptr, nullptr);
}

TEST(MeshData, GetVertexDataEmpty) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateReadDataContainer(0U));
  const VertexP* vertex_read_ptr = mesh.GetVertexData<VertexP>();
  EXPECT_EQ(vertex_read_ptr, nullptr);
}

TEST(MeshDataDeathTest, GetVertexDataWrongFormat) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateReadDataContainer(0U));
  PORT_EXPECT_DEBUG_DEATH(mesh.GetVertexData<VertexPT>(), "");
}

TEST(MeshData, GetMutableVertexData) {
  DataContainer vertex_data =
      DataContainer::CreateHeapDataContainer(3U * sizeof(VertexP));
  VertexP* append_ptr = reinterpret_cast<VertexP*>(
      vertex_data.GetAppendPtr(3U * sizeof(VertexP)));
  ASSERT_NE(append_ptr, nullptr);
  append_ptr[0] = VertexP(1.f, 2.f, 3.f);
  append_ptr[1] = VertexP(4.f, 5.f, 6.f);
  append_ptr[2] = VertexP(7.f, 8.f, 9.f);

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                std::move(vertex_data));
  VertexP* mutable_vertex_data = mesh.GetMutableVertexData<VertexP>();
  ASSERT_NE(mutable_vertex_data, nullptr);
  mutable_vertex_data[1] = VertexP(100.f, 200.f, 300.f);

  const VertexP* readable_vertex_data = mesh.GetVertexData<VertexP>();
  EXPECT_NEAR(readable_vertex_data[0].x, 1.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[0].y, 2.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[0].z, 3.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[1].x, 100.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[1].y, 200.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[1].z, 300.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[2].x, 7.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[2].y, 8.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[2].z, 9.f, kEpsilon);
}

TEST(MeshData, GetMutableVertexDataFailsWithoutReadAccess) {
  DataContainer vertex_data = CreateWriteDataContainer(1U * sizeof(VertexP));
  VertexP* append_ptr = reinterpret_cast<VertexP*>(
      vertex_data.GetAppendPtr(1U * sizeof(VertexP)));
  ASSERT_NE(append_ptr, nullptr);
  append_ptr[0] = VertexP(1.f, 2.f, 3.f);

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                std::move(vertex_data));
  VertexP* mutable_vertex_data = mesh.GetMutableVertexData<VertexP>();
  EXPECT_EQ(mutable_vertex_data, nullptr);
}

TEST(MeshData, GetMutableVertexDataFailsWithoutWriteAccess) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateReadDataContainer(1U * sizeof(VertexP)));
  VertexP* mutable_vertex_data = mesh.GetMutableVertexData<VertexP>();
  EXPECT_EQ(mutable_vertex_data, nullptr);
}

TEST(MeshData, GetMutableVertexDataEmpty) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(0U));
  VertexP* mutable_vertex_data = mesh.GetMutableVertexData<VertexP>();
  EXPECT_EQ(mutable_vertex_data, nullptr);
}

TEST(MeshDataDeathTest, GetMutableVertexDataWrongFormat) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(3U * sizeof(VertexP)));
  PORT_EXPECT_DEBUG_DEATH(mesh.GetMutableVertexData<VertexPT>(), "");
}

TEST(MeshData, AddVertex) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(2U * sizeof(VertexP)));
  EXPECT_EQ(*mesh.AddVertex(VertexP(1.f, 2.f, 3.f)), 0U);
  EXPECT_EQ(*mesh.AddVertex<VertexP>(4.f, 5.f, 6.f), 1U);

  const VertexP* vertex_data = mesh.GetVertexData<VertexP>();
  EXPECT_NEAR(vertex_data[0].x, 1.f, kEpsilon);
  EXPECT_NEAR(vertex_data[0].y, 2.f, kEpsilon);
  EXPECT_NEAR(vertex_data[0].z, 3.f, kEpsilon);
  EXPECT_NEAR(vertex_data[1].x, 4.f, kEpsilon);
  EXPECT_NEAR(vertex_data[1].y, 5.f, kEpsilon);
  EXPECT_NEAR(vertex_data[1].z, 6.f, kEpsilon);
  EXPECT_EQ(mesh.GetNumVertices(), 2U);
}

TEST(MeshData, AddVertexWorksWithOnlyWriteAccess) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateWriteDataContainer(3U * sizeof(VertexP)));
  EXPECT_EQ(*mesh.AddVertex(VertexP(1.f, 2.f, 3.f)), 0U);
  EXPECT_EQ(*mesh.AddVertex<VertexP>(4.f, 5.f, 6.f), 1U);
  EXPECT_EQ(*mesh.AddVertex<VertexP>(7.f, 8.f, 9.f), 2U);
  EXPECT_EQ(mesh.GetNumVertices(), 3U);
}

TEST(MeshDataDeathTest, AddVertexOverCapacity) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateWriteDataContainer(1U * sizeof(VertexP)));
  EXPECT_EQ(*mesh.AddVertex<VertexP>(1.f, 2.f, 3.f), 0U);
  PORT_EXPECT_DEBUG_DEATH(mesh.AddVertex<VertexP>(4.f, 5.f, 6.f), "");
  EXPECT_EQ(mesh.GetNumVertices(), 1U);
}

TEST(MeshDataDeathTest, AddVertexNoWriteAccess) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateReadDataContainer(3U * sizeof(VertexP)));
  PORT_EXPECT_DEBUG_DEATH(mesh.AddVertex<VertexP>(1.f, 2.f, 3.f), "");
  EXPECT_EQ(mesh.GetNumVertices(), 0U);
}

TEST(MeshData, AddVertices) {
  std::unique_ptr<VertexP[]> vertex_buffer =
      std::unique_ptr<VertexP[]>(new VertexP[2]);
  vertex_buffer[0] = VertexP(1.f, 2.f, 3.f);
  vertex_buffer[1] = VertexP(4.f, 5.f, 6.f);

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(3U * sizeof(VertexP)));
  EXPECT_EQ(*mesh.AddVertices<VertexP>(vertex_buffer.get(), 2U), 0U);

  const VertexP third_vertex(7.f, 8.f, 9.f);
  EXPECT_EQ(*mesh.AddVertices(reinterpret_cast<const uint8_t*>(&third_vertex),
                              1U, sizeof(third_vertex)),
            2U);

  const VertexP* readable_vertex_data = mesh.GetVertexData<VertexP>();
  EXPECT_NEAR(readable_vertex_data[0].x, 1.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[0].y, 2.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[0].z, 3.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[1].x, 4.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[1].y, 5.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[1].z, 6.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[2].x, 7.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[2].y, 8.f, kEpsilon);
  EXPECT_NEAR(readable_vertex_data[2].z, 9.f, kEpsilon);
  EXPECT_EQ(mesh.GetNumVertices(), 3U);
}

TEST(MeshData, AddVerticesWorksWithOnlyWriteAccess) {
  std::unique_ptr<VertexP[]> vertex_buffer =
      std::unique_ptr<VertexP[]>(new VertexP[2]);
  vertex_buffer[0] = VertexP(1.f, 2.f, 3.f);
  vertex_buffer[1] = VertexP(4.f, 5.f, 6.f);

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateWriteDataContainer(4U * sizeof(VertexP)));
  EXPECT_EQ(*mesh.AddVertices<VertexP>(vertex_buffer.get(), 2U), 0U);
  EXPECT_EQ(*mesh.AddVertices<VertexP>(vertex_buffer.get(), 2U), 2U);
  EXPECT_EQ(mesh.GetNumVertices(), 4U);
}

TEST(MeshDataDeathTest, AddVerticesOverCapacity) {
  std::unique_ptr<VertexP[]> vertex_buffer =
      std::unique_ptr<VertexP[]>(new VertexP[1]);
  vertex_buffer[0] = VertexP(1.f, 2.f, 3.f);

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateWriteDataContainer(1U * sizeof(VertexP)));
  EXPECT_EQ(*mesh.AddVertices<VertexP>(vertex_buffer.get(), 1U), 0U);
  PORT_EXPECT_DEBUG_DEATH(mesh.AddVertices<VertexP>(vertex_buffer.get(), 1U),
                          "");
  EXPECT_EQ(mesh.GetNumVertices(), 1U);

  PORT_EXPECT_DEBUG_DEATH(
      mesh.AddVertices(reinterpret_cast<const uint8_t*>(vertex_buffer.get()),
                       1U, sizeof(VertexP)),
      "");
  EXPECT_EQ(mesh.GetNumVertices(), 1U);
}

TEST(MeshDataDeathTest, AddVerticesNoWriteAccess) {
  std::unique_ptr<VertexP[]> vertex_buffer =
      std::unique_ptr<VertexP[]>(new VertexP[1]);
  vertex_buffer[0] = VertexP(1.f, 2.f, 3.f);

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateReadDataContainer(3U * sizeof(VertexP)));
  PORT_EXPECT_DEBUG_DEATH(mesh.AddVertices<VertexP>(vertex_buffer.get(), 1U),
                          "");
  EXPECT_EQ(mesh.GetNumVertices(), 0U);
}

TEST(MeshDataDeathTest, AddVerticesWrongFormat) {
  std::vector<VertexPT> vertexPTs;
  vertexPTs.emplace_back(1.0f, 2.0f, 3.0f, 4.0f, 5.0f);

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateWriteDataContainer(3U * sizeof(VertexP)));

  PORT_EXPECT_DEBUG_DEATH(mesh.AddVertex(vertexPTs[0]), "");
  EXPECT_EQ(mesh.GetNumVertices(), 0U);

  PORT_EXPECT_DEBUG_DEATH(
      mesh.AddVertices(reinterpret_cast<const uint8_t*>(vertexPTs.data()),
                       vertexPTs.size(), sizeof(vertexPTs[0])),
      "");
  EXPECT_EQ(mesh.GetNumVertices(), 0U);
}

TEST(MeshData, GetNumVerticesNewInstance) {
  DataContainer vertex_data =
      DataContainer::CreateHeapDataContainer(2U * sizeof(VertexP));
  VertexP* append_ptr = reinterpret_cast<VertexP*>(
      vertex_data.GetAppendPtr(2U * sizeof(VertexP)));
  append_ptr[0] = VertexP(1.f, 2.f, 3.f);
  append_ptr[1] = VertexP(4.f, 5.f, 6.f);

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                std::move(vertex_data));
  EXPECT_EQ(mesh.GetNumVertices(), 2U);
}

TEST(MeshData, GetIndexData) {
  DataContainer index_data =
      DataContainer::CreateHeapDataContainer(3U * sizeof(uint16_t));
  auto* append_ptr = reinterpret_cast<uint16_t*>(
      index_data.GetAppendPtr(3U * sizeof(uint16_t)));
  append_ptr[0] = 1U;
  append_ptr[1] = 2U;
  append_ptr[2] = 3U;

  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateReadDataContainer(3U * sizeof(VertexP)),
                MeshData::kIndexU16, std::move(index_data));
  const uint16_t* readable_index_data = mesh.GetIndexData<uint16_t>();
  EXPECT_EQ(readable_index_data[0], 1U);
  EXPECT_EQ(readable_index_data[1], 2U);
  EXPECT_EQ(readable_index_data[2], 3U);
  EXPECT_EQ(reinterpret_cast<const uint8_t*>(readable_index_data),
            mesh.GetIndexBytes());
}

TEST(MeshDataDeathTest, GetIndexDataFailsWhenGivenIncorrectType) {
  MeshData mesh16(PrimitiveType::kTriangles, VertexP::kFormat,
                  CreateReadDataContainer(3U * sizeof(VertexP)),
                  MeshData::kIndexU16,
                  CreateReadDataContainer(3U * sizeof(uint16_t)));
  PORT_EXPECT_DEBUG_DEATH(mesh16.GetIndexData<uint8_t>(), "Invalid index type");
  PORT_EXPECT_DEBUG_DEATH(mesh16.GetIndexData<uint32_t>(), "type mismatch");

  MeshData mesh32(PrimitiveType::kTriangles, VertexP::kFormat,
                  CreateReadDataContainer(3U * sizeof(VertexP)),
                  MeshData::kIndexU32,
                  CreateReadDataContainer(3U * sizeof(uint32_t)));
  PORT_EXPECT_DEBUG_DEATH(mesh32.GetIndexData<uint8_t>(), "Invalid index type");
  PORT_EXPECT_DEBUG_DEATH(mesh32.GetIndexData<uint16_t>(), "type mismatch");
}

TEST(MeshData, EmptyIndexData) {
  MeshData mesh(PrimitiveType::kPoints, VertexPT::kFormat,
                CreateReadDataContainer(16U * sizeof(VertexPT)),
                MeshData::kIndexU16, CreateReadDataContainer(0U));
  EXPECT_EQ(mesh.GetNumIndices(), 0U);
  EXPECT_EQ(mesh.GetIndexData<uint16_t>(), nullptr);
  EXPECT_EQ(mesh.GetIndexBytes(), nullptr);
}

TEST(MeshData, AddIndex) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(3U * sizeof(VertexP)),
                MeshData::kIndexU16,
                DataContainer::CreateHeapDataContainer(2U * sizeof(uint16_t)));
  mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  mesh.AddVertex<VertexP>(4.f, 5.f, 6.f);
  EXPECT_TRUE(mesh.AddIndex(0U));
  EXPECT_TRUE(mesh.AddIndex(1U));

  const uint16_t* readable_index_data = mesh.GetIndexData<uint16_t>();
  EXPECT_EQ(readable_index_data[0], 0U);
  EXPECT_EQ(readable_index_data[1], 1U);
}

TEST(MeshData, AddIndexFailsWithNoWriteAccess) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateWriteDataContainer(3U * sizeof(VertexP)),
                MeshData::kIndexU16,
                CreateReadDataContainer(3U * sizeof(uint16_t)));
  mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  mesh.AddVertex<VertexP>(4.f, 5.f, 6.f);
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndex(0U), "");
}

TEST(MeshDataDeathTest, AddIndexOverCapacity) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(3U * sizeof(VertexP)),
                MeshData::kIndexU16,
                DataContainer::CreateHeapDataContainer(1U * sizeof(uint16_t)));
  mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  mesh.AddVertex<VertexP>(4.f, 5.f, 6.f);
  mesh.AddVertex<VertexP>(7.f, 8.f, 9.f);
  EXPECT_TRUE(mesh.AddIndex(0U));
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndex(2U), "");
}

TEST(MeshDataDeathTest, AddIndexOutOfBounds) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(3U * sizeof(VertexP)),
                MeshData::kIndexU16,
                DataContainer::CreateHeapDataContainer(3U * sizeof(uint16_t)));
  mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  mesh.AddVertex<VertexP>(4.f, 5.f, 6.f);
  mesh.AddVertex<VertexP>(7.f, 8.f, 9.f);
  EXPECT_TRUE(mesh.AddIndex(0U));
  EXPECT_TRUE(mesh.AddIndex(1U));
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndex(3U), "");
  EXPECT_EQ(mesh.GetNumIndices(), 2U);
}

TEST(MeshData, AddIndices) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(4U * sizeof(VertexP)),
                MeshData::kIndexU16,
                DataContainer::CreateHeapDataContainer(4U * sizeof(uint16_t)));
  for (size_t i = 0; i < 4; ++i) {
    mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  }

  EXPECT_TRUE(mesh.AddIndices({0U, 1U}));
  EXPECT_TRUE(mesh.AddIndex(2U));

  const uint16_t extra_index = 3U;
  EXPECT_TRUE(mesh.AddIndices(&extra_index, 1));

  const uint16_t* readable_index_data = mesh.GetIndexData<uint16_t>();
  EXPECT_EQ(readable_index_data[0], 0U);
  EXPECT_EQ(readable_index_data[1], 1U);
  EXPECT_EQ(readable_index_data[2], 2U);
  EXPECT_EQ(readable_index_data[3], 3U);
}

TEST(MeshData, GetSubMeshes) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(4U * sizeof(VertexP)),
                MeshData::kIndexU16,
                DataContainer::CreateHeapDataContainer(4U * sizeof(uint16_t)),
                DataContainer::CreateHeapDataContainer(8U * sizeof(uint32_t)));
  for (size_t i = 0; i < 4; ++i) {
    mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  }

  std::array<uint16_t, 2> test_indices = {0U, 1U};
  EXPECT_TRUE(mesh.AddIndices(test_indices.data(), test_indices.size()));
  EXPECT_TRUE(mesh.AddIndex(2U));

  const uint16_t extra_index = 3U;
  EXPECT_TRUE(mesh.AddIndices(&extra_index, 1));

  EXPECT_EQ(mesh.GetNumSubMeshes(), 3u);
  EXPECT_EQ(mesh.GetSubMesh(0).start, 0u);
  EXPECT_EQ(mesh.GetSubMesh(0).end, 2u);
  EXPECT_EQ(mesh.GetSubMesh(1).start, 2u);
  EXPECT_EQ(mesh.GetSubMesh(1).end, 3u);
  EXPECT_EQ(mesh.GetSubMesh(2).start, 3u);
  EXPECT_EQ(mesh.GetSubMesh(2).end, 4u);
  EXPECT_EQ(mesh.GetSubMesh(3).start, MeshData::kInvalidIndexU32);
  EXPECT_EQ(mesh.GetSubMesh(3).end, MeshData::kInvalidIndexU32);
}

TEST(MeshData, GetSubMeshesNoSubMeshData) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(4U * sizeof(VertexP)),
                MeshData::kIndexU16,
                DataContainer::CreateHeapDataContainer(4U * sizeof(uint16_t)));
  for (size_t i = 0; i < 4; ++i) {
    mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  }

  const std::array<uint16_t, 2> index_array = {0, 1};
  EXPECT_TRUE(mesh.AddIndices(index_array.data(), index_array.size()));
  EXPECT_TRUE(mesh.AddIndex(2U));

  const uint16_t extra_index = 3U;
  EXPECT_TRUE(mesh.AddIndices(&extra_index, 1));

  EXPECT_EQ(mesh.GetNumSubMeshes(), 1U);
  EXPECT_EQ(mesh.GetSubMesh(0).start, 0U);
  EXPECT_EQ(mesh.GetSubMesh(0).end, 4U);
  EXPECT_EQ(mesh.GetSubMesh(1).start, MeshData::kInvalidIndexU32);
  EXPECT_EQ(mesh.GetSubMesh(1).end, MeshData::kInvalidIndexU32);
}

TEST(MeshData, AddIndicesFailsWithNoWriteAccess) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                CreateWriteDataContainer(3U * sizeof(VertexP)),
                MeshData::kIndexU16,
                CreateReadDataContainer(3U * sizeof(uint16_t)));
  mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  mesh.AddVertex<VertexP>(4.f, 5.f, 6.f);
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndex(0U), "");

  const std::array<uint16_t, 2> indices = {0, 1};
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndices(indices.data(), indices.size()), "");
}

TEST(MeshDataDeathTest, AddIndicesOverCapacity) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(3U * sizeof(VertexP)),
                MeshData::kIndexU16,
                DataContainer::CreateHeapDataContainer(1U * sizeof(uint16_t)));
  mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  mesh.AddVertex<VertexP>(4.f, 5.f, 6.f);
  mesh.AddVertex<VertexP>(7.f, 8.f, 9.f);
  EXPECT_TRUE(mesh.AddIndex(0U));

  const std::array<uint16_t, 2> indices = {0, 1};
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndices(indices.data(), indices.size()), "");
  EXPECT_EQ(mesh.GetNumIndices(), 1U);
}

TEST(MeshDataDeathTest, AddIndicesOutOfBounds) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(3U * sizeof(VertexP)),
                MeshData::kIndexU16,
                DataContainer::CreateHeapDataContainer(3U * sizeof(uint16_t)));
  mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  mesh.AddVertex<VertexP>(4.f, 5.f, 6.f);
  mesh.AddVertex<VertexP>(7.f, 8.f, 9.f);
  EXPECT_TRUE(mesh.AddIndex(0));
  EXPECT_TRUE(mesh.AddIndex(1));
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndex(3U), "");

  const std::array<uint16_t, 2> indices = {3, 0};
  PORT_EXPECT_DEBUG_DEATH(mesh.AddIndices(indices.data(), indices.size()), "");
  EXPECT_EQ(mesh.GetNumIndices(), 2U);
}

TEST(MeshData, GetNumIndicesNewInstance) {
  DataContainer index_data =
      DataContainer::CreateHeapDataContainer(2U * sizeof(uint16_t));
  auto* append_ptr = reinterpret_cast<uint16_t*>(
      index_data.GetAppendPtr(2U * sizeof(uint16_t)));
  append_ptr[0] = 1U;
  append_ptr[1] = 2U;

  MeshData mesh(PrimitiveType::kTriangles, VertexPT::kFormat,
                CreateReadDataContainer(3U * sizeof(VertexP)),
                MeshData::kIndexU16, std::move(index_data));
  EXPECT_EQ(mesh.GetNumIndices(), 2U);
}

TEST(MeshData, GetAabb) {
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(5U * sizeof(VertexP)),
                MeshData::kIndexU16,
                DataContainer::CreateHeapDataContainer(3U * sizeof(uint16_t)));

  // Check the empty mesh for an aabb of zeros.
  EXPECT_EQ(mesh.GetAabb().min, mathfu::kZeros3f);
  EXPECT_EQ(mesh.GetAabb().max, mathfu::kZeros3f);

  // At first, only add 3 verts, to leave room for a later edit to test aabb
  // update.
  mesh.AddVertex<VertexP>(4.f, 8.f, 3.f);
  mesh.AddVertex<VertexP>(7.f, 5.f, 6.f);
  mesh.AddVertex<VertexP>(1.f, 2.f, 9.f);
  const std::array<uint16_t, 3> indices = {0, 1, 2};
  mesh.AddIndices(indices.data(), indices.size());

  EXPECT_EQ(mesh.GetAabb().min, mathfu::vec3(1.f, 2.f, 3.f));
  EXPECT_EQ(mesh.GetAabb().max, mathfu::vec3(7.f, 8.f, 9.f));

  mesh.AddVertex<VertexP>(20.f, 30.f, 40.f);
  mesh.AddVertex<VertexP>(10.f, 80.f, -1.f);

  EXPECT_EQ(mesh.GetAabb().min, mathfu::vec3(1.f, 2.f, -1.f));
  EXPECT_EQ(mesh.GetAabb().max, mathfu::vec3(20.f, 80.f, 40.f));
}

TEST(MeshData, CreateHeapCopy) {
  const std::array<uint16_t, 3> indices = {0, 2, 1};
  MeshData mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                DataContainer::CreateHeapDataContainer(3U * sizeof(VertexP)),
                MeshData::kIndexU16,
                DataContainer::CreateHeapDataContainer(indices.size() *
                                                       sizeof(indices[0])));
  mesh.AddVertex<VertexP>(1.f, 2.f, 3.f);
  mesh.AddVertex<VertexP>(4.f, 5.f, 6.f);
  mesh.AddVertex<VertexP>(7.f, 8.f, 9.f);
  EXPECT_TRUE(mesh.AddIndices(indices.data(), indices.size()));

  MeshData copy = mesh.CreateHeapCopy();
  EXPECT_EQ(mesh.GetPrimitiveType(), copy.GetPrimitiveType());
  EXPECT_EQ(mesh.GetVertexFormat(), copy.GetVertexFormat());
  EXPECT_EQ(mesh.GetNumVertices(), copy.GetNumVertices());
  ASSERT_NE(copy.GetVertexBytes(), nullptr);
  EXPECT_EQ(std::memcmp(
                mesh.GetVertexBytes(), copy.GetVertexBytes(),
                mesh.GetNumVertices() * mesh.GetVertexFormat().GetVertexSize()),
            0);
  EXPECT_EQ(mesh.GetNumIndices(), copy.GetNumIndices());
  ASSERT_NE(copy.GetIndexBytes(), nullptr);
  EXPECT_EQ(std::memcmp(mesh.GetIndexBytes(), copy.GetIndexBytes(),
                        mesh.GetNumIndices() * mesh.GetIndexSize()),
            0);
}

TEST(MeshDataDeathTest, CreateHeapCopyWithoutReadAccess) {
  MeshData uncopyable_mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                           DataContainer(), MeshData::kIndexU16,
                           DataContainer());
  MeshData result = uncopyable_mesh.CreateHeapCopy();
  EXPECT_EQ(result.GetNumVertices(), 0U);
  EXPECT_EQ(result.GetNumIndices(), 0U);
}

TEST(MeshDataTest, IndexTypesHaveCorrectSizes) {
  CHECK_EQ(MeshData::GetIndexSize(MeshData::kIndexU16), sizeof(uint16_t));
  CHECK_EQ(MeshData::GetIndexSize(MeshData::kIndexU32), sizeof(uint32_t));

  MeshData u16_mesh;
  CHECK_EQ(u16_mesh.GetIndexType(), MeshData::kIndexU16);
  CHECK_EQ(u16_mesh.GetIndexSize(), sizeof(uint16_t));

  MeshData u32_mesh(PrimitiveType::kTriangles, VertexP::kFormat,
                    DataContainer(), MeshData::kIndexU32, DataContainer());
  CHECK_EQ(u32_mesh.GetIndexType(), MeshData::kIndexU32);
  CHECK_EQ(u32_mesh.GetIndexSize(), sizeof(uint32_t));
}

}  // namespace
}  // namespace lull
