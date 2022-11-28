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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/var/var.h"
#include "redux/modules/var/var_array.h"
#include "redux/modules/var/var_table.h"

namespace redux {
namespace {

struct ObjectWithDynamicAllocation {
  explicit ObjectWithDynamicAllocation(int value) : ptr(new int(value)) {}

  ObjectWithDynamicAllocation(const ObjectWithDynamicAllocation& rhs)
      : ptr(nullptr) {
    if (rhs.ptr) {
      ptr = new int(*rhs.ptr);
    }
  }

  ObjectWithDynamicAllocation(ObjectWithDynamicAllocation&& rhs)
      : ptr(nullptr) {
    std::swap(ptr, rhs.ptr);
  }

  ObjectWithDynamicAllocation& operator=(
      const ObjectWithDynamicAllocation& rhs) {
    delete ptr;
    if (rhs.ptr) {
      ptr = new int(*rhs.ptr);
    }
    return *this;
  }

  ObjectWithDynamicAllocation& operator=(ObjectWithDynamicAllocation&& rhs) {
    std::swap(ptr, rhs.ptr);
    delete rhs.ptr;
    rhs.ptr = nullptr;
    return *this;
  }

  ~ObjectWithDynamicAllocation() { delete ptr; }

  int* ptr;
};

}  // namespace
}  // namespace redux

REDUX_SETUP_TYPEID(redux::ObjectWithDynamicAllocation);

namespace redux {
namespace {

using ::testing::Eq;

TEST(Var, Empty) {
  Var v;
  EXPECT_TRUE(v.Empty());

  v = 1;
  EXPECT_FALSE(v.Empty());
}

TEST(Var, Clear) {
  Var v = 1;
  EXPECT_FALSE(v.Empty());

  v.Clear();
  EXPECT_TRUE(v.Empty());
}

TEST(Var, Is) {
  Var v1 = 1;
  EXPECT_TRUE(v1.Is<int>());
  EXPECT_FALSE(v1.Is<float>());

  Var v2 = 2.f;
  EXPECT_FALSE(v2.Is<int>());
  EXPECT_TRUE(v2.Is<float>());
}

TEST(Var, GetTypeid) {
  Var v1 = 1;
  EXPECT_THAT(v1.GetTypeId(), Eq(GetTypeId<int>()));

  Var v2 = 2.f;
  EXPECT_THAT(v2.GetTypeId(), Eq(GetTypeId<float>()));
}

TEST(Var, Assign) {
  Var v = 1;
  EXPECT_TRUE(v.Is<int>());

  v = 2.f;
  EXPECT_TRUE(v.Is<float>());
}

TEST(Var, SelfAssign) {
  Var v = ObjectWithDynamicAllocation(123);
  EXPECT_TRUE(v.Is<ObjectWithDynamicAllocation>());
  EXPECT_THAT(*v.Get<ObjectWithDynamicAllocation>()->ptr, Eq(123));

  v = *v.Get<ObjectWithDynamicAllocation>();
  EXPECT_TRUE(v.Is<ObjectWithDynamicAllocation>());
  EXPECT_THAT(*v.Get<ObjectWithDynamicAllocation>()->ptr, Eq(123));
}

TEST(Var, ValueOr) {
  Var v = 1;
  EXPECT_THAT(v.ValueOr(0), Eq(1));
  EXPECT_THAT(v.ValueOr(0.0f), Eq(0.0f));

  v = 2.f;
  EXPECT_THAT(v.ValueOr(0), Eq(0));
  EXPECT_THAT(v.ValueOr(0.0f), Eq(2.0f));
}

TEST(Var, VarArray) {
  VarArray arr;
  arr.PushBack(1);
  arr.PushBack(2.0f);

  Var v = arr;
  EXPECT_TRUE(v[0].Is<int>());
  EXPECT_TRUE(v[1].Is<float>());
  EXPECT_DEATH(v[2].Empty(), "");
}

TEST(Var, VarTable) {
  constexpr HashValue key1 = ConstHash("one");
  constexpr HashValue key2 = ConstHash("two");
  constexpr HashValue key3 = ConstHash("three");

  VarTable tbl;
  tbl.Insert(key1, 1);
  tbl.Insert(key2, 2.0f);

  Var v = tbl;
  EXPECT_TRUE(v[key1].Is<int>());
  EXPECT_TRUE(v[key2].Is<float>());
  EXPECT_TRUE(v[key3].Empty());
}

}  // namespace
}  // namespace redux
