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

#include "lullaby/modules/flatbuffers/flatbuffer_reader.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/util.h"
#include "lullaby/util/math.h"
#include "lullaby/util/typeid.h"
#include "lullaby/generated/tools/flatc_generated.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;

template <typename T, typename Builder>
flatbuffers::Offset<T> Finish(Builder* builder) {
  auto offset = builder->Finish();
  builder->fbb_.Finish(offset);
  return offset;
}

lull::ComplexT Create(const flatbuffers::FlatBufferBuilder& fbb) {
  const uint8_t* buffer = fbb.GetBufferPointer();
  auto table = flatbuffers::GetRoot<flatbuffers::Table>(buffer);

  lull::ComplexT obj;
  ReadFlatbuffer(&obj, table);
  return obj;
}

TEST(FlatbufferReader, Tables) {
  flatbuffers::FlatBufferBuilder fbb;

  const auto str = fbb.CreateString("hello");

  BasicsBuilder basics(fbb);
  basics.add_u8(1);
  basics.add_i8(2);
  basics.add_u16(3);
  basics.add_i16(4);
  basics.add_u32(5);
  basics.add_i32(6);
  basics.add_u64(7);
  basics.add_i64(8);
  basics.add_r32(9.9f);
  basics.add_r64(10.01);
  basics.add_str(str);
  const auto basics_offset = Finish<Basics>(&basics);

  ComplexBuilder complex(fbb);
  complex.add_basic(basics_offset);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.basic.u8, Eq(1));
  EXPECT_THAT(obj.basic.i8, Eq(2));
  EXPECT_THAT(obj.basic.u16, Eq(3));
  EXPECT_THAT(obj.basic.i16, Eq(4));
  EXPECT_THAT(obj.basic.u32, Eq(5u));
  EXPECT_THAT(obj.basic.i32, Eq(6));
  EXPECT_THAT(obj.basic.u64, Eq(7ul));
  EXPECT_THAT(obj.basic.i64, Eq(8));
  EXPECT_THAT(obj.basic.r32, Eq(9.9f));
  EXPECT_THAT(obj.basic.r64, Eq(10.01));
  EXPECT_THAT(obj.basic.str, Eq("hello"));
}

TEST(FlatbufferReader, ArrayOfTables) {
  flatbuffers::FlatBufferBuilder fbb;

  flatbuffers::Offset<Basics> basics[2];

  const auto str1 = fbb.CreateString("hello");
  const auto str2 = fbb.CreateString("world");

  BasicsBuilder basics1(fbb);
  basics1.add_u8(1);
  basics1.add_i8(2);
  basics1.add_u16(3);
  basics1.add_i16(4);
  basics1.add_u32(5);
  basics1.add_i32(6);
  basics1.add_u64(7);
  basics1.add_i64(8);
  basics1.add_r32(9.9f);
  basics1.add_r64(10.01);
  basics1.add_str(str1);
  basics[0] = Finish<Basics>(&basics1);

  BasicsBuilder basics2(fbb);
  basics2.add_u8(10);
  basics2.add_i8(20);
  basics2.add_u16(30);
  basics2.add_i16(40);
  basics2.add_u32(50);
  basics2.add_i32(60);
  basics2.add_u64(70);
  basics2.add_i64(80);
  basics2.add_r32(90.09f);
  basics2.add_r64(100.001);
  basics2.add_str(str2);
  basics[1] = Finish<Basics>(&basics2);

  const auto vec = fbb.CreateVector(basics, 2);

  ComplexBuilder complex(fbb);
  complex.add_basics(vec);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.basics.size(), Eq(2ul));
  EXPECT_THAT(obj.basics[0].u8, Eq(1));
  EXPECT_THAT(obj.basics[0].i8, Eq(2));
  EXPECT_THAT(obj.basics[0].u16, Eq(3));
  EXPECT_THAT(obj.basics[0].i16, Eq(4));
  EXPECT_THAT(obj.basics[0].u32, Eq(5u));
  EXPECT_THAT(obj.basics[0].i32, Eq(6));
  EXPECT_THAT(obj.basics[0].u64, Eq(7ul));
  EXPECT_THAT(obj.basics[0].i64, Eq(8));
  EXPECT_THAT(obj.basics[0].r32, Eq(9.9f));
  EXPECT_THAT(obj.basics[0].r64, Eq(10.01));
  EXPECT_THAT(obj.basics[0].str, Eq("hello"));
  EXPECT_THAT(obj.basics[1].u8, Eq(10));
  EXPECT_THAT(obj.basics[1].i8, Eq(20));
  EXPECT_THAT(obj.basics[1].u16, Eq(30));
  EXPECT_THAT(obj.basics[1].i16, Eq(40));
  EXPECT_THAT(obj.basics[1].u32, Eq(50u));
  EXPECT_THAT(obj.basics[1].i32, Eq(60));
  EXPECT_THAT(obj.basics[1].u64, Eq(70ul));
  EXPECT_THAT(obj.basics[1].i64, Eq(80));
  EXPECT_THAT(obj.basics[1].r32, Eq(90.09f));
  EXPECT_THAT(obj.basics[1].r64, Eq(100.001));
  EXPECT_THAT(obj.basics[1].str, Eq("world"));
}

TEST(FlatbufferReader, Strings) {
  flatbuffers::FlatBufferBuilder fbb;

  const auto str = fbb.CreateString("hello");

  ComplexBuilder complex(fbb);
  complex.add_name(str);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.name, Eq("hello"));
}

TEST(FlatbufferReader, ArrayOfStrings) {
  flatbuffers::FlatBufferBuilder fbb;

  const std::vector<std::string> vec = {"hello", "world"};
  const auto arr = fbb.CreateVectorOfStrings(vec);

  ComplexBuilder complex(fbb);
  complex.add_names(arr);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.names.size(), Eq(2ul));
  EXPECT_THAT(obj.names[0], Eq("hello"));
  EXPECT_THAT(obj.names[1], Eq("world"));
}

TEST(FlatbufferReader, Structs) {
  flatbuffers::FlatBufferBuilder fbb;

  const InnerFixed inner(1, 2, 3);
  const MiddleFixed middle(4, inner, 5, 6);
  const OuterFixed outer(7.7f, middle, 8.8f, 9.9f);

  ComplexBuilder complex(fbb);
  complex.add_out(&outer);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.out.mid.in.a, Eq(1));
  EXPECT_THAT(obj.out.mid.in.b, Eq(2));
  EXPECT_THAT(obj.out.mid.in.c, Eq(3));
  EXPECT_THAT(obj.out.mid.t, Eq(4));
  EXPECT_THAT(obj.out.mid.u, Eq(5));
  EXPECT_THAT(obj.out.mid.v, Eq(6));
  EXPECT_THAT(obj.out.x, Eq(7.7f));
  EXPECT_THAT(obj.out.y, Eq(8.8f));
  EXPECT_THAT(obj.out.z, Eq(9.9f));
}

TEST(FlatbufferReader, ArrayOfStructs) {
  flatbuffers::FlatBufferBuilder fbb;

  const InnerFixed inner1(1, 2, 3);
  const MiddleFixed middle1(4, inner1, 5, 6);
  const OuterFixed outer1(7.7f, middle1, 8.8f, 9.9f);

  const InnerFixed inner2(10, 20, 30);
  const MiddleFixed middle2(40, inner2, 50, 60);
  const OuterFixed outer2(70.07f, middle2, 80.08f, 90.09f);

  OuterFixed arr[2] = {outer1, outer2};
  const auto vec = fbb.CreateVectorOfStructs(arr, 2);

  ComplexBuilder complex(fbb);
  complex.add_outs(vec);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.outs.size(), Eq(2ul));
  EXPECT_THAT(obj.outs[0].mid.in.a, Eq(1));
  EXPECT_THAT(obj.outs[0].mid.in.b, Eq(2));
  EXPECT_THAT(obj.outs[0].mid.in.c, Eq(3));
  EXPECT_THAT(obj.outs[0].mid.t, Eq(4));
  EXPECT_THAT(obj.outs[0].mid.u, Eq(5));
  EXPECT_THAT(obj.outs[0].mid.v, Eq(6));
  EXPECT_THAT(obj.outs[0].x, Eq(7.7f));
  EXPECT_THAT(obj.outs[0].y, Eq(8.8f));
  EXPECT_THAT(obj.outs[0].z, Eq(9.9f));
  EXPECT_THAT(obj.outs[1].mid.in.a, Eq(10));
  EXPECT_THAT(obj.outs[1].mid.in.b, Eq(20));
  EXPECT_THAT(obj.outs[1].mid.in.c, Eq(30));
  EXPECT_THAT(obj.outs[1].mid.t, Eq(40));
  EXPECT_THAT(obj.outs[1].mid.u, Eq(50));
  EXPECT_THAT(obj.outs[1].mid.v, Eq(60));
  EXPECT_THAT(obj.outs[1].x, Eq(70.07f));
  EXPECT_THAT(obj.outs[1].y, Eq(80.08f));
  EXPECT_THAT(obj.outs[1].z, Eq(90.09f));
}

TEST(FlatbufferReader, NativeTypes) {
  flatbuffers::FlatBufferBuilder fbb;
  const Vec2 v2(1.f, 2.f);
  const Vec3 v3(3.f, 4.f, 5.f);
  const Vec4 v4(6.f, 7.f, 8.f, 9.f);
  const Quat qt(10.f, 11.f, 12.f, 13.f);

  ComplexBuilder complex(fbb);
  complex.add_vec2(&v2);
  complex.add_vec3(&v3);
  complex.add_vec4(&v4);
  complex.add_quat(&qt);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.vec2.x, Eq(1.f));
  EXPECT_THAT(obj.vec2.y, Eq(2.f));
  EXPECT_THAT(obj.vec3.x, Eq(3.f));
  EXPECT_THAT(obj.vec3.y, Eq(4.f));
  EXPECT_THAT(obj.vec3.z, Eq(5.f));
  EXPECT_THAT(obj.vec4.x, Eq(6.f));
  EXPECT_THAT(obj.vec4.y, Eq(7.f));
  EXPECT_THAT(obj.vec4.z, Eq(8.f));
  EXPECT_THAT(obj.vec4.w, Eq(9.f));
  EXPECT_THAT(obj.quat.vector().x, Eq(10.f));
  EXPECT_THAT(obj.quat.vector().y, Eq(11.f));
  EXPECT_THAT(obj.quat.vector().z, Eq(12.f));
  EXPECT_THAT(obj.quat.scalar(), Eq(13.f));
}

TEST(FlatbufferReader, ArrayOfNativeTypes) {
  flatbuffers::FlatBufferBuilder fbb;

  const Vec2 v2s[2] = {Vec2(1.f, 2.f), Vec2(10.f, 20.f)};
  const Vec3 v3s[2] = {Vec3(3.f, 4.f, 5.f), Vec3(30.f, 40.f, 50.f)};
  const Vec4 v4s[2] = {Vec4(6.f, 7.f, 8.f, 9.f), Vec4(60.f, 70.f, 80.f, 90.f)};
  const Quat qts[2] = {Quat(10.f, 11.f, 12.f, 13.f),
                       Quat(100.f, 110.f, 120.f, 130.f)};

  const auto vec2s = fbb.CreateVectorOfStructs(v2s, 2);
  const auto vec3s = fbb.CreateVectorOfStructs(v3s, 2);
  const auto vec4s = fbb.CreateVectorOfStructs(v4s, 2);
  const auto quats = fbb.CreateVectorOfStructs(qts, 2);

  ComplexBuilder complex(fbb);
  complex.add_vec2s(vec2s);
  complex.add_vec3s(vec3s);
  complex.add_vec4s(vec4s);
  complex.add_quats(quats);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.vec2s.size(), Eq(2ul));
  EXPECT_THAT(obj.vec3s.size(), Eq(2ul));
  EXPECT_THAT(obj.vec4s.size(), Eq(2ul));
  EXPECT_THAT(obj.quats.size(), Eq(2ul));
  EXPECT_THAT(obj.vec2s[0].x, Eq(1.f));
  EXPECT_THAT(obj.vec2s[0].y, Eq(2.f));
  EXPECT_THAT(obj.vec3s[0].x, Eq(3.f));
  EXPECT_THAT(obj.vec3s[0].y, Eq(4.f));
  EXPECT_THAT(obj.vec3s[0].z, Eq(5.f));
  EXPECT_THAT(obj.vec4s[0].x, Eq(6.f));
  EXPECT_THAT(obj.vec4s[0].y, Eq(7.f));
  EXPECT_THAT(obj.vec4s[0].z, Eq(8.f));
  EXPECT_THAT(obj.vec4s[0].w, Eq(9.f));
  EXPECT_THAT(obj.quats[0].vector().x, Eq(10.f));
  EXPECT_THAT(obj.quats[0].vector().y, Eq(11.f));
  EXPECT_THAT(obj.quats[0].vector().z, Eq(12.f));
  EXPECT_THAT(obj.quats[0].scalar(), Eq(13.f));
  EXPECT_THAT(obj.vec2s[1].x, Eq(10.f));
  EXPECT_THAT(obj.vec2s[1].y, Eq(20.f));
  EXPECT_THAT(obj.vec3s[1].x, Eq(30.f));
  EXPECT_THAT(obj.vec3s[1].y, Eq(40.f));
  EXPECT_THAT(obj.vec3s[1].z, Eq(50.f));
  EXPECT_THAT(obj.vec4s[1].x, Eq(60.f));
  EXPECT_THAT(obj.vec4s[1].y, Eq(70.f));
  EXPECT_THAT(obj.vec4s[1].z, Eq(80.f));
  EXPECT_THAT(obj.vec4s[1].w, Eq(90.f));
  EXPECT_THAT(obj.quats[1].vector().x, Eq(100.f));
  EXPECT_THAT(obj.quats[1].vector().y, Eq(110.f));
  EXPECT_THAT(obj.quats[1].vector().z, Eq(120.f));
  EXPECT_THAT(obj.quats[1].scalar(), Eq(130.f));
}

TEST(FlatbufferReader, Unions) {
  flatbuffers::FlatBufferBuilder fbb;

  auto str = fbb.CreateString("hello");

  DataStringBuilder data(fbb);
  data.add_value(str);
  auto data_buffer = Finish<DataString>(&data).Union();

  ComplexBuilder complex(fbb);
  complex.add_variant(data_buffer);
  complex.add_variant_type(VariantDef_DataString);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.variant.get<DataStringT>(), NotNull());
  EXPECT_THAT(obj.variant.get<DataStringT>()->value, Eq("hello"));
}

TEST(FlatbufferReader, NullableEmpty) {
  flatbuffers::FlatBufferBuilder fbb;

  ComplexBuilder complex(fbb);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.nullable_struct.get(), IsNull());
  EXPECT_THAT(obj.nullable_table.get(), IsNull());
  EXPECT_THAT(obj.nullable_native.get(), IsNull());
}

TEST(FlatbufferReader, NullableStruct) {
  flatbuffers::FlatBufferBuilder fbb;

  const InnerFixed inner(1, 2, 3);
  ComplexBuilder complex(fbb);
  complex.add_nullable_struct(&inner);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.nullable_struct.get(), NotNull());
  EXPECT_THAT(obj.nullable_struct->a, Eq(1));
  EXPECT_THAT(obj.nullable_struct->b, Eq(2));
  EXPECT_THAT(obj.nullable_struct->c, Eq(3));
}

TEST(FlatbufferReader, NullableNativeStruct) {
  flatbuffers::FlatBufferBuilder fbb;

  const Vec2 vec2(1.f, 2.f);
  ComplexBuilder complex(fbb);
  complex.add_nullable_native(&vec2);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.nullable_native.get(), NotNull());
  EXPECT_THAT(obj.nullable_native->x, 1.f);
  EXPECT_THAT(obj.nullable_native->y, 2.f);
}

TEST(FlatbufferReader, NullableTable) {
  flatbuffers::FlatBufferBuilder fbb;

  const auto str = fbb.CreateString("hello");
  BasicsBuilder basic(fbb);
  basic.add_u8(1);
  basic.add_i8(2);
  basic.add_u16(3);
  basic.add_i16(4);
  basic.add_u32(5);
  basic.add_i32(6);
  basic.add_u64(7);
  basic.add_i64(8);
  basic.add_r32(9.9f);
  basic.add_r64(10.01);
  basic.add_str(str);
  const auto basic_offset = Finish<Basics>(&basic);

  ComplexBuilder complex(fbb);
  complex.add_nullable_table(basic_offset);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.nullable_table.get(), NotNull());
  EXPECT_THAT(obj.nullable_table->u8, Eq(1));
  EXPECT_THAT(obj.nullable_table->i8, Eq(2));
  EXPECT_THAT(obj.nullable_table->u16, Eq(3));
  EXPECT_THAT(obj.nullable_table->i16, Eq(4));
  EXPECT_THAT(obj.nullable_table->u32, Eq(5u));
  EXPECT_THAT(obj.nullable_table->i32, Eq(6));
  EXPECT_THAT(obj.nullable_table->u64, Eq(7ul));
  EXPECT_THAT(obj.nullable_table->i64, Eq(8));
  EXPECT_THAT(obj.nullable_table->r32, Eq(9.9f));
  EXPECT_THAT(obj.nullable_table->r64, Eq(10.01));
  EXPECT_THAT(obj.nullable_table->str, Eq("hello"));
}

TEST(FlatbufferReader, DynamicTable) {
  flatbuffers::FlatBufferBuilder fbb;

  const auto str = fbb.CreateString("hello");
  BasicsBuilder basic(fbb);
  basic.add_u8(1);
  basic.add_i8(2);
  basic.add_u16(3);
  basic.add_i16(4);
  basic.add_u32(5);
  basic.add_i32(6);
  basic.add_u64(7);
  basic.add_i64(8);
  basic.add_r32(9.9f);
  basic.add_r64(10.01);
  basic.add_str(str);
  const auto basic_offset = Finish<Basics>(&basic);

  ComplexBuilder dynamic(fbb);
  dynamic.add_basic(basic_offset);
  auto dynamic_offset = Finish<Complex>(&dynamic);

  ComplexBuilder complex(fbb);
  complex.add_dynamic_table(dynamic_offset);
  Finish<Complex>(&complex);

  const auto obj = Create(fbb);
  EXPECT_THAT(obj.dynamic_table.get(), NotNull());
  EXPECT_THAT(obj.dynamic_table->basic.u8, Eq(1));
  EXPECT_THAT(obj.dynamic_table->basic.i8, Eq(2));
  EXPECT_THAT(obj.dynamic_table->basic.u16, Eq(3));
  EXPECT_THAT(obj.dynamic_table->basic.i16, Eq(4));
  EXPECT_THAT(obj.dynamic_table->basic.u32, Eq(5u));
  EXPECT_THAT(obj.dynamic_table->basic.i32, Eq(6));
  EXPECT_THAT(obj.dynamic_table->basic.u64, Eq(7ul));
  EXPECT_THAT(obj.dynamic_table->basic.i64, Eq(8));
  EXPECT_THAT(obj.dynamic_table->basic.r32, Eq(9.9f));
  EXPECT_THAT(obj.dynamic_table->basic.r64, Eq(10.01));
  EXPECT_THAT(obj.dynamic_table->basic.str, Eq("hello"));
}

TEST(FlatbufferReader, All) {
  flatbuffers::FlatBufferBuilder fbb;

  // Structs.
  const InnerFixed inner(1, 2, 3);
  const MiddleFixed middle(4, inner, 5, 6);
  const OuterFixed outer(7.7f, middle, 8.8f, 9.9f);
  const InnerFixed inner1(1, 2, 3);
  const MiddleFixed middle1(4, inner1, 5, 6);
  const OuterFixed outer1(7.7f, middle1, 8.8f, 9.9f);
  const InnerFixed inner2(10, 20, 30);
  const MiddleFixed middle2(40, inner2, 50, 60);
  const OuterFixed outer2(70.07f, middle2, 80.08f, 90.09f);
  OuterFixed outers_arr[2] = {outer1, outer2};

  // Native types.
  const Vec2 vec2(1.f, 2.f);
  const Vec3 vec3(3.f, 4.f, 5.f);
  const Vec4 vec4(6.f, 7.f, 8.f, 9.f);
  const Quat quat(10.f, 11.f, 12.f, 13.f);
  const Vec2 vec2s_arr[2] = {Vec2(1.f, 2.f), Vec2(10.f, 20.f)};
  const Vec3 vec3s_arr[2] = {Vec3(3.f, 4.f, 5.f), Vec3(30.f, 40.f, 50.f)};
  const Vec4 vec4s_arr[2] = {Vec4(6.f, 7.f, 8.f, 9.f),
                             Vec4(60.f, 70.f, 80.f, 90.f)};
  const Quat quats_arr[2] = {Quat(10.f, 11.f, 12.f, 13.f),
                             Quat(100.f, 110.f, 120.f, 130.f)};

  // Strings.
  const std::vector<std::string> str_vec = {"hello", "world"};

  const auto str1 = fbb.CreateString("hello");
  const auto str2 = fbb.CreateString("world");
  const auto arr = fbb.CreateVectorOfStrings(str_vec);

  // Basics.
  BasicsBuilder basic(fbb);
  basic.add_u8(1);
  basic.add_i8(2);
  basic.add_u16(3);
  basic.add_i16(4);
  basic.add_u32(5);
  basic.add_i32(6);
  basic.add_u64(7);
  basic.add_i64(8);
  basic.add_r32(9.9f);
  basic.add_r64(10.01);
  basic.add_str(str1);
  const auto basic_offset = Finish<Basics>(&basic);

  BasicsBuilder basic1(fbb);
  basic1.add_u8(1);
  basic1.add_i8(2);
  basic1.add_u16(3);
  basic1.add_i16(4);
  basic1.add_u32(5);
  basic1.add_i32(6);
  basic1.add_u64(7);
  basic1.add_i64(8);
  basic1.add_r32(9.9f);
  basic1.add_r64(10.01);
  basic1.add_str(str1);
  const auto basic1_offset = Finish<Basics>(&basic1);

  BasicsBuilder basic2(fbb);
  basic2.add_u8(10);
  basic2.add_i8(20);
  basic2.add_u16(30);
  basic2.add_i16(40);
  basic2.add_u32(50);
  basic2.add_i32(60);
  basic2.add_u64(70);
  basic2.add_i64(80);
  basic2.add_r32(90.09f);
  basic2.add_r64(100.001);
  basic2.add_str(str2);
  const auto basic2_offset = Finish<Basics>(&basic2);

  flatbuffers::Offset<Basics> basics_arr[2] = {basic1_offset, basic2_offset};

  // Union.
  DataStringBuilder data_string(fbb);
  data_string.add_value(str1);
  auto variant_offset = Finish<DataString>(&data_string).Union();

  // Vectors
  const auto basics_offset = fbb.CreateVector(basics_arr, 2);
  const auto outers_offset = fbb.CreateVectorOfStructs(outers_arr, 2);
  const auto vec2s_offset = fbb.CreateVectorOfStructs(vec2s_arr, 2);
  const auto vec3s_offset = fbb.CreateVectorOfStructs(vec3s_arr, 2);
  const auto vec4s_offset = fbb.CreateVectorOfStructs(vec4s_arr, 2);
  const auto quats_offset = fbb.CreateVectorOfStructs(quats_arr, 2);

  // Build the root table.
  ComplexBuilder complex(fbb);

  complex.add_basic(basic_offset);
  complex.add_basics(basics_offset);
  complex.add_name(str1);
  complex.add_names(arr);
  complex.add_out(&outer);
  complex.add_outs(outers_offset);
  complex.add_vec2(&vec2);
  complex.add_vec3(&vec3);
  complex.add_vec4(&vec4);
  complex.add_quat(&quat);
  complex.add_vec2s(vec2s_offset);
  complex.add_vec3s(vec3s_offset);
  complex.add_vec4s(vec4s_offset);
  complex.add_quats(quats_offset);
  complex.add_variant(variant_offset);
  complex.add_variant_type(VariantDef_DataString);

  Finish<Complex>(&complex);

  // Load the flatbuffer and verify.
  const auto obj = Create(fbb);
  EXPECT_THAT(obj.basic.u8, Eq(1));
  EXPECT_THAT(obj.basic.i8, Eq(2));
  EXPECT_THAT(obj.basic.u16, Eq(3));
  EXPECT_THAT(obj.basic.i16, Eq(4));
  EXPECT_THAT(obj.basic.u32, Eq(5u));
  EXPECT_THAT(obj.basic.i32, Eq(6));
  EXPECT_THAT(obj.basic.u64, Eq(7ul));
  EXPECT_THAT(obj.basic.i64, Eq(8));
  EXPECT_THAT(obj.basic.r32, Eq(9.9f));
  EXPECT_THAT(obj.basic.r64, Eq(10.01));
  EXPECT_THAT(obj.basic.str, Eq("hello"));
  EXPECT_THAT(obj.basics.size(), Eq(2ul));
  EXPECT_THAT(obj.basics[0].u8, Eq(1));
  EXPECT_THAT(obj.basics[0].i8, Eq(2));
  EXPECT_THAT(obj.basics[0].u16, Eq(3));
  EXPECT_THAT(obj.basics[0].i16, Eq(4));
  EXPECT_THAT(obj.basics[0].u32, Eq(5u));
  EXPECT_THAT(obj.basics[0].i32, Eq(6));
  EXPECT_THAT(obj.basics[0].u64, Eq(7ul));
  EXPECT_THAT(obj.basics[0].i64, Eq(8));
  EXPECT_THAT(obj.basics[0].r32, Eq(9.9f));
  EXPECT_THAT(obj.basics[0].r64, Eq(10.01));
  EXPECT_THAT(obj.basics[0].str, Eq("hello"));
  EXPECT_THAT(obj.basics[1].u8, Eq(10));
  EXPECT_THAT(obj.basics[1].i8, Eq(20));
  EXPECT_THAT(obj.basics[1].u16, Eq(30));
  EXPECT_THAT(obj.basics[1].i16, Eq(40));
  EXPECT_THAT(obj.basics[1].u32, Eq(50u));
  EXPECT_THAT(obj.basics[1].i32, Eq(60));
  EXPECT_THAT(obj.basics[1].u64, Eq(70ul));
  EXPECT_THAT(obj.basics[1].i64, Eq(80));
  EXPECT_THAT(obj.basics[1].r32, Eq(90.09f));
  EXPECT_THAT(obj.basics[1].r64, Eq(100.001));
  EXPECT_THAT(obj.basics[1].str, Eq("world"));
  EXPECT_THAT(obj.name, Eq("hello"));
  EXPECT_THAT(obj.names.size(), Eq(2ul));
  EXPECT_THAT(obj.names[0], Eq("hello"));
  EXPECT_THAT(obj.names[1], Eq("world"));
  EXPECT_THAT(obj.out.mid.in.a, Eq(1));
  EXPECT_THAT(obj.out.mid.in.b, Eq(2));
  EXPECT_THAT(obj.out.mid.in.c, Eq(3));
  EXPECT_THAT(obj.out.mid.t, Eq(4));
  EXPECT_THAT(obj.out.mid.u, Eq(5));
  EXPECT_THAT(obj.out.mid.v, Eq(6));
  EXPECT_THAT(obj.out.x, Eq(7.7f));
  EXPECT_THAT(obj.out.y, Eq(8.8f));
  EXPECT_THAT(obj.out.z, Eq(9.9f));
  EXPECT_THAT(obj.outs.size(), Eq(2ul));
  EXPECT_THAT(obj.outs[0].mid.in.a, Eq(1));
  EXPECT_THAT(obj.outs[0].mid.in.b, Eq(2));
  EXPECT_THAT(obj.outs[0].mid.in.c, Eq(3));
  EXPECT_THAT(obj.outs[0].mid.t, Eq(4));
  EXPECT_THAT(obj.outs[0].mid.u, Eq(5));
  EXPECT_THAT(obj.outs[0].mid.v, Eq(6));
  EXPECT_THAT(obj.outs[0].x, Eq(7.7f));
  EXPECT_THAT(obj.outs[0].y, Eq(8.8f));
  EXPECT_THAT(obj.outs[0].z, Eq(9.9f));
  EXPECT_THAT(obj.outs[1].mid.in.a, Eq(10));
  EXPECT_THAT(obj.outs[1].mid.in.b, Eq(20));
  EXPECT_THAT(obj.outs[1].mid.in.c, Eq(30));
  EXPECT_THAT(obj.outs[1].mid.t, Eq(40));
  EXPECT_THAT(obj.outs[1].mid.u, Eq(50));
  EXPECT_THAT(obj.outs[1].mid.v, Eq(60));
  EXPECT_THAT(obj.outs[1].x, Eq(70.07f));
  EXPECT_THAT(obj.outs[1].y, Eq(80.08f));
  EXPECT_THAT(obj.outs[1].z, Eq(90.09f));
  EXPECT_THAT(obj.vec2.x, Eq(1.f));
  EXPECT_THAT(obj.vec2.y, Eq(2.f));
  EXPECT_THAT(obj.vec3.x, Eq(3.f));
  EXPECT_THAT(obj.vec3.y, Eq(4.f));
  EXPECT_THAT(obj.vec3.z, Eq(5.f));
  EXPECT_THAT(obj.vec4.x, Eq(6.f));
  EXPECT_THAT(obj.vec4.y, Eq(7.f));
  EXPECT_THAT(obj.vec4.z, Eq(8.f));
  EXPECT_THAT(obj.vec4.w, Eq(9.f));
  EXPECT_THAT(obj.quat.vector().x, Eq(10.f));
  EXPECT_THAT(obj.quat.vector().y, Eq(11.f));
  EXPECT_THAT(obj.quat.vector().z, Eq(12.f));
  EXPECT_THAT(obj.quat.scalar(), Eq(13.f));
  EXPECT_THAT(obj.vec2s.size(), Eq(2ul));
  EXPECT_THAT(obj.vec3s.size(), Eq(2ul));
  EXPECT_THAT(obj.vec4s.size(), Eq(2ul));
  EXPECT_THAT(obj.quats.size(), Eq(2ul));
  EXPECT_THAT(obj.vec2s[0].x, Eq(1.f));
  EXPECT_THAT(obj.vec2s[0].y, Eq(2.f));
  EXPECT_THAT(obj.vec3s[0].x, Eq(3.f));
  EXPECT_THAT(obj.vec3s[0].y, Eq(4.f));
  EXPECT_THAT(obj.vec3s[0].z, Eq(5.f));
  EXPECT_THAT(obj.vec4s[0].x, Eq(6.f));
  EXPECT_THAT(obj.vec4s[0].y, Eq(7.f));
  EXPECT_THAT(obj.vec4s[0].z, Eq(8.f));
  EXPECT_THAT(obj.vec4s[0].w, Eq(9.f));
  EXPECT_THAT(obj.quats[0].vector().x, Eq(10.f));
  EXPECT_THAT(obj.quats[0].vector().y, Eq(11.f));
  EXPECT_THAT(obj.quats[0].vector().z, Eq(12.f));
  EXPECT_THAT(obj.quats[0].scalar(), Eq(13.f));
  EXPECT_THAT(obj.vec2s[1].x, Eq(10.f));
  EXPECT_THAT(obj.vec2s[1].y, Eq(20.f));
  EXPECT_THAT(obj.vec3s[1].x, Eq(30.f));
  EXPECT_THAT(obj.vec3s[1].y, Eq(40.f));
  EXPECT_THAT(obj.vec3s[1].z, Eq(50.f));
  EXPECT_THAT(obj.vec4s[1].x, Eq(60.f));
  EXPECT_THAT(obj.vec4s[1].y, Eq(70.f));
  EXPECT_THAT(obj.vec4s[1].z, Eq(80.f));
  EXPECT_THAT(obj.vec4s[1].w, Eq(90.f));
  EXPECT_THAT(obj.quats[1].vector().x, Eq(100.f));
  EXPECT_THAT(obj.quats[1].vector().y, Eq(110.f));
  EXPECT_THAT(obj.quats[1].vector().z, Eq(120.f));
  EXPECT_THAT(obj.quats[1].scalar(), Eq(130.f));
  EXPECT_THAT(obj.variant.get<DataStringT>(), NotNull());
  EXPECT_THAT(obj.variant.get<DataStringT>()->value, Eq("hello"));
}

}  // namespace
}  // namespace lull
