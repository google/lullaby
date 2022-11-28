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
#include "redux/modules/flatbuffers/common.h"

namespace redux {
namespace {

using ::testing::Eq;

TEST(FbsCommon, HashVal) {
  const fbs::HashVal in(123);
  const HashValue out = ReadFbs(in);
  EXPECT_THAT(out.get(), Eq(123));
}

}  // namespace
}  // namespace redux
