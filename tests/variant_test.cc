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

#include "lullaby/util/variant.h"
#include "gtest/gtest.h"
#include "lullaby/modules/serialize/serialize.h"
#include "lullaby/modules/serialize/variant_serializer.h"
#include "lullaby/util/logging.h"
#include "tests/portable_test_macros.h"

namespace lull {
namespace {

struct VariantTestClass {
  VariantTestClass() {}
  VariantTestClass(int key, int value) : key(key), value(value) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&key, lull::Hash("key"));
    archive(&value, lull::Hash("value"));
  }

  int key = 0;
  int value = 0;
};

struct ComplexTestClass : VariantTestClass {
  ComplexTestClass() {}

  template <typename Archive>
  void Serialize(Archive archive) {
    VariantTestClass::Serialize(archive);
    archive(&other, lull::Hash("other"));
    archive(&word, lull::Hash("word"));
    archive(&arr, lull::Hash("arr"));
    archive(&map, lull::Hash("map"));
    archive(&optional, lull::Hash("optional"));
    archive(&optional_unset, lull::Hash("optional_unset"));
  }

  VariantTestClass other;
  std::string word;
  std::vector<int> arr;
  std::unordered_map<lull::HashValue, float> map;
  Optional<float> optional;
  Optional<float> optional_unset;
};

struct MoveOnlyVariantTestClass {
  explicit MoveOnlyVariantTestClass(std::string value)
      : value(std::move(value)) {}

  MoveOnlyVariantTestClass(MoveOnlyVariantTestClass&& rhs)
      : value(std::move(rhs.value)) {}

  MoveOnlyVariantTestClass& operator=(MoveOnlyVariantTestClass&& rhs) {
    value = std::move(rhs.value);
    return *this;
  }

  MoveOnlyVariantTestClass(const MoveOnlyVariantTestClass&) {
    LOG(FATAL) << "This function should not be called.";
  }

  MoveOnlyVariantTestClass& operator=(const MoveOnlyVariantTestClass&) {
    LOG(FATAL) << "This function should not be called.";
    return *this;
  }

  std::string value;
};

TEST(Variant, Basics) {
  lull::Variant var;
  EXPECT_TRUE(var.Empty());

  var = 1;
  EXPECT_FALSE(var.Empty());
  EXPECT_EQ(1, *var.Get<int>());
  EXPECT_EQ(nullptr, var.Get<float>());
  EXPECT_EQ(1, var.ValueOr(0));
  EXPECT_EQ(0.f, var.ValueOr(0.f));

  var = 2.f;
  EXPECT_FALSE(var.Empty());
  EXPECT_EQ(nullptr, var.Get<int>());
  EXPECT_EQ(2.f, *var.Get<float>());
  EXPECT_EQ(0, var.ValueOr(0));
  EXPECT_EQ(2.f, var.ValueOr(0.f));

  var.Clear();
  EXPECT_TRUE(var.Empty());
  EXPECT_EQ(nullptr, var.Get<int>());
  EXPECT_EQ(nullptr, var.Get<float>());
}

TEST(Variant, Class) {
  lull::Variant var;
  EXPECT_TRUE(var.Empty());

  var = VariantTestClass(1, 2);
  auto value = var.Get<VariantTestClass>();
  EXPECT_EQ(1, value->key);
  EXPECT_EQ(2, value->value);

  lull::Variant var2 = var;
  value = var2.Get<VariantTestClass>();
  EXPECT_EQ(1, value->key);
  EXPECT_EQ(2, value->value);
  var2 = VariantTestClass(3, 4);

  var = var2;
  value = var2.Get<VariantTestClass>();
  EXPECT_EQ(3, value->key);
  EXPECT_EQ(4, value->value);

  var2 = 123;
  EXPECT_EQ(123, *var2.Get<int>());

  lull::Variant var3 = std::move(var2);
  EXPECT_EQ(123, *var3.Get<int>());
  EXPECT_EQ(nullptr, var2.Get<int>());

  var2 = std::move(var3);
  EXPECT_EQ(123, *var2.Get<int>());
  EXPECT_EQ(nullptr, var3.Get<int>());
}

TEST(Variant, Move) {
  lull::Variant var1 = MoveOnlyVariantTestClass("hello");
  EXPECT_EQ("hello", var1.Get<MoveOnlyVariantTestClass>()->value);

  lull::Variant var2 = std::move(var1);
  EXPECT_EQ("hello", var2.Get<MoveOnlyVariantTestClass>()->value);

  var1 = std::move(var2);
  EXPECT_EQ("hello", var1.Get<MoveOnlyVariantTestClass>()->value);
}

TEST(Variant, VariantSerializer) {
  ComplexTestClass u1;
  u1.key = 123;
  u1.value = 456;
  u1.other.key = 789;
  u1.other.value = 987654321;
  u1.word = "hello";
  u1.arr.push_back(10);
  u1.arr.push_back(11);
  u1.arr.push_back(12);
  u1.map[123] = 123.f;
  u1.map[456] = 456.f;
  u1.map[789] = 789.f;
  u1.optional = 13.f;

  lull::VariantMap map;
  lull::SaveToVariant save(&map);
  lull::Serialize(&save, &u1, 0);

  ComplexTestClass u2;
  lull::LoadFromVariant load(&map);
  lull::Serialize(&load, &u2, 0);

  EXPECT_EQ(u1.key, u2.key);
  EXPECT_EQ(u1.value, u2.value);
  EXPECT_EQ(u1.word, u2.word);
  EXPECT_EQ(u1.other.key, u2.other.key);
  EXPECT_EQ(u1.other.value, u2.other.value);
  EXPECT_EQ(u1.arr, u2.arr);
  EXPECT_EQ(u1.map, u2.map);
  EXPECT_EQ(u1.optional, u2.optional);
  EXPECT_EQ(u1.optional_unset, u2.optional_unset);
  {
    const auto other_key = lull::Hash("other");
    const auto& other_var = map.find(other_key)->second;
    const auto& other_map = other_var.Get<lull::VariantMap>();

    const auto value_key = lull::Hash("value");
    const auto& value_var = other_map->find(value_key)->second;
    const auto& value_ptr = value_var.Get<int>();
    EXPECT_EQ(987654321, *value_ptr);
  }
  {
    const auto map_key = lull::Hash("map");
    const auto& map_var = map.find(map_key)->second;
    const auto& map_varmap = map_var.Get<lull::VariantMap>();
    const auto& value_var = map_varmap->find(123)->second;
    const auto& value_ptr = value_var.Get<float>();
    EXPECT_EQ(123.f, *value_ptr);
  }
}

TEST(VariantDeathTest, VariantSerializer_NullMaps) {
  PORT_EXPECT_DEBUG_DEATH(lull::SaveToVariant save(nullptr), "");
  PORT_EXPECT_DEBUG_DEATH(lull::LoadFromVariant load(nullptr), "");
}

TEST(VariantDeathTest, VariantSerializer_BadSave) {
  lull::VariantMap map;
  lull::SaveToVariant save(&map);
  const HashValue key = 123;

  // Cannot save without calling Begin().
  int dummy = 0;
  PORT_EXPECT_DEBUG_DEATH(save(&dummy, key), "");

  // Cannot save unsupported types.
  VariantTestClass test;
  PORT_EXPECT_DEBUG_DEATH(save(&test, key), "");

  // Cannot end without calling Begin().
  PORT_EXPECT_DEBUG_DEATH(save.End(), "");
}

TEST(VariantDeathTest, VariantSerializer_BadLoad) {
  lull::VariantMap map;

  lull::LoadFromVariant load(&map);

  // Cannot load without calling Begin().
  int dummy = 0;
  load(&dummy, 0);
  EXPECT_EQ(dummy, 0);

  // Cannot load unsupported types.
  VariantTestClass test;
  PORT_EXPECT_DEBUG_DEATH(load(&test, 0), "");

  // Cannot end without calling Begin().
  PORT_EXPECT_DEBUG_DEATH(load.End(), "");

  load.Begin(0);

  // Begin expects a VariantMap at the specified key.
  map.emplace(123, 456);
  PORT_EXPECT_DEBUG_DEATH(load.Begin(456), "");
  PORT_EXPECT_DEBUG_DEATH(load.Begin(123), "");

  // Load with invalid key.
  load(&dummy, 456);
  EXPECT_EQ(dummy, 0);

  float wrong_type = 0.f;
  load(&wrong_type, 123);
  EXPECT_EQ(wrong_type, 0);

  load(&dummy, 123);
  EXPECT_EQ(dummy, 456);
}

TEST(Variant, Vectors) {
  std::vector<std::string> vect = {"abc", "def", "ghi"};
  Variant v = vect;
  auto* vvect = v.Get<std::vector<std::string>>();
  EXPECT_EQ(nullptr, vvect);
  VariantArray* variant_array = v.Get<VariantArray>();
  EXPECT_NE(nullptr, variant_array);
  EXPECT_EQ("abc", *(*variant_array)[0].Get<std::string>());
  EXPECT_EQ("def", *(*variant_array)[1].Get<std::string>());
  EXPECT_EQ("ghi", *(*variant_array)[2].Get<std::string>());
}

TEST(Variant, Maps) {
  std::map<HashValue, std::string> m = {{0, "abc"}, {1, "def"}, {2, "ghi"}};
  Variant v = m;
  auto* vm = v.Get<std::map<HashValue, std::string>>();
  EXPECT_EQ(nullptr, vm);
  VariantMap* variant_map = v.Get<VariantMap>();
  EXPECT_NE(nullptr, variant_map);
  EXPECT_EQ("abc", *variant_map->find(0)->second.Get<std::string>());
  EXPECT_EQ("def", *variant_map->find(1)->second.Get<std::string>());
  EXPECT_EQ("ghi", *variant_map->find(2)->second.Get<std::string>());
}

TEST(Variant, UnorderedMaps) {
  std::unordered_map<HashValue, std::string> m = {
      {0, "abc"}, {1, "def"}, {2, "ghi"}};
  Variant v = m;
  auto* vm = v.Get<std::unordered_map<HashValue, std::string>>();
  EXPECT_EQ(nullptr, vm);
  VariantMap* variant_map = v.Get<VariantMap>();
  EXPECT_NE(nullptr, variant_map);
  EXPECT_EQ("abc", *variant_map->find(0)->second.Get<std::string>());
  EXPECT_EQ("def", *variant_map->find(1)->second.Get<std::string>());
  EXPECT_EQ("ghi", *variant_map->find(2)->second.Get<std::string>());
}

TEST(Variant, Optionals) {
  Optional<float> o1(2.f);
  Optional<float> o2;
  Variant v1 = o1;
  Variant v2 = o2;
  auto* vo1 = v1.Get<Optional<float>>();
  auto* vo2 = v2.Get<Optional<float>>();
  EXPECT_EQ(nullptr, vo1);
  EXPECT_EQ(nullptr, vo2);
  float* f1 = v1.Get<float>();
  float* f2 = v2.Get<float>();
  EXPECT_NE(nullptr, f1);
  EXPECT_EQ(nullptr, f2);
  EXPECT_EQ(2.f, *f1);
  EXPECT_EQ(true, v2.Empty());
}

struct CopyCounter {
  static int copies;
  static int moves;
  CopyCounter() {}
  CopyCounter(const CopyCounter&) { ++copies; }
  CopyCounter& operator=(const CopyCounter&) {
    ++copies;
    return *this;
  }
  CopyCounter(CopyCounter&&) { ++moves; }
  CopyCounter& operator=(CopyCounter&&) {
    ++moves;
    return *this;
  }
};

int CopyCounter::copies;
int CopyCounter::moves;

TEST(Variant, CopyCountOptional) {
  Optional<CopyCounter> optional = CopyCounter();
  CopyCounter::copies = 0;
  CopyCounter::moves = 0;

  // Copy constructing a optional should copy the elements.
  Variant v = optional;
  EXPECT_EQ(1, CopyCounter::copies);
  EXPECT_EQ(0, CopyCounter::moves);

  // Move constructing a optional should move the elements.
  Variant v2 = std::move(optional);
  EXPECT_EQ(1, CopyCounter::copies);
  EXPECT_EQ(1, CopyCounter::moves);
}

TEST(Variant, CopyCountVector) {
  std::vector<CopyCounter> vect(3);
  CopyCounter::copies = 0;
  CopyCounter::moves = 0;

  // Copy constructing a vector should copy the elements.
  Variant v = vect;
  EXPECT_EQ(3, CopyCounter::copies);
  EXPECT_EQ(0, CopyCounter::moves);

  // Move constructing a vector should move the elements.
  Variant v2 = std::move(vect);
  EXPECT_EQ(3, CopyCounter::copies);
  EXPECT_EQ(3, CopyCounter::moves);
}

TEST(Variant, CopyCountVariantArray) {
  VariantArray va = {CopyCounter(), CopyCounter(), CopyCounter()};
  CopyCounter::copies = 0;
  CopyCounter::moves = 0;

  // Copy constructing a VariantArray should copy the elements.
  Variant v = va;
  EXPECT_EQ(3, CopyCounter::copies);
  EXPECT_EQ(0, CopyCounter::moves);

  // Move constructing a VariantArray should neither move nor copy the elements.
  Variant v2 = std::move(va);
  EXPECT_EQ(3, CopyCounter::copies);
  EXPECT_EQ(0, CopyCounter::moves);
}

TEST(Variant, CopyCountUnorderedMap) {
  std::unordered_map<HashValue, CopyCounter> map = {
      {0, CopyCounter()}, {1, CopyCounter()}, {2, CopyCounter()}};
  CopyCounter::copies = 0;
  CopyCounter::moves = 0;

  // Copy constructing a map should copy the elements.
  Variant v = map;
  EXPECT_EQ(3, CopyCounter::copies);
  EXPECT_EQ(0, CopyCounter::moves);

  // Move constructing a map should move the elements.
  Variant v2 = std::move(map);
  EXPECT_EQ(3, CopyCounter::copies);
  EXPECT_EQ(3, CopyCounter::moves);
}

TEST(Variant, CopyCountVariantMap) {
  VariantMap vm = {
      {0, CopyCounter()}, {1, CopyCounter()}, {2, CopyCounter()}};
  CopyCounter::copies = 0;
  CopyCounter::moves = 0;

  // Copy constructing a VariantMap should copy the elements.
  Variant v = vm;
  EXPECT_EQ(3, CopyCounter::copies);
  EXPECT_EQ(0, CopyCounter::moves);

  // Move constructing a VariantMap should neither move nor copy the elements.
  Variant v2 = std::move(vm);
  EXPECT_EQ(3, CopyCounter::copies);
  EXPECT_EQ(0, CopyCounter::moves);
}

TEST(Variant, CopyCountOrderedMapOfVariants) {
  std::map<HashValue, Variant> map = {
      {0, CopyCounter()}, {1, CopyCounter()}, {2, CopyCounter()}};
  CopyCounter::copies = 0;
  CopyCounter::moves = 0;

  // Copy constructing a map should copy the elements.
  Variant v = map;
  EXPECT_EQ(3, CopyCounter::copies);
  EXPECT_EQ(0, CopyCounter::moves);

  // Move constructing a map should move the elements.
  Variant v2 = std::move(map);
  EXPECT_EQ(3, CopyCounter::copies);
  EXPECT_EQ(3, CopyCounter::moves);
}

}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(VariantTestClass);
LULLABY_SETUP_TYPEID(MoveOnlyVariantTestClass);
LULLABY_SETUP_TYPEID(CopyCounter);
