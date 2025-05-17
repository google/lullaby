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
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace redux::tool {
namespace {

using ::testing::Eq;
using ::testing::Ne;

TEST(TypeId, Is) {
  const TypeId int_type = GetTypeId<int>();
  EXPECT_TRUE(Is<int>(int_type));
  EXPECT_FALSE(Is<float>(int_type));
}

TEST(TypeId, Comparison) {
  const TypeId int_type = GetTypeId<int>();
  const TypeId float_type = GetTypeId<float>();
  EXPECT_THAT(int_type, Eq(GetTypeId<int>()));
  EXPECT_THAT(float_type, Eq(GetTypeId<float>()));
  EXPECT_THAT(int_type, Ne(float_type));
}

TEST(TypeId, DiscardsQualifiers) {
  EXPECT_THAT(GetTypeId<int>(), Eq(GetTypeId<const int>()));
  EXPECT_THAT(GetTypeId<int>(), Eq(GetTypeId<const int&>()));
}

}  // namespace
}  // namespace redux::tool
