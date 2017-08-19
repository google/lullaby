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

#include "lullaby/modules/render/vertex.h"
#include "lullaby/util/data_container.h"
#include "lullaby/tests/portable_test_macros.h"
#include "lullaby/tests/test_data_container.h"

namespace lull {
namespace {

using testing::CreateReadDataContainer;
using testing::CreateWriteDataContainer;

constexpr float kEpsilon = 0.0001f;

TEST(DataContainer, CreatedWithRightSizes) {
  const DataContainer data = DataContainer::CreateHeapDataContainer(16U);
  EXPECT_EQ(data.GetSize(), 0U);
  EXPECT_EQ(data.GetCapacity(), 16U);
}

TEST(DataContainer, ZeroMaxSizeDataContainer) {
  const DataContainer data = DataContainer::CreateHeapDataContainer(0U);
  EXPECT_EQ(data.GetSize(), 0U);
  EXPECT_EQ(data.GetCapacity(), 0U);
  // Data containers with a max size of 0 are considered unreadable and
  // unwritable, regardless of the permissions of the container.
  EXPECT_FALSE(data.IsReadable());
  EXPECT_FALSE(data.IsWritable());
}

TEST(DataContainer, CorrectPermissions) {
  const DataContainer read_data = CreateReadDataContainer(16U);
  EXPECT_TRUE(read_data.IsReadable());
  EXPECT_FALSE(read_data.IsWritable());

  const DataContainer write_data = CreateWriteDataContainer(16U);
  EXPECT_TRUE(write_data.IsWritable());
  EXPECT_FALSE(write_data.IsReadable());

  const DataContainer read_write_data =
      DataContainer::CreateHeapDataContainer(16U);
  EXPECT_TRUE(read_write_data.IsReadable());
  EXPECT_TRUE(read_write_data.IsWritable());
}

TEST(DataContainer, ReadPtrOnlyAccessibleWithReadAccess) {
  const DataContainer read_data = CreateReadDataContainer(16U);
  EXPECT_NE(read_data.GetReadPtr(), nullptr);

  const DataContainer write_data = CreateWriteDataContainer(16U);
  EXPECT_EQ(write_data.GetReadPtr(), nullptr);
}

TEST(DataContainer, MutableDataOnlyAccessibleWithReadWriteAccess) {
  DataContainer read_write_data = DataContainer::CreateHeapDataContainer(16U);
  EXPECT_NE(read_write_data.GetData(), nullptr);

  DataContainer read_data = CreateReadDataContainer(16U);
  EXPECT_EQ(read_data.GetData(), nullptr);

  DataContainer write_data = CreateWriteDataContainer(16U);
  EXPECT_EQ(write_data.GetData(), nullptr);
}

TEST(DataContainer, AppendPtrOnlyAccessibleWithWriteAccess) {
  DataContainer write_data = CreateWriteDataContainer(sizeof(VertexP));
  EXPECT_NE(write_data.GetAppendPtr(sizeof(VertexP)), nullptr);

  DataContainer read_data = CreateReadDataContainer(sizeof(VertexP));
  EXPECT_EQ(read_data.GetAppendPtr(sizeof(VertexP)), nullptr);
}

TEST(DataContainer, AppendPtrIncreasesSize) {
  DataContainer data = DataContainer::CreateHeapDataContainer(16U);
  EXPECT_EQ(data.GetSize(), 0U);
  data.GetAppendPtr(sizeof(VertexP));
  EXPECT_EQ(data.GetSize(), sizeof(VertexP));
}

TEST(DataContainer, AppendPtrAvailableForMaxSize) {
  DataContainer data =
      DataContainer::CreateHeapDataContainer(16U * sizeof(VertexP));
  EXPECT_NE(data.GetAppendPtr(16U * sizeof(VertexP)), nullptr);
}

TEST(DataContainer, AppendPtrUnavailableWithNoCapacity) {
  DataContainer data =
      DataContainer::CreateHeapDataContainer(16U * sizeof(VertexP));
  EXPECT_EQ(data.GetAppendPtr(17U * sizeof(VertexP)), nullptr);
  EXPECT_NE(data.GetAppendPtr(16U * sizeof(VertexP)), nullptr);
  EXPECT_EQ(data.GetAppendPtr(1U * sizeof(VertexP)), nullptr);
}

TEST(DataContainer, AppendPtrDataIsReadable) {
  DataContainer data =
      DataContainer::CreateHeapDataContainer(2U * sizeof(VertexP));

  const VertexP* vertex_read_ptr =
      reinterpret_cast<const VertexP*>(data.GetReadPtr());
  ASSERT_NE(vertex_read_ptr, nullptr);

  VertexP* vertex_append_ptr =
      reinterpret_cast<VertexP*>(data.GetAppendPtr(2U * sizeof(VertexP)));
  ASSERT_NE(vertex_append_ptr, nullptr);
  vertex_append_ptr[0] = VertexP(10.f, 11.f, 12.f);
  vertex_append_ptr[1] = VertexP(20.f, 21.f, 22.f);

  EXPECT_NEAR(vertex_read_ptr[0].x, 10.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[0].y, 11.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[0].z, 12.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[1].x, 20.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[1].y, 21.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[1].z, 22.f, kEpsilon);
}

TEST(DataContainer, AppendOnlyAvailableWithWriteAccess) {
  const uint8_t data_to_append[1] = {1U};

  DataContainer write_data = CreateWriteDataContainer(16U);
  EXPECT_TRUE(write_data.Append(data_to_append, 1U));

  DataContainer read_data = CreateReadDataContainer(16U);
  EXPECT_FALSE(read_data.Append(data_to_append, 1U));
}

TEST(DataContainer, AppendIncreasesSize) {
  const VertexP data_to_append[2] = {VertexP(1.f, 1.f, 1.f),
                                     VertexP(2.f, 2.f, 2.f)};

  DataContainer data = CreateWriteDataContainer(16U * sizeof(VertexP));
  EXPECT_EQ(data.GetSize(), 0U);
  EXPECT_TRUE(data.Append(reinterpret_cast<const uint8_t*>(data_to_append),
                          2U * sizeof(VertexP)));
  EXPECT_EQ(data.GetSize(), 2U * sizeof(VertexP));
}

TEST(DataContainer, AppendAvailableForMaxSize) {
  const VertexP data_to_append[2] = {VertexP(1.f, 1.f, 1.f),
                                     VertexP(2.f, 2.f, 2.f)};

  DataContainer data =
      DataContainer::CreateHeapDataContainer(2U * sizeof(VertexP));
  EXPECT_TRUE(data.Append(reinterpret_cast<const uint8_t*>(data_to_append),
                          2U * sizeof(VertexP)));
}

TEST(DataContainer, AppendUnavailableWithNoCapacity) {
  const uint8_t data_to_append[3] = {1U, 2U, 3U};

  DataContainer data = DataContainer::CreateHeapDataContainer(2U);
  EXPECT_FALSE(data.Append(data_to_append, 3U));
  EXPECT_TRUE(data.Append(data_to_append, 2U));
  EXPECT_FALSE(data.Append(data_to_append, 1U));
}

TEST(DataContainer, AppendDataIsReadable) {
  const VertexP data_to_append[2] = {VertexP(1.f, 2.f, 3.f),
                                     VertexP(4.f, 5.f, 6.f)};

  DataContainer data =
      DataContainer::CreateHeapDataContainer(2U * sizeof(VertexP));

  // Get read pointer first before appending to check that Append is actually
  // writing to the same data as the read pointer is pointing to.
  const VertexP* vertex_read_ptr =
      reinterpret_cast<const VertexP*>(data.GetReadPtr());
  ASSERT_NE(vertex_read_ptr, nullptr);

  EXPECT_TRUE(data.Append(reinterpret_cast<const uint8_t*>(data_to_append),
                          2U * sizeof(VertexP)));
  EXPECT_NEAR(vertex_read_ptr[0].x, 1.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[0].y, 2.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[0].z, 3.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[1].x, 4.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[1].y, 5.f, kEpsilon);
  EXPECT_NEAR(vertex_read_ptr[1].z, 6.f, kEpsilon);
}

TEST(DataContainer, AppendOverwritesMutatedData) {
  const uint8_t data_to_append[2] = {1U, 2U};

  DataContainer data = DataContainer::CreateHeapDataContainer(2U);

  uint8_t* mutable_data = data.GetData();
  mutable_data[0] = 100U;
  mutable_data[1] = 200U;

  EXPECT_TRUE(data.Append(data_to_append, 2U));
  EXPECT_EQ(mutable_data[0], 1U);
  EXPECT_EQ(mutable_data[1], 2U);
}

// If we try to append {1, 2, 3} into a too-small container of size 2, we want
// none of the elements to be appended, instead of the first two elements to be
// appended. This ensures that we leave things untouched when failures are
// happening.
TEST(DataContainer, AppendNoElementsAppendedWhenNotEnoughCapacity) {
  const uint8_t oversized_data[3] = {1U, 2U, 3U};

  DataContainer data = DataContainer::CreateHeapDataContainer(2U);
  // Write some bytes using the mutable data pointer so we know what values
  // to expect. If bytes are appended, they will overwrite these bytes.
  uint8_t* mutable_data = data.GetData();
  ASSERT_NE(mutable_data, nullptr);
  mutable_data[0] = 100U;
  mutable_data[1] = 200U;

  EXPECT_FALSE(data.Append(oversized_data, 3U));

  const uint8_t* readable_data = data.GetReadPtr();
  ASSERT_NE(readable_data, nullptr);
  EXPECT_EQ(readable_data[0], 100U);
  EXPECT_EQ(readable_data[1], 200U);
}

TEST(DataContainer, WrappingData) {
  uint8_t* bytes = new uint8_t[4];
  for (int i = 0; i < 4; i++) {
    bytes[i] = static_cast<uint8_t>(i);
  }

  DataContainer::DataPtr data_ptr(bytes,
                                  [](const uint8_t* ptr) { delete[] ptr; });
  const DataContainer data(std::move(data_ptr), 4U, 8U,
                           DataContainer::AccessFlags::kRead);
  EXPECT_EQ(data.GetSize(), 4U);
  EXPECT_EQ(data.GetCapacity(), 8U);

  const uint8_t* read_ptr = data.GetReadPtr();
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(read_ptr[i], static_cast<uint8_t>(i));
  }
}

TEST(DataContainer, WrappingDataSameSizeAsCapacity) {
  uint8_t* bytes = new uint8_t[4];
  for (int i = 0; i < 4; i++) {
    bytes[i] = static_cast<uint8_t>(i);
  }

  DataContainer::DataPtr data_ptr(bytes,
                                  [](const uint8_t* ptr) { delete[] ptr; });
  const DataContainer data(std::move(data_ptr), 4U, 4U,
                           DataContainer::AccessFlags::kRead);
  EXPECT_EQ(data.GetSize(), 4U);
  EXPECT_EQ(data.GetCapacity(), 4U);

  const uint8_t* read_ptr = data.GetReadPtr();
  for (int i = 0; i < 4; i++) {
    EXPECT_EQ(read_ptr[i], static_cast<uint8_t>(i));
  }
}

TEST(DataContainerDeathTest, WrappingDataLargerThanCapacity) {
  uint8_t* bytes = new uint8_t[8];
  for (int i = 0; i < 8; i++) {
    bytes[i] = static_cast<uint8_t>(i);
  }

  DataContainer::DataPtr data_ptr(bytes,
                                  [](const uint8_t* ptr) { delete[] ptr; });
  PORT_EXPECT_DEBUG_DEATH(
      const DataContainer data(std::move(data_ptr), 8U, 4U,
                               DataContainer::AccessFlags::kRead),
      "");
}

TEST(DataContainer, CreateHeapCopy) {
  constexpr size_t kTestSize = 8;
  DataContainer source = DataContainer::CreateHeapDataContainer(kTestSize);
  for (size_t i = 0; i < kTestSize; i++) {
    const uint8_t value = static_cast<uint8_t>(i);
    EXPECT_TRUE(source.Append(&value, sizeof(value)));
  }

  DataContainer copy = source.CreateHeapCopy();
  EXPECT_TRUE(copy.IsReadable());
  EXPECT_TRUE(copy.IsWritable());
  EXPECT_EQ(source.GetSize(), copy.GetSize());
  EXPECT_EQ(std::memcmp(source.GetReadPtr(), copy.GetReadPtr(), copy.GetSize()),
            0);
}

TEST(DataContainerDeathTest, CreateHeapCopyWithoutReadAccess) {
  constexpr size_t kTestSize = 8;
  uint8_t* bytes = new uint8_t[kTestSize];
  DataContainer::DataPtr data_ptr(bytes,
                                  [](const uint8_t* ptr) { delete[] ptr; });
  DataContainer unreadable(std::move(data_ptr), kTestSize,
                           DataContainer::AccessFlags::kWrite);

  PORT_EXPECT_DEBUG_DEATH(unreadable.CreateHeapCopy(), "");
}

}  // namespace
}  // namespace lull
