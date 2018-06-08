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

#include "lullaby/util/registry.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

struct ClassOne {
  ClassOne() : value(1) {}
  int value;
};

struct ClassTwo {
  ClassTwo() : value(2) {}
  int value;
};

TEST(Registry, Empty) {
  Registry r;
  EXPECT_EQ(nullptr, r.Get<ClassOne>());
  EXPECT_EQ(nullptr, r.Get<ClassTwo>());
}

TEST(Registry, Add) {
  Registry r;
  r.Create<ClassOne>();
  EXPECT_NE(nullptr, r.Get<ClassOne>());
  EXPECT_EQ(nullptr, r.Get<ClassTwo>());
}

TEST(Registry, AddMultiple) {
  Registry r;
  r.Create<ClassOne>();
  r.Create<ClassTwo>();
  EXPECT_NE(nullptr, r.Get<ClassOne>());
  EXPECT_NE(nullptr, r.Get<ClassTwo>());
}

TEST(Registry, MultiAdd) {
  Registry r;
  auto c1 = r.Create<ClassOne>();
  EXPECT_EQ(c1, r.Get<ClassOne>());

  auto c2 = r.Create<ClassOne>();
  EXPECT_EQ(c2, nullptr);

  auto c3 = r.Get<ClassOne>();
  EXPECT_EQ(c3, c1);
}

TEST(Registry, ConstGet) {
  Registry r;
  const auto* c1 = r.Create<ClassOne>();

  const Registry* const_r = &r;
  EXPECT_EQ(c1, const_r->Get<ClassOne>());
}

}  // namespace
}  // namespace lull

LULLABY_SETUP_TYPEID(ClassOne);
LULLABY_SETUP_TYPEID(ClassTwo);
