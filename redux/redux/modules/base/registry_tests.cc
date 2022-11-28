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
#include "redux/modules/base/registry.h"

namespace redux {

struct BaseClass {
  virtual ~BaseClass() {}
};

struct ClassOne : BaseClass {
  ClassOne() : value(1) {}
  int value;
};

struct ClassTwo {
  ClassTwo() : value(2) {}
  int value;
};

namespace {

using ::testing::Eq;
using ::testing::Not;

TEST(RegistryTest, Empty) {
  Registry r;
  EXPECT_THAT(r.Get<ClassOne>(), Eq(nullptr));
  EXPECT_THAT(r.Get<ClassTwo>(), Eq(nullptr));
}

TEST(RegistryTest, Add) {
  Registry r;
  r.Create<ClassOne>();
  EXPECT_THAT(r.Get<ClassOne>(), Not(Eq(nullptr)));
  EXPECT_THAT(r.Get<ClassTwo>(), Eq(nullptr));
}

TEST(RegistryTest, AddMultiple) {
  Registry r;
  r.Create<ClassOne>();
  r.Create<ClassTwo>();
  EXPECT_THAT(r.Get<ClassOne>(), Not(Eq(nullptr)));
  EXPECT_THAT(r.Get<ClassTwo>(), Not(Eq(nullptr)));
}

TEST(RegistryTest, ConstGet) {
  Registry r;
  const auto* c1 = r.Create<ClassOne>();

  const Registry* const_r = &r;
  EXPECT_THAT(const_r->Get<ClassOne>(), Eq(c1));
}

TEST(RegistryTest, Register) {
  Registry r;
  auto* c1 = new ClassOne();
  r.Register(std::unique_ptr<ClassOne>(c1));

  EXPECT_THAT(r.Get<ClassOne>(), Eq(c1));
}

TEST(RegistryTest, RegisterBase) {
  Registry r;
  auto* c1 = new ClassOne();
  r.Register(std::unique_ptr<BaseClass>(c1));

  EXPECT_THAT(r.Get<BaseClass>(), Eq(c1));
}

TEST(RegistryTest, RegisterUniqueBase) {
  Registry r;
  auto* c1 = new ClassOne();
  auto unique = std::unique_ptr<ClassOne>(c1);
  r.Register<BaseClass>(std::move(unique));

  EXPECT_THAT(r.Get<BaseClass>(), Eq(c1));
}

TEST(RegistryTest, RegisterCustomDeleter) {
  ClassOne* c1 = new ClassOne();
  ClassOne* c2 = nullptr;

  {
    Registry r;
    using ClassOnePtr =
        std::unique_ptr<ClassOne, std::function<void(ClassOne*)>>;
    r.Register(ClassOnePtr(c1, [&c2](ClassOne* ptr) { c2 = ptr; }));

    EXPECT_EQ(r.Get<ClassOne>(), c1);
  }

  EXPECT_THAT(c1, Eq(c2));

  // We replaced the default deleter, so now we need to call it.
  delete c1;
}

TEST(RegistryTest, MultiAdd) {
  Registry r;
  r.Create<ClassOne>();
  EXPECT_DEATH(r.Create<ClassOne>(), "");
}

TEST(RegistryTest, Dependency) {
  Registry r;
  r.Create<ClassOne>();
  r.Create<ClassTwo>();
  r.RegisterDependency<ClassOne, ClassTwo>();
  r.Initialize();
}

TEST(RegistryTest, CreateAfterDependency) {
  Registry r;
  r.RegisterDependency<ClassOne, ClassTwo>();
  r.Create<ClassOne>();
  r.Create<ClassTwo>();
  r.Initialize();
}

TEST(RegistryTest, MissingDependency) {
  Registry r;
  r.RegisterDependency<ClassOne, ClassTwo>();
  EXPECT_DEATH(r.Initialize(), "");
}

}  // namespace
}  // namespace redux

REDUX_SETUP_TYPEID(redux::BaseClass);
REDUX_SETUP_TYPEID(redux::ClassOne);
REDUX_SETUP_TYPEID(redux::ClassTwo);
