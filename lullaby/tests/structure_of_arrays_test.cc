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

#include "lullaby/util/structure_of_arrays.h"

#include <string>
#include "gtest/gtest.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using std::string;

TEST(StructureOfArrays, Push) {
  StructureOfArrays<string, int> soa;
  soa.Push("One", 1);
  soa.Emplace("Two", 2);

  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);

  EXPECT_EQ((soa.At<0>(1)), "Two");
  EXPECT_EQ((soa.At<1>(1)), 2);

  EXPECT_NE((soa.At<0>(0)), "Two");
  EXPECT_NE((soa.At<1>(0)), 2);

  EXPECT_NE((soa.At<0>(1)), "One");
  EXPECT_NE((soa.At<1>(1)), 1);
}

TEST(StructureOfArrays, Pop) {
  StructureOfArrays<string, int> soa;
  soa.Push("One", 1);
  soa.Push("Two", 2);

  EXPECT_EQ(soa.Size(), size_t(2));

  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);

  EXPECT_EQ((soa.At<0>(1)), "Two");
  EXPECT_EQ((soa.At<1>(1)), 2);

  soa.Pop();
  EXPECT_EQ(soa.Size(), size_t(1));
  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);
}

TEST(StructureOfArrays, PopTooMuch) {
  StructureOfArrays<string, int> soa;
  soa.Push("One", 1);

  EXPECT_EQ(soa.Size(), size_t(1));
  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);

  soa.Pop();
  EXPECT_EQ(soa.Size(), size_t(0));

  soa.Pop();
  EXPECT_EQ(soa.Size(), size_t(0));
}

TEST(StructureOfArrays, Erase) {
  StructureOfArrays<string, int> soa;
  soa.Push("One", 1);
  soa.Push("Two", 2);
  soa.Push("Three", 3);
  soa.Push("Four", 4);
  soa.Push("Five", 5);

  EXPECT_EQ(soa.Size(), size_t(5));

  soa.Erase(0);
  EXPECT_EQ(soa.Size(), size_t(4));
  EXPECT_EQ((soa.At<0>(0)), "Two");
  EXPECT_EQ((soa.At<1>(0)), 2);

  soa.Erase(1, 2);
  EXPECT_EQ(soa.Size(), size_t(2));
  EXPECT_EQ((soa.At<0>(0)), "Two");
  EXPECT_EQ((soa.At<1>(0)), 2);
  EXPECT_EQ((soa.At<0>(1)), "Five");
  EXPECT_EQ((soa.At<1>(1)), 5);
}

TEST(StructureOfArraysDeathTest, EraseOutOfBounds) {
  StructureOfArrays<string, int> soa;
  soa.Push("One", 1);

  EXPECT_EQ(soa.Size(), size_t(1));
  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);

  PORT_EXPECT_DEBUG_DEATH(soa.Erase(2), "");
  EXPECT_EQ(soa.Size(), size_t(1));
  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);

  PORT_EXPECT_DEBUG_DEATH(soa.Erase(2, 4), "");
  EXPECT_EQ(soa.Size(), size_t(1));
  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);
}

TEST(StructureOfArrays, Move) {
  using StringPtr = std::unique_ptr<std::string>;
  using Soa = StructureOfArrays<StringPtr>;

  Soa soa;
  soa.Emplace(StringPtr(new std::string("hello")));

  Soa other = std::move(soa);
  EXPECT_EQ(soa.Size(), size_t(0));
  EXPECT_EQ(other.Size(), size_t(1));
  EXPECT_EQ((*other.At<0>(0)), "hello");
}

TEST(StructureOfArrays, MoveAssign) {
  using StringPtr = std::unique_ptr<std::string>;
  using Soa = StructureOfArrays<StringPtr>;

  Soa soa;
  soa.Emplace(StringPtr(new std::string("hello")));

  Soa other;
  other = std::move(soa);
  EXPECT_EQ(soa.Size(), size_t(0));
  EXPECT_EQ(other.Size(), size_t(1));
  EXPECT_EQ((*other.At<0>(0)), "hello");
}

TEST(StructureOfArrays, Copy) {
  using Soa = StructureOfArrays<std::string>;

  Soa soa;
  soa.Emplace("hello");

  Soa other;
  EXPECT_EQ(soa.Size(), size_t(1));
  EXPECT_EQ(other.Size(), size_t(0));

  other = soa;
  EXPECT_EQ(soa.Size(), size_t(1));
  EXPECT_EQ(other.Size(), size_t(1));

  EXPECT_EQ((soa.At<0>(0)), "hello");
  EXPECT_EQ((other.At<0>(0)), "hello");
}

TEST(StructureOfArrays, CopyAssign) {
  using Soa = StructureOfArrays<std::string>;

  Soa soa;
  soa.Emplace("hello");

  Soa other(soa);
  EXPECT_EQ(soa.Size(), size_t(1));
  EXPECT_EQ(other.Size(), size_t(1));

  EXPECT_EQ((soa.At<0>(0)), "hello");
  EXPECT_EQ((other.At<0>(0)), "hello");
}

TEST(StructureOfArrays, GetNumElements) {
  StructureOfArrays<string, int> soa;
  EXPECT_EQ(soa.GetNumElements(), size_t(2));
}

TEST(StructureOfArrays, Swap) {
  StructureOfArrays<string, int> soa;
  soa.Push("One", 1);
  soa.Push("Two", 2);

  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);

  EXPECT_EQ((soa.At<0>(1)), "Two");
  EXPECT_EQ((soa.At<1>(1)), 2);

  soa.Swap(0, 1);

  EXPECT_EQ((soa.At<0>(0)), "Two");
  EXPECT_EQ((soa.At<1>(0)), 2);

  EXPECT_EQ((soa.At<0>(1)), "One");
  EXPECT_EQ((soa.At<1>(1)), 1);
}

TEST(StructureOfArrays, SwapSelf) {
  StructureOfArrays<string, int> soa;
  soa.Push("One", 1);

  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);

  soa.Swap(0, 0);

  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);
}

TEST(StructureOfArraysDeathTest, SwapOutOfBounds) {
  StructureOfArrays<string, int> soa;
  soa.Push("One", 1);

  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);

  PORT_EXPECT_DEBUG_DEATH(soa.Swap(0, 6), "");

  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);

  PORT_EXPECT_DEBUG_DEATH(soa.Swap(6, 0), "");

  EXPECT_EQ((soa.At<0>(0)), "One");
  EXPECT_EQ((soa.At<1>(0)), 1);
}

TEST(StructureOfArrays, Size) {
  StructureOfArrays<string, int> soa;
  EXPECT_EQ(soa.Size(), size_t(0));

  soa.Push("One", 1);
  soa.Push("Two", 2);
  EXPECT_EQ(soa.Size(), size_t(2));

  soa.Pop();
  EXPECT_EQ(soa.Size(), size_t(1));

  soa.Pop();
  EXPECT_EQ(soa.Size(), size_t(0));
}

TEST(StructureOfArrays, Resize) {
  StructureOfArrays<string, int> soa;
  EXPECT_EQ(soa.Size(), size_t(0));

  soa.Resize(10);
  EXPECT_EQ(soa.Size(), size_t(10));

  soa.Resize(1);
  EXPECT_EQ(soa.Size(), size_t(1));

  soa.Resize(0);
  EXPECT_EQ(soa.Size(), size_t(0));
}

TEST(StructureOfArrays, Empty) {
  StructureOfArrays<string, int> soa;
  EXPECT_TRUE(soa.Empty());

  soa.Push("One", 1);
  soa.Push("Two", 2);
  EXPECT_FALSE(soa.Empty());

  soa.Pop();
  EXPECT_FALSE(soa.Empty());

  soa.Pop();
  EXPECT_TRUE(soa.Empty());
}

TEST(StructureOfArrays, Data) {
  StructureOfArrays<string, int> soa;
  soa.Push("One", 1);
  soa.Push("Two", 2);

  const string* strings = soa.Data<0>();
  EXPECT_EQ(strings[0], "One");
  EXPECT_EQ(strings[1], "Two");

  const int* ints = soa.Data<1>();
  EXPECT_EQ(ints[0], 1);
  EXPECT_EQ(ints[1], 2);
}

}  // namespace
}  // namespace lull
