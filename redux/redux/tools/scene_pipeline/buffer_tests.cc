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

#include <array>
#include <cstddef>
#include <utility>

#include "redux/tools/scene_pipeline/buffer.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace redux::tool {
namespace {

using ::testing::Eq;
using ::testing::Ne;

TEST(Buffer, DefaultEmpty) {
  Buffer buffer;
  EXPECT_THAT(buffer.size(), Eq(0));
  EXPECT_THAT(buffer.data(), Eq(nullptr));
  EXPECT_TRUE(buffer.span().empty());
}

TEST(Buffer, SizedBuffer) {
  Buffer buffer(64);
  EXPECT_THAT(buffer.size(), Eq(64));
  EXPECT_THAT(buffer.data(), Ne(nullptr));
  EXPECT_THAT(buffer.span().size(), Eq(64));
}

TEST(Buffer, MoveConstructor) {
  Buffer source(64);
  const std::byte* ptr = source.data();

  Buffer target(std::move(source));
  EXPECT_THAT(target.size(), Eq(64));
  EXPECT_THAT(target.data(), Eq(ptr));
  EXPECT_THAT(source.size(), Eq(0));
  EXPECT_THAT(source.data(), Eq(nullptr));
}

TEST(Buffer, MoveAssignment) {
  Buffer source(64);
  const std::byte* ptr = source.data();

  Buffer target;
  target = std::move(source);
  EXPECT_THAT(target.size(), Eq(64));
  EXPECT_THAT(target.data(), Eq(ptr));
  EXPECT_THAT(source.size(), Eq(0));
  EXPECT_THAT(source.data(), Eq(nullptr));
}

TEST(Buffer, Reset) {
  Buffer buffer(64);
  EXPECT_THAT(buffer.size(), Eq(64));
  EXPECT_THAT(buffer.data(), Ne(nullptr));
  EXPECT_THAT(buffer.span().size(), Eq(64));

  buffer.reset();
  EXPECT_THAT(buffer.size(), Eq(0));
  EXPECT_THAT(buffer.data(), Eq(nullptr));
  EXPECT_TRUE(buffer.span().empty());
}

TEST(Buffer, Contains) {
  Buffer buffer(64);
  const std::byte* ptr = buffer.data();
  for (int i = 0; i < 64; ++i) {
    EXPECT_TRUE(buffer.contains(ptr + i));
  }
  EXPECT_FALSE(buffer.contains(ptr - 1));
  EXPECT_FALSE(buffer.contains(ptr + 64));
}

TEST(Buffer, SubSpan) {
  Buffer buffer(64);
  auto full_span = buffer.subspan(0, 64);
  EXPECT_THAT(full_span.data(), Eq(buffer.span().data()));
  EXPECT_THAT(full_span.size(), Eq(buffer.span().size()));

  auto head_span = buffer.subspan(0, 32);
  EXPECT_THAT(head_span.data(), Eq(buffer.span().data()));
  EXPECT_THAT(head_span.size(), Eq(32));

  auto tail_span = buffer.subspan(32, 32);
  EXPECT_THAT(tail_span.data(), Eq(buffer.span().data() + 32));
  EXPECT_THAT(tail_span.size(), Eq(32));

  auto sub_span = buffer.subspan(16, 32);
  EXPECT_THAT(sub_span.data(), Eq(buffer.span().data() + 16));
  EXPECT_THAT(sub_span.size(), Eq(32));
}

TEST(Buffer, Release) {
  Buffer buffer(64);
  const std::byte* original_ptr = buffer.data();
  const std::byte* released_ptr = buffer.release();
  EXPECT_THAT(released_ptr, Eq(original_ptr));
  EXPECT_THAT(buffer.size(), Eq(0));
  EXPECT_THAT(buffer.data(), Eq(nullptr));
  delete [] released_ptr;
}

TEST(Buffer, Own) {
  bool memory_deleted = false;

  std::byte* memory = new std::byte[64];
  Buffer buffer = Buffer::Own(memory, 64, [&memory_deleted](std::byte* ptr) {
    memory_deleted = true;
    delete[] ptr;
  });

  EXPECT_THAT(buffer.size(), Eq(64));
  EXPECT_THAT(buffer.data(), Eq(memory));
  EXPECT_THAT(buffer.span().size(), Eq(64));

  buffer.reset();
  EXPECT_TRUE(memory_deleted);
}

TEST(Buffer, Copy) {
  std::array<int, 3> data = {123, 456, 789};
  Buffer buffer = Buffer::Copy(data.data(), data.size());

  EXPECT_THAT(buffer.size(), Eq(data.size() * sizeof(int)));
  const int* ptr = reinterpret_cast<const int*>(buffer.data());
  for (int i = 0; i < data.size(); ++i) {
    EXPECT_THAT(ptr[i], Eq(data[i]));
  }
}

TEST(Buffer, SpanAs) {
  std::array<int, 3> data = {123, 456, 789};
  Buffer buffer = Buffer::Copy(data.data(), data.size());

  auto span = buffer.span_as<int>();
  EXPECT_THAT(span.size(), Eq(data.size()));
  for (int i = 0; i < data.size(); ++i) {
    EXPECT_THAT(span[i], Eq(data[i]));
  }
}

}  // namespace
}  // namespace redux::tool
