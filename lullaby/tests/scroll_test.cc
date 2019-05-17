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

#include <unordered_map>

#include "gtest/gtest.h"
#include "lullaby/events/input_events.h"
#include "lullaby/events/scroll_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/contrib/scroll/scroll_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/scroll_def_generated.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

// TODO - figure out why this epsilon is needed when checking
// scroll offset.  After an animated scroll, why doesn't it hit the target
// exactly?
static const float kEpsilon = 1e-4f;

static const Clock::duration oneMillisecond(1000 * 1000);
static const Clock::duration oneMicrosecond(1000);
static const Clock::duration oneSecond(1000 * 1000 * 1000);

class ScrollTest : public ::testing::Test {
 protected:
  ScrollTest() {
    auto dispatcher = registry_.Create<Dispatcher>();

    registry_.Create<InputManager>();

    // Tell the input manager it has a controller
    DeviceProfile profile;
    profile.buttons.resize(3);
    profile.touchpads.resize(1);
    profile.rotation_dof = DeviceProfile::kRealDof;
    input_manager_ = registry_.Get<InputManager>();
    input_manager_->ConnectDevice(InputManager::kController, profile);

    entity_factory_ = registry_.Create<EntityFactory>(&registry_);
    entity_factory_->CreateSystem<AnimationSystem>();
    entity_factory_->CreateSystem<DispatcherSystem>();
    entity_factory_->CreateSystem<ScrollSystem>();
    entity_factory_->CreateSystem<TransformSystem>();

    scroll_system_ = registry_.Get<ScrollSystem>();
    animation_system_ = registry_.Get<AnimationSystem>();
    dispatcher->Connect(this, [this](const ScrollOffsetChanged& event) {
      OnOffsetChanged(event);
    });
    entity_factory_->Initialize();
    scroll_offset_changed_count_ = 0;
  }

  void OnOffsetChanged(const ScrollOffsetChanged& event) {
    ++scroll_offset_changed_count_;
    new_offset_ = event.new_offset;

    if (scroll_offset_changed_counts_.count(event.target)) {
      ++scroll_offset_changed_counts_[event.target];
    } else {
      scroll_offset_changed_counts_[event.target] = 1;
    }
  }

  void CheckNewOffset() {
    EXPECT_NEAR(new_offset_.x, expect_offset_.x, kEpsilon);
    EXPECT_NEAR(new_offset_.y, expect_offset_.y, kEpsilon);
  }

  void SetExpectedOffset(const mathfu::vec2& offset) {
    expect_offset_ = offset;
  }

  Entity CreateChild(const mathfu::vec3& pos, const mathfu::vec3& extents) {
    auto* entity_factory = registry_.Get<EntityFactory>();
    auto* transform_system = registry_.Get<TransformSystem>();
    Sqt sqt;
    sqt.translation = pos;

    const Entity child = entity_factory->Create();
    transform_system->Create(child, sqt);
    transform_system->AddChild(scroll_view_, child);
    transform_system->SetAabb(child, Aabb(-extents, extents));
    return child;
  }

  Entity CreateChild(const mathfu::vec3& pos) {
    return CreateChild(pos, mathfu::kZeros3f);
  }

  void CreateScrollView(const mathfu::vec2& content_size,
                        const mathfu::vec2& touch_speed = mathfu::kOnes2f,
                        int active_priority = ScrollSystem::kHoverPriority) {
    scroll_view_ =
        CreateScrollViewEntity(content_size, touch_speed, active_priority);
  }

  Entity CreateScrollViewEntity(const mathfu::vec2& content_size,
                                const mathfu::vec2& touch_speed,
                                int active_priority) {
    TransformDefT transform;
    ScrollDefT scroll;
    scroll.content_bounds.min = mathfu::vec3(0.f, 0.f, 0.f);
    scroll.content_bounds.max = mathfu::vec3(content_size, 0.f);
    scroll.touch_sensitivity = touch_speed;
    scroll.active_priority = active_priority;

    Blueprint blueprint;
    blueprint.Write(&transform);
    blueprint.Write(&scroll);
    return entity_factory_->Create(&blueprint);
  }

  void GenerateTouchMovement(mathfu::vec2 amount) {
    // Fake a touch movement
    input_manager_->UpdateTouch(InputManager::kController,
                                mathfu::vec2(0.0f, 0.0f), true);
    input_manager_->AdvanceFrame(oneMillisecond);
    // Note that the InputManager uses the system clock to compute event times
    // and thus touch velocity, so we might want to either change InputManager
    // or sleep here so that some meaningful time passes between the two
    // UpdateTouch calls.
    input_manager_->UpdateTouch(InputManager::kController, amount, true);
    input_manager_->AdvanceFrame(oneMillisecond);
  }

  void GenerateTouchRelease() {
    // Fake a touch movement
    input_manager_->UpdateTouch(InputManager::kController,
                                mathfu::vec2(0.0f, 0.0f), false);
    input_manager_->AdvanceFrame(oneMillisecond);
  }

  void GenerateHoverEvent() { GenerateHoverEventOnEntity(scroll_view_); }

  void GenerateHoverEventOnEntity(Entity entity) {
    // Fake a hover event
    auto dispatcher = registry_.Get<Dispatcher>();
    StartHoverEvent event;
    event.target = entity;
    dispatcher->Send(event);
  }

  void GenerateStopHoverEventOnEntity(Entity entity) {
    // Fake a hover event
    auto dispatcher = registry_.Get<Dispatcher>();
    StopHoverEvent event;
    event.target = entity;
    dispatcher->Send(event);
  }

  Registry registry_;
  InputManager* input_manager_;
  ScrollSystem* scroll_system_;
  AnimationSystem* animation_system_;
  EntityFactory* entity_factory_;
  Entity scroll_view_;
  Entity left_child_;
  Entity right_child_;
  mathfu::vec2 expect_offset_;
  mathfu::vec2 new_offset_;
  int scroll_offset_changed_count_;
  std::unordered_map<Entity, int> scroll_offset_changed_counts_;
};

TEST_F(ScrollTest, Offset) {
  const mathfu::vec2 content_size(2, 2);
  CreateScrollView(content_size);

  SetExpectedOffset(mathfu::vec2(.1f, .1f));
  scroll_system_->SetViewOffset(scroll_view_, mathfu::vec2(.1f, .1f));
  scroll_system_->AdvanceFrame(oneMillisecond);
  animation_system_->AdvanceFrame(oneMillisecond);
  EXPECT_EQ(scroll_offset_changed_count_, 1);
  this->CheckNewOffset();

  SetExpectedOffset(mathfu::vec2(2.0f, 2.0f));
  scroll_system_->SetViewOffset(scroll_view_, mathfu::vec2(10, 9));
  scroll_system_->AdvanceFrame(oneMillisecond);
  animation_system_->AdvanceFrame(oneMillisecond);
  EXPECT_EQ(scroll_offset_changed_count_, 2);
  this->CheckNewOffset();

  SetExpectedOffset(mathfu::vec2(0.0f, 0.0f));
  scroll_system_->SetViewOffset(scroll_view_, mathfu::vec2(-34, -5));
  scroll_system_->AdvanceFrame(oneMillisecond);
  animation_system_->AdvanceFrame(oneMillisecond);
  EXPECT_EQ(scroll_offset_changed_count_, 3);
  this->CheckNewOffset();
}

TEST_F(ScrollTest, StopAndRestartSameEntity) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(2, 2);

  this->CreateScrollView(content_size, touch_speed, 0);
  this->GenerateTouchMovement(mathfu::vec2(1, 0));
  this->GenerateHoverEvent();

  // Allow the scroll system to react
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 1);

  // Stop scrolling on this frame
  scroll_system_->Deactivate(scroll_view_);
  scroll_system_->Activate(scroll_view_);

  // Wait for EndTouch momentum_time animation
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 2);

  // Confirm that scrolling has stopped.
  this->GenerateTouchMovement(mathfu::vec2(1, 0));
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 2);

  // Generate another touch with hover and expect scrolling to restart
  this->GenerateTouchMovement(mathfu::vec2(1, 0));
  this->GenerateHoverEvent();
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 3);
}

TEST_F(ScrollTest, DeactivateAndActivate) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(2, 2);

  this->CreateScrollView(content_size, touch_speed, 0);
  this->GenerateTouchMovement(mathfu::vec2(1, 0));
  this->GenerateHoverEvent();

  // Allow the scroll system to react
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 1);

  // Deactivate scrolling and prevent restarting.
  scroll_system_->Deactivate(scroll_view_);

  // Wait for EndTouch momentum_time animation
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 2);

  // Confirm that scrolling has been disabled even while hovering.
  this->GenerateTouchMovement(mathfu::vec2(1, 0));
  this->GenerateHoverEvent();
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 2);

  // Start scrolling again
  scroll_system_->Activate(scroll_view_);
  this->GenerateTouchMovement(mathfu::vec2(1, 0));
  // A current limitation is that you must re-hover again after activating for
  // kHoverPriority. We do not cache the currently hovered but deactivated
  // scroll views, only hovered activated ones.
  this->GenerateHoverEvent();
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 3);
}

TEST_F(ScrollTest, MoveTouchAndHover) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(2, 2);

  this->CreateScrollView(content_size, touch_speed, 0);

  this->GenerateTouchMovement(mathfu::vec2(1, 0));

  this->GenerateHoverEvent();

  // Allow the scroll system to react
  scroll_system_->AdvanceFrame(oneMicrosecond);
  animation_system_->AdvanceFrame(oneMicrosecond);

  EXPECT_EQ(scroll_offset_changed_count_, 1);
}

TEST_F(ScrollTest, MoveTouchAndNoHover) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(2, 2);

  this->CreateScrollView(content_size, touch_speed, 0);

  this->GenerateTouchMovement(mathfu::vec2(1, 0));

  // Allow the scroll system to react
  scroll_system_->AdvanceFrame(oneMicrosecond);
  animation_system_->AdvanceFrame(oneMicrosecond);

  EXPECT_EQ(scroll_offset_changed_count_, 0);
}

TEST_F(ScrollTest, MoveTouchAndNoHoverWithForceActive) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(2, 2);

  this->CreateScrollView(content_size, touch_speed, 1);

  this->GenerateTouchMovement(mathfu::vec2(1, 0));

  // Allow the scroll system to react
  scroll_system_->AdvanceFrame(oneMicrosecond);
  animation_system_->AdvanceFrame(oneMicrosecond);

  EXPECT_EQ(scroll_offset_changed_count_, 1);
}

TEST_F(ScrollTest, MoveTouchAndHoverWithForceActive) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(2, 2);

  this->CreateScrollView(content_size, touch_speed, 1);

  this->GenerateTouchMovement(mathfu::vec2(1, 0));
  this->GenerateHoverEvent();

  // Allow the scroll system to react
  scroll_system_->AdvanceFrame(oneMicrosecond);
  animation_system_->AdvanceFrame(oneMicrosecond);

  EXPECT_EQ(scroll_offset_changed_count_, 1);
}

TEST_F(ScrollTest, SetPriority) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(2, 2);

  CreateScrollView(content_size, touch_speed, ScrollSystem::kHoverPriority);

  // View shouldn't move here, since it's not hovered.
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 0);

  // Set an active priority, and expect the view to move.
  scroll_system_->SetPriority(scroll_view_, 1);
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 1);
}

TEST_F(ScrollTest, PriorityAndDeactivate) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(2, 2);

  CreateScrollView(content_size, touch_speed, 1);

  // View should scroll due to priority.
  GenerateTouchMovement(mathfu::vec2(1, 0));
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 1);

  // Deactivate scrolling
  scroll_system_->Deactivate(scroll_view_);

  // Wait for EndTouch momentum_time animation
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 2);

  // Confirm that scrolling has stopped.
  this->GenerateTouchMovement(mathfu::vec2(1, 0));
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 2);

  // Start scrolling again, priority requires no hover.
  scroll_system_->Activate(scroll_view_);
  this->GenerateTouchMovement(mathfu::vec2(1, 0));
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_count_, 3);
}

TEST_F(ScrollTest, HoverAndPriorityAndReactivate) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(2, 2);

  Entity scroll_view_1 = CreateScrollViewEntity(content_size, touch_speed, 1);
  Entity scroll_view_2 = CreateScrollViewEntity(content_size, touch_speed, 3);

  // Higher priority view should scroll.
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 0);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 1);

  // Hover over the lower priority view and it should take over scrolling.
  // scroll_view_2 will have one frame of EndTouch animation.
  GenerateHoverEventOnEntity(scroll_view_1);
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 2);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 2);

  // Reactivate the currently hovered entity. This should not change anything
  // since it is already activated and should remain scrolling. scroll_view_2
  // should be untouched.
  scroll_system_->Activate(scroll_view_1);
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 4);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 2);

  // Change the priority of the currently hovered entity to 2, This should also
  // not change anything since it is still hovered. scroll_view_2 should be
  // untouched.
  scroll_system_->SetPriority(scroll_view_1, 2);
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 6);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 2);
}

TEST_F(ScrollTest, GetActiveInputView) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(-2, 2);  // Touch delta x is inverted.

  Entity scroll_view_1 = CreateScrollViewEntity(content_size, touch_speed, 1);
  Entity scroll_view_2 = CreateScrollViewEntity(content_size, touch_speed, 2);
  Entity scroll_view_3 = CreateScrollViewEntity(content_size, touch_speed, 3);

  // scroll_view_3 should move here since it has highest priority and no one is
  // hovered.
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 0);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 0);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_3], 1);

  // Hover over scroll_view_1, and it should now move for both frames.
  // scroll_view_3 will have one frame of EndTouch animation.
  GenerateHoverEventOnEntity(scroll_view_1);
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 2);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 0);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_3], 2);

  // Hover over scroll_view_2, and it should now move for both frames.
  // scroll_view_1 will have one frame of EndTouch animation.
  GenerateHoverEventOnEntity(scroll_view_2);
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 3);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 2);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_3], 2);

  // Stop hovering, and scroll_view_3 should move again for both frames.
  // scroll_view_2 will have one frame of EndTouch animation.
  GenerateStopHoverEventOnEntity(scroll_view_2);
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 3);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 3);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_3], 4);
}

TEST_F(ScrollTest, HoverHighestPriority) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(-2, 2);  // Touch delta x is inverted.

  Entity scroll_view_1 = CreateScrollViewEntity(content_size, touch_speed, 1);
  Entity scroll_view_2 = CreateScrollViewEntity(content_size, touch_speed, 2);
  Entity scroll_view_3 = CreateScrollViewEntity(content_size, touch_speed, 3);

  // scroll_view_3 should move here since it has highest priority and no one is
  // hovered.
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 0);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 0);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_3], 1);

  // Hover over scroll_view_3, and it should still move because it is hovered.
  GenerateHoverEventOnEntity(scroll_view_3);
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 0);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 0);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_3], 3);

  // Stop hovering, and scroll_view_3 should again still move since no one is
  // hovered and it is highest priority.
  GenerateStopHoverEventOnEntity(scroll_view_3);
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 0);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 0);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_3], 5);
}

TEST_F(ScrollTest, EndTouch) {
  const mathfu::vec2 content_size(3, 1);
  const mathfu::vec2 touch_speed(-2, 2);  // Touch delta x is inverted.

  Entity scroll_view_1 = CreateScrollViewEntity(content_size, touch_speed, 1);
  Entity scroll_view_2 = CreateScrollViewEntity(content_size, touch_speed, 2);

  // Hover over scroll_view_1, and it should now move for both frames.
  // scroll_view_2 was active so it gets one frame of EndTouch animation.
  GenerateHoverEventOnEntity(scroll_view_1);
  GenerateTouchMovement(mathfu::vec2(1, 0));

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 2);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 1);

  // Stop hovering, and scroll_view_1 should have an EndTouch animation.
  GenerateTouchRelease();

  scroll_system_->AdvanceFrame(oneSecond);
  animation_system_->AdvanceFrame(oneSecond);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_1], 3);
  EXPECT_EQ(scroll_offset_changed_counts_[scroll_view_2], 1);
}

#if 0
TEST(ScrollSystem, SnapPositionToGrid) {
  const mathfu::vec2 grid_interval(3.0f, 3.0f);
  const mathfu::vec2 content_size(10.0f, 10.0f);

  // First test that points outside of the grid are snapped into the grid.
  mathfu::vec2 pos = ScrollSystem::SnapPositionToGrid(
      -2.0f * grid_interval, grid_interval, content_size);
  EXPECT_NEAR(pos.x, 0.0f, kEpsilon);
  EXPECT_NEAR(pos.y, 0.0f, kEpsilon);

  pos = ScrollSystem::SnapPositionToGrid(content_size + grid_interval,
                                               grid_interval, content_size);
  EXPECT_LE(pos.x, content_size.x);
  EXPECT_LE(pos.y, content_size.y);
  EXPECT_NEAR(fmodf(pos.x, grid_interval.x), 0.0f, kEpsilon);
  EXPECT_NEAR(fmodf(pos.y, grid_interval.y), 0.0f, kEpsilon);

  // Now test some points inside the grid and make sure they round correctly.
  pos = ScrollSystem::SnapPositionToGrid(.49f * grid_interval,
                                               grid_interval, content_size);
  EXPECT_NEAR(pos.x, 0.0f, kEpsilon);
  EXPECT_NEAR(pos.y, 0.0f, kEpsilon);

  pos = ScrollSystem::SnapPositionToGrid(.51f * grid_interval,
                                               grid_interval, content_size);
  EXPECT_NEAR(pos.x, grid_interval.x, kEpsilon);
  EXPECT_NEAR(pos.y, grid_interval.y, kEpsilon);
}
#endif

}  // namespace
}  // namespace lull
