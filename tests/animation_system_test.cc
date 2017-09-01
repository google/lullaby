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

#include "lullaby/systems/animation/animation_system.h"
#include "gtest/gtest.h"
#include "lullaby/modules/animation_channels/transform_channels.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/tests/portable_test_macros.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

class AnimationSystemTest : public testing::Test {
 public:
  void SetUp() override {
    registry_.reset(new Registry());

    auto* entity_factory = registry_->Create<EntityFactory>(registry_.get());
    entity_factory->CreateSystem<TransformSystem>();
    entity_factory->CreateSystem<AnimationSystem>();
    entity_factory->CreateSystem<RenderSystem>();
    entity_factory->Initialize();

    PositionChannel::Setup(registry_.get(), 32);
    ScaleChannel::Setup(registry_.get(), 32);
  }

 protected:
  std::unique_ptr<Registry> registry_;
};

TEST_F(AnimationSystemTest, Create) {
  Blueprint blueprint(512);
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(0.f, 0.f, 0.f);
    transform.rotation = mathfu::vec3(0.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(1.f, 1.f, 1.f);
    blueprint.Write(&transform);
  }

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity entity = entity_factory->Create(&blueprint);

  const mathfu::vec3 target_pos(1.f, 2.f, 3.f);
  const mathfu::vec3 target_scale(10.f, 20.f, 30.f);

  auto animation_system = registry_->Get<AnimationSystem>();
  animation_system->SetTarget(entity, PositionChannel::kChannelName,
                              &target_pos.x, 3, std::chrono::seconds(1));
  animation_system->SetTarget(entity, ScaleChannel::kChannelName,
                              &target_scale.x, 3, std::chrono::seconds(1));
  animation_system->AdvanceFrame(std::chrono::seconds(1));

  auto transform_system = registry_->Get<TransformSystem>();
  const Sqt* sqt = transform_system->GetSqt(entity);

  static const float kEpsilon = 0.001f;

  EXPECT_NE(sqt, nullptr);
  EXPECT_NEAR(sqt->translation.x, 1.f, kEpsilon);
  EXPECT_NEAR(sqt->translation.y, 2.f, kEpsilon);
  EXPECT_NEAR(sqt->translation.z, 3.f, kEpsilon);

  EXPECT_NEAR(sqt->scale.x, 10.f, kEpsilon);
  EXPECT_NEAR(sqt->scale.y, 20.f, kEpsilon);
  EXPECT_NEAR(sqt->scale.z, 30.f, kEpsilon);
}

TEST_F(AnimationSystemTest, CreateWithDelay) {
  Blueprint blueprint(512);
  {
    TransformDefT transform;
    transform.position = mathfu::vec3(0.f, 0.f, 0.f);
    transform.rotation = mathfu::vec3(0.f, 0.f, 0.f);
    transform.scale = mathfu::vec3(1.f, 1.f, 1.f);
    blueprint.Write(&transform);
  }

  auto* entity_factory = registry_->Get<EntityFactory>();
  const Entity entity = entity_factory->Create(&blueprint);

  const mathfu::vec3 target_pos(1.f, 2.f, 3.f);
  const mathfu::vec3 target_scale(10.f, 20.f, 30.f);

  auto animation_system = registry_->Get<AnimationSystem>();
  animation_system->SetTarget(entity, PositionChannel::kChannelName,
                              &target_pos.x, 3, std::chrono::seconds(1),
                              std::chrono::seconds(1));
  animation_system->SetTarget(entity, ScaleChannel::kChannelName,
                              &target_scale.x, 3, std::chrono::seconds(1),
                              std::chrono::seconds(1));
  const auto transform_system = registry_->Get<TransformSystem>();
  static const float kEpsilon = 0.001f;

  animation_system->AdvanceFrame(std::chrono::seconds(1));
  const Sqt* sqt = transform_system->GetSqt(entity);
  EXPECT_NE(sqt, nullptr);
  EXPECT_NEAR(sqt->translation.x, 0.f, kEpsilon);
  EXPECT_NEAR(sqt->translation.y, 0.f, kEpsilon);
  EXPECT_NEAR(sqt->translation.z, 0.f, kEpsilon);

  EXPECT_NEAR(sqt->scale.x, 1.f, kEpsilon);
  EXPECT_NEAR(sqt->scale.y, 1.f, kEpsilon);
  EXPECT_NEAR(sqt->scale.z, 1.f, kEpsilon);

  animation_system->AdvanceFrame(std::chrono::seconds(1));
  EXPECT_NE(sqt, nullptr);
  EXPECT_NEAR(sqt->translation.x, 1.f, kEpsilon);
  EXPECT_NEAR(sqt->translation.y, 2.f, kEpsilon);
  EXPECT_NEAR(sqt->translation.z, 3.f, kEpsilon);

  EXPECT_NEAR(sqt->scale.x, 10.f, kEpsilon);
  EXPECT_NEAR(sqt->scale.y, 20.f, kEpsilon);
  EXPECT_NEAR(sqt->scale.z, 30.f, kEpsilon);
}

TEST(AnimationSystemDeathTest, SplitListFilenameAndIndex) {
  std::string filename;
  int index = 0;
  PORT_EXPECT_DEBUG_DEATH(
      AnimationSystem::SplitListFilenameAndIndex(nullptr, &index), "");
  PORT_EXPECT_DEBUG_DEATH(
      AnimationSystem::SplitListFilenameAndIndex(&filename, nullptr), "");

  // Test that a valid filename/index pair is parsed.
  filename = "foo.motivelist:5";
  AnimationSystem::SplitListFilenameAndIndex(&filename, &index);
  EXPECT_EQ(filename.compare("foo.motivelist"), 0);
  EXPECT_EQ(index, 5);

  // Test that unrecognized filenames are untouched.
  filename = "test:foo.baz";
  index = -3;
  AnimationSystem::SplitListFilenameAndIndex(&filename, &index);
  EXPECT_EQ(filename.compare("test:foo.baz"), 0);
  EXPECT_EQ(index, -3);

  filename = "foo.baz:34";
  index = 1;
  AnimationSystem::SplitListFilenameAndIndex(&filename, &index);
  EXPECT_EQ(filename.compare("foo.baz:34"), 0);
  EXPECT_EQ(index, 1);
}

}  // namespace
}  // namespace lull
