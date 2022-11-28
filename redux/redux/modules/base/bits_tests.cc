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
#include "redux/modules/base/bits.h"

namespace redux {
namespace {

using ::testing::Eq;
using ::testing::Ne;

TEST(BitsTest, StaticValues) {
  EXPECT_THAT(Bits8::None().Value(), Eq(0x00));
  EXPECT_THAT(Bits8::All().Value(), Eq(0xff));
  EXPECT_THAT(Bits8::Nth<4>().Value(), Eq(0x10));
}

TEST(BitsTest, Clear) {
  Bits8 bits = Bits8::All();
  EXPECT_TRUE(bits.Full());
  EXPECT_THAT(bits.Value(), Ne(0x00));

  bits.Clear();
  EXPECT_TRUE(bits.Empty());
  EXPECT_THAT(bits.Value(), Eq(0x00));
}

TEST(BitsTest, Flip) {
  Bits8 bits(0x0f);
  bits.Flip();
  EXPECT_THAT(bits.Value(), Eq(0xf0));
}

TEST(BitsTest, Functions) {
  Bits8 test_bit1 = Bits8::Nth<0>();
  Bits8 test_bit2 = Bits8::Nth<1>();

  Bits8 target_bits;

  EXPECT_FALSE(target_bits.Any(test_bit1));
  EXPECT_FALSE(target_bits.Any(test_bit2));

  target_bits.Set(test_bit1);

  EXPECT_TRUE(target_bits.Any(test_bit1));
  EXPECT_FALSE(target_bits.Any(test_bit2));

  target_bits.Set(test_bit2);

  EXPECT_TRUE(target_bits.Any(test_bit1));
  EXPECT_TRUE(target_bits.Any(test_bit2));

  target_bits.Clear(test_bit2);

  EXPECT_TRUE(target_bits.Any(test_bit1));
  EXPECT_FALSE(target_bits.Any(test_bit2));

  target_bits.Set(test_bit2);
  target_bits.Intersect(test_bit1);

  EXPECT_TRUE(target_bits.Any(test_bit1));
  EXPECT_FALSE(target_bits.Any(test_bit2));
}

}  // namespace
}  // namespace redux
