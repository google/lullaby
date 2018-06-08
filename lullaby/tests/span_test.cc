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

#include "lullaby/util/span.h"

#include <sstream>
#include <unordered_map>

#include "gtest/gtest.h"

namespace lull {

TEST(SpanTest, DefaultCtor) {
  Span<int> span;
  EXPECT_EQ(0u, span.size());
  EXPECT_TRUE(span.empty());
  EXPECT_EQ(nullptr, span.data());
}

TEST(SpanTest, CArrayCtor) {
  const int arr[] = { 1, 2, 3 };

  Span<int> span(arr);

  EXPECT_EQ(3u, span.size());
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(arr, span.data());
  EXPECT_EQ(1, span[0]);
  EXPECT_EQ(2, span[1]);
  EXPECT_EQ(3, span[2]);
}

TEST(SpanTest, PointerCtor) {
  const int arr[] = { 1, 2, 3 };

  Span<int> span(arr, sizeof(arr) / sizeof(arr[0]));

  EXPECT_EQ(3u, span.size());
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(arr, span.data());
  EXPECT_EQ(1, span[0]);
  EXPECT_EQ(2, span[1]);
  EXPECT_EQ(3, span[2]);
}

TEST(SpanTest, StdArrayCtor) {
  std::array<int, 3> arr = {{ 1, 2, 3 }};
  Span<int> span(arr);

  EXPECT_EQ(3u, span.size());
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(arr.data(), span.data());
  EXPECT_EQ(1, span[0]);
  EXPECT_EQ(2, span[1]);
  EXPECT_EQ(3, span[2]);
}

TEST(SpanTest, StdVectorCtor) {
  std::vector<int> vec = { 1, 2, 3 };
  Span<int> span(vec);

  EXPECT_EQ(3u, span.size());
  EXPECT_FALSE(span.empty());
  EXPECT_EQ(vec.data(), span.data());
  EXPECT_EQ(1, span[0]);
  EXPECT_EQ(2, span[1]);
  EXPECT_EQ(3, span[2]);
}

TEST(SpanTest, Iteration) {
  std::array<int, 3> arr = {{ 1, 2, 3 }};
  Span<int> span(arr);

  int sum = 0;
  for (int value : span) {
    sum += value;
  }
  EXPECT_EQ(6, sum);
}


}  // namespace lull
