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

#include "benchmark/benchmark.h"
#include "gmock/gmock.h"
#include "lullaby/util/flatbuffer_reader.h"
#include "lullaby/util/flatbuffer_writer.h"
#include "lullaby/lullaby/generated/flatc_generated.h"

namespace lull {
namespace {

using ::testing::Eq;

template <typename T, typename Builder>
flatbuffers::Offset<T> Finish(Builder* builder) {
  auto offset = builder->Finish();
  builder->fbb_.Finish(offset);
  return offset;
}

void Flatbuffer_Write_Benchmark(::benchmark::State& state) {
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

  for (auto _ : state) {
    InwardBuffer buffer(1024);
    const void* flatbuffer = FlatbufferWriter::SerializeObject(&obj, &buffer);
    (void)flatbuffer;
  }
}

void Flatbuffer_Read_Benchmark(::benchmark::State& state) {
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

  Finish<Complex>(&complex);
  auto* flatbuffer = fbb.GetBufferPointer();

  std::string name;
  for (auto _ : state) {
    auto table = flatbuffers::GetRoot<flatbuffers::Table>(flatbuffer);

    ComplexT t;
    FlatbufferReader::SerializeObject(&t, table);

    name = t.name;
  }
  EXPECT_THAT(name, Eq("hello"));
}

}  // namespace
}  // namespace lull

BENCHMARK(lull::Flatbuffer_Write_Benchmark);
BENCHMARK(lull::Flatbuffer_Read_Benchmark);
