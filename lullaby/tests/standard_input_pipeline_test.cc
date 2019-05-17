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

#include "lullaby/modules/reticle/standard_input_pipeline.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "mathfu/constants.h"

namespace lull {
namespace {

void Connect3DoFController(InputManager* input) {
  DeviceProfile profile;
  profile.position_dof = DeviceProfile::kFakeDof;
  profile.rotation_dof = DeviceProfile::kRealDof;
  input->ConnectDevice(InputManager::kController, profile);
}

void Connect6DoFController(InputManager* input) {
  DeviceProfile profile;
  profile.position_dof = DeviceProfile::kRealDof;
  profile.rotation_dof = DeviceProfile::kRealDof;
  input->ConnectDevice(InputManager::kController, profile);
}

void Connect3DoFHmd(InputManager* input) {
  DeviceProfile profile;
  profile.position_dof = DeviceProfile::kFakeDof;
  profile.rotation_dof = DeviceProfile::kRealDof;
  input->ConnectDevice(InputManager::kHmd, profile);
}

void Connect6DoFHmd(InputManager* input) {
  DeviceProfile profile;
  profile.position_dof = DeviceProfile::kRealDof;
  profile.rotation_dof = DeviceProfile::kRealDof;
  input->ConnectDevice(InputManager::kHmd, profile);
}

class StandardInputPipelineTest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry_.reset(new Registry());

    auto* entity_factory = registry_->Create<EntityFactory>(registry_.get());
    registry_->Create<InputManager>();
    registry_->Create<Dispatcher>();
    entity_factory->CreateSystem<TransformSystem>();
    entity_factory->CreateSystem<DispatcherSystem>();
    entity_factory->CreateSystem<RenderSystem>();
    registry_->Create<InputProcessor>(registry_.get());
    registry_->Create<StandardInputPipeline>(registry_.get());
    entity_factory->Initialize();
  }

  std::unique_ptr<Registry> registry_;
};

TEST_F(StandardInputPipelineTest,
       MaybeMakeRayComeFromHmdDoesNothingFor6DoFControllerByDefault) {
  static const mathfu::vec3 kMockCollisionRayOrigin = mathfu::kOnes3f * 2.f;
  static const mathfu::vec3 kMockCollisionRayDirection = mathfu::kAxisY3f;

  auto* input = registry_->Get<InputManager>();
  Connect3DoFHmd(input);
  Connect6DoFController(input);
  input->UpdatePosition(InputManager::kHmd, mathfu::kOnes3f * -2.f);
  input->UpdateRotation(InputManager::kHmd, mathfu::kQuatIdentityf);
  input->AdvanceFrame(Clock::duration::zero());

  InputFocus focus;
  focus.device = InputManager::kController;
  focus.collision_ray.origin = kMockCollisionRayOrigin;
  focus.collision_ray.direction = kMockCollisionRayDirection;

  auto* input_pipeline = registry_->Get<StandardInputPipeline>();
  input_pipeline->MaybeMakeRayComeFromHmd(&focus);

  EXPECT_THAT(
      focus.collision_ray.origin,
      testing::NearMathfuVec3(kMockCollisionRayOrigin, kDefaultEpsilon));
  EXPECT_THAT(
      focus.collision_ray.direction,
      testing::NearMathfuVec3(kMockCollisionRayDirection, kDefaultEpsilon));
}

TEST_F(StandardInputPipelineTest,
       MaybeMakeRayComeFromHmdUsesHmdFor3DoFControllerByDefault) {
  static const mathfu::vec3 kMockHmdPosition = mathfu::kOnes3f * 2.f;
  static const mathfu::vec3 kExpectedRayDirection = mathfu::kAxisY3f;

  auto* input = registry_->Get<InputManager>();
  Connect6DoFHmd(input);
  Connect3DoFController(input);
  input->UpdatePosition(InputManager::kHmd, kMockHmdPosition);
  input->UpdateRotation(InputManager::kHmd,
                        mathfu::quat::FromEulerAngles(kExpectedRayDirection));
  input->AdvanceFrame(Clock::duration::zero());

  InputFocus focus;
  focus.device = InputManager::kController;
  focus.cursor_position = kMockHmdPosition + mathfu::kAxisY3f;
  focus.collision_ray.origin = mathfu::kOnes3f * -2.f;
  focus.collision_ray.direction = mathfu::kAxisX3f;

  auto* input_pipeline = registry_->Get<StandardInputPipeline>();
  input_pipeline->MaybeMakeRayComeFromHmd(&focus);

  EXPECT_THAT(focus.collision_ray.origin,
              testing::NearMathfuVec3(kMockHmdPosition, kDefaultEpsilon));
  EXPECT_THAT(focus.collision_ray.direction,
              testing::NearMathfuVec3(kExpectedRayDirection, kDefaultEpsilon));
}

TEST_F(
    StandardInputPipelineTest,
    MaybeMakeRayComeFromHmdDoesNothingFor3DoFControllerWhenControllerOriginForced) {
  static const mathfu::vec3 kMockCollisionRayOrigin = mathfu::kOnes3f * 2.f;
  static const mathfu::vec3 kMockCollisionRayDirection = mathfu::kAxisY3f;

  auto* input = registry_->Get<InputManager>();
  Connect3DoFHmd(input);
  Connect3DoFController(input);
  input->UpdatePosition(InputManager::kHmd, mathfu::kOnes3f * -2.f);
  input->UpdateRotation(InputManager::kHmd, mathfu::kQuatIdentityf);
  input->AdvanceFrame(Clock::duration::zero());

  InputFocus focus;
  focus.device = InputManager::kController;
  focus.collision_ray.origin = kMockCollisionRayOrigin;
  focus.collision_ray.direction = kMockCollisionRayDirection;

  auto* input_pipeline = registry_->Get<StandardInputPipeline>();
  input_pipeline->SetForceRayFromOriginMode(
      StandardInputPipeline::kAlwaysFromController);
  input_pipeline->MaybeMakeRayComeFromHmd(&focus);

  EXPECT_THAT(
      focus.collision_ray.origin,
      testing::NearMathfuVec3(kMockCollisionRayOrigin, kDefaultEpsilon));
  EXPECT_THAT(
      focus.collision_ray.direction,
      testing::NearMathfuVec3(kMockCollisionRayDirection, kDefaultEpsilon));
}

TEST_F(StandardInputPipelineTest,
       MaybeMakeRayComeFromHmdUsesHmdFor6DoFControllerWhenHmdOriginForced) {
  static const mathfu::vec3 kMockHmdPosition = mathfu::kOnes3f * 2.f;
  static const mathfu::vec3 kExpectedRayDirection = mathfu::kAxisY3f;

  auto* input = registry_->Get<InputManager>();
  Connect6DoFHmd(input);
  Connect6DoFController(input);
  input->UpdatePosition(InputManager::kHmd, kMockHmdPosition);
  input->UpdateRotation(InputManager::kHmd,
                        mathfu::quat::FromEulerAngles(kExpectedRayDirection));
  input->AdvanceFrame(Clock::duration::zero());

  InputFocus focus;
  focus.device = InputManager::kController;
  focus.cursor_position = kMockHmdPosition + mathfu::kAxisY3f;
  focus.collision_ray.origin = mathfu::kOnes3f * -2.f;
  focus.collision_ray.direction = mathfu::kAxisX3f;

  auto* input_pipeline = registry_->Get<StandardInputPipeline>();
  input_pipeline->SetForceRayFromOriginMode(
      StandardInputPipeline::kAlwaysFromHmd);
  input_pipeline->MaybeMakeRayComeFromHmd(&focus);

  EXPECT_THAT(focus.collision_ray.origin,
              testing::NearMathfuVec3(kMockHmdPosition, kDefaultEpsilon));
  EXPECT_THAT(focus.collision_ray.direction,
              testing::NearMathfuVec3(kExpectedRayDirection, kDefaultEpsilon));
}

}  // namespace
}  // namespace lull
