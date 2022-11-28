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
#include "redux/modules/math/testing.h"
#include "redux/systems/constraint/constraint_system.h"

namespace redux {
namespace {

using ::testing::Eq;

class ConstraintSystemTest : public testing::Test {
 protected:
  void SetUp() override {
    ScriptEngine::Create(&registry_);
    entity_factory_ = registry_.Create<EntityFactory>(&registry_);
    rig_system_ = entity_factory_->CreateSystem<RigSystem>();
    transform_system_ = entity_factory_->CreateSystem<TransformSystem>();
    constraint_system_ = entity_factory_->CreateSystem<ConstraintSystem>();
    registry_.Initialize();
  }

  static constexpr float kEpsilon = 1e-5f;

  Registry registry_;
  RigSystem* rig_system_;
  TransformSystem* transform_system_;
  ConstraintSystem* constraint_system_;
  EntityFactory* entity_factory_;
};

TEST_F(ConstraintSystemTest, AttachChild) {
  const Entity child(1);
  const Entity parent(2);

  Transform transform;
  transform.translation = vec3(1.0f, 2.0f, 3.0f);
  transform.rotation = QuaternionFromEulerAngles(vec3(kHalfPi, 0.0f, 0.0f));

  transform_system_->SetTransform(child, transform);
  transform_system_->SetTransform(parent, transform);
  constraint_system_->AttachChild(parent, child, {.local_offset = transform});

  constraint_system_->UpdateTransforms();

  transform = transform_system_->GetTransform(child);
  EXPECT_THAT(transform.translation,
              MathNear(vec3(2.0f, -1.0f, 5.0f), kEpsilon));
  EXPECT_THAT(ToEulerAngles(transform.rotation),
              MathNear(vec3(kPi, 0.0f, 0.0f), kEpsilon));

  EXPECT_THAT(constraint_system_->GetParent(child), Eq(parent));
  EXPECT_THAT(constraint_system_->GetRoot(child), Eq(parent));
  EXPECT_TRUE(constraint_system_->IsAncestorOf(parent, child));
}

TEST_F(ConstraintSystemTest, AttachChildChain) {
  const Entity child(1);
  const Entity parent_1(2);
  const Entity parent_2(3);

  Transform transform;
  transform.translation = vec3(1.0f, 2.0f, 3.0f);
  transform.rotation = QuaternionFromEulerAngles(vec3(kHalfPi, 0.0f, 0.0f));

  transform_system_->SetTransform(parent_1, transform);
  constraint_system_->AttachChild(parent_1, parent_2,
                                  {.local_offset = transform});
  constraint_system_->AttachChild(parent_2, child, {.local_offset = transform});

  constraint_system_->UpdateTransforms();

  transform = transform_system_->GetTransform(child);
  EXPECT_THAT(transform.translation,
              MathNear(vec3(3.0f, -3.0f, 2.0f), kEpsilon));
  EXPECT_THAT(ToEulerAngles(transform.rotation),
              MathNear(vec3(-kHalfPi, 0.0f, 0.0f), kEpsilon));

  EXPECT_THAT(constraint_system_->GetParent(child), Eq(parent_2));
  EXPECT_THAT(constraint_system_->GetParent(parent_2), Eq(parent_1));
  EXPECT_THAT(constraint_system_->GetRoot(child), Eq(parent_1));
  EXPECT_TRUE(constraint_system_->IsAncestorOf(parent_1, child));
  EXPECT_TRUE(constraint_system_->IsAncestorOf(parent_2, child));
}

TEST_F(ConstraintSystemTest, RemoveParent) {
  const Entity child(1);
  const Entity parent(2);

  Transform transform;
  transform.translation = vec3(1.0f, 2.0f, 3.0f);
  transform.rotation = QuaternionFromEulerAngles(vec3(kHalfPi, 0.0f, 0.0f));

  transform_system_->SetTransform(child, transform);
  transform_system_->SetTransform(parent, transform);
  constraint_system_->AttachChild(parent, child, {.local_offset = transform});

  constraint_system_->UpdateTransforms();

  transform = transform_system_->GetTransform(child);
  EXPECT_THAT(constraint_system_->GetParent(child), Eq(parent));
  EXPECT_THAT(transform.translation,
              MathNear(vec3(2.0f, -1.0f, 5.0f), kEpsilon));
  EXPECT_THAT(ToEulerAngles(transform.rotation),
              MathNear(vec3(kPi, 0.0f, 0.0f), kEpsilon));

  constraint_system_->DetachFromParent(child);

  transform = transform_system_->GetTransform(child);
  EXPECT_THAT(constraint_system_->GetParent(child), Eq(kNullEntity));
  EXPECT_THAT(transform.translation,
              MathNear(vec3(2.0f, -1.0f, 5.0f), kEpsilon));
  EXPECT_THAT(ToEulerAngles(transform.rotation),
              MathNear(vec3(kPi, 0.0f, 0.0f), kEpsilon));
}

TEST_F(ConstraintSystemTest, DestroyAlsoDestroysChildren) {
  const Entity parent = entity_factory_->Create();
  const Entity child = entity_factory_->Create();

  constraint_system_->AttachChild(parent, child, {});
  EXPECT_TRUE(entity_factory_->IsAlive(parent));
  EXPECT_TRUE(entity_factory_->IsAlive(child));

  entity_factory_->DestroyNow(parent);
  EXPECT_FALSE(entity_factory_->IsAlive(parent));
  EXPECT_FALSE(entity_factory_->IsAlive(child));
}

TEST_F(ConstraintSystemTest, EnableDisable) {
  const Entity parent = entity_factory_->Create();
  const Entity child = entity_factory_->Create();

  constraint_system_->AttachChild(parent, child, {});
  EXPECT_TRUE(entity_factory_->IsEnabled(parent));
  EXPECT_TRUE(entity_factory_->IsEnabled(child));

  entity_factory_->Disable(parent);
  EXPECT_FALSE(entity_factory_->IsEnabled(parent));
  EXPECT_FALSE(entity_factory_->IsEnabled(child));

  entity_factory_->Enable(parent);
  EXPECT_TRUE(entity_factory_->IsEnabled(parent));
  EXPECT_TRUE(entity_factory_->IsEnabled(child));
}

TEST_F(ConstraintSystemTest, AttachChildToDisabledParent) {
  const Entity parent = entity_factory_->Create();
  const Entity child = entity_factory_->Create();

  entity_factory_->Disable(parent);
  constraint_system_->AttachChild(parent, child, {});
  EXPECT_FALSE(entity_factory_->IsEnabled(parent));
  EXPECT_FALSE(entity_factory_->IsEnabled(child));
}

TEST_F(ConstraintSystemTest, DetachChildFromDisabledParent) {
  const Entity parent = entity_factory_->Create();
  const Entity child = entity_factory_->Create();

  constraint_system_->AttachChild(parent, child, {});
  entity_factory_->Disable(parent);
  EXPECT_FALSE(entity_factory_->IsEnabled(parent));
  EXPECT_FALSE(entity_factory_->IsEnabled(child));

  constraint_system_->DetachFromParent(child);
  EXPECT_FALSE(entity_factory_->IsEnabled(parent));
  EXPECT_FALSE(entity_factory_->IsEnabled(child));

  entity_factory_->Enable(parent);
  EXPECT_TRUE(entity_factory_->IsEnabled(parent));
  EXPECT_FALSE(entity_factory_->IsEnabled(child));
}

TEST_F(ConstraintSystemTest, EnableDisableDeepHierarchy) {
  Entity entities[10];
  for (int i = 0; i < 10; ++i) {
    entities[i] = entity_factory_->Create();
  }

  entity_factory_->Disable(entities[6]);

  // Create a hierarchy that looks like this:
  //       0
  //    /  |  \
  //    1  2  3
  //  / |     |
  //  4 5     6
  //        / | \
  //       7  8  9

  constraint_system_->AttachChild(entities[0], entities[1], {});
  constraint_system_->AttachChild(entities[0], entities[2], {});
  constraint_system_->AttachChild(entities[0], entities[3], {});
  constraint_system_->AttachChild(entities[1], entities[4], {});
  constraint_system_->AttachChild(entities[1], entities[5], {});
  constraint_system_->AttachChild(entities[3], entities[6], {});
  constraint_system_->AttachChild(entities[6], entities[7], {});
  constraint_system_->AttachChild(entities[6], entities[8], {});
  constraint_system_->AttachChild(entities[6], entities[9], {});

  entity_factory_->Disable(entities[0]);
  for (int i = 0; i < 10; ++i) {
    EXPECT_FALSE(entity_factory_->IsEnabled(entities[i]));
  }

  entity_factory_->Enable(entities[0]);
  for (int i = 0; i < 10; ++i) {
    const bool expect_enabled = i < 6;
    EXPECT_THAT(entity_factory_->IsEnabled(entities[i]), Eq(expect_enabled));
  }

  constraint_system_->DetachFromParent(entities[3]);
  for (int i = 0; i < 10; ++i) {
    const bool expect_enabled = i < 6;
    EXPECT_THAT(entity_factory_->IsEnabled(entities[i]), Eq(expect_enabled));
  }

  constraint_system_->AttachChild(entities[1], entities[6], {});
  for (int i = 0; i < 10; ++i) {
    const bool expect_enabled = i < 6;
    EXPECT_THAT(entity_factory_->IsEnabled(entities[i]), Eq(expect_enabled));
  }

  entity_factory_->Disable(entities[1]);
  for (int i = 0; i < 10; ++i) {
    const bool expect_enabled = i != 1 && i < 4;
    EXPECT_THAT(entity_factory_->IsEnabled(entities[i]), Eq(expect_enabled));
  }

  entity_factory_->Enable(entities[6]);
  for (int i = 0; i < 10; ++i) {
    const bool expect_enabled = i != 1 && i < 4;
    EXPECT_THAT(entity_factory_->IsEnabled(entities[i]), Eq(expect_enabled));
  }

  entity_factory_->Enable(entities[1]);
  for (int i = 0; i < 10; ++i) {
    const bool expect_enabled = true;
    EXPECT_THAT(entity_factory_->IsEnabled(entities[i]), Eq(expect_enabled));
  }
}

}  // namespace
}  // namespace redux
