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

#include <deque>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/tests/portable_test_macros.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;
using testing::EqualsMathfuVec3;
using testing::EqualsMathfuQuat;
using testing::NearMathfuVec3;
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
    dispatcher->Connect(this, [this](const ParentChangedImmediateEvent& event) {
      OnParentChangedImmediate(event);
    });
    dispatcher->Connect(
        this, [this](const ChildAddedEvent& event) { OnChildAdded(event); });
    dispatcher->Connect(this, [this](const ChildRemovedEvent& event) {
      OnChildRemoved(event);
    });
    DisallowAllEvents();
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
  // Clears out the cached state of events received from the ParentChanged,
  // ChildAdded, and ChildRemoved event handlers.
  void ClearAllEventsReceived();
  void ClearParentChangedEventsReceived();
  void ClearChildAddedEventsReceived();
  void ClearChildRemovedEventsReceived();

  // Enforces whether or not an event callback is expected for the given events.
  void AllowAllEvents(bool allow = true);
  void DisallowAllEvents() { AllowAllEvents(false); }

  void AllowParentChangedEvents(bool allow = true);
  void DisallowParentChangedEvents() { AllowParentChangedEvents(false); }

  void AllowChildAddedEvents(bool allow = true);
  void DisallowChildAddedEvents() { AllowChildAddedEvents(false); }

  void AllowChildRemovedEvents(bool allow = true);
  void DisallowChildRemovedEvents() { AllowChildRemovedEvents(false); }

  // Enforces that only a single pair of ParentChanged[Immediate]Event with the
  // given |target|, |old_parent|, and |new_parent| was received. This is
  // equivalent to calling ExpectParentChangedEventSequence with an
  // expected_sequence containing only one event. This should be called
  // immediately after the function expected to trigger the event.
  void ExpectParentChangedEvent(Entity target, Entity old_parent,
                                Entity new_parent);

  // Enforces that all ParentChanged[Immediate]Events in the |expected_sequence|
  // were received in order and that no additional events were received. This
  // should be called immediately after the function(s) expected to trigger the
  // events.
  void ExpectParentChangedEventSequence(
      const std::deque<ParentChangedEvent>& expected_sequence);

  // Enforces that only a single ChildAddedEvent with the given |child| and
  // |new_parent| was received. This is equivalent to calling
  // ExpectChildAddedEventSequence with an expected_sequence containing only one
  // event. This should be called immediately after the function expected to
  // trigger the event.
  void ExpectChildAddedEvent(Entity child, Entity new_parent);

  // Enforces that all ChildAddedEvents in the |expected_sequence| were received
  // in order and that no additional events were received. This should be called
  // immediately after the function(s) expected to trigger the events.
  void ExpectChildAddedEventSequence(
      const std::deque<ChildAddedEvent>& expected_sequence);

  // Enforces that only a single ChildRemovedEvent with the given |child| and
  // |old_parent| was received. This is equivalent to calling
  // ExpectChildRemovedEventSequence with an expected_sequence containing only
  // one event. This should be called immediately after the function expected to
  // trigger the event.
  void ExpectChildRemovedEvent(Entity child, Entity old_parent);

  // Enforces that all ChildRemovedEvents in the |expected_sequence| were
  // received in order and that no additional events were received. This should
  // be called immediately after the function(s) expected to trigger the
  // events.
  void ExpectChildRemovedEventSequence(
      const std::deque<ChildRemovedEvent>& expected_sequence);

  // Enforces that exactly |n| entities currently have transforms.
  void ExpectTransformsCount(int n);

  void OnParentChanged(const ParentChangedEvent& e);
  void OnParentChangedImmediate(const ParentChangedImmediateEvent& e);
  void OnChildAdded(const ChildAddedEvent& e);
  void OnChildRemoved(const ChildRemovedEvent& e);

  Registry registry_;

  bool expect_parent_changed_event_ = false;
  bool expect_child_added_event_ = false;
  bool expect_child_removed_event_ = false;
  std::deque<ParentChangedEvent> parent_changed_events_received_;
  std::deque<ParentChangedImmediateEvent>
      parent_changed_immediate_events_received_;
  std::deque<ChildAddedEvent> child_added_events_received_;
  std::deque<ChildRemovedEvent> child_removed_events_received_;
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

TEST_F(TransformSystemTest, AddChildPreserveWorldToEntity) {
  auto* transform_system = registry_.Get<TransformSystem>();
  const Entity child = 1;
  const Entity parent_1 = 2;
  const Entity parent_2 = 3;

  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 2.f, 3.f);
    transform.rotation = mathfu::vec3(90.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(2.f, 3.f, 4.f);
    Blueprint blueprint(&transform);
    transform_system->CreateComponent(child, blueprint);
    transform_system->CreateComponent(parent_1, blueprint);
    transform_system->AddChild(parent_1, child);
  }
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(1.f, 1.f, 2.f);
    transform.rotation = mathfu::vec3(0.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(1.f, 1.f, 2.f);
    Blueprint blueprint(&transform);
    transform_system->CreateComponent(parent_2, blueprint);
  }
  Sqt sqt =
      CalculateSqtFromMatrix(transform_system->GetWorldFromEntityMatrix(child));
  EXPECT_THAT(sqt.translation, NearMathfuVec3({3.f, -10.f, 9.f}, kEpsilon));
  EXPECT_THAT(sqt.rotation.ToEulerAngles(),
              NearMathfuVec3(mathfu::vec3(180.f, 0.f, 0.f) * kDegreesToRadians,
                             kEpsilon));
  EXPECT_THAT(sqt.scale, NearMathfuVec3({4.f, 12.f, 12.f}, kEpsilon));

  transform_system->AddChild(
      parent_2, child,
      TransformSystem::AddChildMode::kPreserveWorldToEntityTransform);

  // Child has not changed world pose.
  sqt =
      CalculateSqtFromMatrix(transform_system->GetWorldFromEntityMatrix(child));
  EXPECT_THAT(sqt.translation, NearMathfuVec3({3.f, -10.f, 9.f}, kEpsilon));
  EXPECT_THAT(sqt.rotation.ToEulerAngles(),
              NearMathfuVec3(mathfu::vec3(180.f, 0.f, 0.f) * kDegreesToRadians,
                             kEpsilon));
  EXPECT_THAT(sqt.scale, NearMathfuVec3({4.f, 12.f, 12.f}, kEpsilon));

  // Child has new local sqt based on new parent's sqt.
  sqt = *transform_system->GetSqt(child);
  EXPECT_THAT(sqt.translation, NearMathfuVec3({2.f, -11.f, 3.5f}, kEpsilon));
  EXPECT_THAT(sqt.rotation.ToEulerAngles(),
              NearMathfuVec3(mathfu::vec3(180.f, 0.f, 0.f) * kDegreesToRadians,
                             kEpsilon));
  EXPECT_THAT(sqt.scale, NearMathfuVec3({4.f, 12.f, 6.f}, kEpsilon));
}

TEST_F(TransformSystemTest, GetChildCount) {
  auto* transform_system = registry_.Get<TransformSystem>();

  CreateDefaultTransform(1);
  EXPECT_THAT(transform_system->GetChildCount(1), Eq(0ul));

  // Add 2 children.
  //
  //   1
  //  / \
  // 2   3
  CreateDefaultTransform(2);
  CreateDefaultTransform(3);
  transform_system->AddChild(1, 2);
  transform_system->AddChild(1, 3);

  EXPECT_THAT(transform_system->GetChildCount(1), Eq(2ul));

  // Add 2 grandchildren. This should not affect the child count.
  //
  //   1
  //  / \
  // 2   3
  //    / \
  //   4   5
  CreateDefaultTransform(4);
  CreateDefaultTransform(5);
  transform_system->AddChild(3, 4);
  transform_system->AddChild(3, 5);

  EXPECT_THAT(transform_system->GetChildCount(1), Eq(2ul));

  // Ask for child count of entity which doesn't have a TransformDef component.
  EXPECT_THAT(transform_system->GetChildCount(6), Eq(0ul));
}

TEST_F(TransformSystemTest, GetChildIndex) {
  auto* transform_system = registry_.Get<TransformSystem>();

  // Create a simple family tree.
  //
  //   _1_
  //  / | \
  // 2  3  4
  //
  CreateDefaultTransform(1);
  CreateDefaultTransform(2);
  CreateDefaultTransform(3);
  CreateDefaultTransform(4);
  transform_system->AddChild(1, 2);
  transform_system->AddChild(1, 3);
  transform_system->AddChild(1, 4);

  // An entity without a parent should have index of 0.
  EXPECT_THAT(transform_system->GetChildIndex(1), Eq(0ul));

  EXPECT_THAT(transform_system->GetChildIndex(2), Eq(0ul));
  EXPECT_THAT(transform_system->GetChildIndex(3), Eq(1ul));
  EXPECT_THAT(transform_system->GetChildIndex(4), Eq(2ul));

  // Index of entity without a TransformDef component should be 0, DFATAL on
  // debug builds.
  PORT_EXPECT_DEBUG_DEATH(transform_system->GetChildIndex(5), "");
#if !ION_DEBUG
  EXPECT_THAT(transform_system->GetChildIndex(5), Eq(0ul));
#endif
}

TEST_F(TransformSystemTest, InsertChild) {
  SetupEventHandlers();
  auto* transform_system = registry_.Get<TransformSystem>();

  // Create a simple family tree with a parent and two children.
  //
  //   1
  //  / \
  // 2   3
  CreateDefaultTransform(1);
  CreateDefaultTransform(2);
  CreateDefaultTransform(3);
  AllowAllEvents();
  transform_system->AddChild(1, 2);
  transform_system->AddChild(1, 3);
  DisallowAllEvents();

  // Insert new child at index 1.
  //
  //   _1_
  //  / | \
  // 2  4  3
  //
  ClearAllEventsReceived();
  AllowParentChangedEvents();
  AllowChildAddedEvents();

  CreateDefaultTransform(4);
  transform_system->InsertChild(1, 4, 1);

  ExpectParentChangedEvent(4, kNullEntity, 1);
  ExpectChildAddedEvent(4, 1);
  DisallowAllEvents();

  EXPECT_THAT(transform_system->GetChildCount(1), Eq(3ul));

  EXPECT_THAT(transform_system->GetChildIndex(2), Eq(0ul));
  EXPECT_THAT(transform_system->GetChildIndex(3), Eq(2ul));
  EXPECT_THAT(transform_system->GetChildIndex(4), Eq(1ul));

  // Inserting an existing child should just move the child to the new index.
  ClearAllEventsReceived();
  DisallowAllEvents();

  transform_system->InsertChild(1, 4, 2);

  // Total child count should not change.
  EXPECT_THAT(transform_system->GetChildCount(1), Eq(3ul));

  EXPECT_THAT(transform_system->GetChildIndex(2), Eq(0ul));
  EXPECT_THAT(transform_system->GetChildIndex(3), Eq(1ul));
  EXPECT_THAT(transform_system->GetChildIndex(4), Eq(2ul));

  // InsertChild from a different parent should re-parent and move to the
  // correct index.
  AllowAllEvents();
  CreateDefaultTransform(5);
  CreateDefaultTransform(6);
  transform_system->AddChild(5, 6);
  DisallowAllEvents();

  ClearAllEventsReceived();
  AllowParentChangedEvents();
  AllowChildAddedEvents();

  transform_system->InsertChild(1, 6, 3);

  ExpectParentChangedEvent(6, 5, 1);
  ExpectChildAddedEvent(6, 1);
  DisallowAllEvents();

  EXPECT_THAT(transform_system->GetChildCount(1), Eq(4ul));
  EXPECT_THAT(transform_system->GetChildCount(5), Eq(0ul));

  EXPECT_THAT(transform_system->GetChildIndex(6), Eq(3ul));
}

TEST_F(TransformSystemTest, MoveChild) {
  SetupEventHandlers();
  auto* transform_system = registry_.Get<TransformSystem>();

  // Create a simple family tree with a parent and three children.
  CreateDefaultTransform(1);
  CreateDefaultTransform(2);
  CreateDefaultTransform(3);
  CreateDefaultTransform(4);
  AllowAllEvents();
  transform_system->AddChild(1, 2);
  transform_system->AddChild(1, 3);
  transform_system->AddChild(1, 4);
  DisallowAllEvents();

  // Move to new location in the list.
  // Move '4' to the beginning of the list.
  transform_system->MoveChild(4, 0);

  // List should be [4, 2, 3].
  EXPECT_THAT(transform_system->GetChildIndex(2), Eq(1ul));
  EXPECT_THAT(transform_system->GetChildIndex(3), Eq(2ul));
  EXPECT_THAT(transform_system->GetChildIndex(4), Eq(0ul));

  // Move past the end of the list.
  transform_system->MoveChild(4, 6);

  // List should be [2, 3, 4].
  EXPECT_THAT(transform_system->GetChildIndex(2), Eq(0ul));
  EXPECT_THAT(transform_system->GetChildIndex(3), Eq(1ul));
  EXPECT_THAT(transform_system->GetChildIndex(4), Eq(2ul));

  // Move with negative index where '-1' = last element in the list.
  transform_system->MoveChild(4, -2);

  // List should be [2, 4, 3].
  EXPECT_THAT(transform_system->GetChildIndex(2), Eq(0ul));
  EXPECT_THAT(transform_system->GetChildIndex(3), Eq(2ul));
  EXPECT_THAT(transform_system->GetChildIndex(4), Eq(1ul));

  // Move '2' to the back of the list.
  transform_system->MoveChild(2, -1);

  // List should be [4, 3, 2].
  EXPECT_THAT(transform_system->GetChildIndex(2), Eq(2ul));
  EXPECT_THAT(transform_system->GetChildIndex(3), Eq(1ul));
  EXPECT_THAT(transform_system->GetChildIndex(4), Eq(0ul));

  // Move with negative index past the beginning of the list. This should clamp
  // to the size of the list.
  transform_system->MoveChild(3, -6);

  // List should be [3, 4, 2].
  EXPECT_THAT(transform_system->GetChildIndex(2), Eq(2ul));
  EXPECT_THAT(transform_system->GetChildIndex(3), Eq(0ul));
  EXPECT_THAT(transform_system->GetChildIndex(4), Eq(1ul));

  // Move a child with no parent.
  transform_system->MoveChild(1, 2);
  EXPECT_THAT(transform_system->GetChildIndex(1), Eq(0ul));
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
  ClearAllEventsReceived();
  AllowParentChangedEvents();
  AllowChildAddedEvents();

  transform_system->AddChild(parent, child);
  transform_system->AddChild(child, grand_child);

  const std::deque<ParentChangedEvent> parent_add_sequence = {
      {child, kNullEntity, parent}, {grand_child, kNullEntity, child}};
  ExpectParentChangedEventSequence(parent_add_sequence);

  const std::deque<ChildAddedEvent> child_add_sequence = {
      {parent, child}, {child, grand_child}};
  ExpectChildAddedEventSequence(child_add_sequence);

  DisallowAllEvents();

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
  ClearAllEventsReceived();
  AllowParentChangedEvents();
  AllowChildRemovedEvents();

  transform_system->RemoveParent(child);

  ExpectParentChangedEvent(child, parent, kNullEntity);
  ExpectChildRemovedEvent(child, parent);

  DisallowAllEvents();

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

  ClearAllEventsReceived();

  // No events should be sent out when attempting to add a child to a null
  // parent entity.
  DisallowParentChangedEvents();
  DisallowChildAddedEvents();

  transform_system->AddChild(parent, child);

  DisallowAllEvents();

  EXPECT_FALSE(transform_system->IsAncestorOf(parent, child));
  EXPECT_FALSE(transform_system->IsAncestorOf(parent, parent));
  EXPECT_FALSE(transform_system->IsAncestorOf(child, parent));
}

TEST_F(TransformSystemTest, DestroyChild) {
  SetupEventHandlers();

  TransformDefT transform;
  transform.position = mathfu::vec3(1.f, 0.f, 0.f);
  transform.rotation = mathfu::vec3(0.f, 0.f, 0.f);
  transform.scale = mathfu::vec3(1.f, 1.f, 1.f);
  Blueprint blueprint(&transform);

  const Entity parent = 1;
  const Entity child = 2;
  const Entity grand_child_a = 3;
  const Entity grand_child_b = 4;

  auto* transform_system = registry_.Get<TransformSystem>();
  transform_system->CreateComponent(parent, blueprint);
  transform_system->CreateComponent(child, blueprint);
  transform_system->CreateComponent(grand_child_a, blueprint);
  transform_system->CreateComponent(grand_child_b, blueprint);

  ExpectTransformsCount(4);

  // Create a simple family with a parent, child, and 2 grandchildren.
  ClearAllEventsReceived();
  AllowParentChangedEvents();
  AllowChildAddedEvents();

  transform_system->AddChild(parent, child);
  transform_system->AddChild(child, grand_child_a);
  transform_system->AddChild(child, grand_child_b);

  // Check that we received the expected events.
  const std::deque<ParentChangedEvent> parent_add_sequence = {
      {child, kNullEntity, parent},
      {grand_child_a, kNullEntity, child},
      {grand_child_b, kNullEntity, child}};
  ExpectParentChangedEventSequence(parent_add_sequence);
  const std::deque<ChildAddedEvent> child_add_sequence = {
      {parent, child}, {child, grand_child_a}, {child, grand_child_b}};
  ExpectChildAddedEventSequence(child_add_sequence);

  DisallowAllEvents();

  // Ensure the ancestry is set up correctly.
  EXPECT_TRUE(transform_system->IsAncestorOf(parent, child));
  EXPECT_TRUE(transform_system->IsAncestorOf(child, grand_child_a));
  EXPECT_TRUE(transform_system->IsAncestorOf(child, grand_child_b));
  EXPECT_TRUE(transform_system->IsAncestorOf(parent, grand_child_a));
  EXPECT_TRUE(transform_system->IsAncestorOf(parent, grand_child_b));

  // Destroying the child should also destroy grand_child_a & b.
  ClearAllEventsReceived();
  AllowParentChangedEvents();
  AllowChildRemovedEvents();

  auto* entity_factory = registry_.Get<EntityFactory>();
  entity_factory->Destroy(child);

  // Check that the correct sequence of events was received.
  const std::deque<ParentChangedEvent> parent_destroy_sequence = {
      {grand_child_a, child, kNullEntity},
      {grand_child_b, child, kNullEntity},
      {child, parent, kNullEntity}};
  ExpectParentChangedEventSequence(parent_destroy_sequence);
  const std::deque<ChildRemovedEvent> child_destroy_sequence = {
      {child, grand_child_a}, {child, grand_child_b}, {parent, child}};
  ExpectChildRemovedEventSequence(child_destroy_sequence);

  ExpectTransformsCount(1);
}

TEST_F(TransformSystemTest, DestroyChildren) {
  SetupEventHandlers();

  TransformDefT transform;
  transform.position = mathfu::vec3(1.f, 0.f, 0.f);
  transform.rotation = mathfu::vec3(0.f, 0.f, 0.f);
  transform.scale = mathfu::vec3(1.f, 1.f, 1.f);
  Blueprint blueprint(&transform);

  const Entity parent = 1;
  const Entity child_a = 2;
  const Entity grand_child_a = 3;
  const Entity grand_child_b = 4;
  const Entity child_b = 5;
  const Entity grand_child_c = 6;
  const Entity grand_child_d = 7;

  auto* transform_system = registry_.Get<TransformSystem>();
  transform_system->CreateComponent(parent, blueprint);
  transform_system->CreateComponent(child_a, blueprint);
  transform_system->CreateComponent(grand_child_a, blueprint);
  transform_system->CreateComponent(grand_child_b, blueprint);
  transform_system->CreateComponent(child_b, blueprint);
  transform_system->CreateComponent(grand_child_c, blueprint);
  transform_system->CreateComponent(grand_child_d, blueprint);

  ExpectTransformsCount(7);

  // Create a simple family with a parent, child, and 2 grandchildren.
  ClearAllEventsReceived();
  AllowParentChangedEvents();
  AllowChildAddedEvents();

  transform_system->AddChild(parent, child_a);
  transform_system->AddChild(child_a, grand_child_a);
  transform_system->AddChild(child_a, grand_child_b);
  transform_system->AddChild(parent, child_b);
  transform_system->AddChild(child_b, grand_child_c);
  transform_system->AddChild(child_b, grand_child_d);

  // Check that we received the expected events.
  const std::deque<ParentChangedEvent> parent_add_sequence = {
      {child_a, kNullEntity, parent},
      {grand_child_a, kNullEntity, child_a},
      {grand_child_b, kNullEntity, child_a},
      {child_b, kNullEntity, parent},
      {grand_child_c, kNullEntity, child_b},
      {grand_child_d, kNullEntity, child_b}};
  ExpectParentChangedEventSequence(parent_add_sequence);
  const std::deque<ChildAddedEvent> child_add_sequence = {
      {parent, child_a}, {child_a, grand_child_a}, {child_a, grand_child_b},
      {parent, child_b}, {child_b, grand_child_c}, {child_b, grand_child_d}};
  ExpectChildAddedEventSequence(child_add_sequence);

  DisallowAllEvents();

  // Ensure the ancestry is set up correctly.
  EXPECT_TRUE(transform_system->IsAncestorOf(parent, child_a));
  EXPECT_TRUE(transform_system->IsAncestorOf(parent, child_b));
  EXPECT_TRUE(transform_system->IsAncestorOf(child_a, grand_child_a));
  EXPECT_TRUE(transform_system->IsAncestorOf(child_a, grand_child_b));
  EXPECT_TRUE(transform_system->IsAncestorOf(child_b, grand_child_c));
  EXPECT_TRUE(transform_system->IsAncestorOf(child_b, grand_child_d));
  EXPECT_TRUE(transform_system->IsAncestorOf(parent, grand_child_a));
  EXPECT_TRUE(transform_system->IsAncestorOf(parent, grand_child_b));
  EXPECT_TRUE(transform_system->IsAncestorOf(parent, grand_child_c));
  EXPECT_TRUE(transform_system->IsAncestorOf(parent, grand_child_d));

  // Destroying the child should also destroy grand_child_a & b.
  ClearAllEventsReceived();
  AllowParentChangedEvents();
  AllowChildRemovedEvents();

  transform_system->DestroyChildren(parent);

  // Check that the correct sequence of events was received.
  const std::deque<ParentChangedEvent> parent_destroy_sequence = {
      {grand_child_a, child_a, kNullEntity},
      {grand_child_b, child_a, kNullEntity},
      {child_a, parent, kNullEntity},
      {grand_child_c, child_b, kNullEntity},
      {grand_child_d, child_b, kNullEntity},
      {child_b, parent, kNullEntity}};
  ExpectParentChangedEventSequence(parent_destroy_sequence);
  const std::deque<ChildRemovedEvent> child_destroy_sequence = {
      {child_a, grand_child_a}, {child_a, grand_child_b}, {parent, child_a},
      {child_b, grand_child_c}, {child_b, grand_child_d}, {parent, child_b}};
  ExpectChildRemovedEventSequence(child_destroy_sequence);

  ExpectTransformsCount(1);
}

void TransformSystemTest::ClearAllEventsReceived() {
  ClearParentChangedEventsReceived();
  ClearChildAddedEventsReceived();
  ClearChildRemovedEventsReceived();
}

void TransformSystemTest::ClearParentChangedEventsReceived() {
  parent_changed_events_received_.clear();
  parent_changed_immediate_events_received_.clear();
}

void TransformSystemTest::ClearChildAddedEventsReceived() {
  child_added_events_received_.clear();
}

void TransformSystemTest::ClearChildRemovedEventsReceived() {
  child_removed_events_received_.clear();
}

void TransformSystemTest::AllowAllEvents(bool allow) {
  AllowParentChangedEvents(allow);
  AllowChildAddedEvents(allow);
  AllowChildRemovedEvents(allow);
}

void TransformSystemTest::AllowParentChangedEvents(bool allow) {
  expect_parent_changed_event_ = allow;
}

void TransformSystemTest::AllowChildAddedEvents(bool allow) {
  expect_child_added_event_ = allow;
}

void TransformSystemTest::AllowChildRemovedEvents(bool allow) {
  expect_child_removed_event_ = allow;
}

// Enforces that two ordered sequences of events match. That is, they should
// have the same number of events and the events should appear in the same
// order in each sequence. The |expectation_func| should implement the EXPECT_*
// statements comparing the fields of the event type T.
template <typename T>
void ExpectEventSequencesMatch(
    std::deque<T> expected_sequence, std::deque<T> actual_sequence,
    std::function<void(const T& expected_event, const T& actual_event)>
        expectation_func) {
  // Check that we received exactly the number of events we expected.
  EXPECT_THAT(actual_sequence.size(), Eq(expected_sequence.size()));

  // Check that the contents of the individual events match.
  while (!expected_sequence.empty()) {
    auto expected_event = expected_sequence.front();
    auto actual_event = actual_sequence.front();
    expectation_func(expected_event, actual_event);
    expected_sequence.pop_front();
    actual_sequence.pop_front();
  }
}

void TransformSystemTest::ExpectParentChangedEvent(Entity target,
                                                   Entity old_parent,
                                                   Entity new_parent) {
  const std::deque<ParentChangedEvent> expected_sequence = {
      {target, old_parent, new_parent}};
  ExpectParentChangedEventSequence(expected_sequence);
}

void TransformSystemTest::ExpectParentChangedEventSequence(
    const std::deque<ParentChangedEvent>& expected_sequence) {
  // Manually test the immediate events since their types differ. Don't drain
  // expected_sequence while we do this since we need it for the second test
  // below.
  EXPECT_THAT(parent_changed_immediate_events_received_.size(),
              Eq(expected_sequence.size()));
  for (size_t i = 0; i < expected_sequence.size(); ++i) {
    const ParentChangedImmediateEvent& actual_event =
        parent_changed_immediate_events_received_[i];
    const ParentChangedEvent& expected_event = expected_sequence[i];
    EXPECT_THAT(actual_event.target, Eq(expected_event.target));
    EXPECT_THAT(actual_event.old_parent, Eq(expected_event.old_parent));
    EXPECT_THAT(actual_event.new_parent, Eq(expected_event.new_parent));
  }

  ExpectEventSequencesMatch<ParentChangedEvent>(
      expected_sequence, parent_changed_events_received_,
      [](const ParentChangedEvent& expected_event,
         const ParentChangedEvent& actual_event) {
        EXPECT_THAT(actual_event.target, Eq(expected_event.target));
        EXPECT_THAT(actual_event.old_parent, Eq(expected_event.old_parent));
        EXPECT_THAT(actual_event.new_parent, Eq(expected_event.new_parent));
      });
}

void TransformSystemTest::ExpectChildAddedEvent(Entity child,
                                                Entity new_parent) {
  const std::deque<ChildAddedEvent> expected_sequence = {{new_parent, child}};
  ExpectChildAddedEventSequence(expected_sequence);
}

void TransformSystemTest::ExpectChildAddedEventSequence(
    const std::deque<ChildAddedEvent>& expected_sequence) {
  ExpectEventSequencesMatch<ChildAddedEvent>(
      expected_sequence, child_added_events_received_,
      [](const ChildAddedEvent& expected_event,
         const ChildAddedEvent& actual_event) {
        EXPECT_THAT(actual_event.target, Eq(expected_event.target));
        EXPECT_THAT(actual_event.child, Eq(expected_event.child));
      });
}

void TransformSystemTest::ExpectChildRemovedEvent(Entity child,
                                                  Entity old_parent) {
  const std::deque<ChildRemovedEvent> expected_sequence = {
      {old_parent, child}};
  ExpectChildRemovedEventSequence(expected_sequence);
}

void TransformSystemTest::ExpectChildRemovedEventSequence(
    const std::deque<ChildRemovedEvent>& expected_sequence) {
  ExpectEventSequencesMatch<ChildRemovedEvent>(
      expected_sequence, child_removed_events_received_,
      [](const ChildRemovedEvent& expected_event,
         const ChildRemovedEvent& actual_event) {
        EXPECT_THAT(actual_event.target, Eq(expected_event.target));
        EXPECT_THAT(actual_event.child, Eq(expected_event.child));
      });
}

void TransformSystemTest::OnParentChanged(const ParentChangedEvent& e) {
  EXPECT_TRUE(expect_parent_changed_event_);
  parent_changed_events_received_.push_back(e);
}

void TransformSystemTest::OnParentChangedImmediate(
    const ParentChangedImmediateEvent& e) {
  EXPECT_TRUE(expect_parent_changed_event_);
  parent_changed_immediate_events_received_.push_back(e);
}

void TransformSystemTest::OnChildAdded(const ChildAddedEvent& e) {
  EXPECT_TRUE(expect_child_added_event_);
  child_added_events_received_.push_back(e);
}

void TransformSystemTest::OnChildRemoved(const ChildRemovedEvent& e) {
  EXPECT_TRUE(expect_child_removed_event_);
  child_removed_events_received_.push_back(e);
}

void TransformSystemTest::ExpectTransformsCount(int n) {
  auto* transform_system = registry_.Get<TransformSystem>();
  int count = 0;
  transform_system->ForAll(
      [&count](Entity, const mathfu::mat4&, const Aabb&, Bits) { count++; });
  EXPECT_EQ(count, n);
}

}  // namespace
}  // namespace lull
