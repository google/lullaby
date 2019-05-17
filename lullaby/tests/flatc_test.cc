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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/lullaby/generated/flatc_generated.h"

// The main testing for flatc.cc is performed by flatbuffer_serializer_test.cc.
// The purpose of flatc.cc is to generate code that can be processed by the
// flatbuffer serializer.  As such there is very limited testing we can do
// for the code generator itself.

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::Not;

TEST(FlatcTest, VerifyIntegerTypes) {
  BasicsT b;
  EXPECT_THAT(sizeof(decltype(b.i8)), Eq(1u));
  EXPECT_THAT(sizeof(decltype(b.u8)), Eq(1u));
  EXPECT_THAT(sizeof(decltype(b.i16)), Eq(2u));
  EXPECT_THAT(sizeof(decltype(b.u16)), Eq(2u));
  EXPECT_THAT(sizeof(decltype(b.i32)), Eq(4u));
  EXPECT_THAT(sizeof(decltype(b.u32)), Eq(4u));
  EXPECT_THAT(sizeof(decltype(b.i64)), Eq(8u));
  EXPECT_THAT(sizeof(decltype(b.u64)), Eq(8u));

  EXPECT_THAT(alignof(decltype(b.i8)), Eq(1u));
  EXPECT_THAT(alignof(decltype(b.u8)), Eq(1u));
  EXPECT_THAT(alignof(decltype(b.i16)), Eq(2u));
  EXPECT_THAT(alignof(decltype(b.u16)), Eq(2u));
  EXPECT_THAT(alignof(decltype(b.i32)), Eq(4u));
  EXPECT_THAT(alignof(decltype(b.u32)), Eq(4u));
  EXPECT_THAT(alignof(decltype(b.i64)), Eq(8u));
  EXPECT_THAT(alignof(decltype(b.u64)), Eq(8u));

  EXPECT_THAT(std::is_integral<decltype(b.i8)>::value, Eq(true));
  EXPECT_THAT(std::is_integral<decltype(b.u8)>::value, Eq(true));
  EXPECT_THAT(std::is_integral<decltype(b.i16)>::value, Eq(true));
  EXPECT_THAT(std::is_integral<decltype(b.u16)>::value, Eq(true));
  EXPECT_THAT(std::is_integral<decltype(b.i32)>::value, Eq(true));
  EXPECT_THAT(std::is_integral<decltype(b.u32)>::value, Eq(true));
  EXPECT_THAT(std::is_integral<decltype(b.i64)>::value, Eq(true));
  EXPECT_THAT(std::is_integral<decltype(b.u64)>::value, Eq(true));

  EXPECT_THAT(std::is_signed<decltype(b.i8)>::value, Eq(true));
  EXPECT_THAT(std::is_unsigned<decltype(b.u8)>::value, Eq(true));
  EXPECT_THAT(std::is_signed<decltype(b.i16)>::value, Eq(true));
  EXPECT_THAT(std::is_unsigned<decltype(b.u16)>::value, Eq(true));
  EXPECT_THAT(std::is_signed<decltype(b.i32)>::value, Eq(true));
  EXPECT_THAT(std::is_unsigned<decltype(b.u32)>::value, Eq(true));
  EXPECT_THAT(std::is_signed<decltype(b.i64)>::value, Eq(true));
  EXPECT_THAT(std::is_unsigned<decltype(b.u64)>::value, Eq(true));
}

TEST(FlatcTest, VerifyFloatingPointTypes) {
  BasicsT b;
  EXPECT_THAT(sizeof(decltype(b.r32)), Eq(4u));
  EXPECT_THAT(sizeof(decltype(b.r64)), Eq(8u));

  EXPECT_THAT(alignof(decltype(b.r32)), Eq(4u));
  EXPECT_THAT(alignof(decltype(b.r64)), Eq(8u));

  EXPECT_THAT(std::is_floating_point<decltype(b.r32)>::value, Eq(true));
  EXPECT_THAT(std::is_floating_point<decltype(b.r64)>::value, Eq(true));
}

TEST(FlatcTest, VerifyStringType) {
  ComplexT c;
  EXPECT_THAT((std::is_same<std::string, decltype(c.name)>::value), Eq(true));
}

TEST(FlatcTest, VerifyArrayType) {
  ComplexT c;
  EXPECT_THAT((std::is_same<decltype(c.names)::value_type, std::string>::value),
              Eq(true));
  EXPECT_THAT((std::is_same<decltype(c.basics)::value_type, BasicsT>::value),
              Eq(true));
}

TEST(FlatcTest, VerifyDefaultValues) {
  ComplexT c;
  EXPECT_THAT(c.vec2.x, Eq(0.f));
  EXPECT_THAT(c.vec2.y, Eq(0.f));
  EXPECT_THAT(c.vec3.x, Eq(0.f));
  EXPECT_THAT(c.vec3.y, Eq(0.f));
  EXPECT_THAT(c.vec3.z, Eq(0.f));
  EXPECT_THAT(c.vec4.x, Eq(0.f));
  EXPECT_THAT(c.vec4.y, Eq(0.f));
  EXPECT_THAT(c.vec4.z, Eq(0.f));
  EXPECT_THAT(c.vec4.w, Eq(0.f));
}

TEST(FlatcTest, VerifyUnion) {
  VariantDefT var;
  EXPECT_THAT(var.get<DataStringT>(), IsNull());

  auto data = var.set<DataStringT>();
  EXPECT_THAT(data, Not(IsNull()));
  EXPECT_THAT(var.get<DataStringT>(), Eq(data));

  data->value = "hello";
  EXPECT_THAT(var.get<DataStringT>()->value, Eq("hello"));

  VariantDefT other;
  EXPECT_THAT(other.get<DataStringT>(), IsNull());

  other = var;
  EXPECT_THAT(var.get<DataStringT>()->value, Eq("hello"));
  EXPECT_THAT(other.get<DataStringT>()->value, Eq("hello"));

  other.get<DataStringT>()->value = "world";
  EXPECT_THAT(var.get<DataStringT>()->value, Eq("hello"));
  EXPECT_THAT(other.get<DataStringT>()->value, Eq("world"));

  other.reset();
  EXPECT_THAT(var.get<DataStringT>()->value, Eq("hello"));
  EXPECT_THAT(other.get<DataStringT>(), IsNull());

  var.reset();
  EXPECT_THAT(var.get<DataStringT>(), IsNull());

  auto invalid = var.set<BasicsT>();
  EXPECT_THAT(invalid, IsNull());
}

}  // namespace
}  // namespace lull
