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

#include <unordered_set>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/tests/portable_test_macros.h"

namespace {

// Acts as mock dependencies for TestDependent.
class TestDependency {};

// Has a mock dependency on TestDependency.
class TestDependent {};

}  // namespace

LULLABY_SETUP_TYPEID(TestDependency);
LULLABY_SETUP_TYPEID(TestDependent);

namespace lull {
namespace {

class DependencyCheckerTest : public ::testing::Test {
 protected:
  DependencyCheckerTest() {
    dependent_id_ = GetTypeId<TestDependent>();
    dependent_name_ = GetTypeName<TestDependent>();
    dependency_id_ = GetTypeId<TestDependency>();
    dependency_name_ = GetTypeName<TestDependency>();
  }

  ~DependencyCheckerTest() override {}

  TypeId dependent_id_;
  const char* dependent_name_;

  TypeId dependency_id_;
  const char* dependency_name_;
};

using DependencyCheckerDeathTest = DependencyCheckerTest;

TEST_F(DependencyCheckerDeathTest, MissingDependency) {
  DependencyChecker dependency_checker;
  dependency_checker.RegisterDependency(dependent_id_, dependent_name_,
                                        dependency_id_, dependency_name_);

  PORT_EXPECT_DEBUG_DEATH(dependency_checker.CheckAllDependencies(),
                          "Must have all dependencies!");
}

TEST_F(DependencyCheckerTest, SatisfiedDependency) {
  DependencyChecker dependency_checker;
  dependency_checker.RegisterDependency(dependent_id_, dependent_name_,
                                        dependency_id_, dependency_name_);

  dependency_checker.SatisfyDependency(dependency_id_);
  // Expecting no death.
  dependency_checker.CheckAllDependencies();
}

TEST_F(DependencyCheckerTest, SatisfyDependencyBeforeRegister) {
  // Checks that the order of registering/satisfying dependencies doesn't
  // matter.
  DependencyChecker dependency_checker;
  dependency_checker.SatisfyDependency(dependency_id_);
  dependency_checker.RegisterDependency(dependent_id_, dependent_name_,
                                        dependency_id_, dependency_name_);
  // Expecting no death.
  dependency_checker.CheckAllDependencies();
}

TEST_F(DependencyCheckerTest, NoDependencies) {
  // Make sure CheckAllDependencies is fine without any dependencies.
  DependencyChecker dependency_checker;
  // Expecting no death.
  dependency_checker.CheckAllDependencies();
}

}  // namespace
}  // namespace lull
