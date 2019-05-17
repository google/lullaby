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

#include "lullaby/modules/ecs/blueprint.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "flatbuffers/flatbuffers.h"
#include "lullaby/generated/datastore_def_generated.h"
#include "lullaby/util/flatbuffer_reader.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::Eq;

struct ClassOne {};
struct ClassTwo {};

TEST(BlueprintTest, Empty) {
  Blueprint bp;

  int count = 0;
  bp.ForEachComponent([&](const Blueprint& blueprint) { ++count; });
  EXPECT_THAT(count, Eq(0));
}

TEST(BlueprintTest, EmptyFinalize) {
  Blueprint bp;

  int count = 0;
  auto span =
      bp.Finalize([&](FlatbufferWriter* writer, Blueprint* bp) { ++count; });
  EXPECT_THAT(count, Eq(0));
  EXPECT_THAT(span.data(), Eq(nullptr));
  EXPECT_TRUE(span.empty());
}

TEST(BlueprintDeathTest, ReadFromWrite) {
  Blueprint bp;

  DataBoolT bad;
  PORT_EXPECT_DEBUG_DEATH(bp.Read(&bad), "");
}

TEST(BlueprintDeathTest, WriteFromRead) {
  Blueprint bp;
  bp.FinishWriting();

  DataBoolT bad;
  PORT_EXPECT_DEBUG_DEATH(bp.Write(&bad), "");
}

TEST(BlueprintTest, ObjectPointer) {
  DataStringT data;
  data.value = "Hello";
  Blueprint bp(&data);

  EXPECT_FALSE(bp.Is<DataBoolT>());
  EXPECT_TRUE(bp.Is<DataStringT>());

  DataStringT other;
  bp.Read(&other);
  EXPECT_THAT(other.value, Eq("Hello"));

  int count = 0;
  bp.ForEachComponent([&](const Blueprint& blueprint) {
    DataStringT tmp;
    bp.Read(&tmp);
    EXPECT_THAT(tmp.value, Eq("Hello"));
    ++count;
  });
  EXPECT_THAT(count, Eq(1));
}

TEST(BlueprintDeathTest, BadRead) {
  DataStringT data;
  data.value = "Hello";
  Blueprint bp(&data);

  PORT_EXPECT_DEATH(bp.Read<DataStringT>(nullptr), "");

  DataBoolT bad;
  PORT_EXPECT_DEBUG_DEATH(bp.Read(&bad), "");

  DataBoolT* bad_ptr = nullptr;
  Blueprint bp2(bad_ptr);
  PORT_EXPECT_DEBUG_DEATH(bp2.Read(&bad), "");
}

TEST(BlueprintDeathTest, Write) {
  Blueprint bp;

  DataStringT data;
  data.value = "Hello";
  bp.Write(&data);
  bp.FinishWriting();

  DataStringT other;
  bp.Read(&other);
  EXPECT_THAT(other.value, Eq("Hello"));

  int count = 0;
  bp.ForEachComponent([&](const Blueprint& blueprint) {
    DataStringT tmp;
    bp.Read(&tmp);
    EXPECT_THAT(tmp.value, Eq("Hello"));
    ++count;
  });
  EXPECT_THAT(count, Eq(1));
}

TEST(BlueprintDeathTest, MultiWrite) {
  Blueprint bp;

  DataBoolT data_bool;
  data_bool.value = true;
  bp.Write(&data_bool);

  DataIntT data_int;
  data_int.value = 123;
  bp.Write(&data_int);

  DataFloatT data_float;
  data_float.value = 456.f;
  bp.Write(&data_float);

  DataStringT data_str;
  data_str.value = "Hello";
  bp.Write(&data_str);

  int count = 0;
  bp.ForEachComponent([&](const Blueprint& blueprint) {
    if (count == 0) {
      EXPECT_TRUE(blueprint.Is<DataBoolT>());
      DataBoolT tmp;
      blueprint.Read(&tmp);
      EXPECT_TRUE(tmp.value);
    } else if (count == 1) {
      EXPECT_TRUE(blueprint.Is<DataIntT>());
      DataIntT tmp;
      blueprint.Read(&tmp);
      EXPECT_THAT(tmp.value, Eq(123));
    } else if (count == 2) {
      EXPECT_TRUE(blueprint.Is<DataFloatT>());
      DataFloatT tmp;
      blueprint.Read(&tmp);
      EXPECT_THAT(tmp.value, Eq(456.f));
    } else if (count == 3) {
      EXPECT_TRUE(blueprint.Is<DataStringT>());
      DataStringT tmp;
      blueprint.Read(&tmp);
      EXPECT_THAT(tmp.value, Eq("Hello"));
    }
    ++count;
  });
  EXPECT_THAT(count, Eq(4));
}

TEST(BlueprintTest, FinalizeObjectPointer) {
  DataStringT data;
  data.value = "Hello";
  Blueprint bp(&data);

  int count = 0;
  auto span =
      bp.Finalize([&](FlatbufferWriter* writer, Blueprint* bp) { ++count; });
  EXPECT_THAT(count, Eq(1));

  auto flatbuffer = flatbuffers::GetRoot<const flatbuffers::Table>(span.data());
  DataStringT other;
  FlatbufferReader::SerializeObject(&other, flatbuffer);
  EXPECT_THAT(other.value, Eq("Hello"));
}

TEST(BlueprintDeathTest, Legacy) {
  DataStringT data;
  data.value = "Hello";
  Blueprint bp(&data);

  EXPECT_THAT(bp.GetLegacyDefType(), Eq(Hash("DataString")));

  const flatbuffers::Table* table = bp.GetLegacyDefData();
  const DataString* other = (DataString*)table;
  EXPECT_THAT(other->value()->str(), Eq("Hello"));
}

TEST(BlueprintTypeTest, TypeEquality) {
  BlueprintType type1a = BlueprintType::Create<ClassOne>();
  BlueprintType type1b = BlueprintType::Create<ClassOne>();
  BlueprintType type2 = BlueprintType::Create<ClassTwo>();
  EXPECT_TRUE(type1a == type1b);
  EXPECT_FALSE(type1a != type1b);
  EXPECT_TRUE(type1a != type2);
  EXPECT_FALSE(type1a == type2);
}

TEST(BlueprintTypeTest, NameEquality) {
  BlueprintType type1a = BlueprintType::CreateFromSchemaNameHash(Hash("One"));
  BlueprintType type1b = BlueprintType::CreateFromSchemaNameHash(Hash("One"));
  BlueprintType type2 = BlueprintType::CreateFromSchemaNameHash(Hash("Two"));
  EXPECT_TRUE(type1a == type1b);
  EXPECT_FALSE(type1a != type1b);
  EXPECT_TRUE(type1a != type2);
  EXPECT_FALSE(type1a == type2);
}

}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(ClassOne);
LULLABY_SETUP_TYPEID(ClassTwo);
