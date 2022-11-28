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
#include "redux/modules/base/data_builder.h"
#include "redux/modules/base/data_container.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(DataBuilderTest, Empty) {
  DataBuilder builder(32);
  EXPECT_THAT(builder.GetSize(), Eq(0));
  EXPECT_THAT(builder.GetCapacity(), Eq(32));

  DataContainer data = builder.Release();
  EXPECT_THAT(builder.GetSize(), Eq(0));
  EXPECT_THAT(builder.GetCapacity(), Eq(0));
  EXPECT_THAT(data.GetBytes(), Eq(nullptr));
  EXPECT_THAT(data.GetNumBytes(), Eq(0));
}

TEST(DataBuilderTest, Append) {
  int values[] = {123, 456};

  DataBuilder builder(32);
  builder.Append(values, 2);
  EXPECT_THAT(builder.GetSize(), Eq(sizeof(values)));
  EXPECT_THAT(builder.GetCapacity(), Eq(32));

  DataContainer data = builder.Release();
  EXPECT_THAT(builder.GetSize(), Eq(0));
  EXPECT_THAT(builder.GetCapacity(), Eq(0));
  EXPECT_THAT(data.GetNumBytes(), Eq(sizeof(values)));

  const int* mem = reinterpret_cast<const int*>(data.GetBytes());
  for (size_t i = 0; i < 2; ++i) {
    EXPECT_THAT(mem[i], Eq(values[i]));
  }
}

TEST(DataBuilderTest, TooMuchData) {
  int values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

  DataBuilder builder(32);
  EXPECT_DEATH(builder.Append(values, 9), "");
}

}  // namespace
}  // namespace redux
