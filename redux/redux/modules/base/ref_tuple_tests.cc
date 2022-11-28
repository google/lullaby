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

// RefTuple's exist primarily to work with DataTables, so we'll test RefTuple
// functionality using them. Note that DataTable::Row is a type-alias to a
// RefTuple for that DataTable.

struct Integer : DataColumn<int> {};
struct Float : DataColumn<float> {};
struct Boolean : DataColumn<bool> {};
struct String : DataColumn<std::string> {};

TEST(RefTuple, Has) {
  DataTable<Integer, Float, Boolean, String> map;
  auto tuple = map.FindRow(1);
  EXPECT_FALSE(tuple);
  EXPECT_FALSE(tuple.Has<Integer>());
  EXPECT_FALSE(tuple.Has<Float>());
  EXPECT_FALSE(tuple.Has<Boolean>());
  EXPECT_FALSE(tuple.Has<String>());

  map.TryEmplace(1, 2.f, true, "hello");
  tuple = map.FindRow(1);
  EXPECT_TRUE(tuple);
  EXPECT_TRUE(tuple.Has<Integer>());
  EXPECT_TRUE(tuple.Has<Float>());
  EXPECT_TRUE(tuple.Has<Boolean>());
  EXPECT_TRUE(tuple.Has<String>());
}

TEST(RefTuple, Get) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  auto tuple = map.FindRow(1);

  EXPECT_THAT(tuple.Get<Integer>(), Eq(1));
  EXPECT_THAT(tuple.Get<Float>(), Eq(2.0f));
  EXPECT_THAT(tuple.Get<Boolean>(), Eq(true));
  EXPECT_THAT(tuple.Get<String>(), Eq("hello"));
}

TEST(RefTuple, Nth) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  auto tuple = map.FindRow(1);

  EXPECT_THAT(tuple.Nth<0>(), Eq(1));
  EXPECT_THAT(tuple.Nth<1>(), Eq(2.0f));
  EXPECT_THAT(tuple.Nth<2>(), Eq(true));
  EXPECT_THAT(tuple.Nth<3>(), Eq("hello"));
}

TEST(RefTuple, Mutate) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  auto tuple = map.FindRow(1);

  // tuple.Get<Integer>() = 3;  Cannot modify key
  tuple.Get<Float>() = 4.0f;
  tuple.Get<Boolean>() = false;
  tuple.Get<String>() = "bye";

  auto tuple2 = map.FindRow(1);
  EXPECT_THAT(tuple2.Get<Integer>(), Eq(1));
  EXPECT_THAT(tuple2.Get<Float>(), Eq(4.0f));
  EXPECT_THAT(tuple2.Get<Boolean>(), Eq(false));
  EXPECT_THAT(tuple2.Get<String>(), Eq("bye"));
}

TEST(RefTuple, AssignFromTuple) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  auto tuple = map.Find<Float, Boolean, String>(1);

  tuple = std::make_tuple(4.0f, false, std::string("bye"));

  auto tuple2 = map.FindRow(1);
  EXPECT_THAT(tuple2.Get<Integer>(), Eq(1));
  EXPECT_THAT(tuple2.Get<Float>(), Eq(4.0f));
  EXPECT_THAT(tuple2.Get<Boolean>(), Eq(false));
  EXPECT_THAT(tuple2.Get<String>(), Eq("bye"));
}

TEST(RefTuple, SingleElement) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");

  auto tuple = map.Find<String>(1);
  EXPECT_TRUE(tuple);
  EXPECT_THAT(*tuple, Eq("hello"));

  tuple = "bye";

  auto tuple2 = map.Find<String>(1);
  EXPECT_TRUE(tuple2);
  EXPECT_THAT(*tuple2, Eq("bye"));
}

TEST(RefTuple, StructuredBindings) {
  DataTable<Integer, Float, Boolean, String> map;
  map.TryEmplace(1, 2.f, true, "hello");
  map.TryEmplace(3, 4.f, true, "world");
  map.TryEmplace(5, 6.f, false, "bubye");

  {
    auto [b] = map.Find<Float>(5);
    EXPECT_THAT(b, Eq(6.f));
  }
  {
    auto [a, b] = map.Find<Integer, Float>(5);
    EXPECT_THAT(a, Eq(5));
    EXPECT_THAT(b, Eq(6.f));
  }
  {
    auto [a, b, c] = map.Find<Integer, Float, String>(5);
    EXPECT_THAT(a, Eq(5));
    EXPECT_THAT(b, Eq(6.f));
    EXPECT_THAT(c, Eq("bubye"));
  }
  {
    auto [a, b, c, d] = map.Find<Integer, Float, Boolean, String>(5);
    EXPECT_THAT(a, Eq(5));
    EXPECT_THAT(b, Eq(6.f));
    EXPECT_THAT(c, Eq(false));
    EXPECT_THAT(d, Eq("bubye"));
  }
}

}  // namespace
}  // namespace redux
