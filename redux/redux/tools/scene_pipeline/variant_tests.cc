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

#include "redux/tools/scene_pipeline/type_id.h"
#include "redux/tools/scene_pipeline/variant.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace redux::tool {
namespace {

using ::testing::Eq;

TEST(Variant, DefaultIsEmpty) {
  Variant var;
  EXPECT_THAT(var.type(), Eq(kInvalidTypeId));
  EXPECT_THAT(var.size(), Eq(0));
  EXPECT_THAT(var.data(), Eq(nullptr));
}

TEST(Variant, FromValue) {
  Variant var(123);
  EXPECT_TRUE(var.is<int>());
  EXPECT_THAT(var.type(), Eq(GetTypeId<int>()));
  EXPECT_THAT(var.size(), Eq(1));

  auto span = var.span<int>();
  EXPECT_THAT(span.size(), Eq(1));
  EXPECT_THAT(span[0], Eq(123));
}

TEST(Variant, SetFromValue) {
  Variant var;
  var.Set(123);
  EXPECT_TRUE(var.is<int>());
  EXPECT_THAT(var.type(), Eq(GetTypeId<int>()));
  EXPECT_THAT(var.size(), Eq(1));

  auto span = var.span<int>();
  EXPECT_THAT(span.size(), Eq(1));
  EXPECT_THAT(span[0], Eq(123));
}

TEST(Variant, FromInitializerList) {
  Variant var({123, 456, 789});
  EXPECT_TRUE(var.is<int>());
  EXPECT_THAT(var.type(), Eq(GetTypeId<int>()));
  EXPECT_THAT(var.size(), Eq(3));

  auto span = var.span<int>();
  EXPECT_THAT(span.size(), Eq(3));
  EXPECT_THAT(span[0], Eq(123));
  EXPECT_THAT(span[1], Eq(456));
  EXPECT_THAT(span[2], Eq(789));
}

TEST(Variant, SetFromInitializerList) {
  Variant var;
  var.Set({123, 456, 789});
  EXPECT_TRUE(var.is<int>());
  EXPECT_THAT(var.type(), Eq(GetTypeId<int>()));
  EXPECT_THAT(var.size(), Eq(3));

  auto span = var.span<int>();
  EXPECT_THAT(span.size(), Eq(3));
  EXPECT_THAT(span[0], Eq(123));
  EXPECT_THAT(span[1], Eq(456));
  EXPECT_THAT(span[2], Eq(789));
}

TEST(Variant, Reset) {
  Variant var(123);
  EXPECT_TRUE(var.is<int>());
  EXPECT_THAT(var.size(), Eq(1));
  EXPECT_FALSE(var.span<int>().empty());

  var.reset();
  EXPECT_FALSE(var.is<int>());
  EXPECT_THAT(var.size(), Eq(0));
  EXPECT_TRUE(var.span<int>().empty());
}

TEST(Variant, WrongTypeAccessIsEmpty) {
  Variant var(123);
  EXPECT_FALSE(var.is<float>());
  EXPECT_TRUE(var.span<float>().empty());
}

}  // namespace
}  // namespace redux::tool
