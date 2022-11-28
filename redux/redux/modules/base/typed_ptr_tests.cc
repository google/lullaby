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
#include "redux/modules/base/typed_ptr.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Not;

TEST(TypedPtrTest, Empty) {
  TypedPtr ptr;
  EXPECT_TRUE(ptr.Empty());
  EXPECT_FALSE(ptr);
}

TEST(TypedPtrTest, Set) {
  int value = 123;
  TypedPtr ptr(&value);;
  EXPECT_FALSE(ptr.Empty());
  EXPECT_TRUE(ptr);
}

TEST(TypedPtrTest, Reset) {
  int value = 123;
  TypedPtr ptr(&value);
  EXPECT_FALSE(ptr.Empty());
  EXPECT_TRUE(ptr);
  ptr.Reset();
  EXPECT_TRUE(ptr.Empty());
  EXPECT_FALSE(ptr);
}

TEST(TypedPtrTest, Is) {
  int value = 123;
  TypedPtr ptr(&value);
  EXPECT_TRUE(ptr.Is<int>());
  EXPECT_FALSE(ptr.Is<float>());
}

TEST(TypedPtrTest, Get) {
  int value = 123;
  TypedPtr ptr(&value);
  EXPECT_THAT(ptr.Get<int>(), Not(Eq(nullptr)));
  EXPECT_THAT(*ptr.Get<int>(), Eq(value));
}

TEST(TypedPtrTest, GetNull) {
  int value = 123;
  TypedPtr ptr(&value);
  EXPECT_THAT(ptr.Get<float>(), Eq(nullptr));
}

}  // namespace
}  // namespace redux
