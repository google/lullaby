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

#include "lullaby/systems/nine_patch/nine_patch_system.h"

#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/testing/mock_render_system_impl.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/registry.h"
#include "lullaby/generated/nine_patch_def_generated.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

using testing::NearMathfuVec3;
static const float kEpsilon = 0.001f;

class NinePatchSystemTest : public ::testing::Test {
 public:
  void SetUp() override {
    registry_.reset(new Registry());

    registry_->Register(std::unique_ptr<Dispatcher>(new Dispatcher()));
    entity_factory_ = registry_->Create<EntityFactory>(registry_.get());
    transform_system_ = entity_factory_->CreateSystem<TransformSystem>();
    render_system_ = entity_factory_->CreateSystem<RenderSystem>()->GetImpl();
    nine_patch_system_ = entity_factory_->CreateSystem<NinePatchSystem>();

    entity_factory_->Initialize();
  }

 protected:
  std::unique_ptr<Registry> registry_;
  EntityFactory* entity_factory_ = nullptr;
  TransformSystem* transform_system_ = nullptr;
  MockRenderSystemImpl* render_system_ = nullptr;
  NinePatchSystem* nine_patch_system_ = nullptr;
};

TEST_F(NinePatchSystemTest, Create) {
  TransformDefT transform;
  NinePatchDefT nine_patch;
  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&nine_patch);

  Entity entity = entity_factory_->Create(&blueprint);

  EXPECT_NE(entity, kNullEntity);
  EXPECT_TRUE(nine_patch_system_->GetSize(entity));
}

TEST_F(NinePatchSystemTest, Aabb) {
  const float kWidth = 6.0f;
  const float kHeight = 4.0f;

  Blueprint blueprint;
  TransformDefT transform;
  blueprint.Write(&transform);

  NinePatchDefT nine_patch;
  nine_patch.size = mathfu::vec2(kWidth, kHeight);
  blueprint.Write(&nine_patch);

  const Entity nine_patch_entity = entity_factory_->Create(&blueprint);

  const lull::Aabb* aabb = transform_system_->GetAabb(nine_patch_entity);

  const mathfu::vec3 half_dims(kWidth * 0.5f, kHeight * 0.5f, 0.0f);
  EXPECT_THAT(aabb->min, NearMathfuVec3(-half_dims, kEpsilon));
  EXPECT_THAT(aabb->max, NearMathfuVec3(half_dims, kEpsilon));
}

}  // namespace
}  // namespace lull
