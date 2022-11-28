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
#include "absl/types/span.h"
#include "redux/modules/base/data_container.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Ne;

TEST(DataContainerTest, Empty) {
  DataContainer data;
  EXPECT_THAT(data.GetBytes(), Eq(nullptr));
  EXPECT_THAT(data.GetNumBytes(), Eq(0));
  EXPECT_THAT(data.GetByteSpan().data(), Eq(nullptr));
  EXPECT_THAT(data.GetByteSpan().size(), Eq(0));
}

TEST(DataContainerTest, WrapDataAsReadOnly) {
  int arr[] = {1, 2, 3, 4, 5, 6, 7, 8};

  DataContainer data =
      DataContainer::WrapData(arr, sizeof(arr) / sizeof(arr[0]));
  EXPECT_THAT(data.GetNumBytes(), Eq(sizeof(arr)));
  EXPECT_THAT(data.GetBytes(), Eq(reinterpret_cast<std::byte*>(arr)));
}

TEST(DataContainerTest, Clone) {
  int arr[] = {1, 2, 3, 4, 5, 6, 7, 8};

  DataContainer data =
      DataContainer::WrapData(arr, sizeof(arr) / sizeof(arr[0]));
  DataContainer clone = data.Clone();

  EXPECT_THAT(clone.GetBytes(), Ne(data.GetBytes()));
  EXPECT_THAT(clone.GetNumBytes(), Eq(data.GetNumBytes()));

  const int* mem = reinterpret_cast<const int*>(clone.GetBytes());
  for (size_t i = 0; i < 8; ++i) {
    EXPECT_THAT(mem[i], Eq(arr[i]));
  }
}

}  // namespace
}  // namespace redux
