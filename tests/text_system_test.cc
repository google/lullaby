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

#include "lullaby/systems/text/text_system.h"

#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/testing/mock_render_system_impl.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/registry.h"
#include "lullaby/generated/text_def_generated.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

using ::testing::IsNull;

class TextSystemTest : public testing::Test {
 public:
  void SetUp() override {
    registry_.reset(new Registry());

    registry_->Register(std::unique_ptr<Dispatcher>(new Dispatcher()));
    entity_factory_ = registry_->Create<EntityFactory>(registry_.get());
    transform_system_ = entity_factory_->CreateSystem<TransformSystem>();
    render_system_ = entity_factory_->CreateSystem<RenderSystem>()->GetImpl();
    text_system_ = entity_factory_->CreateSystem<TextSystem>();

    entity_factory_->Initialize();
  }

 protected:
  std::unique_ptr<Registry> registry_;
  EntityFactory* entity_factory_ = nullptr;
  TransformSystem* transform_system_ = nullptr;
  MockRenderSystemImpl* render_system_ = nullptr;
  TextSystem* text_system_ = nullptr;
};

TEST_F(TextSystemTest, UnknownEntityHasNoData) {
  TransformDefT transform;
  Blueprint blueprint;
  blueprint.Write(&transform);

  const Entity entity = entity_factory_->Create(&blueprint);

  EXPECT_THAT(text_system_->GetText(entity), IsNull());
  EXPECT_THAT(text_system_->GetLinkTags(entity), IsNull());
  EXPECT_THAT(text_system_->GetCaretPositions(entity), IsNull());
}

// TODO(b/33947628) Write more tests.

}  // namespace
}  // namespace lull
