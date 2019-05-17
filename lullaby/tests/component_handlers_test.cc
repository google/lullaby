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

#include "lullaby/modules/ecs/component_handlers.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "flatbuffers/flatbuffers.h"
#include "lullaby/util/inward_buffer.h"
#include "lullaby/util/variant.h"
#include "lullaby/tests/portable_test_macros.h"
#include "lullaby/tests/test_def_generated.h"

namespace lull {
namespace testing {

using ::testing::Eq;
using ::testing::NotNull;

TEST(ComponentHandlersTest, IsRegistered) {
  ComponentHandlers handlers;
  EXPECT_FALSE(handlers.IsRegistered(Hash("ValueDef")));

  handlers.RegisterComponentDefT<ValueDefT>();
  EXPECT_TRUE(handlers.IsRegistered(Hash("ValueDef")));
}

TEST(ComponentHandlersTest, IsRegisteredFalse) {
  ComponentHandlers handlers;
  EXPECT_FALSE(handlers.IsRegistered(Hash("ComplexDef")));

  handlers.RegisterComponentDefT<ValueDefT>();
  EXPECT_FALSE(handlers.IsRegistered(Hash("ComplexDef")));
}

TEST(ComponentHandlersTest, Verify) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ValueDefT>();

  flatbuffers::FlatBufferBuilder fbb;
  const auto str = fbb.CreateString("hello world");
  ValueDefBuilder value_builder(fbb);
  value_builder.add_name(str);
  value_builder.add_value(42);
  fbb.Finish(value_builder.Finish());

  EXPECT_TRUE(handlers.Verify(Hash("ValueDef"),
                              {fbb.GetBufferPointer(), fbb.GetSize()}));
}

TEST(ComponentHandlersTest, VerifyNotRegistered) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ValueDefT>();

  flatbuffers::FlatBufferBuilder fbb;
  const auto str = fbb.CreateString("hello world");
  ValueDefBuilder value_builder(fbb);
  value_builder.add_name(str);
  value_builder.add_value(42);
  fbb.Finish(value_builder.Finish());

  EXPECT_FALSE(handlers.Verify(Hash("ComplexDef"),
                               {fbb.GetBufferPointer(), fbb.GetSize()}));
}

TEST(ComponentHandlersTest, VerifyFalse) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ValueDefT>();

  uint8_t empty[16];
  memset(empty, 0, sizeof(empty));
  EXPECT_FALSE(handlers.Verify(Hash("ValueDef"), {empty}));
}

TEST(ComponentHandlersTest, ReadFromFlatbuffer) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ValueDefT>();

  flatbuffers::FlatBufferBuilder fbb;
  const auto str = fbb.CreateString("hello world");
  ValueDefBuilder value_builder(fbb);
  value_builder.add_name(str);
  value_builder.add_value(42);
  fbb.Finish(value_builder.Finish());

  Variant variant;
  handlers.ReadFromFlatbuffer(
      Hash("ValueDef"), &variant,
      flatbuffers::GetRoot<flatbuffers::Table>(fbb.GetBufferPointer()));

  EXPECT_THAT(variant.GetTypeId(), Eq(GetTypeId<ValueDefT>()));
  ValueDefT* value_def = variant.Get<ValueDefT>();
  ASSERT_THAT(value_def, NotNull());
  EXPECT_THAT(value_def->name, Eq("hello world"));
  EXPECT_THAT(value_def->value, Eq(42));
}

TEST(ComponentHandlersTest, ReadFromFlatbufferNotRegistered) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ValueDefT>();

  flatbuffers::FlatBufferBuilder fbb;
  const auto str = fbb.CreateString("hello world");
  ValueDefBuilder value_builder(fbb);
  value_builder.add_name(str);
  value_builder.add_value(42);
  fbb.Finish(value_builder.Finish());

  Variant variant;
  handlers.ReadFromFlatbuffer(
      Hash("ComplexDef"), &variant,
      flatbuffers::GetRoot<flatbuffers::Table>(fbb.GetBufferPointer()));

  EXPECT_TRUE(variant.Empty());
}

TEST(ComponentHandlersTest, WriteToFlatbuffer) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ValueDefT>();

  ValueDefT value_def;
  value_def.name = "hello world";
  value_def.value = 42;
  Variant variant = value_def;

  InwardBuffer buffer(256);
  void* start = handlers.WriteToFlatbuffer(Hash("ValueDef"), &variant, &buffer);

  flatbuffers::Verifier verifier(static_cast<uint8_t*>(start),
                                 buffer.BackSize());
  EXPECT_TRUE(verifier.VerifyBuffer<ValueDef>());

  auto* root = flatbuffers::GetRoot<ValueDef>(start);
  EXPECT_THAT(root->name()->str(), Eq("hello world"));
  EXPECT_THAT(root->value(), Eq(42));
}

TEST(ComponentHandlersTest, WriteToFlatbufferNotRegistered) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ValueDefT>();

  ValueDefT value_def;
  value_def.name = "hello world";
  value_def.value = 42;
  Variant variant = value_def;

  InwardBuffer buffer(256);
  void* start =
      handlers.WriteToFlatbuffer(Hash("ComplexDef"), &variant, &buffer);
  EXPECT_THAT(buffer.BackSize(), Eq(0));
  EXPECT_THAT(start, Eq(buffer.BackAt(0)));
}

TEST(ComponentHandlersDeathTest, WriteToFlatbufferWrongVariant) {
  ComponentHandlers handlers;
  handlers.RegisterComponentDefT<ComplexDefT>();

  ValueDefT value_def;
  value_def.name = "hello world";
  value_def.value = 42;
  Variant variant = value_def;

  InwardBuffer buffer(256);
  PORT_EXPECT_DEBUG_DEATH(
      handlers.WriteToFlatbuffer(Hash("ComplexDef"), &variant, &buffer), "");
}

}  // namespace testing
}  // namespace lull
