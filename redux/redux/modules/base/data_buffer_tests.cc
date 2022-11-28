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

#include <cstddef>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/types/span.h"
#include "redux/modules/base/data_buffer.h"

namespace redux {
namespace {

using ::testing::Eq;

static std::vector<std::byte> GenerateData(size_t n) {
  std::vector<std::byte> vec;
  for (size_t i = 0; i < n; ++i) {
    vec.push_back(static_cast<std::byte>(i));
  }
  return vec;
}

TEST(DataBuffer, Empty) {
  DataBuffer buffer;
  EXPECT_THAT(buffer.GetNumBytes(), Eq(0));
  EXPECT_THAT(buffer.GetByteSpan().size(), Eq(0));
}

TEST(DataBuffer, Small) {
  DataBuffer buffer;
  buffer.Assign(GenerateData(4));

  EXPECT_THAT(buffer.GetNumBytes(), Eq(4));
  EXPECT_THAT(buffer.GetByteSpan().size(), Eq(4));
  const std::byte* ptr = buffer.GetByteSpan().data();
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_THAT(ptr[i], Eq(static_cast<std::byte>(i)));
  }
}

TEST(DataBuffer, Big) {
  DataBuffer buffer;
  buffer.Assign(GenerateData(64));

  EXPECT_THAT(buffer.GetNumBytes(), Eq(64));
  EXPECT_THAT(buffer.GetByteSpan().size(), Eq(64));
  const std::byte* ptr = buffer.GetByteSpan().data();
  for (size_t i = 0; i < 64; ++i) {
    EXPECT_THAT(ptr[i], Eq(static_cast<std::byte>(i)));
  }
}

TEST(DataBuffer, SmallToBig) {
  DataBuffer buffer;
  buffer.Assign(GenerateData(4));
  EXPECT_THAT(buffer.GetNumBytes(), Eq(4));

  buffer.Assign(GenerateData(64));
  EXPECT_THAT(buffer.GetNumBytes(), Eq(64));
  EXPECT_THAT(buffer.GetByteSpan().size(), Eq(64));
  const std::byte* ptr = buffer.GetByteSpan().data();
  for (size_t i = 0; i < 64; ++i) {
    EXPECT_THAT(ptr[i], Eq(static_cast<std::byte>(i)));
  }
}

TEST(DataBuffer, BigToSmall) {
  DataBuffer buffer;
  buffer.Assign(GenerateData(64));
  EXPECT_THAT(buffer.GetNumBytes(), Eq(64));

  buffer.Assign(GenerateData(4));
  EXPECT_THAT(buffer.GetNumBytes(), Eq(4));
  EXPECT_THAT(buffer.GetByteSpan().size(), Eq(4));
  const std::byte* ptr = buffer.GetByteSpan().data();
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_THAT(ptr[i], Eq(static_cast<std::byte>(i)));
  }
}

TEST(DataBuffer, CopySmall) {
  DataBuffer buffer;
  buffer.Assign(GenerateData(4));
  EXPECT_THAT(buffer.GetNumBytes(), Eq(4));

  DataBuffer copy = buffer;
  EXPECT_THAT(copy.GetNumBytes(), Eq(4));
  EXPECT_THAT(copy.GetByteSpan().size(), Eq(4));
  const std::byte* ptr = copy.GetByteSpan().data();
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_THAT(ptr[i], Eq(static_cast<std::byte>(i)));
  }
}

TEST(DataBuffer, CopyBig) {
  DataBuffer buffer;
  buffer.Assign(GenerateData(64));
  EXPECT_THAT(buffer.GetNumBytes(), Eq(64));

  DataBuffer copy = buffer;
  EXPECT_THAT(copy.GetNumBytes(), Eq(64));
  EXPECT_THAT(copy.GetByteSpan().size(), Eq(64));
  const std::byte* ptr = copy.GetByteSpan().data();
  for (size_t i = 0; i < 64; ++i) {
    EXPECT_THAT(ptr[i], Eq(static_cast<std::byte>(i)));
  }
}

TEST(DataBuffer, AssignSmallSmall) {
  DataBuffer buffer1;
  buffer1.Assign(GenerateData(4));
  EXPECT_THAT(buffer1.GetNumBytes(), Eq(4));

  DataBuffer buffer2;
  buffer2.Assign(GenerateData(8));
  EXPECT_THAT(buffer2.GetNumBytes(), Eq(8));

  buffer2 = buffer1;
  EXPECT_THAT(buffer2.GetNumBytes(), Eq(4));
  EXPECT_THAT(buffer2.GetByteSpan().size(), Eq(4));
  const std::byte* ptr = buffer2.GetByteSpan().data();
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_THAT(ptr[i], Eq(static_cast<std::byte>(i)));
  }
}

TEST(DataBuffer, AssignBigBig) {
  DataBuffer buffer1;
  buffer1.Assign(GenerateData(64));
  EXPECT_THAT(buffer1.GetNumBytes(), Eq(64));

  DataBuffer buffer2;
  buffer2.Assign(GenerateData(128));
  EXPECT_THAT(buffer2.GetNumBytes(), Eq(128));

  buffer2 = buffer1;
  EXPECT_THAT(buffer2.GetNumBytes(), Eq(64));
  EXPECT_THAT(buffer2.GetByteSpan().size(), Eq(64));
  const std::byte* ptr = buffer2.GetByteSpan().data();
  for (size_t i = 0; i < 64; ++i) {
    EXPECT_THAT(ptr[i], Eq(static_cast<std::byte>(i)));
  }
}

TEST(DataBuffer, AssignSmallToBig) {
  DataBuffer buffer1;
  buffer1.Assign(GenerateData(4));
  EXPECT_THAT(buffer1.GetNumBytes(), Eq(4));

  DataBuffer buffer2;
  buffer2.Assign(GenerateData(128));
  EXPECT_THAT(buffer2.GetNumBytes(), Eq(128));

  buffer2 = buffer1;
  EXPECT_THAT(buffer2.GetNumBytes(), Eq(4));
  EXPECT_THAT(buffer2.GetByteSpan().size(), Eq(4));
  const std::byte* ptr = buffer2.GetByteSpan().data();
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_THAT(ptr[i], Eq(static_cast<std::byte>(i)));
  }
}

TEST(DataBuffer, AssignBigToSmall) {
  DataBuffer buffer1;
  buffer1.Assign(GenerateData(64));
  EXPECT_THAT(buffer1.GetNumBytes(), Eq(64));

  DataBuffer buffer2;
  buffer2.Assign(GenerateData(8));
  EXPECT_THAT(buffer2.GetNumBytes(), Eq(8));

  buffer2 = buffer1;
  EXPECT_THAT(buffer2.GetNumBytes(), Eq(64));
  EXPECT_THAT(buffer2.GetByteSpan().size(), Eq(64));
  const std::byte* ptr = buffer2.GetByteSpan().data();
  for (size_t i = 0; i < 64; ++i) {
    EXPECT_THAT(ptr[i], Eq(static_cast<std::byte>(i)));
  }
}

}  // namespace
}  // namespace redux
