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

#include "lullaby/modules/flatbuffers/flatbuffer_writer.h"
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

TEST(FlatbufferWriter, Tables) {
  ComplexT obj;
  obj.basic.b = true;
  obj.basic.u8 = 1;
  obj.basic.i8 = 2;
  obj.basic.u16 = 3;
  obj.basic.i16 = 4;
  obj.basic.u32 = 5;
  obj.basic.i32 = 6;
  obj.basic.u64 = 7;
  obj.basic.i64 = 8;
  obj.basic.r32 = 9.9f;
  obj.basic.r64 = 10.01;
  obj.basic.str = "world";

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->basic()->b(), Eq(true));
  EXPECT_THAT(c->basic()->u8(), Eq(1));
  EXPECT_THAT(c->basic()->i8(), Eq(2));
  EXPECT_THAT(c->basic()->u16(), Eq(3));
  EXPECT_THAT(c->basic()->i16(), Eq(4));
  EXPECT_THAT(c->basic()->u32(), Eq(5u));
  EXPECT_THAT(c->basic()->i32(), Eq(6));
  EXPECT_THAT(c->basic()->u64(), Eq(7ul));
  EXPECT_THAT(c->basic()->i64(), Eq(8l));
  EXPECT_THAT(c->basic()->r32(), Eq(9.9f));
  EXPECT_THAT(c->basic()->r64(), Eq(10.01));
  EXPECT_THAT(c->basic()->str()->str(), Eq("world"));
}

TEST(FlatbufferWriter, ArrayOfTables) {
  ComplexT obj;
  obj.basics.resize(2);
  obj.basics[0].b = true;
  obj.basics[0].u8 = 1;
  obj.basics[0].i8 = 2;
  obj.basics[0].u16 = 3;
  obj.basics[0].i16 = 4;
  obj.basics[0].u32 = 5;
  obj.basics[0].i32 = 6;
  obj.basics[0].u64 = 7;
  obj.basics[0].i64 = 8;
  obj.basics[0].r32 = 9.9f;
  obj.basics[0].r64 = 10.01;
  obj.basics[0].str = "foo";
  obj.basics[1].b = false;
  obj.basics[1].u8 = 10;
  obj.basics[1].i8 = 20;
  obj.basics[1].u16 = 30;
  obj.basics[1].i16 = 40;
  obj.basics[1].u32 = 50;
  obj.basics[1].i32 = 60;
  obj.basics[1].u64 = 70;
  obj.basics[1].i64 = 80;
  obj.basics[1].r32 = 90.09f;
  obj.basics[1].r64 = 100.001;
  obj.basics[1].str = "bar";

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->basics()->size(), Eq(2u));
  EXPECT_THAT(c->basics()->Get(0)->b(), Eq(true));
  EXPECT_THAT(c->basics()->Get(0)->u8(), Eq(1));
  EXPECT_THAT(c->basics()->Get(0)->i8(), Eq(2));
  EXPECT_THAT(c->basics()->Get(0)->u16(), Eq(3));
  EXPECT_THAT(c->basics()->Get(0)->i16(), Eq(4));
  EXPECT_THAT(c->basics()->Get(0)->u32(), Eq(5u));
  EXPECT_THAT(c->basics()->Get(0)->i32(), Eq(6));
  EXPECT_THAT(c->basics()->Get(0)->u64(), Eq(7ul));
  EXPECT_THAT(c->basics()->Get(0)->i64(), Eq(8l));
  EXPECT_THAT(c->basics()->Get(0)->r32(), Eq(9.9f));
  EXPECT_THAT(c->basics()->Get(0)->r64(), Eq(10.01));
  EXPECT_THAT(c->basics()->Get(0)->str()->str(), Eq("foo"));
  EXPECT_THAT(c->basics()->Get(1)->b(), Eq(false));
  EXPECT_THAT(c->basics()->Get(1)->u8(), Eq(10));
  EXPECT_THAT(c->basics()->Get(1)->i8(), Eq(20));
  EXPECT_THAT(c->basics()->Get(1)->u16(), Eq(30));
  EXPECT_THAT(c->basics()->Get(1)->i16(), Eq(40));
  EXPECT_THAT(c->basics()->Get(1)->u32(), Eq(50u));
  EXPECT_THAT(c->basics()->Get(1)->i32(), Eq(60));
  EXPECT_THAT(c->basics()->Get(1)->u64(), Eq(70ul));
  EXPECT_THAT(c->basics()->Get(1)->i64(), Eq(80l));
  EXPECT_THAT(c->basics()->Get(1)->r32(), Eq(90.09f));
  EXPECT_THAT(c->basics()->Get(1)->r64(), Eq(100.001));
  EXPECT_THAT(c->basics()->Get(1)->str()->str(), Eq("bar"));
}

TEST(FlatbufferWriter, Strings) {
  ComplexT obj;
  obj.name = "hello";

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->name()->str(), Eq("hello"));
}

TEST(FlatbufferWriter, ArrayOfStrings) {
  ComplexT obj;
  obj.names = {"a", "bc", "def"};

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->names()->size(), Eq(3u));
  EXPECT_THAT(c->names()->Get(0)->str(), Eq("a"));
  EXPECT_THAT(c->names()->Get(1)->str(), Eq("bc"));
  EXPECT_THAT(c->names()->Get(2)->str(), Eq("def"));
}

TEST(FlatbufferWriter, Structs) {
  ComplexT obj;
  obj.out.mid.in.a = 1;
  obj.out.mid.in.b = 2;
  obj.out.mid.in.c = 3;
  obj.out.mid.t = 4;
  obj.out.mid.u = 5;
  obj.out.mid.v = 6;
  obj.out.x = 7.7f;
  obj.out.y = 8.8f;
  obj.out.z = 9.9f;

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->out()->mid().in().a(), Eq(1));
  EXPECT_THAT(c->out()->mid().in().b(), Eq(2));
  EXPECT_THAT(c->out()->mid().in().c(), Eq(3));
  EXPECT_THAT(c->out()->mid().t(), Eq(4));
  EXPECT_THAT(c->out()->mid().u(), Eq(5));
  EXPECT_THAT(c->out()->mid().v(), Eq(6));
  EXPECT_THAT(c->out()->x(), Eq(7.7f));
  EXPECT_THAT(c->out()->y(), Eq(8.8f));
  EXPECT_THAT(c->out()->z(), Eq(9.9f));
}

TEST(FlatbufferWriter, ArrayOfStructs) {
  ComplexT obj;
  obj.outs.resize(2);
  obj.outs[0].mid.in.a = 1;
  obj.outs[0].mid.in.b = 2;
  obj.outs[0].mid.in.c = 3;
  obj.outs[0].mid.t = 4;
  obj.outs[0].mid.u = 5;
  obj.outs[0].mid.v = 6;
  obj.outs[0].x = 7.7f;
  obj.outs[0].y = 8.8f;
  obj.outs[0].z = 9.9f;
  obj.outs[1].mid.in.a = 10;
  obj.outs[1].mid.in.b = 20;
  obj.outs[1].mid.in.c = 30;
  obj.outs[1].mid.t = 40;
  obj.outs[1].mid.u = 50;
  obj.outs[1].mid.v = 60;
  obj.outs[1].x = 70.07f;
  obj.outs[1].y = 80.08f;
  obj.outs[1].z = 90.09f;

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->outs()->size(), Eq(2u));
  EXPECT_THAT(c->outs()->Get(0)->mid().in().a(), Eq(1));
  EXPECT_THAT(c->outs()->Get(0)->mid().in().b(), Eq(2));
  EXPECT_THAT(c->outs()->Get(0)->mid().in().c(), Eq(3));
  EXPECT_THAT(c->outs()->Get(0)->mid().t(), Eq(4));
  EXPECT_THAT(c->outs()->Get(0)->mid().u(), Eq(5));
  EXPECT_THAT(c->outs()->Get(0)->mid().v(), Eq(6));
  EXPECT_THAT(c->outs()->Get(0)->x(), Eq(7.7f));
  EXPECT_THAT(c->outs()->Get(0)->y(), Eq(8.8f));
  EXPECT_THAT(c->outs()->Get(0)->z(), Eq(9.9f));
  EXPECT_THAT(c->outs()->Get(1)->mid().in().a(), Eq(10));
  EXPECT_THAT(c->outs()->Get(1)->mid().in().b(), Eq(20));
  EXPECT_THAT(c->outs()->Get(1)->mid().in().c(), Eq(30));
  EXPECT_THAT(c->outs()->Get(1)->mid().t(), Eq(40));
  EXPECT_THAT(c->outs()->Get(1)->mid().u(), Eq(50));
  EXPECT_THAT(c->outs()->Get(1)->mid().v(), Eq(60));
  EXPECT_THAT(c->outs()->Get(1)->x(), Eq(70.07f));
  EXPECT_THAT(c->outs()->Get(1)->y(), Eq(80.08f));
  EXPECT_THAT(c->outs()->Get(1)->z(), Eq(90.09f));
}

TEST(FlatbufferWriter, NativeTypes) {
  ComplexT obj;
  obj.vec2 = {1.f, 2.f};
  obj.vec3 = {3.f, 4.f, 5.f};
  obj.vec4 = {6.f, 7.f, 8.f, 9.f};
  obj.quat = {10.f, 11.f, 12.f, 13.f};

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->vec2()->x(), Eq(1.f));
  EXPECT_THAT(c->vec2()->y(), Eq(2.f));
  EXPECT_THAT(c->vec3()->x(), Eq(3.f));
  EXPECT_THAT(c->vec3()->y(), Eq(4.f));
  EXPECT_THAT(c->vec3()->z(), Eq(5.f));
  EXPECT_THAT(c->vec4()->x(), Eq(6.f));
  EXPECT_THAT(c->vec4()->y(), Eq(7.f));
  EXPECT_THAT(c->vec4()->z(), Eq(8.f));
  EXPECT_THAT(c->vec4()->w(), Eq(9.f));
  EXPECT_THAT(c->quat()->x(), Eq(11.f));
  EXPECT_THAT(c->quat()->y(), Eq(12.f));
  EXPECT_THAT(c->quat()->z(), Eq(13.f));
  EXPECT_THAT(c->quat()->w(), Eq(10.f));
}

TEST(FlatbufferWriter, ArrayOfNativeTypes) {
  ComplexT obj;
  obj.vec2s = {
      {1.f, 2.f}, {10.f, 20.f},
  };
  obj.vec3s = {
      {3.f, 4.f, 5.f}, {30.f, 40.f, 50.f},
  };
  obj.vec4s = {
      {6.f, 7.f, 8.f, 9.f}, {60.f, 70.f, 80.f, 90.f},
  };
  obj.quats = {
      {10.f, 11.f, 12.f, 13.f}, {10.01f, 11.11f, 12.21f, 13.31f},
  };

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->vec2s()->size(), Eq(2u));
  EXPECT_THAT(c->vec3s()->size(), Eq(2u));
  EXPECT_THAT(c->vec4s()->size(), Eq(2u));
  EXPECT_THAT(c->quats()->size(), Eq(2u));
  EXPECT_THAT(c->vec2s()->Get(0)->x(), Eq(1.f));
  EXPECT_THAT(c->vec2s()->Get(0)->y(), Eq(2.f));
  EXPECT_THAT(c->vec3s()->Get(0)->x(), Eq(3.f));
  EXPECT_THAT(c->vec3s()->Get(0)->y(), Eq(4.f));
  EXPECT_THAT(c->vec3s()->Get(0)->z(), Eq(5.f));
  EXPECT_THAT(c->vec4s()->Get(0)->x(), Eq(6.f));
  EXPECT_THAT(c->vec4s()->Get(0)->y(), Eq(7.f));
  EXPECT_THAT(c->vec4s()->Get(0)->z(), Eq(8.f));
  EXPECT_THAT(c->vec4s()->Get(0)->w(), Eq(9.f));
  EXPECT_THAT(c->quats()->Get(0)->x(), Eq(11.f));
  EXPECT_THAT(c->quats()->Get(0)->y(), Eq(12.f));
  EXPECT_THAT(c->quats()->Get(0)->z(), Eq(13.f));
  EXPECT_THAT(c->quats()->Get(0)->w(), Eq(10.f));
  EXPECT_THAT(c->vec2s()->Get(1)->x(), Eq(10.f));
  EXPECT_THAT(c->vec2s()->Get(1)->y(), Eq(20.f));
  EXPECT_THAT(c->vec3s()->Get(1)->x(), Eq(30.f));
  EXPECT_THAT(c->vec3s()->Get(1)->y(), Eq(40.f));
  EXPECT_THAT(c->vec3s()->Get(1)->z(), Eq(50.f));
  EXPECT_THAT(c->vec4s()->Get(1)->x(), Eq(60.f));
  EXPECT_THAT(c->vec4s()->Get(1)->y(), Eq(70.f));
  EXPECT_THAT(c->vec4s()->Get(1)->z(), Eq(80.f));
  EXPECT_THAT(c->vec4s()->Get(1)->w(), Eq(90.f));
  EXPECT_THAT(c->quats()->Get(1)->x(), Eq(11.11f));
  EXPECT_THAT(c->quats()->Get(1)->y(), Eq(12.21f));
  EXPECT_THAT(c->quats()->Get(1)->z(), Eq(13.31f));
  EXPECT_THAT(c->quats()->Get(1)->w(), Eq(10.01f));
}

TEST(FlatbufferWriter, Unions) {
  ComplexT obj;
  obj.variant.set<DataStringT>()->value = "baz";
  EXPECT_THAT(obj.variant.get<DataStringT>()->value, Eq("baz"));

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  // Ensure serialization didn't destroy the original.
  EXPECT_THAT(obj.variant.get<DataStringT>()->value, Eq("baz"));

  EXPECT_THAT(c->variant(), NotNull());
  EXPECT_THAT(c->variant_type(), Eq(VariantDef_DataString));

  const auto ds = reinterpret_cast<const DataString*>(c->variant());
  EXPECT_THAT(ds->value(), NotNull());
  EXPECT_THAT(ds->value()->str(), Eq("baz"));
}

TEST(FlatbufferWriter, NullableEmpty) {
  ComplexT obj;

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  LOG(ERROR) << c;

  EXPECT_THAT(c->nullable_struct(), IsNull());
  EXPECT_THAT(c->nullable_table(), IsNull());
  EXPECT_THAT(c->nullable_native(), IsNull());
}

TEST(FlatbufferWriter, NullableStruct) {
  ComplexT obj;
  obj.nullable_struct.emplace();
  obj.nullable_struct->a = 1;
  obj.nullable_struct->b = 2;
  obj.nullable_struct->c = 3;

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->nullable_struct(), NotNull());
  EXPECT_THAT(c->nullable_struct()->a(), Eq(1));
  EXPECT_THAT(c->nullable_struct()->b(), Eq(2));
  EXPECT_THAT(c->nullable_struct()->c(), Eq(3));
}

TEST(FlatbufferWriter, NullableNativeStruct) {
  ComplexT obj;
  obj.nullable_native.emplace();
  obj.nullable_native->x = 1.f;
  obj.nullable_native->y = 2.f;

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->nullable_native(), NotNull());
  EXPECT_THAT(c->nullable_native()->x(), Eq(1.f));
  EXPECT_THAT(c->nullable_native()->y(), Eq(2.f));
}

TEST(FlatbufferWriter, NullableTable) {
  ComplexT obj;
  obj.nullable_table.emplace();
  obj.nullable_table->b = true;
  obj.nullable_table->u8 = 1;
  obj.nullable_table->i8 = 2;
  obj.nullable_table->u16 = 3;
  obj.nullable_table->i16 = 4;
  obj.nullable_table->u32 = 5;
  obj.nullable_table->i32 = 6;
  obj.nullable_table->u64 = 7;
  obj.nullable_table->i64 = 8;
  obj.nullable_table->r32 = 9.9f;
  obj.nullable_table->r64 = 10.01;
  obj.nullable_table->str = "world";

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->nullable_table(), NotNull());
  EXPECT_THAT(c->nullable_table()->b(), Eq(true));
  EXPECT_THAT(c->nullable_table()->u8(), Eq(1));
  EXPECT_THAT(c->nullable_table()->i8(), Eq(2));
  EXPECT_THAT(c->nullable_table()->u16(), Eq(3));
  EXPECT_THAT(c->nullable_table()->i16(), Eq(4));
  EXPECT_THAT(c->nullable_table()->u32(), Eq(5u));
  EXPECT_THAT(c->nullable_table()->i32(), Eq(6));
  EXPECT_THAT(c->nullable_table()->u64(), Eq(7ul));
  EXPECT_THAT(c->nullable_table()->i64(), Eq(8l));
  EXPECT_THAT(c->nullable_table()->r32(), Eq(9.9f));
  EXPECT_THAT(c->nullable_table()->r64(), Eq(10.01));
  EXPECT_THAT(c->nullable_table()->str()->str(), Eq("world"));
}

TEST(FlatbufferWriter, DynamicTable) {
  ComplexT obj;
  obj.dynamic_table.reset(new ComplexT());
  obj.dynamic_table->basic.b = true;
  obj.dynamic_table->basic.u8 = 1;
  obj.dynamic_table->basic.i8 = 2;
  obj.dynamic_table->basic.u16 = 3;
  obj.dynamic_table->basic.i16 = 4;
  obj.dynamic_table->basic.u32 = 5;
  obj.dynamic_table->basic.i32 = 6;
  obj.dynamic_table->basic.u64 = 7;
  obj.dynamic_table->basic.i64 = 8;
  obj.dynamic_table->basic.r32 = 9.9f;
  obj.dynamic_table->basic.r64 = 10.01;
  obj.dynamic_table->basic.str = "world";

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->dynamic_table(), NotNull());
  EXPECT_THAT(c->dynamic_table()->basic()->b(), Eq(true));
  EXPECT_THAT(c->dynamic_table()->basic()->u8(), Eq(1));
  EXPECT_THAT(c->dynamic_table()->basic()->i8(), Eq(2));
  EXPECT_THAT(c->dynamic_table()->basic()->u16(), Eq(3));
  EXPECT_THAT(c->dynamic_table()->basic()->i16(), Eq(4));
  EXPECT_THAT(c->dynamic_table()->basic()->u32(), Eq(5u));
  EXPECT_THAT(c->dynamic_table()->basic()->i32(), Eq(6));
  EXPECT_THAT(c->dynamic_table()->basic()->u64(), Eq(7ul));
  EXPECT_THAT(c->dynamic_table()->basic()->i64(), Eq(8l));
  EXPECT_THAT(c->dynamic_table()->basic()->r32(), Eq(9.9f));
  EXPECT_THAT(c->dynamic_table()->basic()->r64(), Eq(10.01));
  EXPECT_THAT(c->dynamic_table()->basic()->str()->str(), Eq("world"));
}

TEST(FlatbufferWriter, All) {
  ComplexT obj;
  obj.name = "hello";
  obj.basic.b = true;
  obj.basic.u8 = 1;
  obj.basic.i8 = 2;
  obj.basic.u16 = 3;
  obj.basic.i16 = 4;
  obj.basic.u32 = 5;
  obj.basic.i32 = 6;
  obj.basic.u64 = 7;
  obj.basic.i64 = 8;
  obj.basic.r32 = 9.9f;
  obj.basic.r64 = 10.01;
  obj.basic.str = "world";
  obj.basics.resize(2);
  obj.basics[0].b = true;
  obj.basics[0].u8 = 1;
  obj.basics[0].i8 = 2;
  obj.basics[0].u16 = 3;
  obj.basics[0].i16 = 4;
  obj.basics[0].u32 = 5;
  obj.basics[0].i32 = 6;
  obj.basics[0].u64 = 7;
  obj.basics[0].i64 = 8;
  obj.basics[0].r32 = 9.9f;
  obj.basics[0].r64 = 10.01;
  obj.basics[0].str = "foo";
  obj.basics[1].b = false;
  obj.basics[1].u8 = 10;
  obj.basics[1].i8 = 20;
  obj.basics[1].u16 = 30;
  obj.basics[1].i16 = 40;
  obj.basics[1].u32 = 50;
  obj.basics[1].i32 = 60;
  obj.basics[1].u64 = 70;
  obj.basics[1].i64 = 80;
  obj.basics[1].r32 = 90.09f;
  obj.basics[1].r64 = 100.001;
  obj.basics[1].str = "bar";
  obj.out.mid.in.a = 1;
  obj.out.mid.in.b = 2;
  obj.out.mid.in.c = 3;
  obj.out.mid.t = 4;
  obj.out.mid.u = 5;
  obj.out.mid.v = 6;
  obj.out.x = 7.7f;
  obj.out.y = 8.8f;
  obj.out.z = 9.9f;
  obj.numbers = {1, 2, 3};
  obj.names = {"a", "bc", "def"};
  obj.outs.resize(2);
  obj.outs[0].mid.in.a = 1;
  obj.outs[0].mid.in.b = 2;
  obj.outs[0].mid.in.c = 3;
  obj.outs[0].mid.t = 4;
  obj.outs[0].mid.u = 5;
  obj.outs[0].mid.v = 6;
  obj.outs[0].x = 7.7f;
  obj.outs[0].y = 8.8f;
  obj.outs[0].z = 9.9f;
  obj.outs[1].mid.in.a = 10;
  obj.outs[1].mid.in.b = 20;
  obj.outs[1].mid.in.c = 30;
  obj.outs[1].mid.t = 40;
  obj.outs[1].mid.u = 50;
  obj.outs[1].mid.v = 60;
  obj.outs[1].x = 70.07f;
  obj.outs[1].y = 80.08f;
  obj.outs[1].z = 90.09f;
  obj.vec2 = {1.f, 2.f};
  obj.vec3 = {3.f, 4.f, 5.f};
  obj.vec4 = {6.f, 7.f, 8.f, 9.f};
  obj.quat = {10.f, 11.f, 12.f, 13.f};
  obj.vec2s = {
      {1.f, 2.f}, {10.f, 20.f},
  };
  obj.vec3s = {
      {3.f, 4.f, 5.f}, {30.f, 40.f, 50.f},
  };
  obj.vec4s = {
      {6.f, 7.f, 8.f, 9.f}, {60.f, 70.f, 80.f, 90.f},
  };
  obj.quats = {
      {10.f, 11.f, 12.f, 13.f}, {10.01f, 11.11f, 12.21f, 13.31f},
  };
  obj.variant.set<DataStringT>()->value = "baz";
  EXPECT_THAT(obj.variant.get<DataStringT>()->value, Eq("baz"));

  InwardBuffer buffer(32);
  const void* flatbuffer = WriteFlatbuffer(&obj, &buffer);
  const Complex* c = flatbuffers::GetRoot<Complex>(flatbuffer);

  EXPECT_THAT(c->name()->str(), Eq("hello"));
  EXPECT_THAT(c->basic()->b(), Eq(true));
  EXPECT_THAT(c->basic()->u8(), Eq(1));
  EXPECT_THAT(c->basic()->i8(), Eq(2));
  EXPECT_THAT(c->basic()->u16(), Eq(3));
  EXPECT_THAT(c->basic()->i16(), Eq(4));
  EXPECT_THAT(c->basic()->u32(), Eq(5u));
  EXPECT_THAT(c->basic()->i32(), Eq(6));
  EXPECT_THAT(c->basic()->u64(), Eq(7ul));
  EXPECT_THAT(c->basic()->i64(), Eq(8l));
  EXPECT_THAT(c->basic()->r32(), Eq(9.9f));
  EXPECT_THAT(c->basic()->r64(), Eq(10.01));
  EXPECT_THAT(c->basic()->str()->str(), Eq("world"));
  EXPECT_THAT(c->basics()->size(), Eq(2u));
  EXPECT_THAT(c->basics()->Get(0)->b(), Eq(true));
  EXPECT_THAT(c->basics()->Get(0)->u8(), Eq(1));
  EXPECT_THAT(c->basics()->Get(0)->i8(), Eq(2));
  EXPECT_THAT(c->basics()->Get(0)->u16(), Eq(3));
  EXPECT_THAT(c->basics()->Get(0)->i16(), Eq(4));
  EXPECT_THAT(c->basics()->Get(0)->u32(), Eq(5u));
  EXPECT_THAT(c->basics()->Get(0)->i32(), Eq(6));
  EXPECT_THAT(c->basics()->Get(0)->u64(), Eq(7ul));
  EXPECT_THAT(c->basics()->Get(0)->i64(), Eq(8l));
  EXPECT_THAT(c->basics()->Get(0)->r32(), Eq(9.9f));
  EXPECT_THAT(c->basics()->Get(0)->r64(), Eq(10.01));
  EXPECT_THAT(c->basics()->Get(0)->str()->str(), Eq("foo"));
  EXPECT_THAT(c->basics()->Get(1)->b(), Eq(false));
  EXPECT_THAT(c->basics()->Get(1)->u8(), Eq(10));
  EXPECT_THAT(c->basics()->Get(1)->i8(), Eq(20));
  EXPECT_THAT(c->basics()->Get(1)->u16(), Eq(30));
  EXPECT_THAT(c->basics()->Get(1)->i16(), Eq(40));
  EXPECT_THAT(c->basics()->Get(1)->u32(), Eq(50u));
  EXPECT_THAT(c->basics()->Get(1)->i32(), Eq(60));
  EXPECT_THAT(c->basics()->Get(1)->u64(), Eq(70ul));
  EXPECT_THAT(c->basics()->Get(1)->i64(), Eq(80l));
  EXPECT_THAT(c->basics()->Get(1)->r32(), Eq(90.09f));
  EXPECT_THAT(c->basics()->Get(1)->r64(), Eq(100.001));
  EXPECT_THAT(c->basics()->Get(1)->str()->str(), Eq("bar"));
  EXPECT_THAT(c->out()->mid().in().a(), Eq(1));
  EXPECT_THAT(c->out()->mid().in().b(), Eq(2));
  EXPECT_THAT(c->out()->mid().in().c(), Eq(3));
  EXPECT_THAT(c->out()->mid().t(), Eq(4));
  EXPECT_THAT(c->out()->mid().u(), Eq(5));
  EXPECT_THAT(c->out()->mid().v(), Eq(6));
  EXPECT_THAT(c->out()->x(), Eq(7.7f));
  EXPECT_THAT(c->out()->y(), Eq(8.8f));
  EXPECT_THAT(c->out()->z(), Eq(9.9f));
  EXPECT_THAT(c->numbers()->size(), Eq(3u));
  EXPECT_THAT(c->numbers()->Get(0), Eq(1));
  EXPECT_THAT(c->numbers()->Get(1), Eq(2));
  EXPECT_THAT(c->numbers()->Get(2), Eq(3));
  EXPECT_THAT(c->names()->size(), Eq(3u));
  EXPECT_THAT(c->names()->Get(0)->str(), Eq("a"));
  EXPECT_THAT(c->names()->Get(1)->str(), Eq("bc"));
  EXPECT_THAT(c->names()->Get(2)->str(), Eq("def"));
  EXPECT_THAT(c->outs()->size(), Eq(2u));
  EXPECT_THAT(c->outs()->Get(0)->mid().in().a(), Eq(1));
  EXPECT_THAT(c->outs()->Get(0)->mid().in().b(), Eq(2));
  EXPECT_THAT(c->outs()->Get(0)->mid().in().c(), Eq(3));
  EXPECT_THAT(c->outs()->Get(0)->mid().t(), Eq(4));
  EXPECT_THAT(c->outs()->Get(0)->mid().u(), Eq(5));
  EXPECT_THAT(c->outs()->Get(0)->mid().v(), Eq(6));
  EXPECT_THAT(c->outs()->Get(0)->x(), Eq(7.7f));
  EXPECT_THAT(c->outs()->Get(0)->y(), Eq(8.8f));
  EXPECT_THAT(c->outs()->Get(0)->z(), Eq(9.9f));
  EXPECT_THAT(c->outs()->Get(1)->mid().in().a(), Eq(10));
  EXPECT_THAT(c->outs()->Get(1)->mid().in().b(), Eq(20));
  EXPECT_THAT(c->outs()->Get(1)->mid().in().c(), Eq(30));
  EXPECT_THAT(c->outs()->Get(1)->mid().t(), Eq(40));
  EXPECT_THAT(c->outs()->Get(1)->mid().u(), Eq(50));
  EXPECT_THAT(c->outs()->Get(1)->mid().v(), Eq(60));
  EXPECT_THAT(c->outs()->Get(1)->x(), Eq(70.07f));
  EXPECT_THAT(c->outs()->Get(1)->y(), Eq(80.08f));
  EXPECT_THAT(c->outs()->Get(1)->z(), Eq(90.09f));
  EXPECT_THAT(c->vec2()->x(), Eq(1.f));
  EXPECT_THAT(c->vec2()->y(), Eq(2.f));
  EXPECT_THAT(c->vec3()->x(), Eq(3.f));
  EXPECT_THAT(c->vec3()->y(), Eq(4.f));
  EXPECT_THAT(c->vec3()->z(), Eq(5.f));
  EXPECT_THAT(c->vec4()->x(), Eq(6.f));
  EXPECT_THAT(c->vec4()->y(), Eq(7.f));
  EXPECT_THAT(c->vec4()->z(), Eq(8.f));
  EXPECT_THAT(c->vec4()->w(), Eq(9.f));
  EXPECT_THAT(c->quat()->x(), Eq(11.f));
  EXPECT_THAT(c->quat()->y(), Eq(12.f));
  EXPECT_THAT(c->quat()->z(), Eq(13.f));
  EXPECT_THAT(c->quat()->w(), Eq(10.f));
  EXPECT_THAT(c->vec2s()->size(), Eq(2u));
  EXPECT_THAT(c->vec3s()->size(), Eq(2u));
  EXPECT_THAT(c->vec4s()->size(), Eq(2u));
  EXPECT_THAT(c->quats()->size(), Eq(2u));
  EXPECT_THAT(c->vec2s()->Get(0)->x(), Eq(1.f));
  EXPECT_THAT(c->vec2s()->Get(0)->y(), Eq(2.f));
  EXPECT_THAT(c->vec3s()->Get(0)->x(), Eq(3.f));
  EXPECT_THAT(c->vec3s()->Get(0)->y(), Eq(4.f));
  EXPECT_THAT(c->vec3s()->Get(0)->z(), Eq(5.f));
  EXPECT_THAT(c->vec4s()->Get(0)->x(), Eq(6.f));
  EXPECT_THAT(c->vec4s()->Get(0)->y(), Eq(7.f));
  EXPECT_THAT(c->vec4s()->Get(0)->z(), Eq(8.f));
  EXPECT_THAT(c->vec4s()->Get(0)->w(), Eq(9.f));
  EXPECT_THAT(c->quats()->Get(0)->x(), Eq(11.f));
  EXPECT_THAT(c->quats()->Get(0)->y(), Eq(12.f));
  EXPECT_THAT(c->quats()->Get(0)->z(), Eq(13.f));
  EXPECT_THAT(c->quats()->Get(0)->w(), Eq(10.f));
  EXPECT_THAT(c->vec2s()->Get(1)->x(), Eq(10.f));
  EXPECT_THAT(c->vec2s()->Get(1)->y(), Eq(20.f));
  EXPECT_THAT(c->vec3s()->Get(1)->x(), Eq(30.f));
  EXPECT_THAT(c->vec3s()->Get(1)->y(), Eq(40.f));
  EXPECT_THAT(c->vec3s()->Get(1)->z(), Eq(50.f));
  EXPECT_THAT(c->vec4s()->Get(1)->x(), Eq(60.f));
  EXPECT_THAT(c->vec4s()->Get(1)->y(), Eq(70.f));
  EXPECT_THAT(c->vec4s()->Get(1)->z(), Eq(80.f));
  EXPECT_THAT(c->vec4s()->Get(1)->w(), Eq(90.f));
  EXPECT_THAT(c->quats()->Get(1)->x(), Eq(11.11f));
  EXPECT_THAT(c->quats()->Get(1)->y(), Eq(12.21f));
  EXPECT_THAT(c->quats()->Get(1)->z(), Eq(13.31f));
  EXPECT_THAT(c->quats()->Get(1)->w(), Eq(10.01f));
  EXPECT_THAT(c->variant(), NotNull());
  EXPECT_THAT(c->variant_type(), Eq(VariantDef_DataString));
  const auto* ds = reinterpret_cast<const DataString*>(c->variant());
  EXPECT_THAT(ds->value(), NotNull());
  EXPECT_THAT(ds->value()->str(), Eq("baz"));
}

TEST(FlatbufferWriter, Manual) {
  InwardBuffer buffer(32);
  FlatbufferWriter writer(&buffer);

  auto table1_start = writer.StartTable();
  {
    bool b = true;
    uint8_t u8 = 1;
    int8_t i8 = 2;
    uint16_t u16 = 3;
    int16_t i16 = 4;
    uint32_t u32 = 5;
    int32_t i32 = 6;
    uint64_t u64 = 7;
    int64_t i64 = 8;
    float r32 = 9.9f;
    double r64 = 10.01;
    std::string str = "foo";
    writer.Scalar<bool>(&b, Basics::VT_B, false);
    writer.Scalar<uint8_t>(&u8, Basics::VT_U8, 0);
    writer.Scalar<int8_t>(&i8, Basics::VT_I8, 0);
    writer.Scalar<uint16_t>(&u16, Basics::VT_U16, 0);
    writer.Scalar<int16_t>(&i16, Basics::VT_I16, 0);
    writer.Scalar<uint32_t>(&u32, Basics::VT_U32, 0);
    writer.Scalar<int32_t>(&i32, Basics::VT_I32, 0);
    writer.Scalar<uint64_t>(&u64, Basics::VT_U64, 0);
    writer.Scalar<int64_t>(&i64, Basics::VT_I64, 0);
    writer.Scalar<float>(&r32, Basics::VT_R32, 0);
    writer.Scalar<double>(&r64, Basics::VT_R64, 0);
    writer.String(&str, Basics::VT_STR);
  }
  auto table1_end = writer.EndTable(table1_start);

  auto table2_start = writer.StartTable();
  {
    bool b = false;
    uint8_t u8 = 10;
    int8_t i8 = 20;
    uint16_t u16 = 30;
    int16_t i16 = 40;
    uint32_t u32 = 50;
    int32_t i32 = 60;
    uint64_t u64 = 70;
    int64_t i64 = 80;
    float r32 = 90.09f;
    double r64 = 100.001;
    std::string str = "bar";
    writer.Scalar<bool>(&b, Basics::VT_B, false);
    writer.Scalar<uint8_t>(&u8, Basics::VT_U8, 0);
    writer.Scalar<int8_t>(&i8, Basics::VT_I8, 0);
    writer.Scalar<uint16_t>(&u16, Basics::VT_U16, 0);
    writer.Scalar<int16_t>(&i16, Basics::VT_I16, 0);
    writer.Scalar<uint32_t>(&u32, Basics::VT_U32, 0);
    writer.Scalar<int32_t>(&i32, Basics::VT_I32, 0);
    writer.Scalar<uint64_t>(&u64, Basics::VT_U64, 0);
    writer.Scalar<int64_t>(&i64, Basics::VT_I64, 0);
    writer.Scalar<float>(&r32, Basics::VT_R32, 0);
    writer.Scalar<double>(&r64, Basics::VT_R64, 0);
    writer.String(&str, Basics::VT_STR);
  }
  auto table2_end = writer.EndTable(table2_start);

  auto vec_start = writer.StartVector();
  writer.AddVectorReference(table1_end);
  writer.AddVectorReference(table2_end);
  auto vec_end = writer.EndVector(vec_start, 2);

  auto table_start = writer.StartTable();
  writer.Reference(vec_end, Complex::VT_BASICS);
  auto table_end = writer.EndTable(table_start);

  const void* data = writer.Finish(table_end);

  const Complex* c = flatbuffers::GetRoot<Complex>(data);

  EXPECT_THAT(c->basics()->size(), Eq(2u));
  EXPECT_THAT(c->basics()->Get(0)->b(), Eq(true));
  EXPECT_THAT(c->basics()->Get(0)->u8(), Eq(1));
  EXPECT_THAT(c->basics()->Get(0)->i8(), Eq(2));
  EXPECT_THAT(c->basics()->Get(0)->u16(), Eq(3));
  EXPECT_THAT(c->basics()->Get(0)->i16(), Eq(4));
  EXPECT_THAT(c->basics()->Get(0)->u32(), Eq(5u));
  EXPECT_THAT(c->basics()->Get(0)->i32(), Eq(6));
  EXPECT_THAT(c->basics()->Get(0)->u64(), Eq(7ul));
  EXPECT_THAT(c->basics()->Get(0)->i64(), Eq(8l));
  EXPECT_THAT(c->basics()->Get(0)->r32(), Eq(9.9f));
  EXPECT_THAT(c->basics()->Get(0)->r64(), Eq(10.01));
  EXPECT_THAT(c->basics()->Get(0)->str()->str(), Eq("foo"));
  EXPECT_THAT(c->basics()->Get(1)->b(), Eq(false));
  EXPECT_THAT(c->basics()->Get(1)->u8(), Eq(10));
  EXPECT_THAT(c->basics()->Get(1)->i8(), Eq(20));
  EXPECT_THAT(c->basics()->Get(1)->u16(), Eq(30));
  EXPECT_THAT(c->basics()->Get(1)->i16(), Eq(40));
  EXPECT_THAT(c->basics()->Get(1)->u32(), Eq(50u));
  EXPECT_THAT(c->basics()->Get(1)->i32(), Eq(60));
  EXPECT_THAT(c->basics()->Get(1)->u64(), Eq(70ul));
  EXPECT_THAT(c->basics()->Get(1)->i64(), Eq(80l));
  EXPECT_THAT(c->basics()->Get(1)->r32(), Eq(90.09f));
  EXPECT_THAT(c->basics()->Get(1)->r64(), Eq(100.001));
  EXPECT_THAT(c->basics()->Get(1)->str()->str(), Eq("bar"));
}

}  // namespace
}  // namespace lull
