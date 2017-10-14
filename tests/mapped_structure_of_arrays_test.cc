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

#include "lullaby/util/mapped_structure_of_arrays.h"

#include <string>
#include "gtest/gtest.h"
#include "tests/portable_test_macros.h"

namespace lull {
namespace {

using std::string;

TEST(MappedStructureOfArrays, Add) {
  MappedStructureOfArrays<string, int> soa;
  EXPECT_EQ(soa.Size(), size_t(0));

  soa.Insert("One");
  EXPECT_EQ(soa.Size(), size_t(1));

  soa.Insert("Two", 2);
  soa.Emplace("Three", 3);
  EXPECT_EQ(soa.Size(), size_t(3));

  EXPECT_EQ((soa.At<0>("Two")), 2);
}

TEST(MappedStructureOfArraysDeathTest, AddDupe) {
  MappedStructureOfArrays<string, int> soa;
  EXPECT_EQ(soa.Size(), size_t(0));

  soa.Insert("One");
  EXPECT_EQ(soa.Size(), size_t(1));

  PORT_EXPECT_DEBUG_DEATH(soa.Insert("One"), "");
  EXPECT_EQ(soa.Size(), size_t(1));

  soa.Insert("Two", 2);
  EXPECT_EQ(soa.Size(), size_t(2));

  PORT_EXPECT_DEBUG_DEATH(soa.Insert("Two", 2), "");
  EXPECT_EQ(soa.Size(), size_t(2));

  soa.Emplace("Three", 3);
  EXPECT_EQ(soa.Size(), size_t(3));

  PORT_EXPECT_DEBUG_DEATH(soa.Emplace("Three", 3), "");
  EXPECT_EQ(soa.Size(), size_t(3));
}

TEST(MappedStructureOfArrays, Size) {
  MappedStructureOfArrays<string, int> soa;
  EXPECT_EQ(soa.Size(), size_t(0));

  soa.Insert("One");
  EXPECT_EQ(soa.Size(), size_t(1));

  soa.Insert("Two", 2);
  soa.Insert("Three");
  EXPECT_EQ(soa.Size(), size_t(3));

  soa.Remove("Two");
  EXPECT_EQ(soa.Size(), size_t(2));
}

TEST(MappedStructureOfArrays, Remove) {
  MappedStructureOfArrays<string, int> soa;
  EXPECT_EQ(soa.Size(), size_t(0));

  soa.Insert("One");
  soa.Insert("Two", 2);
  soa.Insert("Three");
  EXPECT_EQ(soa.Contains("One"), true);
  EXPECT_EQ(soa.Contains("Two"), true);
  EXPECT_EQ(soa.Contains("Three"), true);

  soa.Remove("Two");
  EXPECT_EQ(soa.Size(), size_t(2));
  EXPECT_EQ(soa.Contains("One"), true);
  EXPECT_EQ(soa.Contains("Three"), true);
  EXPECT_EQ(soa.Contains("Two"), false);
}

TEST(MappedStructureOfArraysDeathTest, RemoveInvalid) {
  MappedStructureOfArrays<string, int> soa;
  EXPECT_EQ(soa.Size(), size_t(0));

  soa.Insert("One");
  EXPECT_EQ(soa.Contains("One"), true);

  PORT_EXPECT_DEBUG_DEATH(soa.Remove("Two"), "");
  EXPECT_EQ(soa.Contains("One"), true);
}

TEST(MappedStructureOfArrays, Swap) {
  MappedStructureOfArrays<string, int> soa;

  soa.Insert("One");
  soa.Insert("Two", 2);
  soa.Insert("Three");

  EXPECT_EQ(soa.GetIndex("One"), size_t(0));
  EXPECT_EQ(soa.GetIndex("Two"), size_t(1));

  soa.Swap(0, 1);
  EXPECT_EQ(soa.GetIndex("One"), size_t(1));
  EXPECT_EQ(soa.GetIndex("Two"), size_t(0));
}

TEST(MappedStructureOfArraysDeathTest, SwapOutOfBounds) {
  MappedStructureOfArrays<string, int> soa;

  soa.Insert("One");
  soa.Insert("Two", 2);
  soa.Insert("Three");

  EXPECT_EQ(soa.GetIndex("One"), size_t(0));
  EXPECT_EQ(soa.GetIndex("Two"), size_t(1));

  PORT_EXPECT_DEBUG_DEATH(soa.Swap(5, 6), "");
  EXPECT_EQ(soa.GetIndex("One"), size_t(0));
  EXPECT_EQ(soa.GetIndex("Two"), size_t(1));
}

TEST(MappedStructureOfArrays, Has) {
  MappedStructureOfArrays<string, int> soa;
  EXPECT_EQ(soa.Size(), size_t(0));

  EXPECT_EQ(soa.Contains("One"), false);

  soa.Insert("One");
  soa.Insert("Two", 2);
  soa.Insert("Three");
  EXPECT_EQ(soa.Contains("One"), true);
  EXPECT_EQ(soa.Contains("Two"), true);
  EXPECT_EQ(soa.Contains("Three"), true);

  soa.Remove("Two");
  EXPECT_EQ(soa.Size(), size_t(2));
  EXPECT_EQ(soa.Contains("One"), true);
  EXPECT_EQ(soa.Contains("Three"), true);
  EXPECT_EQ(soa.Contains("Two"), false);
}

TEST(MappedStructureOfArrays, GetIndex) {
  MappedStructureOfArrays<string, int> soa;

  soa.Insert("One");
  soa.Insert("Two", 2);
  soa.Insert("Three");

  EXPECT_EQ(soa.GetIndex("One"), size_t(0));
  EXPECT_EQ(soa.GetIndex("Two"), size_t(1));
}

TEST(MappedStructureOfArrays, At) {
  MappedStructureOfArrays<string, int> soa;

  soa.Emplace("One", 1);
  soa.Insert("Two", 2);
  soa.Insert("Three", 3);

  EXPECT_EQ((soa.At<0>("One")), 1);
  EXPECT_EQ((soa.At<0>("Two")), 2);
  EXPECT_EQ((soa.At<0>("Three")), 3);
}

}  // namespace
}  // namespace lull
