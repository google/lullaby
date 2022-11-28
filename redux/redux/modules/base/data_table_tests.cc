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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/base/data_table.h"

namespace redux {
namespace {

using ::testing::Eq;

struct Integer : DataColumn<int> {};
struct Float : DataColumn<float> {};
struct Boolean : DataColumn<bool> {};
struct String : DataColumn<std::string> {};

TEST(DataTable, TryEmplaceDefault) {
  DataTable<Integer, Float, Boolean, String> map;
  EXPECT_THAT(map.Size(), Eq(0));

  map.TryEmplace(1);
  map.TryEmplace(3);
  map.TryEmplace(5);
  EXPECT_THAT(map.Size(), Eq(3));

  auto value = map.FindRow(3);
  EXPECT_TRUE(value);
  EXPECT_THAT(value.Get<Integer>(), Eq(3));
  EXPECT_THAT(value.Get<Float>(), Eq(0.f));
  EXPECT_THAT(value.Get<Boolean>(), Eq(false));
  EXPECT_THAT(value.Get<String>(), Eq(""));
}

TEST(DataTable, TryEmplaceArgs) {
  DataTable<Integer, Float, Boolean, String> map;
  EXPECT_THAT(map.Size(), Eq(0));

  map.TryEmplace(1, 2.f, true, "hello");
  map.TryEmplace(3, 4.f, true, "world");
  map.TryEmplace(5, 6.f, false, "bubye");
  EXPECT_THAT(map.Size(), Eq(3));

  auto value = map.FindRow(1);
  EXPECT_TRUE(value);
  EXPECT_THAT(value.Get<Integer>(), Eq(1));
  EXPECT_THAT(value.Get<Float>(), Eq(2.f));
  EXPECT_THAT(value.Get<Boolean>(), Eq(true));
  EXPECT_THAT(value.Get<String>(), Eq("hello"));

  value = map.FindRow(3);
  EXPECT_TRUE(value);
  EXPECT_THAT(value.Get<Integer>(), Eq(3));
  EXPECT_THAT(value.Get<Float>(), Eq(4.f));
  EXPECT_THAT(value.Get<Boolean>(), Eq(true));
  EXPECT_THAT(value.Get<String>(), Eq("world"));

  value = map.FindRow(5);
  EXPECT_TRUE(value);
  EXPECT_THAT(value.Get<Integer>(), Eq(5));
  EXPECT_THAT(value.Get<Float>(), Eq(6.f));
  EXPECT_THAT(value.Get<Boolean>(), Eq(false));
  EXPECT_THAT(value.Get<String>(), Eq("bubye"));
}

TEST(DataTable, TryEmplaceDupe) {
  DataTable<Integer, Float, Boolean, String> map;
  EXPECT_THAT(map.Size(), Eq(0));

  map.TryEmplace(1);
  map.TryEmplace(3);
  map.TryEmplace(5);
  EXPECT_THAT(map.Size(), Eq(3));

  map.TryEmplace(3, 4.f, true, "world");
  EXPECT_THAT(map.Size(), Eq(3));

  auto value = map.FindRow(3);
  EXPECT_TRUE(value);
  EXPECT_THAT(value.Get<Integer>(), Eq(3));
  EXPECT_THAT(value.Get<Float>(), Eq(0.f));
  EXPECT_THAT(value.Get<Boolean>(), Eq(false));
  EXPECT_THAT(value.Get<String>(), Eq(""));
}

TEST(DataTable, Mutate) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  map.TryEmplace(3, 4.f, true, "world");
  map.TryEmplace(5, 6.f, false, "bubye");

  auto value = map.FindRow(3);
  EXPECT_THAT(value.Get<Integer>(), Eq(3));
  EXPECT_THAT(value.Get<Float>(), Eq(4.f));
  EXPECT_THAT(value.Get<Boolean>(), Eq(true));
  EXPECT_THAT(value.Get<String>(), Eq("world"));

  // value.Get<Integer>() = 8;  // compile error; cannot assign to const
  value.Get<Float>() = 10.f;
  value.Get<Boolean>() = false;
  value.Get<String>() = "updated";

  auto value2 = map.FindRow(3);
  EXPECT_THAT(value2.Get<Integer>(), Eq(3));
  EXPECT_THAT(value2.Get<Float>(), Eq(10.f));
  EXPECT_THAT(value2.Get<Boolean>(), Eq(false));
  EXPECT_THAT(value2.Get<String>(), Eq("updated"));

  value.Get<Float>() = 10.f;
  value.Get<Boolean>() = false;
  value.Get<String>() = "updated";
}

TEST(DataTable, Find) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  map.TryEmplace(3, 4.f, true, "world");
  map.TryEmplace(5, 6.f, false, "bubye");

  auto value = map.Find<Float, String>(3);
  EXPECT_TRUE(value);
  EXPECT_FALSE(value.Has<Integer>());
  EXPECT_TRUE(value.Has<Float>());
  EXPECT_THAT(value.Get<Float>(), Eq(4.f));
  EXPECT_FALSE(value.Has<Boolean>());
  EXPECT_TRUE(value.Has<String>());
  EXPECT_THAT(value.Get<String>(), Eq("world"));

  value = std::make_tuple(10.f, "updated");

  auto value2 = map.Find<Float, String>(3);
  EXPECT_TRUE(value2);
  EXPECT_FALSE(value2.Has<Integer>());
  EXPECT_TRUE(value2.Has<Float>());
  EXPECT_THAT(value2.Get<Float>(), Eq(10.f));
  EXPECT_FALSE(value2.Has<Boolean>());
  EXPECT_TRUE(value2.Has<String>());
  EXPECT_THAT(value2.Get<String>(), Eq("updated"));
}

TEST(DataTable, Contains) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1);
  map.TryEmplace(3);
  map.TryEmplace(5);
  EXPECT_TRUE(map.Contains(1));
  EXPECT_FALSE(map.Contains(2));
  EXPECT_TRUE(map.Contains(3));
}

TEST(DataTable, Clear) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1);
  map.TryEmplace(3);
  map.TryEmplace(5);
  EXPECT_THAT(map.Size(), Eq(3));

  map.Clear();
  EXPECT_THAT(map.Size(), Eq(0));
}

TEST(DataTable, Erase) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1);
  map.TryEmplace(3);
  map.TryEmplace(5);
  EXPECT_THAT(map.Size(), Eq(3));
  EXPECT_TRUE(map.Contains(1));
  EXPECT_TRUE(map.Contains(3));

  map.Erase(2);  // no such key, should be a no-op
  EXPECT_THAT(map.Size(), Eq(3));
  EXPECT_TRUE(map.Contains(1));
  EXPECT_TRUE(map.Contains(3));

  map.Erase(3);
  EXPECT_THAT(map.Size(), Eq(2));
  EXPECT_TRUE(map.Contains(1));
  EXPECT_FALSE(map.Contains(3));
}

TEST(DataTable, Swap) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  map.TryEmplace(3, 4.f, true, "world");
  map.TryEmplace(5, 6.f, false, "bubye");

  const float v1 = map.At(0).Get<Float>();
  const float v2 = map.At(2).Get<Float>();
  map.Swap(0, 2);

  const float v3 = map.At(0).Get<Float>();
  const float v4 = map.At(2).Get<Float>();
  EXPECT_THAT(v3, Eq(v2));
  EXPECT_THAT(v4, Eq(v1));

  map.Swap(2, 2);  // should be a no-op

  const float v5 = map.At(0).Get<Float>();
  const float v6 = map.At(2).Get<Float>();
  EXPECT_THAT(v5, Eq(v2));
  EXPECT_THAT(v6, Eq(v1));
}

TEST(DataTable, At) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  map.TryEmplace(3, 4.f, true, "world");
  map.TryEmplace(5, 6.f, false, "bubye");

  float sum = 0.f;
  for (std::size_t i = 0; i < map.Size(); ++i) {
    auto row = map.At(i);
    sum += row.Get<Float>();
  }
  EXPECT_THAT(sum, Eq(2.f + 4.f + 6.f));
}

TEST(DataTable, RangeBasedForLoop) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  map.TryEmplace(3, 4.f, true, "world");
  map.TryEmplace(5, 6.f, false, "bubye");

  float sum = 0.f;
  for (auto row : map) {
    sum += row.Get<Float>();
  }
  EXPECT_THAT(sum, Eq(2.f + 4.f + 6.f));
}

TEST(DataTable, RangeBasedForLoopConst) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  map.TryEmplace(3, 4.f, true, "world");
  map.TryEmplace(5, 6.f, false, "bubye");
  const auto& cmap = map;

  float sum = 0.f;
  for (const auto& row : cmap) {
    sum += row.Get<Float>();
  }
  EXPECT_THAT(sum, Eq(2.f + 4.f + 6.f));
}

TEST(DataTable, ForEach) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  map.TryEmplace(3, 4.f, true, "world");
  map.TryEmplace(5, 6.f, false, "bubye");

  float sum = 0.f;
  map.ForEach<Float>([&](float v) { sum += v; });
  EXPECT_THAT(sum, Eq(2.f + 4.f + 6.f));
}

TEST(DataTable, MultiplePages) {
  DataTable<Integer, Float, Boolean, String> map;
  int expect = 0;
  for (int i = 0; i < 1000; ++i) {
    map.TryEmplace(i);
    expect += i;
  }
  EXPECT_THAT(map.Size(), Eq(1000));
  for (int i = 400; i < 800; i += 5) {
    map.Erase(i);
    expect -= i;
  }
  EXPECT_THAT(map.Size(), Eq(920));

  int sum = 0;
  int count = 0;
  map.ForEach<Integer>([&](int value) {
    sum += value;
    ++count;
  });

  EXPECT_THAT(sum, Eq(expect));
  EXPECT_THAT(map.Size(), Eq(count));
}

TEST(RefTuple, StructuredBindingsOutOfOrder) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  map.TryEmplace(3, 4.f, true, "world");
  map.TryEmplace(5, 6.f, false, "bubye");

  {
    auto [b, a] = map.Find<Float, Integer>(5);
    EXPECT_THAT(a, Eq(5));
    EXPECT_THAT(b, Eq(6.f));
  }
  {
    auto [b, a, d, c] = map.Find<Float, Integer, String, Float>(5);
    EXPECT_THAT(a, Eq(5));
    EXPECT_THAT(b, Eq(6.f));
    EXPECT_THAT(c, Eq(6.f));
    EXPECT_THAT(d, Eq("bubye"));
  }
}

}  // namespace
}  // namespace redux
