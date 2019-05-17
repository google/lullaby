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

#include "lullaby/systems/datastore/datastore_system.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/generated/datastore_def_generated.h"
#include "lullaby/modules/ecs/blueprint.h"
#include "lullaby/util/common_types.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;

const Entity kTestEntity1 = Hash("test-entity1");
const Entity kTestEntity2 = Hash("test-entity2");
const HashValue kTestKey1 = Hash("test-key1");
const HashValue kTestKey2 = Hash("test-key2");
const HashValue kDatastoreDef = Hash("DatastoreDef");

template <typename T>
void AddVariant(DatastoreDefT* def, const std::string& key,
                const decltype(T::value)& value) {
  KeyVariantPairDefT pair;
  pair.key = key;
  pair.value.set<T>()->value = value;
  def->key_value_pairs.emplace_back(std::move(pair));
}

void AddVariantArray(DatastoreDefT* def, const std::string& key,
                     const VariantArrayDefT& arr) {
  KeyVariantPairDefT pair;
  pair.key = key;
  *(pair.value.set<VariantArrayDefT>()) = arr;
  def->key_value_pairs.emplace_back(std::move(pair));
}

void AddVariantMap(DatastoreDefT* def, const std::string& key,
                     const VariantMapDefT& map) {
  KeyVariantPairDefT pair;
  pair.key = key;
  *(pair.value.set<VariantMapDefT>()) = map;
  def->key_value_pairs.emplace_back(std::move(pair));
}

TEST(DatastoreSystem, InitiallyEmpty) {
  Registry r;
  DatastoreSystem d(&r);
  EXPECT_THAT(d.Get<int>(kTestEntity1, kTestKey1), IsNull());
  EXPECT_THAT(d.Get<int>(kTestEntity1, kTestKey2), IsNull());
  EXPECT_THAT(d.Get<int>(kTestEntity2, kTestKey1), IsNull());
  EXPECT_THAT(d.Get<int>(kTestEntity2, kTestKey2), IsNull());
}

TEST(DatastoreSystem, SetGet) {
  Registry r;
  DatastoreSystem d(&r);

  d.Set(kTestEntity1, kTestKey1, 123);
  auto* int_value = d.Get<int>(kTestEntity1, kTestKey1);
  auto* float_value = d.Get<float>(kTestEntity1, kTestKey1);
  EXPECT_THAT(int_value, NotNull());
  EXPECT_THAT(float_value, IsNull());
  EXPECT_THAT(*int_value, Eq(123));
}

TEST(DatastoreSystem, SetVariant) {
  Registry r;
  DatastoreSystem d(&r);

  Variant var = 123;
  d.Set(kTestEntity1, kTestKey1, var);

  auto* int_value = d.Get<int>(kTestEntity1, kTestKey1);
  auto* bool_value = d.Get<bool>(kTestEntity1, kTestKey1);
  auto* float_value = d.Get<float>(kTestEntity1, kTestKey2);
  EXPECT_THAT(int_value, NotNull());
  EXPECT_THAT(bool_value, IsNull());
  EXPECT_THAT(float_value, IsNull());
  EXPECT_THAT(*int_value, Eq(123));
}

TEST(DatastoreSystem, SetChangeType) {
  Registry r;
  DatastoreSystem d(&r);

  d.Set(kTestEntity1, kTestKey1, 123);
  auto* int_value = d.Get<int>(kTestEntity1, kTestKey1);
  auto* float_value = d.Get<float>(kTestEntity1, kTestKey1);
  EXPECT_THAT(int_value, NotNull());
  EXPECT_THAT(float_value, IsNull());
  EXPECT_THAT(*int_value, Eq(123));

  d.Set(kTestEntity1, kTestKey1, 456.f);
  int_value = d.Get<int>(kTestEntity1, kTestKey1);
  float_value = d.Get<float>(kTestEntity1, kTestKey1);
  EXPECT_THAT(int_value, IsNull());
  EXPECT_THAT(float_value, NotNull());
  EXPECT_THAT(*float_value, Eq(456.f));
}

TEST(DatastoreSystem, GetInvalidKey) {
  Registry r;
  DatastoreSystem d(&r);

  d.Set(kTestEntity1, kTestKey1, 123);
  auto* int_value = d.Get<int>(kTestEntity2, kTestKey1);
  auto* float_value = d.Get<float>(kTestEntity2, kTestKey1);
  EXPECT_THAT(int_value, IsNull());
  EXPECT_THAT(float_value, IsNull());
}

TEST(DatastoreSystem, Remove) {
  Registry r;
  DatastoreSystem d(&r);

  d.Set(kTestEntity1, kTestKey1, 123);
  d.Remove(kTestEntity1, kTestKey1);
  auto* int_value = d.Get<int>(kTestEntity1, kTestKey1);
  auto* float_value = d.Get<float>(kTestEntity1, kTestKey1);
  EXPECT_THAT(int_value, IsNull());
  EXPECT_THAT(float_value, IsNull());
}

TEST(DatastoreSystem, Destroy) {
  Registry r;
  DatastoreSystem d(&r);

  d.Set(kTestEntity1, kTestKey1, 123);
  d.Destroy(kTestEntity1);
  auto* int_value = d.Get<int>(kTestEntity1, kTestKey1);
  auto* float_value = d.Get<float>(kTestEntity1, kTestKey1);
  EXPECT_THAT(int_value, IsNull());
  EXPECT_THAT(float_value, IsNull());
}

TEST(DatastoreSystem, RemoveEmpty) {
  Registry r;
  DatastoreSystem d(&r);

  d.Remove(kTestEntity1, kTestKey1);
  auto* int_value = d.Get<int>(kTestEntity1, kTestKey1);
  auto* float_value = d.Get<float>(kTestEntity1, kTestKey1);
  EXPECT_THAT(int_value, IsNull());
  EXPECT_THAT(float_value, IsNull());
}

TEST(DatastoreSystem, SetNullEntity) {
  Registry r;
  DatastoreSystem d(&r);

  d.Set(kNullEntity, kTestKey1, 123);
  auto* int_value = d.Get<int>(kNullEntity, kTestKey1);
  auto* float_value = d.Get<float>(kNullEntity, kTestKey1);
  EXPECT_THAT(int_value, IsNull());
  EXPECT_THAT(float_value, IsNull());

  Variant var = 123;
  d.Set(kNullEntity, kTestKey2, var);
  int_value = d.Get<int>(kNullEntity, kTestKey2);
  float_value = d.Get<float>(kNullEntity, kTestKey2);
  EXPECT_THAT(int_value, IsNull());
  EXPECT_THAT(float_value, IsNull());
}

TEST(DatastoreSystem, CreateFromNullDatastoreDef) {
  Registry r;
  DatastoreSystem d(&r);
  d.Create(kTestEntity1, kDatastoreDef, nullptr);
  auto* int_value = d.Get<int>(kNullEntity, kTestKey1);
  auto* float_value = d.Get<float>(kNullEntity, kTestKey1);
  EXPECT_THAT(int_value, IsNull());
  EXPECT_THAT(float_value, IsNull());
}

TEST(DatastoreSystem, CreateFromDatastoreDef) {
  Registry r;
  DatastoreSystem d(&r);

  DatastoreDefT data;
  AddVariant<DataBoolT>(&data, "bool_key", true);
  AddVariant<DataIntT>(&data, "int_key", 123);
  AddVariant<DataFloatT>(&data, "float_key", 456.f);
  AddVariant<DataStringT>(&data, "string_key", "hello");
  AddVariant<DataHashValueT>(&data, "hash_key", Hash("world"));
  AddVariant<DataVec2T>(&data, "vec2_key", mathfu::vec2(1, 2));
  AddVariant<DataVec3T>(&data, "vec3_key", mathfu::vec3(3, 4, 5));
  AddVariant<DataVec4T>(&data, "vec4_key", mathfu::vec4(6, 7, 8, 9));
  AddVariant<DataQuatT>(&data, "quat_key", mathfu::quat(1, 0, 0, 0));

  VariantArrayDefT arr;
  arr.values.resize(3);
  arr.values[0].value.set<DataIntT>()->value = 123;
  arr.values[1].value.set<DataFloatT>()->value = 456.f;
  arr.values[2].value.set<DataStringT>()->value = std::string("hello");
  AddVariantArray(&data, "arr_key", arr);

  VariantMapDefT map;
  map.values.resize(3);
  map.values[0].hash_key = ConstHash("a");
  map.values[0].value.set<DataIntT>()->value = 123;
  map.values[1].hash_key = ConstHash("b");
  map.values[1].value.set<DataFloatT>()->value = 456.f;
  map.values[2].hash_key = ConstHash("c");
  map.values[2].value.set<DataStringT>()->value = std::string("hello");
  AddVariantMap(&data, "map_key", map);

  const Blueprint blueprint(&data);
  d.CreateComponent(kTestEntity1, blueprint);

  EXPECT_THAT(*d.Get<bool>(kTestEntity1, Hash("bool_key")), Eq(true));
  EXPECT_THAT(*d.Get<int>(kTestEntity1, Hash("int_key")), Eq(123));
  EXPECT_THAT(*d.Get<float>(kTestEntity1, Hash("float_key")), Eq(456.f));
  EXPECT_THAT(*d.Get<HashValue>(kTestEntity1, Hash("hash_key")),
              Eq(Hash("world")));
  EXPECT_THAT(*d.Get<std::string>(kTestEntity1, Hash("string_key")),
              Eq("hello"));
  EXPECT_THAT(*d.Get<mathfu::vec2>(kTestEntity1, Hash("vec2_key")),
              Eq(mathfu::vec2(1, 2)));
  EXPECT_THAT(*d.Get<mathfu::vec3>(kTestEntity1, Hash("vec3_key")),
              Eq(mathfu::vec3(3, 4, 5)));
  EXPECT_THAT(*d.Get<mathfu::vec4>(kTestEntity1, Hash("vec4_key")),
              Eq(mathfu::vec4(6, 7, 8, 9)));
  EXPECT_THAT(d.Get<mathfu::quat>(kTestEntity1, Hash("quat_key"))->vector(),
              Eq(mathfu::quat(1, 0, 0, 0).vector()));
  EXPECT_THAT(d.Get<mathfu::quat>(kTestEntity1, Hash("quat_key"))->scalar(),
              Eq(mathfu::quat(1, 0, 0, 0).scalar()));

  const VariantArray* test_arr =
      d.Get<VariantArray>(kTestEntity1, Hash("arr_key"));
  EXPECT_THAT(test_arr, NotNull());
  EXPECT_THAT(test_arr->size(), Eq(3));
  EXPECT_THAT(test_arr->at(0).ValueOr<int>(0), Eq(123));
  EXPECT_THAT(test_arr->at(1).ValueOr<float>(0.f), Eq(456.f));
  EXPECT_THAT(test_arr->at(2).ValueOr<std::string>(""), Eq("hello"));

  const VariantMap* test_map = d.Get<VariantMap>(kTestEntity1, Hash("map_key"));
  EXPECT_THAT(test_map, NotNull());
  EXPECT_THAT(test_map->size(), Eq(3));
  EXPECT_THAT(test_map->at(ConstHash("a")).ValueOr<int>(0), Eq(123));
  EXPECT_THAT(test_map->at(ConstHash("b")).ValueOr<float>(0.f), Eq(456.f));
  EXPECT_THAT(test_map->at(ConstHash("c")).ValueOr<std::string>(""),
              Eq("hello"));
}

}  // namespace
}  // namespace lull
