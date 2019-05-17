/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/util/bits.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

TEST(Bits, Simple) {
  Bits test_bit1 = 1 << 0;
  Bits test_bit2 = 1 << 1;

  Bits target_bits = 0;

  EXPECT_FALSE(CheckBit(target_bits, test_bit1));
  EXPECT_FALSE(CheckBit(target_bits, test_bit2));

  target_bits = SetBit(target_bits, test_bit1);

  EXPECT_TRUE(CheckBit(target_bits, test_bit1));
  EXPECT_FALSE(CheckBit(target_bits, test_bit2));

  target_bits = SetBit(target_bits, test_bit2);

  EXPECT_TRUE(CheckBit(target_bits, test_bit1));
  EXPECT_TRUE(CheckBit(target_bits, test_bit2));

  target_bits = ClearBit(target_bits, test_bit2);

  EXPECT_TRUE(CheckBit(target_bits, test_bit1));
  EXPECT_FALSE(CheckBit(target_bits, test_bit2));
}

}  // namespace
}  // namespace lull
