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

#include "lullaby/systems/transform/transform_system.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/base/dispatcher.h"
#include "lullaby/base/entity_factory.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/generated/tests/mathfu_matchers.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;
using testing::EqualsMathfuVec3;
using testing::EqualsMathfuQuat;
using testing::NearMathfuQuat;
static const float kEpsilon = 0.001f;

class TransformSystemTest : public ::testing::Test {
 public:
  void SetUp() override {
    registry_.Create<Dispatcher>();
    auto entity_factory = registry_.Create<EntityFactory>(&registry_);
    entity_factory->CreateSystem<TransformSystem>();
  }

  void SetupEventHandlers() {
    Dispatcher* dispatcher = registry_.Get<Dispatcher>();
    dispatcher->Connect(this, [this](const ParentChangedEvent& event) {
      OnParentChanged(event);
    });
    dispatcher->Connect(
        this, [this](const ChildAddedEvent& event) { OnChildAdded(event); });
    dispatcher->Connect(this, [this](const ChildRemovedEvent& event) {
      OnChildRemoved(event);
    });
    DisallowParentChangedEvent();
  }

  void CreateDefaultTransform(Entity entity) {
    TransformDefT transform;
    transform.position = mathfu::vec3(0.f, 0.f, 0.f);
    transform.rotation = mathfu::vec3(0.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(1.f, 1.f, 1.f);

    Blueprint blueprint(&transform);

    auto* transform_system = registry_.Get<TransformSystem>();
    transform_system->CreateComponent(entity, blueprint);
  }

  void TearDown() override {
    auto dispatcher = registry_.Get<Dispatcher>();
    dispatcher->DisconnectAll(this);
  }

 protected:
  void ExpectParentChangedEvent(Entity target, Entity old_parent,
                                Entity new_parent);
  void DisallowParentChangedEvent();
  void OnParentChanged(const ParentChangedEvent& e);
  void OnChildAdded(const ChildAddedEvent& e);
  void OnChildRemoved(const ChildRemovedEvent& e);

  Registry registry_;
  Entity test_event_child_;
  Entity test_event_old_parent_;
  Entity test_event_new_parent_;
};

using TransformSystemDeathTest = TransformSystemTest;

TEST_F(TransformSystemTest, CreatePositionRotationScale) {
  auto* transform_system = registry_.Get<TransformSystem>();

  TransformDefT transform;
  transform.position = mathfu::vec3(1.f, 2.f, 3.f);
  transform.rotation = mathfu::vec3(45.f, 0.f, 0.f);
  transform.scale = mathfu::vec3(1.f, 1.f, 1.f);
  Blueprint blueprint(&transform);

  const Entity entity = 1;
  transform_system->CreateComponent(entity, blueprint);

  const Sqt* sqt = transform_system->GetSqt(entity);
  EXPECT_THAT(sqt, NotNull());
  EXPECT_THAT(sqt->translation, EqualsMathfuVec3({1.f, 2.f, 3.f}));
  EXPECT_THAT(sqt->rotation,
              NearMathfuQuat({0.9238f, 0.3826f, 0.f, 0.f}, kEpsilon));
  EXPECT_THAT(sqt->scale, EqualsMathfuVec3({1.f, 1.f, 1.f}));
}

TEST_F(TransformSystemTest, CreatePositionQuaternionScale) {
  auto* transform_system = registry_.Get<TransformSystem>();

  TransformDefT transform;
  transform.position = mathfu::vec3(1.f, 2.f, 3.f);
  transform.quaternion.emplace(1.f, 0.f, 0.f, 0.f);
  transform.scale = mathfu::vec3(4.f, 5.f, 7.f);
  Blueprint blueprint(&transform);

  const Entity entity = 1;
  transform_system->CreateComponent(entity, blueprint);

  const Sqt* sqt = transform_system->GetSqt(entity);
  EXPECT_THAT(sqt, NotNull());
  EXPECT_THAT(sqt->translation, EqualsMathfuVec3({1.f, 2.f, 3.f}));
  EXPECT_THAT(sqt->rotation, EqualsMathfuQuat({0.f, 1.f, 0.f, 0.f}));
  EXPECT_THAT(sqt->scale, EqualsMathfuVec3({4.f, 5.f, 7.f}));
}

TEST_F(TransformSystemTest, CreateAabb) {
  auto* transform_system = registry_.Get<TransformSystem>();

  TransformDefT transform;
  transform.position = mathfu::vec3(0.f, 0.f, 0.f);
  transform.rotation = mathfu::vec3(0.f, 0.f, 0.f);
  transform.scale = mathfu::vec3(1.f, 1.f, 1.f);
  transform.aabb.min = mathfu::vec3(-1.f, -2.f, -3.f);
  transform.aabb.max = mathfu::vec3(1.f, 2.f, 3.f);
  Blueprint blueprint(&transform);

  const Entity entity = 1;
  transform_system->CreateComponent(entity, blueprint);

  const Aabb* aabb = transform_system->GetAabb(entity);
  EXPECT_THAT(aabb, NotNull());
  EXPECT_THAT(aabb->min, EqualsMathfuVec3({-1.f, -2.f, -3.f}));
  EXPECT_THAT(aabb->max, EqualsMathfuVec3({1.f, 2.f, 3.f}));
}

TEST_F(TransformSystemTest, CreateAabbPadding) {
  auto* transform_system = registry_.Get<TransformSystem>();

  TransformDefT transform;
  transform.position = mathfu::vec3(0.f, 0.f, 0.f);
  transform.rotation = mathfu::vec3(0.f, 0.f, 0.f);
  transform.scale = mathfu::vec3(1.f, 1.f, 1.f);
  transform.aabb.min = mathfu::vec3(-1.f, -2.f, -3.f);
  transform.aabb.max = mathfu::vec3(1.f, 2.f, 3.f);
  transform.aabb_padding.min = mathfu::vec3(-1.f, -2.f, -3.f);
  transform.aabb_padding.max = mathfu::vec3(1.f, 2.f, 3.f);
  Blueprint blueprint(&transform);

  const Entity entity = 1;
  transform_system->CreateComponent(entity, blueprint);

  const Aabb* aabb = transform_system->GetAabb(entity);
  EXPECT_THAT(aabb, NotNull());
  EXPECT_THAT(aabb->min, EqualsMathfuVec3({-2.f, -4.f, -6.f}));
  EXPECT_THAT(aabb->max, EqualsMathfuVec3({2.f, 4.f, 6.f}));
}

TEST_F(TransformSystemTest, SetSqt) {
  const Entity entity = 1;
  CreateDefaultTransform(entity);

  auto* transform_system = registry_.Get<TransformSystem>();
  const Sqt* sqt = transform_system->GetSqt(entity);
  EXPECT_THAT(sqt, NotNull());
  EXPECT_THAT(sqt->translation, EqualsMathfuVec3(mathfu::kZeros3f));
  EXPECT_THAT(sqt->rotation, EqualsMathfuQuat(mathfu::quat::identity));
  EXPECT_THAT(sqt->scale, EqualsMathfuVec3(mathfu::kOnes3f));

  Sqt target;
  target.translation = mathfu::vec3(1.f, 2.f, 3.f);
  target.rotation = mathfu::quat(0.f, 0.f, 0.f, 1.f);
  target.scale = mathfu::vec3(2.f, 3.f, 4.f);
  transform_system->SetSqt(entity, target);

  sqt = transform_system->GetSqt(entity);
  EXPECT_THAT(sqt, NotNull());
  EXPECT_THAT(sqt->translation, EqualsMathfuVec3({1.f, 2.f, 3.f}));
  EXPECT_THAT(sqt->rotation, EqualsMathfuQuat({0.f, 0.f, 0.f, 1.f}));
  EXPECT_THAT(sqt->scale, EqualsMathfuVec3({2.f, 3.f, 4.f}));
}

TEST_F(TransformSystemTest, ApplySqt) {
  const Entity entity = 1;
  CreateDefaultTransform(entity);

  auto* transform_system = registry_.Get<TransformSystem>();
  const Sqt* sqt = transform_system->GetSqt(entity);
  EXPECT_THAT(sqt, NotNull());
  EXPECT_THAT(sqt->translation, EqualsMathfuVec3(mathfu::kZeros3f));
  EXPECT_THAT(sqt->rotation, EqualsMathfuQuat(mathfu::quat::identity));
  EXPECT_THAT(sqt->scale, EqualsMathfuVec3(mathfu::kOnes3f));

  Sqt target;
  target.translation = mathfu::vec3(1.f, 2.f, 3.f);
  target.rotation = mathfu::quat(0.f, 0.f, 0.f, 1.f);
  target.scale = mathfu::vec3(2.f, 3.f, 4.f);
  transform_system->SetSqt(entity, target);

  sqt = transform_system->GetSqt(entity);
  EXPECT_THAT(sqt, NotNull());
  EXPECT_THAT(sqt->translation, EqualsMathfuVec3({1.f, 2.f, 3.f}));
  EXPECT_THAT(sqt->rotation, EqualsMathfuQuat({0.f, 0.f, 0.f, 1.f}));
  EXPECT_THAT(sqt->scale, EqualsMathfuVec3({2.f, 3.f, 4.f}));

  transform_system->ApplySqt(entity, target);

  sqt = transform_system->GetSqt(entity);
  EXPECT_THAT(sqt, NotNull());
  EXPECT_THAT(sqt->translation, EqualsMathfuVec3({2.f, 4.f, 6.f}));
  EXPECT_THAT(sqt->rotation, EqualsMathfuQuat({-1.f, 0.f, 0.f, 0.f}));
  EXPECT_THAT(sqt->scale, EqualsMathfuVec3({4.f, 9.f, 16.f}));
}

TEST_F(TransformSystemTest, SetMatrix) {
  const Entity entity = 1;
  CreateDefaultTransform(entity);

  auto* transform_system = registry_.Get<TransformSystem>();
  const Sqt* sqt = transform_system->GetSqt(entity);
  EXPECT_THAT(sqt, NotNull());
  EXPECT_THAT(sqt->translation, EqualsMathfuVec3(mathfu::kZeros3f));
  EXPECT_THAT(sqt->rotation, EqualsMathfuQuat(mathfu::quat::identity));
  EXPECT_THAT(sqt->scale, EqualsMathfuVec3(mathfu::kOnes3f));

  Sqt target;
  target.translation = mathfu::vec3(1.f, 2.f, 3.f);
  target.rotation = mathfu::quat(0.f, 0.f, 0.f, 1.f);
  target.scale = mathfu::vec3(2.f, 3.f, 4.f);
  const mathfu::mat4 matrix = CalculateTransformMatrix(target);
  transform_system->SetWorldFromEntityMatrix(entity, matrix);

  sqt = transform_system->GetSqt(entity);
  EXPECT_THAT(sqt, NotNull());
  EXPECT_THAT(sqt->translation, EqualsMathfuVec3({1.f, 2.f, 3.f}));
  EXPECT_THAT(sqt->rotation, EqualsMathfuQuat({0.f, 0.f, 0.f, 1.f}));
  EXPECT_THAT(sqt->scale, EqualsMathfuVec3({2.f, 3.f, 4.f}));
}

TEST_F(TransformSystemTest, SetAabb) {
  const Entity entity = 1;
  CreateDefaultTransform(entity);

  auto* transform_system = registry_.Get<TransformSystem>();
  const Aabb* aabb = transform_system->GetAabb(entity);
  EXPECT_THAT(aabb, NotNull());
  EXPECT_THAT(aabb->min, EqualsMathfuVec3(mathfu::kZeros3f));
  EXPECT_THAT(aabb->max, EqualsMathfuVec3(mathfu::kZeros3f));

  const mathfu::vec3 min(-1.f, -2.f, -3.f);
  const mathfu::vec3 max(1.f, 2.f, 3.f);
  transform_system->SetAabb(entity, Aabb(min, max));

  aabb = transform_system->GetAabb(entity);
  EXPECT_THAT(aabb, NotNull());
  EXPECT_THAT(aabb->min, EqualsMathfuVec3({-1.f, -2.f, -3.f}));
  EXPECT_THAT(aabb->max, EqualsMathfuVec3({1.f, 2.f, 3.f}));
}

TEST_F(TransformSystemTest, SetAabbPadding) {
  const Entity entity = 1;
  CreateDefaultTransform(entity);

  auto* transform_system = registry_.Get<TransformSystem>();
  const Aabb* aabb = transform_system->GetAabb(entity);
  EXPECT_THAT(aabb, NotNull());
  EXPECT_THAT(aabb->min, EqualsMathfuVec3(mathfu::kZeros3f));
  EXPECT_THAT(aabb->max, EqualsMathfuVec3(mathfu::kZeros3f));

  const mathfu::vec3 min(-1.f, -2.f, -3.f);
  const mathfu::vec3 max(1.f, 2.f, 3.f);
  transform_system->SetAabbPadding(entity, Aabb(min, max));
  aabb = transform_system->GetAabb(entity);
  EXPECT_THAT(aabb, NotNull());
  EXPECT_THAT(aabb->min, EqualsMathfuVec3({-1.f, -2.f, -3.f}));
  EXPECT_THAT(aabb->max, EqualsMathfuVec3({1.f, 2.f, 3.f}));

  transform_system->SetAabb(entity, Aabb(min, max));
  aabb = transform_system->GetAabb(entity);
  EXPECT_THAT(aabb, NotNull());
  EXPECT_THAT(aabb->min, EqualsMathfuVec3({-2.f, -4.f, -6.f}));
  EXPECT_THAT(aabb->max, EqualsMathfuVec3({2.f, 4.f, 6.f}));
}

TEST_F(TransformSystemTest, SetInvalidEntity) {
  auto* transform_system = registry_.Get<TransformSystem>();

  const Entity entity = 1;
  const Sqt* sqt = transform_system->GetSqt(entity);
  const mathfu::mat4* mx = transform_system->GetWorldFromEntityMatrix(entity);
  const Aabb* aabb = transform_system->GetAabb(entity);
  const Aabb* padding = transform_system->GetAabbPadding(entity);
  EXPECT_THAT(sqt, IsNull());
  EXPECT_THAT(mx, IsNull());
  EXPECT_THAT(padding, IsNull());
  EXPECT_THAT(aabb, IsNull());

  transform_system->SetSqt(entity, Sqt());
  transform_system->SetWorldFromEntityMatrix(entity, mathfu::mat4::Identity());
  transform_system->SetAabb(entity, Aabb());
  transform_system->SetAabbPadding(entity, Aabb());

  sqt = transform_system->GetSqt(entity);
  mx = transform_system->GetWorldFromEntityMatrix(entity);
  aabb = transform_system->GetAabb(entity);
  padding = transform_system->GetAabbPadding(entity);
  EXPECT_THAT(sqt, IsNull());
  EXPECT_THAT(mx, IsNull());
  EXPECT_THAT(padding, IsNull());
  EXPECT_THAT(aabb, IsNull());
}

TEST_F(TransformSystemTest, EnableDisable) {
  CreateDefaultTransform(1);
  auto* transform_system = registry_.Get<TransformSystem>();
  EXPECT_THAT(transform_system->IsEnabled(1), Eq(true));
  EXPECT_THAT(transform_system->IsLocallyEnabled(1), Eq(true));

  transform_system->Disable(1);
  EXPECT_THAT(transform_system->IsEnabled(1), Eq(false));
  EXPECT_THAT(transform_system->IsLocallyEnabled(1), Eq(false));

  transform_system->Enable(1);
  EXPECT_THAT(transform_system->IsEnabled(1), Eq(true));
  EXPECT_THAT(transform_system->IsLocallyEnabled(1), Eq(true));

  CreateDefaultTransform(2);
  transform_system->AddChild(1, 2);

  EXPECT_THAT(transform_system->IsEnabled(2), Eq(true));
  EXPECT_THAT(transform_system->IsLocallyEnabled(2), Eq(true));

  transform_system->Disable(1);
  EXPECT_THAT(transform_system->IsEnabled(2), Eq(false));
  EXPECT_THAT(transform_system->IsLocallyEnabled(2), Eq(true));

  transform_system->Enable(1);
  EXPECT_THAT(transform_system->IsEnabled(2), Eq(true));
  EXPECT_THAT(transform_system->IsLocallyEnabled(2), Eq(true));
}

TEST_F(TransformSystemTest, Flags) {
  CreateDefaultTransform(1);

  auto* transform_system = registry_.Get<TransformSystem>();
  const TransformSystem::TransformFlags flag1 = transform_system->RequestFlag();
  const TransformSystem::TransformFlags flag2 = transform_system->RequestFlag();

  EXPECT_THAT(transform_system->HasFlag(1, flag1), Eq(false));
  EXPECT_THAT(transform_system->HasFlag(1, flag2), Eq(false));

  transform_system->SetFlag(1, flag1);
  EXPECT_THAT(transform_system->HasFlag(1, flag1), Eq(true));
  EXPECT_THAT(transform_system->HasFlag(1, flag2), Eq(false));

  transform_system->SetFlag(1, flag2);
  EXPECT_THAT(transform_system->HasFlag(1, flag1), Eq(true));
  EXPECT_THAT(transform_system->HasFlag(1, flag2), Eq(true));

  transform_system->ClearFlag(1, flag1);
  EXPECT_THAT(transform_system->HasFlag(1, flag1), Eq(false));
  EXPECT_THAT(transform_system->HasFlag(1, flag2), Eq(true));

  transform_system->ClearFlag(1, flag2);
  EXPECT_THAT(transform_system->HasFlag(1, flag1), Eq(false));
  EXPECT_THAT(transform_system->HasFlag(1, flag2), Eq(false));

  transform_system->SetFlag(1, flag1);
  transform_system->SetFlag(1, flag2);
  transform_system->Destroy(1);
  EXPECT_THAT(transform_system->HasFlag(1, flag1), Eq(false));
  EXPECT_THAT(transform_system->HasFlag(1, flag2), Eq(false));
}

TEST_F(TransformSystemDeathTest, TooManyFlags) {
  auto* transform_system = registry_.Get<TransformSystem>();
  for (int i = 0; i < 32; ++i) {
    transform_system->RequestFlag();
  }
  EXPECT_DEATH_IF_SUPPORTED(transform_system->RequestFlag(), "");
}

TEST_F(TransformSystemTest, ForAll) {
  CreateDefaultTransform(1);
  CreateDefaultTransform(2);
  CreateDefaultTransform(3);

  int count = 0;
  auto fn = [&](Entity entity, const mathfu::mat4& matrix, const Aabb& aabb,
                uint32_t flags) { count += static_cast<int>(entity); };

  auto* transform_system = registry_.Get<TransformSystem>();
  transform_system->ForAll(fn);
  EXPECT_THAT(count, Eq(6));

  transform_system->Destroy(2);
  transform_system->ForAll(fn);
  EXPECT_THAT(count, Eq(10));
}

TEST_F(TransformSystemTest, ForEach) {
  CreateDefaultTransform(1);
  CreateDefaultTransform(2);
  CreateDefaultTransform(3);

  int count = 0;
  auto fn = [&](Entity entity, const mathfu::mat4& matrix, const Aabb& aabb) {
    count += static_cast<int>(entity);
  };

  auto* transform_system = registry_.Get<TransformSystem>();
  transform_system->ForEach(TransformSystem::kAllFlags, fn);
  EXPECT_THAT(count, Eq(6));

  transform_system->Destroy(2);
  transform_system->ForEach(TransformSystem::kAllFlags, fn);
  EXPECT_THAT(count, Eq(10));
}

TEST_F(TransformSystemTest, ForEachFiltered) {
  CreateDefaultTransform(1);
  CreateDefaultTransform(2);
  CreateDefaultTransform(3);

  int count = 0;
  auto fn = [&](Entity entity, const mathfu::mat4& matrix, const Aabb& aabb) {
    count += static_cast<int>(entity);
  };

  auto* transform_system = registry_.Get<TransformSystem>();
  const TransformSystem::TransformFlags flag = transform_system->RequestFlag();
  transform_system->SetFlag(1, flag);
  transform_system->SetFlag(2, flag);

  transform_system->ForEach(flag, fn);
  EXPECT_THAT(count, Eq(3));

  transform_system->Destroy(2);
  transform_system->ForEach(flag, fn);
  EXPECT_THAT(count, Eq(4));
}

TEST_F(TransformSystemTest, ForAllDescendants) {
  CreateDefaultTransform(1);
  CreateDefaultTransform(2);
  CreateDefaultTransform(3);
  CreateDefaultTransform(4);
  CreateDefaultTransform(5);

  int count = 0;
  auto fn = [&](Entity entity) { count += static_cast<int>(entity); };

  auto* transform_system = registry_.Get<TransformSystem>();
  transform_system->AddChild(1, 2);
  transform_system->AddChild(2, 3);
  transform_system->AddChild(2, 4);
  transform_system->AddChild(4, 5);

  transform_system->ForAllDescendants(1, fn);
  EXPECT_THAT(count, Eq(15));

  transform_system->ForAllDescendants(2, fn);
  EXPECT_THAT(count, Eq(29));

  transform_system->Destroy(4);
  transform_system->ForAllDescendants(1, fn);
  EXPECT_THAT(count, Eq(35));
}

TEST_F(TransformSystemTest, Parenting) {
  SetupEventHandlers();

  TransformDefT transform;
  transform.position = mathfu::vec3(1.f, 0.f, 0.f);
  transform.rotation = mathfu::vec3(0.f, 0.f, 0.f);
  transform.scale = mathfu::vec3(1.f, 1.f, 1.f);
  Blueprint blueprint(&transform);

  const Entity parent = 1;
  const Entity child = 2;
  const Entity grand_child = 3;

  auto* transform_system = registry_.Get<TransformSystem>();
  transform_system->CreateComponent(parent, blueprint);
  transform_system->CreateComponent(child, blueprint);
  transform_system->CreateComponent(grand_child, blueprint);

  // Test adding and getting children
  ExpectParentChangedEvent(child, kNullEntity, parent);
  transform_system->AddChild(parent, child);
  ExpectParentChangedEvent(grand_child, kNullEntity, child);
  transform_system->AddChild(child, grand_child);
  DisallowParentChangedEvent();

  EXPECT_TRUE(transform_system->IsAncestorOf(parent, child));
  EXPECT_TRUE(transform_system->IsAncestorOf(parent, grand_child));
  EXPECT_FALSE(transform_system->IsAncestorOf(parent, parent));
  EXPECT_FALSE(transform_system->IsAncestorOf(child, parent));

  auto children = transform_system->GetChildren(parent);
  EXPECT_THAT(static_cast<int>(children->size()), Eq(1));
  Entity test = children->at(0);
  EXPECT_THAT(test, Eq(child));

  children = transform_system->GetChildren(child);
  EXPECT_THAT(static_cast<int>(children->size()), Eq(1));
  test = children->at(0);
  EXPECT_THAT(test, Eq(grand_child));

  // expect the transforms of parents to be inherited
  auto mat = *transform_system->GetWorldFromEntityMatrix(grand_child);

  EXPECT_NEAR(mat(0, 3), 3.0f, 0.001f);

  // Test removing a child
  ExpectParentChangedEvent(child, parent, kNullEntity);
  transform_system->RemoveParent(child);
  DisallowParentChangedEvent();

  children = transform_system->GetChildren(parent);
  EXPECT_THAT(static_cast<int>(children->size()), Eq(0));

  children = transform_system->GetChildren(child);
  EXPECT_THAT(static_cast<int>(children->size()), Eq(1));
  test = children->at(0);
  EXPECT_THAT(test, Eq(grand_child));

  EXPECT_FALSE(transform_system->IsAncestorOf(parent, child));
  EXPECT_FALSE(transform_system->IsAncestorOf(parent, grand_child));
}

TEST_F(TransformSystemTest, ParentingWithNullParents) {
  SetupEventHandlers();

  TransformDefT transform;
  transform.position = mathfu::vec3(1.f, 0.f, 0.f);
  transform.rotation = mathfu::vec3(0.f, 0.f, 0.f);
  transform.scale = mathfu::vec3(1.f, 1.f, 1.f);
  Blueprint blueprint(&transform);

  const Entity parent = kNullEntity;
  const Entity child = 2;

  auto* transform_system = registry_.Get<TransformSystem>();
  transform_system->CreateComponent(parent, blueprint);
  transform_system->CreateComponent(child, blueprint);

  ExpectParentChangedEvent(child, kNullEntity, parent);
  transform_system->AddChild(parent, child);
  DisallowParentChangedEvent();

  EXPECT_FALSE(transform_system->IsAncestorOf(parent, child));
  EXPECT_FALSE(transform_system->IsAncestorOf(parent, parent));
  EXPECT_FALSE(transform_system->IsAncestorOf(child, parent));
}

void TransformSystemTest::ExpectParentChangedEvent(Entity child,
                                                   Entity old_parent,
                                                   Entity new_parent) {
  test_event_child_ = child;
  test_event_old_parent_ = old_parent;
  test_event_new_parent_ = new_parent;
}

void TransformSystemTest::DisallowParentChangedEvent() {
  test_event_child_ = kNullEntity;
  test_event_old_parent_ = kNullEntity;
  test_event_new_parent_ = kNullEntity;
}

void TransformSystemTest::OnParentChanged(const ParentChangedEvent& e) {
  EXPECT_THAT(e.target, Eq(test_event_child_));
  EXPECT_THAT(e.old_parent, Eq(test_event_old_parent_));
  EXPECT_THAT(e.new_parent, Eq(test_event_new_parent_));
}

void TransformSystemTest::OnChildAdded(const ChildAddedEvent& e) {
  EXPECT_THAT(e.target, Eq(test_event_new_parent_));
  EXPECT_THAT(e.child, Eq(test_event_child_));
}

void TransformSystemTest::OnChildRemoved(const ChildRemovedEvent& e) {
  EXPECT_THAT(e.target, Eq(test_event_old_parent_));
  EXPECT_THAT(e.child, Eq(test_event_child_));
}

}  // namespace
}  // namespace lull
