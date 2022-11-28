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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/base/data_reader.h"

namespace redux {
namespace {

using ::testing::Eq;

DataReader FromString(const std::string& str) {
  const std::byte* ptr = reinterpret_cast<const std::byte*>(str.data());
  return DataReader::FromByteSpan({ptr, str.length()});
}

std::string ToString(const std::byte* ptr, size_t length) {
  const char* cstr = reinterpret_cast<const char*>(ptr);
  return std::string(cstr, length);
}

TEST(DataReaderTest, Empty) {
  DataReader reader;
  EXPECT_FALSE(reader.IsOpen());
  EXPECT_THAT(reader.GetTotalLength(), Eq(0));
}

TEST(DataReaderTest, Open) {
  const std::string str = "hello world";
  DataReader reader = FromString(str);
  EXPECT_TRUE(reader.IsOpen());
  EXPECT_THAT(reader.GetTotalLength(), Eq(11));
  EXPECT_THAT(reader.GetCurrentPosition(), Eq(0));
  EXPECT_FALSE(reader.IsAtEndOfStream());
}

TEST(DataReaderTest, SetCurrentPosition) {
  const std::string str = "hello world";
  DataReader reader = FromString(str);
  EXPECT_THAT(reader.GetCurrentPosition(), Eq(0));

  EXPECT_THAT(reader.SetCurrentPosition(3), Eq(3));
  EXPECT_THAT(reader.SetCurrentPosition(7), Eq(7));
  EXPECT_THAT(reader.SetCurrentPosition(15), Eq(11));
  EXPECT_TRUE(reader.IsAtEndOfStream());
}

TEST(DataReaderTest, Advance) {
  const std::string str = "hello world";
  DataReader reader = FromString(str);
  EXPECT_THAT(reader.GetCurrentPosition(), Eq(0));

  EXPECT_THAT(reader.Advance(3), Eq(3));
  EXPECT_THAT(reader.Advance(5), Eq(5));
  EXPECT_THAT(reader.Advance(5), Eq(3));
  EXPECT_TRUE(reader.IsAtEndOfStream());
}

TEST(DataReaderTest, Read) {
  const std::string str = "hello world";
  DataReader reader = FromString(str);

  std::vector<std::byte> buffer(30);
  EXPECT_THAT(reader.Read(buffer.data(), 5), Eq(5));
  EXPECT_THAT(ToString(buffer.data(), 5), Eq("hello"));

  EXPECT_THAT(reader.Read(buffer.data(), 5), Eq(5));
  EXPECT_THAT(ToString(buffer.data(), 5), Eq(" worl"));

  EXPECT_THAT(reader.Read(buffer.data(), 5), Eq(1));
  EXPECT_THAT(ToString(buffer.data(), 1), Eq("d"));
  EXPECT_TRUE(reader.IsAtEndOfStream());
}

TEST(DataReaderTest, Move) {
  const std::string str = "hello world";
  DataReader reader1 = FromString(str);
  DataReader reader2 = std::move(reader1);
  EXPECT_FALSE(reader1.IsOpen());
  EXPECT_THAT(reader1.GetTotalLength(), Eq(0));

  EXPECT_TRUE(reader2.IsOpen());
  EXPECT_THAT(reader2.GetTotalLength(), Eq(11));
}

}  // namespace
}  // namespace redux
