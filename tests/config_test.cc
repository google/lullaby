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

#include "lullaby/modules/config/config.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/generated/config_def_generated.h"
#include "lullaby/modules/flatbuffers/flatbuffer_writer.h"
#include "lullaby/util/inward_buffer.h"

namespace lull {
namespace {

using ::testing::Eq;

TEST(ConfigTest, Empty) {
  const HashValue key = Hash("key");

  Config cfg;
  int value = cfg.Get(key, 12);
  EXPECT_THAT(value, Eq(12));
}

TEST(ConfigTest, SetGet) {
  const HashValue key = Hash("key");

  Config cfg;
  cfg.Set(key, 34);
  EXPECT_THAT(cfg.Get(key, 12), Eq(34));

  cfg.Set(key, 56);
  EXPECT_THAT(cfg.Get(key, 12), Eq(56));
}

TEST(ConfigTest, Remove) {
  const HashValue key = Hash("key");

  Config cfg;
  cfg.Set(key, 34);
  EXPECT_THAT(cfg.Get(key, 12), Eq(34));

  cfg.Remove(key);
  EXPECT_THAT(cfg.Get(key, 12), Eq(12));
}

template <typename T>
void AddVariant(ConfigDefT* def, const std::string& key,
                const decltype(T::value) & value) {
  KeyVariantPairDefT pair;
  pair.key = key;
  pair.value.set<T>()->value = value;
  def->values.emplace_back(std::move(pair));
}

TEST(ConfigTest, SetFromFlatbuffer) {
  ConfigDefT data;

  AddVariant<DataBoolT>(&data, "bool_key", true);
  AddVariant<DataIntT>(&data, "int_key", 123);
  AddVariant<DataFloatT>(&data, "float_key", 456.f);
  AddVariant<DataStringT>(&data, "string_key", "hello");
  AddVariant<DataHashValueT>(&data, "hash_key", Hash("world"));

  InwardBuffer buffer(256);
  auto flatbuffer = WriteFlatbuffer(&data, &buffer);

  Config cfg;
  SetConfigFromFlatbuffer(&cfg, flatbuffers::GetRoot<ConfigDef>(flatbuffer));
  EXPECT_TRUE(cfg.Get(Hash("bool_key"), false));
  EXPECT_THAT(cfg.Get(Hash("int_key"), 0), Eq(123));
  EXPECT_THAT(cfg.Get(Hash("float_key"), 0.f), Eq(456.f));
  EXPECT_THAT(cfg.Get(Hash("string_key"), std::string("")), Eq("hello"));
  EXPECT_THAT(cfg.Get(Hash("hash_key"), HashValue(0)), Eq(Hash("world")));
}

TEST(ConfigTest, SetFromVariantMap) {
  VariantMap var;

  var[Hash("bool_key")] = true;
  var[Hash("int_key")] = 123;
  var[Hash("float_key")] = 456.f;
  var[Hash("string_key")] = std::string("hello");
  var[Hash("hash_key")] = Hash("world");

  Config cfg;
  SetConfigFromVariantMap(&cfg, &var);
  EXPECT_TRUE(cfg.Get(Hash("bool_key"), false));
  EXPECT_THAT(cfg.Get(Hash("int_key"), 0), Eq(123));
  EXPECT_THAT(cfg.Get(Hash("float_key"), 0.f), Eq(456.f));
  EXPECT_THAT(cfg.Get(Hash("string_key"), std::string("")), Eq("hello"));
  EXPECT_THAT(cfg.Get(Hash("hash_key"), HashValue(0)), Eq(Hash("world")));
}

}  // namespace
}  // namespace lull
