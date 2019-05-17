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

#include "lullaby/modules/config/config.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/generated/config_def_generated.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/util/flatbuffer_writer.h"
#include "lullaby/util/inward_buffer.h"
#include "lullaby/tests/flatbuffers/test_def_generated.h"
#include "lullaby/tests/test_def_generated.h"
#include "lullaby/tests/util/fake_file_system.h"

namespace lull {
namespace {

using ::testing::Eq;

class ConfigTest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry_.Create<AssetLoader>(
        [&](const char* name, std::string* out) -> bool {
          return fake_file_system_.LoadFromDisk(name, out);
        });
  }

  template <typename T>
  void Save(string_view name, T* data) {
    InwardBuffer buffer(256);
    auto* flatbuffer = WriteFlatbuffer(data, &buffer);
    fake_file_system_.SaveToDisk(std::string(name),
                                 static_cast<const uint8_t*>(flatbuffer),
                                 buffer.BackSize());
  }

  Registry registry_;
  FakeFileSystem fake_file_system_;
};

TEST_F(ConfigTest, Empty) {
  const HashValue key = Hash("key");

  Config cfg;
  int value = cfg.Get(key, 12);
  EXPECT_THAT(value, Eq(12));
}

TEST_F(ConfigTest, SetGet) {
  const HashValue key = Hash("key");

  Config cfg;
  cfg.Set(key, 34);
  EXPECT_THAT(cfg.Get(key, 12), Eq(34));

  cfg.Set(key, 56);
  EXPECT_THAT(cfg.Get(key, 12), Eq(56));
}

TEST_F(ConfigTest, Remove) {
  const HashValue key = Hash("key");

  Config cfg;
  cfg.Set(key, 34);
  EXPECT_THAT(cfg.Get(key, 12), Eq(34));

  cfg.Remove(key);
  EXPECT_THAT(cfg.Get(key, 12), Eq(12));
}

template <typename T>
void AddVariant(ConfigDefT* def, const std::string& key,
                const decltype(T::value)& value) {
  KeyVariantPairDefT pair;
  pair.key = key;
  pair.value.set<T>()->value = value;
  def->values.emplace_back(std::move(pair));
}

TEST_F(ConfigTest, SetFromVariantMap) {
  VariantMap var;

  var[Hash("bool_key")] = true;
  var[Hash("int_key")] = 123;
  var[Hash("float_key")] = 456.f;
  var[Hash("string_key")] = std::string("hello");
  var[Hash("hash_key")] = Hash("world");

  Config cfg;
  cfg.Set(var);
  EXPECT_TRUE(cfg.Get(Hash("bool_key"), false));
  EXPECT_THAT(cfg.Get(Hash("int_key"), 0), Eq(123));
  EXPECT_THAT(cfg.Get(Hash("float_key"), 0.f), Eq(456.f));
  EXPECT_THAT(cfg.Get(Hash("string_key"), std::string("")), Eq("hello"));
  EXPECT_THAT(cfg.Get(Hash("hash_key"), HashValue(0)), Eq(Hash("world")));
}

TEST_F(ConfigTest, NullCheck) {
  Registry registry;
  Config cfg;
  VariantMap var;
  ConfigDefT data;
  Save("test", &data);

  cfg.LoadConfig(nullptr, "");
  cfg.LoadConfig(nullptr, "test");
  cfg.LoadConfig(&registry_, "");
  cfg.LoadObject<lull::testing::UnknownDefT>(nullptr, "");
  cfg.LoadObject<lull::testing::UnknownDefT>(&registry_, "");

  const HashValue key = Hash("key");
  int value = cfg.Get(key, 12);
  EXPECT_THAT(value, Eq(12));
}

TEST_F(ConfigTest, EmptyFlatbuffer) {
  Config cfg;
  ConfigDefT data;
  Save("test", &data);

  // Should not fatal with empty flatbuffer.
  cfg.LoadConfig(&registry_, "test");
  const HashValue key = Hash("key");
  int value = cfg.Get(key, 12);
  EXPECT_THAT(value, Eq(12));
}

TEST_F(ConfigTest, EmptyVariant) {
  Config cfg;
  ConfigDefT data;
  data.values.emplace_back(KeyVariantPairDefT());
  Save("test", &data);

  // Should not fatal with empty variant.
  cfg.LoadConfig(&registry_, "test");
  const HashValue key = Hash("key");
  int value = cfg.Get(key, 12);
  EXPECT_THAT(value, Eq(12));
}

TEST_F(ConfigTest, WrongFileName) {
  // Should not fatal with wrong file name.
  Config cfg;
  cfg.LoadConfig(&registry_, "wrong_file_name");
  const HashValue key = Hash("key");
  int value = cfg.Get(key, 12);
  EXPECT_THAT(value, Eq(12));
}

TEST_F(ConfigTest, LoadConfigFromFile) {
  ConfigDefT data;
  AddVariant<DataBoolT>(&data, "bool_key", true);
  AddVariant<DataIntT>(&data, "int_key", 123);
  AddVariant<DataFloatT>(&data, "float_key", 456.f);
  AddVariant<DataStringT>(&data, "string_key", "hello");
  AddVariant<DataHashValueT>(&data, "hash_key", Hash("world"));
  Save("config.cfg", &data);

  Config cfg;
  cfg.LoadConfig(&registry_, "config.cfg");
  EXPECT_TRUE(cfg.Get(Hash("bool_key"), false));
  EXPECT_THAT(cfg.Get(Hash("int_key"), 0), Eq(123));
  EXPECT_THAT(cfg.Get(Hash("float_key"), 0.f), Eq(456.f));
  EXPECT_THAT(cfg.Get(Hash("string_key"), std::string("")), Eq("hello"));
  EXPECT_THAT(cfg.Get(Hash("hash_key"), HashValue(0)), Eq(Hash("world")));
}

TEST_F(ConfigTest, SetGetObject) {
  using UnknownDefT = ::lull::testing::UnknownDefT;

  Config cfg;
  EXPECT_THAT(cfg.GetObject<UnknownDefT>().name, Eq(""));
  EXPECT_THAT(cfg.GetObject<UnknownDefT>().value, Eq(0));

  UnknownDefT obj;
  obj.name = "test";
  obj.value = 123;
  cfg.SetObject(obj);
  EXPECT_THAT(cfg.GetObject<UnknownDefT>().name, Eq("test"));
  EXPECT_THAT(cfg.GetObject<UnknownDefT>().value, Eq(123));

  cfg.RemoveObject<UnknownDefT>();
  EXPECT_THAT(cfg.GetObject<UnknownDefT>().name, Eq(""));
  EXPECT_THAT(cfg.GetObject<UnknownDefT>().value, Eq(0));
}

TEST_F(ConfigTest, LoadObjectFromFile) {
  using UnknownDefT = ::lull::testing::UnknownDefT;

  UnknownDefT data;
  data.name = "test";
  data.value = 123;
  Save("config.obj", &data);

  Config cfg;
  cfg.LoadObject<UnknownDefT>(&registry_, "config.obj");
  EXPECT_THAT(cfg.GetObject<UnknownDefT>().name, Eq("test"));
  EXPECT_THAT(cfg.GetObject<UnknownDefT>().value, Eq(123));
}

}  // namespace
}  // namespace lull
