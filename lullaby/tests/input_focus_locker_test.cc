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

#include "lullaby/modules/reticle/input_focus_locker.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/registry.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

const float kEpsilon = 0.001f;
const Clock::duration kDeltaTime = std::chrono::milliseconds(17);
const Clock::duration kLongPressTime = std::chrono::milliseconds(500);

class InputFocusLockerTest : public testing::Test {
 public:
  void SetUp() override {
    registry_.Create<InputFocusLocker>(&registry_);

    auto* entity_factory = registry_.Create<EntityFactory>(&registry_);
    entity_factory->CreateSystem<TransformSystem>();

    entity_factory->Initialize();
  }

 protected:
  Registry registry_;
};

TEST_F(InputFocusLockerTest, Unlocked) {
  auto* input_focus_locker = registry_.Get<InputFocusLocker>();
  InputFocus state;
  state.device = InputManager::kController;

  input_focus_locker->UpdateInputFocus(&state);

  EXPECT_EQ(state.target, kNullEntity);
  EXPECT_NEAR(state.cursor_position.x, 0.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.y, 0.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.z, 0.0f, kEpsilon);
}

TEST_F(InputFocusLockerTest, Locked) {
  Blueprint blueprint;

  TransformDefT transform;
  blueprint.Write(&transform);

  auto* entity_factory = registry_.Get<EntityFactory>();
  auto* input_focus_locker = registry_.Get<InputFocusLocker>();
  const Entity entity = entity_factory->Create(&blueprint);

  EXPECT_NE(entity, kNullEntity);

  InputFocus state;
  state.device = InputManager::kController;

  input_focus_locker->LockOn(InputManager::kController, entity,
                             mathfu::vec3(0.0f, 1.0f, 2.0f));

  bool locked =
      input_focus_locker->UpdateInputFocus(&state);

  EXPECT_TRUE(locked);
  EXPECT_EQ(state.target, entity);
  EXPECT_NEAR(state.cursor_position.x, 0.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.y, 1.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.z, 2.0f, kEpsilon);
}
TEST_F(InputFocusLockerTest, Moving) {
  Blueprint blueprint;

  TransformDefT transform;
  blueprint.Write(&transform);

  auto* entity_factory = registry_.Get<EntityFactory>();
  auto* input_focus_locker = registry_.Get<InputFocusLocker>();
  auto* transform_system = registry_.Get<TransformSystem>();
  const Entity entity = entity_factory->Create(&blueprint);

  EXPECT_NE(entity, kNullEntity);

  InputFocus state;
  state.device = InputManager::kController;

  transform_system->SetLocalTranslation(entity, mathfu::kOnes3f);
  input_focus_locker->LockOn(InputManager::kController, entity,
                             mathfu::vec3(0.0f, 1.0f, 2.0f));
  bool locked =
      input_focus_locker->UpdateInputFocus(&state);

  EXPECT_TRUE(locked);
  EXPECT_EQ(state.target, entity);
  EXPECT_NEAR(state.cursor_position.x, 1.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.y, 2.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.z, 3.0f, kEpsilon);

  transform_system->SetLocalTranslation(entity, -1.0f * mathfu::kOnes3f);
  locked =
      input_focus_locker->UpdateInputFocus(&state);

  EXPECT_TRUE(locked);
  EXPECT_EQ(state.target, entity);
  EXPECT_NEAR(state.cursor_position.x, -1.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.y, 0.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.z, 1.0f, kEpsilon);
}

TEST_F(InputFocusLockerTest, LockThenUnlock) {
  Blueprint blueprint;

  TransformDefT transform;
  blueprint.Write(&transform);

  auto* entity_factory = registry_.Get<EntityFactory>();
  auto* input_focus_locker = registry_.Get<InputFocusLocker>();
  auto* transform_system = registry_.Get<TransformSystem>();
  const Entity entity = entity_factory->Create(&blueprint);

  EXPECT_NE(entity, kNullEntity);

  InputFocus state, state2;
  state.device = InputManager::kController;
  state2.device = InputManager::kController;

  input_focus_locker->LockOn(InputManager::kController, entity,
                             mathfu::vec3(0.0f, 1.0f, 2.0f));

  bool locked =
      input_focus_locker->UpdateInputFocus(&state);

  EXPECT_TRUE(locked);
  EXPECT_EQ(state.target, entity);
  EXPECT_NEAR(state.cursor_position.x, 0.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.y, 1.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.z, 2.0f, kEpsilon);

  transform_system->SetLocalTranslation(entity, mathfu::kOnes3f);

  input_focus_locker->LockOn(InputManager::kController, kNullEntity,
                             mathfu::vec3(2.0f, 1.0f, 0.0f));

  locked =
      input_focus_locker->UpdateInputFocus(&state2);

  EXPECT_FALSE(locked);
  EXPECT_EQ(state2.target, kNullEntity);
  EXPECT_NEAR(state2.cursor_position.x, 0.0f, kEpsilon);
  EXPECT_NEAR(state2.cursor_position.y, 0.0f, kEpsilon);
  EXPECT_NEAR(state2.cursor_position.z, 0.0f, kEpsilon);
}

TEST_F(InputFocusLockerTest, MultipleDevices) {
  Blueprint blueprint;

  TransformDefT transform;
  blueprint.Write(&transform);

  auto* entity_factory = registry_.Get<EntityFactory>();
  auto* input_focus_locker = registry_.Get<InputFocusLocker>();
  const Entity entity = entity_factory->Create(&blueprint);

  EXPECT_NE(entity, kNullEntity);

  InputFocus state, state2;
  state.device = InputManager::kController;
  state2.device = InputManager::kHmd;

  input_focus_locker->LockOn(InputManager::kController, entity,
                             mathfu::vec3(0.0f, 1.0f, 2.0f));

  bool locked1 =
      input_focus_locker->UpdateInputFocus(&state);
  bool locked2 =
      input_focus_locker->UpdateInputFocus(&state2);

  EXPECT_TRUE(locked1);
  EXPECT_EQ(state.target, entity);
  EXPECT_NEAR(state.cursor_position.x, 0.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.y, 1.0f, kEpsilon);
  EXPECT_NEAR(state.cursor_position.z, 2.0f, kEpsilon);

  EXPECT_FALSE(locked2);
  EXPECT_EQ(state2.target, kNullEntity);
  EXPECT_NEAR(state2.cursor_position.x, 0.0f, kEpsilon);
  EXPECT_NEAR(state2.cursor_position.y, 0.0f, kEpsilon);
  EXPECT_NEAR(state2.cursor_position.z, 0.0f, kEpsilon);
}

}  // namespace
}  // namespace lull
