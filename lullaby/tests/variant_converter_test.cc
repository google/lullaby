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

#include "lullaby/modules/function/variant_converter.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

struct Serializable {
  Serializable() {}
  Serializable(int a, float b, const std::string& c) : a(a), b(b), c(c) {}
  int a = 0;
  float b = 0.f;
  std::string c;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&a, Hash("a"));
    archive(&b, Hash("b"));
    archive(&c, Hash("c"));
  }
};

TEST(VariantConverter, ToVariant) {
  Variant a = true;
  Variant b = false;
  bool res = VariantConverter::ToVariant(a, &b);
  EXPECT_TRUE(res);
  EXPECT_EQ(a.GetTypeId(), b.GetTypeId());
  EXPECT_EQ(*(a.Get<bool>()), *(b.Get<bool>()));

  a = 13;
  res = VariantConverter::ToVariant(a, &b);
  EXPECT_TRUE(res);
  EXPECT_EQ(a.GetTypeId(), b.GetTypeId());
  EXPECT_EQ(*(a.Get<int>()), *(b.Get<int>()));
}

TEST(VariantConverter, FromVariant) {
  Variant a = true;
  Variant b = false;
  bool res = VariantConverter::FromVariant(a, &b);
  EXPECT_TRUE(res);
  EXPECT_EQ(a.GetTypeId(), b.GetTypeId());
  EXPECT_EQ(*(a.Get<bool>()), *(b.Get<bool>()));

  a = 13;
  res = VariantConverter::FromVariant(a, &b);
  EXPECT_TRUE(res);
  EXPECT_EQ(a.GetTypeId(), b.GetTypeId());
  EXPECT_EQ(*(a.Get<int>()), *(b.Get<int>()));
}

TEST(VariantConverter, FromBool) {
  Variant var = true;
  bool value = false;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value, true);
}

TEST(VariantConverter, ToBool) {
  Variant var;
  bool value = true;
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(false), true);
}

TEST(VariantConverter, FromInt32) {
  Variant var = 123;
  int32_t value = 0;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value, 123);
}

TEST(VariantConverter, ToInt32) {
  Variant var;
  int32_t value = 123;
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(0), 123);
}

TEST(VariantConverter, FromUInt32) {
  Variant var = uint32_t(123);
  uint32_t value = 0;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value, uint32_t(123));
}

TEST(VariantConverter, ToUInt32) {
  Variant var;
  uint32_t value = 123;
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(uint32_t(0)), uint32_t(123));
}

TEST(VariantConverter, FromInt64) {
  Variant var = int64_t(123);
  int64_t value = 0;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value, int64_t(123));
}

TEST(VariantConverter, ToInt64) {
  Variant var;
  int64_t value = 123;
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(int64_t(0)), int64_t(123));
}

TEST(VariantConverter, FromUInt64) {
  Variant var = uint64_t(123);
  uint64_t value = 0;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value, uint64_t(123));
}

TEST(VariantConverter, ToUInt64) {
  Variant var;
  uint64_t value = 123;
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(uint64_t(0)), uint64_t(123));
}

TEST(VariantConverter, FromFloat) {
  Variant var = 123.f;
  float value = 0.f;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value, 123.f);
}

TEST(VariantConverter, ToFloat) {
  Variant var;
  float value = 123.f;
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(0.f), 123.f);
}

TEST(VariantConverter, FromDouble) {
  Variant var = 123.0;
  double value = 0.0;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value, 123.0);
}

TEST(VariantConverter, ToDouble) {
  Variant var;
  double value = 123.0;
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(0.0), 123.0);
}

TEST(VariantConverter, FromString) {
  Variant var = std::string("hello");
  std::string value;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value, std::string("hello"));
}

TEST(VariantConverter, ToString) {
  Variant var;
  std::string value = "hello";
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(std::string()), std::string("hello"));
}

TEST(VariantConverter, FromVec2) {
  Variant var = mathfu::vec2(1.f, 2.f);
  mathfu::vec2 value;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value, mathfu::vec2(1.f, 2.f));
}

TEST(VariantConverter, ToVec2) {
  Variant var;
  mathfu::vec2 value(1.f, 2.f);
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(mathfu::kZeros2f), mathfu::vec2(1.f, 2.f));
}

TEST(VariantConverter, FromVec2i) {
  Variant var = mathfu::vec2i(1, 2);
  mathfu::vec2i value;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value, mathfu::vec2i(1, 2));
}

TEST(VariantConverter, ToVec2i) {
  Variant var;
  mathfu::vec2i value(1, 2);
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(mathfu::kZeros2i), mathfu::vec2i(1, 2));
}

TEST(VariantConverter, FromOptionalEmpty) {
  Variant var = Optional<float>();
  Optional<float> value = 123.f;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_FALSE(value);
}

TEST(VariantConverter, ToOptionalEmpty) {
  Variant var = Optional<float>(123.f);
  Optional<float> value;
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_TRUE(var.Empty());
}

TEST(VariantConverter, FromOptional) {
  Variant var = Optional<float>(123.f);
  Optional<float> value;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_TRUE(value);
  EXPECT_EQ(value.value_or(0.f), 123.f);
}

TEST(VariantConverter, ToOptional) {
  Variant var;
  Optional<float> value = 123.f;
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  EXPECT_EQ(var.ValueOr(0.f), 123.f);
}

TEST(VariantConverter, FromSerializable) {
  VariantMap map;
  map[Hash("a")] = 1;
  map[Hash("b")] = 2.f;
  map[Hash("c")] = std::string("3");
  Variant var = map;
  Serializable value;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value.a, 1);
  EXPECT_EQ(value.b, 2.f);
  EXPECT_EQ(value.c, std::string("3"));
}

TEST(VariantConverter, ToSerializable) {
  Variant var;
  Serializable value(1, 2.f, "3");
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  const VariantMap* map = var.Get<VariantMap>();
  EXPECT_TRUE(map != nullptr);

  auto iter1 = map->find(Hash("a"));
  EXPECT_NE(iter1, map->end());
  EXPECT_EQ(iter1->second.ValueOr(0), 1);

  auto iter2 = map->find(Hash("b"));
  EXPECT_NE(iter2, map->end());
  EXPECT_EQ(iter2->second.ValueOr(0.f), 2);

  auto iter3 = map->find(Hash("c"));
  EXPECT_NE(iter3, map->end());
  EXPECT_EQ(iter3->second.ValueOr(std::string()), std::string("3"));
}

TEST(VariantConverter, FromVector) {
  VariantArray arr;
  arr.emplace_back(1);
  arr.emplace_back(2);
  arr.emplace_back(3);
  Variant var = arr;
  std::vector<int> value;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value[0], 1);
  EXPECT_EQ(value[1], 2);
  EXPECT_EQ(value[2], 3);
}

TEST(VariantConverter, ToVector) {
  Variant var;
  std::vector<int> value = {1, 2, 3};
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);

  const VariantArray* arr = var.Get<VariantArray>();
  EXPECT_TRUE(arr != nullptr);
  EXPECT_EQ(arr->size(), size_t(3));
  EXPECT_EQ((*arr)[0].ValueOr(0), 1);
  EXPECT_EQ((*arr)[1].ValueOr(0), 2);
  EXPECT_EQ((*arr)[2].ValueOr(0), 3);
}

TEST(VariantConverter, FromUnorderedMap) {
  VariantMap map;
  map.emplace(Hash("a"), std::string("a"));
  map.emplace(Hash("b"), std::string("b"));
  map.emplace(Hash("c"), std::string("c"));
  Variant var = map;
  std::unordered_map<HashValue, std::string> value;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value[Hash("a")], std::string("a"));
  EXPECT_EQ(value[Hash("b")], std::string("b"));
  EXPECT_EQ(value[Hash("c")], std::string("c"));
}

TEST(VariantConverter, ToUnorderedMap) {
  Variant var;
  std::unordered_map<HashValue, std::string> value = {
      {Hash("a"), "a"},
      {Hash("b"), "b"},
      {Hash("c"), "c"},
  };
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);
  const VariantMap* map = var.Get<VariantMap>();
  EXPECT_TRUE(map != nullptr);
  EXPECT_EQ(map->size(), size_t(3));

  auto iter1 = map->find(Hash("a"));
  EXPECT_NE(iter1, map->end());
  EXPECT_EQ(iter1->second.ValueOr(std::string()), std::string("a"));

  auto iter2 = map->find(Hash("b"));
  EXPECT_NE(iter2, map->end());
  EXPECT_EQ(iter2->second.ValueOr(std::string()), std::string("b"));

  auto iter3 = map->find(Hash("c"));
  EXPECT_NE(iter3, map->end());
  EXPECT_EQ(iter3->second.ValueOr(std::string()), std::string("c"));
}

TEST(VariantConverter, FromCompoundType) {
  using Array = std::vector<Serializable>;
  using InnerMap = std::unordered_map<HashValue, Array>;
  using OuterMap = std::unordered_map<HashValue, InnerMap>;

  VariantMap s1;
  s1[Hash("a")] = 1;
  s1[Hash("b")] = 1.f;
  s1[Hash("c")] = std::string("1");

  VariantMap s2;
  s2[Hash("a")] = 2;
  s2[Hash("b")] = 2.f;
  s2[Hash("c")] = std::string("2");

  VariantMap s3;
  s3[Hash("a")] = 3;
  s3[Hash("b")] = 3.f;
  s3[Hash("c")] = std::string("3");

  VariantMap s4;
  s4[Hash("a")] = 4;
  s4[Hash("b")] = 4.f;
  s4[Hash("c")] = std::string("4");

  VariantArray a1;
  a1.emplace_back(s1);
  a1.emplace_back(s2);

  VariantArray a2;
  a2.emplace_back(s3);
  a2.emplace_back(s4);

  VariantMap inner1;
  inner1.emplace(Hash("1"), a1);

  VariantMap inner2;
  inner2.emplace(Hash("2"), a2);

  VariantMap outer;
  outer.emplace(Hash("a"), inner1);
  outer.emplace(Hash("b"), inner2);

  Variant var = outer;
  OuterMap value;
  const bool res = VariantConverter::FromVariant(var, &value);
  EXPECT_TRUE(res);
  EXPECT_EQ(value.size(), size_t(2));

  auto iter1 = value.find(Hash("a"));
  EXPECT_NE(iter1, value.end());
  EXPECT_EQ(iter1->second.size(), size_t(1));

  auto iter2 = iter1->second.find(Hash("1"));
  EXPECT_NE(iter2, iter1->second.end());

  EXPECT_EQ(iter2->second.size(), size_t(2));
  EXPECT_EQ(iter2->second[0].a, 1);
  EXPECT_EQ(iter2->second[0].b, 1.f);
  EXPECT_EQ(iter2->second[0].c, std::string("1"));
  EXPECT_EQ(iter2->second[1].a, 2);
  EXPECT_EQ(iter2->second[1].b, 2.f);
  EXPECT_EQ(iter2->second[1].c, std::string("2"));

  iter1 = value.find(Hash("b"));
  EXPECT_NE(iter1, value.end());
  EXPECT_EQ(iter1->second.size(), size_t(1));

  iter2 = iter1->second.find(Hash("2"));
  EXPECT_NE(iter2, iter1->second.end());

  EXPECT_EQ(iter2->second.size(), size_t(2));
  EXPECT_EQ(iter2->second[0].a, 3);
  EXPECT_EQ(iter2->second[0].b, 3.f);
  EXPECT_EQ(iter2->second[0].c, std::string("3"));
  EXPECT_EQ(iter2->second[1].a, 4);
  EXPECT_EQ(iter2->second[1].b, 4.f);
  EXPECT_EQ(iter2->second[1].c, std::string("4"));
}

TEST(VariantConverter, ToCompoundType) {
  using Array = std::vector<Serializable>;
  using InnerMap = std::unordered_map<HashValue, Array>;
  using OuterMap = std::unordered_map<HashValue, InnerMap>;

  Serializable s1(1, 1.f, "1");
  Serializable s2(2, 2.f, "2");
  Serializable s3(3, 3.f, "3");
  Serializable s4(4, 4.f, "4");

  Array a1 = {s1, s2};
  Array a2 = {s3, s4};
  InnerMap m1 = {{Hash("a"), a1}};
  InnerMap m2 = {{Hash("b"), a2}};

  Variant var;
  OuterMap value = {
      {Hash("1"), m1},
      {Hash("2"), m2},
  };
  const bool res = VariantConverter::ToVariant(value, &var);
  EXPECT_TRUE(res);

  const VariantMap* outer = var.Get<VariantMap>();
  EXPECT_TRUE(outer != nullptr);
  EXPECT_EQ(outer->size(), size_t(2));

  auto iter1 = outer->find(Hash("1"));
  EXPECT_NE(iter1, outer->end());

  const VariantMap* inner1 = iter1->second.Get<VariantMap>();
  EXPECT_TRUE(inner1 != nullptr);
  EXPECT_EQ(inner1->size(), size_t(1));

  auto iter11 = inner1->find(Hash("a"));
  EXPECT_NE(iter11, inner1->end());

  const VariantArray* arr1 = iter11->second.Get<VariantArray>();
  EXPECT_TRUE(arr1 != nullptr);
  EXPECT_EQ(arr1->size(), size_t(2));

  const auto* obj1 = arr1->at(0).Get<VariantMap>();
  EXPECT_TRUE(obj1 != nullptr);
  EXPECT_EQ(obj1->size(), size_t(3));
  EXPECT_EQ(obj1->find(Hash("a"))->second.ValueOr(0), 1);
  EXPECT_EQ(obj1->find(Hash("b"))->second.ValueOr(0.f), 1.f);
  EXPECT_EQ(obj1->find(Hash("c"))->second.ValueOr(std::string()),
            std::string("1"));

  const auto* obj2 = arr1->at(1).Get<VariantMap>();
  EXPECT_TRUE(obj2 != nullptr);
  EXPECT_EQ(obj2->size(), size_t(3));
  EXPECT_EQ(obj2->find(Hash("a"))->second.ValueOr(0), 2);
  EXPECT_EQ(obj2->find(Hash("b"))->second.ValueOr(0.f), 2.f);
  EXPECT_EQ(obj2->find(Hash("c"))->second.ValueOr(std::string()),
            std::string("2"));

  auto iter2 = outer->find(Hash("2"));
  EXPECT_NE(iter2, outer->end());

  const VariantMap* inner2 = iter2->second.Get<VariantMap>();
  EXPECT_TRUE(inner2 != nullptr);
  EXPECT_EQ(inner2->size(), size_t(1));

  auto iter22 = inner2->find(Hash("b"));
  EXPECT_NE(iter22, inner2->end());

  const VariantArray* arr2 = iter22->second.Get<VariantArray>();
  EXPECT_TRUE(arr2 != nullptr);
  EXPECT_EQ(arr2->size(), size_t(2));

  const auto* obj3 = arr2->at(0).Get<VariantMap>();
  EXPECT_TRUE(obj3 != nullptr);
  EXPECT_EQ(obj3->size(), size_t(3));
  EXPECT_EQ(obj3->find(Hash("a"))->second.ValueOr(0), 3);
  EXPECT_EQ(obj3->find(Hash("b"))->second.ValueOr(0.f), 3.f);
  EXPECT_EQ(obj3->find(Hash("c"))->second.ValueOr(std::string()),
            std::string("3"));

  const auto* obj4 = arr2->at(1).Get<VariantMap>();
  EXPECT_TRUE(obj4 != nullptr);
  EXPECT_EQ(obj4->size(), size_t(3));
  EXPECT_EQ(obj4->find(Hash("a"))->second.ValueOr(0), 4);
  EXPECT_EQ(obj4->find(Hash("b"))->second.ValueOr(0.f), 4.f);
  EXPECT_EQ(obj4->find(Hash("c"))->second.ValueOr(std::string()),
            std::string("4"));
}

}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::Serializable);
