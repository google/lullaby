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

#include "lullaby/util/variant.h"

#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/dispatcher/event_wrapper.h"
#include "lullaby/modules/serialize/serialize.h"
#include "lullaby/modules/serialize/variant_serializer.h"
#include "lullaby/util/logging.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {

enum VariantTestEnum {
  Foo,
  Bar,
  Baz,
};

struct VariantTestClass {
  VariantTestClass() {}
  VariantTestClass(int key, int value) : key(key), value(value) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&key, lull::ConstHash("key"));
    archive(&value, lull::ConstHash("value"));
  }

  int key = 0;
  int value = 0;
};

struct ComplexTestClass : VariantTestClass {
  ComplexTestClass() {}

  template <typename Archive>
  void Serialize(Archive archive) {
    VariantTestClass::Serialize(archive);
    archive(&other, lull::ConstHash("other"));
    archive(&bytes, lull::ConstHash("bytes"));
    archive(&word, lull::ConstHash("word"));
    archive(&arr, lull::ConstHash("arr"));
    archive(&map, lull::ConstHash("map"));
    archive(&optional, lull::ConstHash("optional"));
    archive(&optional_unset, lull::ConstHash("optional_unset"));
    archive(&entity, lull::ConstHash("entity"));
  }

  VariantTestClass other;
  std::string word;
  lull::ByteArray bytes;
  std::vector<int> arr;
  std::unordered_map<lull::HashValue, float> map;
  Optional<float> optional;
  Optional<float> optional_unset;
  Entity entity;
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

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::VariantTestClass);
LULLABY_SETUP_TYPEID(lull::MoveOnlyVariantTestClass);
LULLABY_SETUP_TYPEID(lull::CopyCounter);
LULLABY_SETUP_TYPEID(lull::VariantTestEnum);

namespace lull {
namespace {

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

TEST(Variant, Strings) {
  lull::Variant var;
  EXPECT_TRUE(var.Empty());

  var = std::string("abc");
  EXPECT_FALSE(var.Empty());
  ASSERT_NE(var.Get<std::string>(), nullptr);
  EXPECT_EQ(*var.Get<std::string>(), "abc");
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

TEST(Variant, Enum) {
  Variant v1 = Bar;
  EXPECT_EQ(v1.GetTypeId(), GetTypeId<VariantTestEnum>());

  Variant v2 = v1;
  EXPECT_EQ(v2.GetTypeId(), GetTypeId<VariantTestEnum>());

  // Test lvalue instead of rvalue.
  VariantTestEnum e = Baz;
  Variant v3 = e;
  EXPECT_EQ(v3.GetTypeId(), GetTypeId<VariantTestEnum>());
  Variant v4;

  v2 = 0;
  EXPECT_EQ(v2.GetTypeId(), GetTypeId<int>());

  lull::VariantMap map;
  lull::SaveToVariant save(&map);
  lull::Serialize(&save, &v1, 0);
  lull::Serialize(&save, &v3, 1);

  lull::LoadFromVariant load(&map);
  lull::Serialize(&load, &v2, 0);
  lull::Serialize(&load, &v4, 1);

  EXPECT_EQ(v2.GetTypeId(), GetTypeId<VariantTestEnum>());
  EXPECT_EQ(v4.GetTypeId(), GetTypeId<VariantTestEnum>());
}

TEST(Variant, VariantSerializer) {
  ComplexTestClass u1;
  u1.key = 123;
  u1.value = 456;
  u1.other.key = 789;
  u1.other.value = 987654321;
  u1.bytes = {1, 2, 3, 4, 5};
  u1.word = "hello";
  u1.arr.push_back(10);
  u1.arr.push_back(11);
  u1.arr.push_back(12);
  u1.map[123] = 123.f;
  u1.map[456] = 456.f;
  u1.map[789] = 789.f;
  u1.optional = 13.f;
  u1.entity = Entity(111);

  lull::VariantMap map;
  lull::SaveToVariant save(&map);
  lull::Serialize(&save, &u1, 0);

  ComplexTestClass u2;
  lull::LoadFromVariant load(&map);
  lull::Serialize(&load, &u2, 0);

  EXPECT_EQ(u1.key, u2.key);
  EXPECT_EQ(u1.value, u2.value);
  EXPECT_EQ(u1.word, u2.word);
  EXPECT_EQ(u1.bytes, u2.bytes);
  EXPECT_EQ(u1.other.key, u2.other.key);
  EXPECT_EQ(u1.other.value, u2.other.value);
  EXPECT_EQ(u1.arr, u2.arr);
  EXPECT_EQ(u1.map, u2.map);
  EXPECT_EQ(u1.optional, u2.optional);
  EXPECT_EQ(u1.optional_unset, u2.optional_unset);
  EXPECT_EQ(u1.entity, u2.entity);
  {
    const auto other_key = lull::ConstHash("other");
    const auto& other_var = map.find(other_key)->second;
    const auto& other_map = other_var.Get<lull::VariantMap>();

    const auto value_key = lull::ConstHash("value");
    const auto& value_var = other_map->find(value_key)->second;
    const auto& value_ptr = value_var.Get<int>();
    EXPECT_EQ(987654321, *value_ptr);
  }
  {
    const auto map_key = lull::ConstHash("map");
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
  ASSERT_NE(nullptr, f1);
  EXPECT_EQ(nullptr, f2);
  EXPECT_EQ(2.f, *f1);
  EXPECT_EQ(true, v2.Empty());
}

TEST(Variant, Entities) {
  Entity e(123);
  uint32_t u = 456;
  Variant ve = e;
  Variant vu = u;
  auto* e1 = ve.Get<uint32_t>();
  auto* u1 = vu.Get<Entity>();
  EXPECT_EQ(nullptr, e1);
  EXPECT_EQ(nullptr, u1);
  Entity* e2 = ve.Get<Entity>();
  uint32_t* u2 = vu.Get<uint32_t>();
  ASSERT_NE(nullptr, e2);
  ASSERT_NE(nullptr, u2);
  EXPECT_EQ(*e2, Entity(123));
  EXPECT_EQ(*u2, 456);
}

TEST(Variant, EventHandlers) {
  int count = 0;
  Variant var;
  {
    Dispatcher::EventHandler handler = [&count](const EventWrapper& event) {
      ++count;
      EXPECT_EQ(event.GetTypeId(), Hash("myEvent"));
      auto* ptr = event.GetValue<int>(Hash("myInt"));
      ASSERT_NE(ptr, nullptr);
      EXPECT_EQ(*ptr, 123);
    };
    var = handler;
  }
  auto* ptr = var.Get<Dispatcher::EventHandler>();
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(count, 0);
  EventWrapper event(Hash("myEvent"));
  event.SetValue(Hash("myInt"), 123);
  (*ptr)(event);
  EXPECT_EQ(count, 1);
}

TEST(Variant, LargeTypes) {
  const mathfu::mat4 d0(8, 7, 9, 1, 2, 3, 7, 8, 2, 5, 1, 7, 8, 0, 2, 3);
  Variant v0 = d0;
  const mathfu::mat4* const result_d0 = v0.Get<mathfu::mat4>();
  ASSERT_NE(result_d0, nullptr);
  EXPECT_EQ(*result_d0, d0);

  // Copy large type to large type.
  Variant copy_v0 = v0;
  const mathfu::mat4* const result_copy_d0 = copy_v0.Get<mathfu::mat4>();
  ASSERT_NE(result_copy_d0, nullptr);
  EXPECT_EQ(*result_copy_d0, d0);

  // Replace large type with large type.
  const mathfu::mat4 d1(7, 8, 0, 2, 3, 7, 8, 9, 4, 9, 0, 8, 2, 3, 7, 2);
  Variant v1 = d1;
  v1 = v0;
  const mathfu::mat4* const result_replace_d1 = v1.Get<mathfu::mat4>();
  ASSERT_NE(result_replace_d1, nullptr);
  EXPECT_EQ(*result_replace_d1, d0);

  // Replace small type with large type.
  const int d2 = 342;
  Variant v2 = d2;
  v2 = v0;
  const mathfu::mat4* const result_replace_d2 = v2.Get<mathfu::mat4>();
  ASSERT_NE(result_replace_d2, nullptr);
  EXPECT_EQ(*result_replace_d2, d0);

  // Replace large type with small type.
  const mathfu::mat4 d3(7, 8, 9, 0, 1, 2, 7, 1, 2, 0, 9, 1, 2, 7, 3, 0);
  const int i3 = 7912;
  Variant v3 = d3;
  Variant vi3 = i3;
  v3 = vi3;
  const int* const result_replace_i3 = v3.Get<int>();
  ASSERT_NE(result_replace_i3, nullptr);
  EXPECT_EQ(*result_replace_i3, i3);

  // Move large type to large type.
  const mathfu::mat4 d4(7, 8, 0, 2, 3, 7, 8, 9, 4, 9, 0, 8, 2, 3, 7, 2);
  Variant v4 = d4;
  v4 = std::move(v0);
  const mathfu::mat4* const result_move_d4 = v4.Get<mathfu::mat4>();
  ASSERT_NE(result_move_d4, nullptr);
  EXPECT_EQ(*result_move_d4, d0);
  v0 = d0;

  // Move small type to large type.
  const int d5 = 342;
  Variant v5 = d5;
  v5 = std::move(v0);
  const mathfu::mat4* const result_move_d5 = v5.Get<mathfu::mat4>();
  ASSERT_NE(result_move_d5, nullptr);
  EXPECT_EQ(*result_move_d5, d0);
  v0 = d0;

  // Move large type to small type.
  const mathfu::mat4 d6(7, 8, 9, 0, 1, 2, 7, 1, 2, 0, 9, 1, 2, 7, 3, 0);
  const int i6 = 43570;
  Variant v6 = d6;
  Variant vi6 = i6;
  v6 = std::move(vi6);
  const int* const result_move_d6 = v6.Get<int>();
  ASSERT_NE(result_move_d6, nullptr);
  EXPECT_EQ(*result_move_d6, i6);
}

TEST(Variant, ImplicitCastNumeric) {
  int i = 1;
  Variant vi = i;
  auto vi1 = vi.ImplicitCast<unsigned int>();
  auto vi2 = vi.ImplicitCast<float>();
  auto vi3 = vi.ImplicitCast<VariantTestClass>();
  ASSERT_TRUE(vi1);
  ASSERT_TRUE(vi2);
  EXPECT_FALSE(vi3);
  EXPECT_EQ(1u, *vi1);
  EXPECT_EQ(1.f, *vi2);

  unsigned int u = 2;
  Variant vu = u;
  auto vu1 = vu.ImplicitCast<int>();
  auto vu2 = vu.ImplicitCast<float>();
  auto vu3 = vu.ImplicitCast<VariantTestClass>();
  ASSERT_TRUE(vu1);
  ASSERT_TRUE(vu2);
  EXPECT_FALSE(vu3);
  EXPECT_EQ(2, *vu1);
  EXPECT_EQ(2.f, *vu2);

  float f = 1.5f;
  Variant vf = f;
  auto vf1 = vf.ImplicitCast<int>();
  auto vf2 = vf.ImplicitCast<unsigned int>();
  auto vf3 = vf.ImplicitCast<VariantTestClass>();
  ASSERT_TRUE(vf1);
  ASSERT_TRUE(vf2);
  EXPECT_FALSE(vf3);
  EXPECT_EQ(1, *vf1);
  EXPECT_EQ(1u, *vf2);

  Variant empty;
  ASSERT_TRUE(empty.Empty());
  auto empty_int = empty.ImplicitCast<int>();
  auto empty_uint = empty.ImplicitCast<unsigned int>();
  auto empty_float = empty.ImplicitCast<float>();
  auto empty_enum = empty.ImplicitCast<VariantTestEnum>();
  auto empty_class = empty.ImplicitCast<VariantTestClass>();
  EXPECT_FALSE(empty_int);
  EXPECT_FALSE(empty_uint);
  EXPECT_FALSE(empty_float);
  EXPECT_FALSE(empty_enum);
  EXPECT_FALSE(empty_class);
}

TEST(Variant, ImplicitCastEnum) {
  int i = 1;
  unsigned int u = 2;
  VariantTestEnum e = Baz;
  Variant vi = i;
  Variant vu = u;
  Variant ve = e;
  auto ci = vi.ImplicitCast<VariantTestEnum>();
  auto cu = vu.ImplicitCast<VariantTestEnum>();
  auto ce = ve.ImplicitCast<unsigned int>();
  ASSERT_TRUE(ci);
  ASSERT_TRUE(cu);
  ASSERT_TRUE(ce);
  EXPECT_EQ(Bar, *ci);
  EXPECT_EQ(Baz, *cu);
  EXPECT_EQ(2u, *ce);
}

TEST(Variant, ImplicitCastEntity) {
  Entity e(123);
  uint32_t u = 456;
  Variant ve = e;
  Variant vu = u;
  auto ce = ve.ImplicitCast<uint32_t>();
  auto cu = vu.ImplicitCast<Entity>();
  ASSERT_TRUE(ce);
  ASSERT_TRUE(cu);
  EXPECT_EQ(*ce, 123);
  EXPECT_EQ(*cu, Entity(456));

  Variant empty;
  auto cempty = empty.ImplicitCast<Entity>();
  ASSERT_TRUE(cempty);
  EXPECT_EQ(*cempty, kNullEntity);
}

TEST(Variant, ImplicitCastRect) {
  mathfu::vec4 vec(1.1f, 2.2f, 3.3f, 4.4f);
  mathfu::vec4i veci(1, 2, 3, 4);
  mathfu::rectf rf(1.1f, 2.2f, 3.3f, 4.4f);
  mathfu::recti ri(1, 2, 3, 4);
  Variant v_vec = vec;
  Variant v_veci = veci;
  Variant v_rf = rf;
  Variant v_ri = ri;

  auto recti_vec = v_vec.ImplicitCast<mathfu::recti>();
  auto rectf_vec = v_vec.ImplicitCast<mathfu::rectf>();
  auto veci_vec = v_vec.ImplicitCast<mathfu::vec4i>();
  ASSERT_TRUE(recti_vec);
  ASSERT_TRUE(rectf_vec);
  ASSERT_TRUE(veci_vec);
  EXPECT_EQ(mathfu::recti(1, 2, 3, 4), *recti_vec);
  EXPECT_EQ(mathfu::rectf(1.1f, 2.2f, 3.3f, 4.4f), *rectf_vec);
  EXPECT_EQ(mathfu::vec4i(1, 2, 3, 4), *veci_vec);

  auto recti_veci = v_veci.ImplicitCast<mathfu::recti>();
  auto rectf_veci = v_veci.ImplicitCast<mathfu::rectf>();
  auto vec_veci = v_veci.ImplicitCast<mathfu::vec4>();
  ASSERT_TRUE(recti_veci);
  ASSERT_TRUE(rectf_veci);
  ASSERT_TRUE(vec_veci);
  EXPECT_EQ(mathfu::recti(1, 2, 3, 4), *recti_veci);
  EXPECT_EQ(mathfu::rectf(1.0f, 2.0f, 3.0f, 4.0f), *rectf_veci);
  EXPECT_EQ(mathfu::vec4(1.0f, 2.0f, 3.0f, 4.0f), *vec_veci);

  auto recti_rectf = v_rf.ImplicitCast<mathfu::recti>();
  auto vec_rectf = v_rf.ImplicitCast<mathfu::vec4>();
  auto veci_rectf = v_rf.ImplicitCast<mathfu::vec4i>();
  ASSERT_TRUE(recti_rectf);
  ASSERT_TRUE(vec_rectf);
  ASSERT_TRUE(veci_rectf);
  EXPECT_EQ(mathfu::recti(1, 2, 3, 4), *recti_rectf);
  EXPECT_EQ(mathfu::vec4(1.1f, 2.2f, 3.3f, 4.4f), *vec_rectf);
  EXPECT_EQ(mathfu::vec4i(1, 2, 3, 4), *veci_rectf);

  auto rectf_recti = v_ri.ImplicitCast<mathfu::rectf>();
  auto vec_recti = v_ri.ImplicitCast<mathfu::vec4>();
  auto veci_recti = v_ri.ImplicitCast<mathfu::vec4i>();
  ASSERT_TRUE(rectf_recti);
  ASSERT_TRUE(vec_recti);
  ASSERT_TRUE(veci_recti);
  EXPECT_EQ(mathfu::rectf(1.0f, 2.0f, 3.0f, 4.0f), *rectf_recti);
  EXPECT_EQ(mathfu::vec4(1.0f, 2.0f, 3.0f, 4.0f), *vec_recti);
  EXPECT_EQ(mathfu::vec4i(1, 2, 3, 4), *veci_recti);
}

TEST(Variant, ImplicitCastDuration) {
  int64_t time = 123;
  uint64_t utime = 123u;
  Variant v_time = time;
  Variant v_utime = utime;
  auto v_duration1 = v_time.ImplicitCast<Clock::duration>();
  auto v_duration2 = v_utime.ImplicitCast<Clock::duration>();
  ASSERT_TRUE(v_duration1);
  ASSERT_TRUE(v_duration2);
  EXPECT_EQ(Clock::duration(123), *v_duration1);
  EXPECT_EQ(Clock::duration(123), *v_duration2);
}

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
