/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/var/var_convert.h"

namespace redux {
namespace {

enum class TestEnum {
  Monday,
  Tuesday,
  Wednesday,
  Thursday,
  Friday,
};

}  // namespace
}  // namespace redux

REDUX_SETUP_TYPEID(redux::TestEnum);

namespace redux {
namespace {

using ::testing::Eq;

TEST(VarConvert, PrimitiveToVar) {
  Var var;
  int value = 123;
  EXPECT_TRUE(ToVar(value, &var));
  EXPECT_THAT(var.ValueOr(0), Eq(123));
}

TEST(VarConvert, PrimitiveFromVar) {
  Var var = 123;
  int value = 0;
  ASSERT_TRUE(FromVar(var, &value));
  EXPECT_THAT(value, Eq(123));
}

TEST(VarConvert, EnumToFromEnum) {
  Var var = TestEnum::Thursday;
  TestEnum value = TestEnum::Monday;
  ASSERT_TRUE(FromVar(var, &value));
  EXPECT_THAT(value, Eq(TestEnum::Thursday));
}

TEST(VarConvert, EnumFromHashValues) {
  Var var = ConstHash("Thursday");
  TestEnum value = TestEnum::Monday;
  ASSERT_TRUE(FromVar(var, &value));
  EXPECT_THAT(value, Eq(TestEnum::Thursday));
}

TEST(VarConvert, PrimitiveFromEnum) {
  Var var = 2;
  TestEnum value = TestEnum::Monday;
  ASSERT_TRUE(FromVar(var, &value));
  EXPECT_THAT(value, Eq(TestEnum::Wednesday));
}

TEST(VarConvert, InvalidPrimitiveFromEnum) {
  Var var = 20;
  TestEnum value = TestEnum::Monday;
  ASSERT_FALSE(FromVar(var, &value));
}

TEST(VarConvert, StringToVar) {
  Var var;
  std::string value = "hello";
  EXPECT_TRUE(ToVar(value, &var));
  EXPECT_THAT(var.ValueOr(std::string()), Eq("hello"));
}

TEST(VarConvert, StringFromVar) {
  Var var = std::string("hello");
  std::string value;
  ASSERT_TRUE(FromVar(var, &value));
  EXPECT_THAT(value, Eq("hello"));
}

TEST(VarConvert, RawPtrToVar) {
  auto ptr = std::make_unique<std::string>("hello");

  Var var;
  std::string* value = ptr.get();
  EXPECT_TRUE(ToVar(value, &var));

  TypedPtr typed_ptr = var.ValueOr(TypedPtr());
  EXPECT_THAT(typed_ptr.GetTypeId(), Eq(GetTypeId<std::string>()));
  EXPECT_THAT(*typed_ptr.Get<std::string>(), Eq("hello"));
}

TEST(VarConvert, UniquePtrToVar) {
  Var var;
  auto ptr = std::make_unique<std::string>("hello");
  EXPECT_TRUE(ToVar(ptr, &var));

  TypedPtr typed_ptr = var.ValueOr(TypedPtr());
  EXPECT_THAT(typed_ptr.GetTypeId(), Eq(GetTypeId<std::string>()));
  EXPECT_THAT(*typed_ptr.Get<std::string>(), Eq("hello"));
}

TEST(VarConvert, RawPtrFromVar) {
  auto ptr = std::make_unique<std::string>("hello");

  Var var = TypedPtr(ptr.get());
  std::string* value = nullptr;
  ASSERT_TRUE(FromVar(var, &value));
  EXPECT_TRUE(value != nullptr);
  EXPECT_THAT(*value, Eq("hello"));
}

TEST(VarConvert, OptionalToVar) {
  Var var;
  std::optional<std::string> value = "hello";
  EXPECT_TRUE(ToVar(value, &var));
  EXPECT_THAT(var.ValueOr(std::string()), Eq("hello"));
}

TEST(VarConvert, NullOptionalToVar) {
  Var var;
  std::optional<std::string> value = std::nullopt;
  EXPECT_TRUE(ToVar(value, &var));
  EXPECT_TRUE(var.Empty());
}

TEST(VarConvert, OptionalFromVar) {
  Var var = std::string("hello");
  std::optional<std::string> value;
  ASSERT_TRUE(FromVar(var, &value));
  EXPECT_TRUE(value);
  EXPECT_THAT(*value, Eq("hello"));
}

TEST(VarConvert, NullOptionalFromVar) {
  Var var;
  std::optional<std::string> value;
  ASSERT_TRUE(FromVar(var, &value));
  EXPECT_FALSE(value);
}

TEST(VarConvert, NullOptionalFromWrongVar) {
  Var var = 123;
  std::optional<std::string> value;
  ASSERT_FALSE(FromVar(var, &value));
}

TEST(VarConvert, VectorToVar) {
  Var var;
  std::vector<std::string> value = {"hello", "world"};
  EXPECT_TRUE(ToVar(value, &var));
  EXPECT_TRUE(var.Is<VarArray>());
  EXPECT_THAT(var.Count(), Eq(2));
  EXPECT_THAT(var[0].ValueOr(std::string()), Eq("hello"));
  EXPECT_THAT(var[1].ValueOr(std::string()), Eq("world"));
}

TEST(VarConvert, VectorFromVar) {
  VarArray array;
  array.PushBack(std::string("hello"));
  array.PushBack(std::string("world"));
  Var var = std::move(array);

  std::vector<std::string> value;
  ASSERT_TRUE(FromVar(var, &value));
  EXPECT_THAT(value.size(), Eq(2));
  EXPECT_THAT(value[0], Eq("hello"));
  EXPECT_THAT(value[1], Eq("world"));
}

TEST(VarConvert, MapToVar) {
  Var var;
  absl::flat_hash_map<HashValue, std::string> value;
  value.emplace(HashValue(123), "hello");
  value.emplace(HashValue(456), "world");
  EXPECT_TRUE(ToVar(value, &var));
  EXPECT_TRUE(var.Is<VarTable>());
  EXPECT_THAT(var.Count(), Eq(2));
  EXPECT_THAT(var[HashValue(123)].ValueOr(std::string()), Eq("hello"));
  EXPECT_THAT(var[HashValue(456)].ValueOr(std::string()), Eq("world"));
}

TEST(VarConvert, MapFromVar) {
  VarTable table;
  table.Insert(HashValue(123), std::string("hello"));
  table.Insert(HashValue(456), std::string("world"));
  Var var = std::move(table);

  absl::flat_hash_map<HashValue, std::string> value;
  ASSERT_TRUE(FromVar(var, &value));
  EXPECT_THAT(value.size(), Eq(2));
  EXPECT_THAT(value[HashValue(123)], Eq("hello"));
  EXPECT_THAT(value[HashValue(456)], Eq("world"));
}

}  // namespace
}  // namespace redux
