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

#include "lullaby/systems/name/name_system.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/generated/name_def_generated.h"
#include "lullaby/modules/ecs/blueprint.h"
#include "lullaby/systems/transform/transform_system.h"
#include "tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::Eq;

class NameSystemTest : public ::testing::Test {
 protected:
  Registry registry_;
};

using NameSystemDeathTest = NameSystemTest;

TEST_F(NameSystemDeathTest, InvalidCreate) {
  NameDefT name;
  name.name = "left_button";
  const Blueprint blueprint(&name);

  NameSystem* name_system = registry_.Create<NameSystem>(&registry_);
  PORT_EXPECT_DEBUG_DEATH(name_system->Create(kNullEntity, 0, nullptr), "");
  PORT_EXPECT_DEBUG_DEATH(
      name_system->CreateComponent(kNullEntity, blueprint), "");
  PORT_EXPECT_DEBUG_DEATH(name_system->SetName(kNullEntity, "left_button"), "");
}

TEST_F(NameSystemTest, CreateName) {
  NameDefT name;
  name.name = "left_button";
  const Blueprint blueprint(&name);

  const Entity kTestEntity = 1;
  NameSystem* name_system = registry_.Create<NameSystem>(&registry_);
  name_system->CreateComponent(kTestEntity, blueprint);
  EXPECT_THAT(name_system->FindEntity("left_button"), Eq(kTestEntity));
}

TEST_F(NameSystemTest, SetAndGetByName) {
  const Entity kTestEntity = 1;
  NameSystem* name_system = registry_.Create<NameSystem>(&registry_);
  name_system->SetName(kTestEntity, "left_button");

  EXPECT_THAT(name_system->FindEntity("left_button"), Eq(kTestEntity));
  EXPECT_THAT(name_system->GetName(kTestEntity), Eq("left_button"));
}

TEST_F(NameSystemTest, SetDuplicateNames) {
  const bool kAllowDuplicateNames = true;
  const Entity kTestEntity1 = 1;
  const Entity kTestEntity2 = 2;
  NameSystem* name_system =
      registry_.Create<NameSystem>(&registry_, kAllowDuplicateNames);
  name_system->SetName(kTestEntity1, "left_button");
  name_system->SetName(kTestEntity2, "left_button");

  EXPECT_THAT(name_system->GetName(kTestEntity1), Eq("left_button"));
  EXPECT_THAT(name_system->GetName(kTestEntity2), Eq("left_button"));
}

TEST_F(NameSystemTest, OverwriteName) {
  const Entity kTestEntity = 1;

  NameSystem* name_system = registry_.Create<NameSystem>(&registry_);
  name_system->SetName(kTestEntity, "left_button");
  name_system->SetName(kTestEntity, "right_button");

  EXPECT_THAT(name_system->FindEntity("left_button"), Eq(kNullEntity));
  EXPECT_THAT(name_system->FindEntity("right_button"), Eq(kTestEntity));
}

TEST_F(NameSystemTest, OverwriteSameName) {
  const Entity kTestEntity = 1;

  NameSystem* name_system = registry_.Create<NameSystem>(&registry_);
  name_system->SetName(kTestEntity, "left_button");
  EXPECT_THAT(name_system->FindEntity("left_button"), Eq(kTestEntity));

  name_system->SetName(kTestEntity, "left_button");
  EXPECT_THAT(name_system->FindEntity("left_button"), Eq(kTestEntity));
}

TEST_F(NameSystemDeathTest, ReassignName) {
  const Entity kTestEntity1 = 1;
  const Entity kTestEntity2 = 2;

  NameSystem* name_system = registry_.Create<NameSystem>(&registry_);
  name_system->SetName(kTestEntity1, "left_button");
  PORT_EXPECT_DEBUG_DEATH(name_system->SetName(kTestEntity2, "left_button"),
                          "");

  EXPECT_THAT(name_system->FindEntity("left_button"), Eq(kTestEntity1));
  EXPECT_THAT(name_system->GetName(kTestEntity1), Eq("left_button"));
  EXPECT_THAT(name_system->GetName(kTestEntity2), Eq(""));
}

TEST_F(NameSystemTest, FindDescendant) {
  const Entity kRootEntity = 1;
  const Entity kParentEntity1 = 2;
  const Entity kParentEntity2 = 3;
  const Entity kChildEntity1 = 4;
  Sqt sqt;
  auto* transform_system = registry_.Create<TransformSystem>(&registry_);
  transform_system->Create(kRootEntity, sqt);
  transform_system->Create(kParentEntity1, sqt);
  transform_system->Create(kParentEntity2, sqt);
  transform_system->Create(kChildEntity1, sqt);
  transform_system->AddChild(kRootEntity, kParentEntity1);
  transform_system->AddChild(kRootEntity, kParentEntity2);
  transform_system->AddChild(kParentEntity1, kChildEntity1);
  NameSystem* name_system = registry_.Create<NameSystem>(&registry_);
  name_system->SetName(kChildEntity1, "child1");
  name_system->SetName(kParentEntity1, "parent1");

  EXPECT_THAT(name_system->FindDescendant(kRootEntity, "parent1"),
              Eq(kParentEntity1));
  EXPECT_THAT(name_system->FindDescendant(kRootEntity, "child1"),
              Eq(kChildEntity1));
  EXPECT_THAT(name_system->FindDescendant(kParentEntity1, "child1"),
              Eq(kChildEntity1));
  EXPECT_THAT(name_system->FindDescendant(kParentEntity2, "child1"),
              Eq(kNullEntity));
}

TEST_F(NameSystemTest, FindDescendantWithDuplicateNames) {
  const bool kAllowDuplicateNames = true;
  const Entity kRootEntity = 1;
  const Entity kParentEntity1 = 2;
  const Entity kParentEntity2 = 3;
  const Entity kChildEntity1 = 4;
  const Entity kChildEntity2 = 5;
  const Entity kChildEntity3 = 6;
  Sqt sqt;
  auto* transform_system = registry_.Create<TransformSystem>(&registry_);
  transform_system->Create(kRootEntity, sqt);
  transform_system->Create(kParentEntity1, sqt);
  transform_system->Create(kParentEntity2, sqt);
  transform_system->Create(kChildEntity1, sqt);
  transform_system->Create(kChildEntity2, sqt);
  transform_system->Create(kChildEntity3, sqt);
  transform_system->AddChild(kRootEntity, kParentEntity1);
  transform_system->AddChild(kRootEntity, kParentEntity2);
  transform_system->AddChild(kRootEntity, kChildEntity3);
  transform_system->AddChild(kParentEntity1, kChildEntity1);
  transform_system->AddChild(kParentEntity2, kChildEntity2);
  auto* name_system =
      registry_.Create<NameSystem>(&registry_, kAllowDuplicateNames);
  name_system->SetName(kChildEntity1, "left_button");
  name_system->SetName(kChildEntity2, "left_button");
  name_system->SetName(kChildEntity3, "child3");
  name_system->SetName(kParentEntity1, "parent1");

  EXPECT_THAT(name_system->FindDescendant(kRootEntity, "parent1"),
              Eq(kParentEntity1));
  EXPECT_THAT(name_system->FindDescendant(kRootEntity, "child3"),
              Eq(kChildEntity3));
  EXPECT_THAT(name_system->FindDescendant(kParentEntity1, "child3"),
              Eq(kNullEntity));
  EXPECT_THAT(name_system->FindDescendant(kParentEntity1, "left_button"),
              Eq(kChildEntity1));
  EXPECT_THAT(name_system->FindDescendant(kParentEntity2, "left_button"),
              Eq(kChildEntity2));
}

}  // namespace
}  // namespace lull
