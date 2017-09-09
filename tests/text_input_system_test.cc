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

#include <unordered_map>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/testing/mock_render_system_impl.h"
#include "lullaby/systems/text_input/text_input_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/text_input_def_generated.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

class TextInputSystemTest : public ::testing::Test {
 protected:
  TextInputSystemTest() {
    registry_.reset(new Registry());
    dispatcher_ = registry_->Create<Dispatcher>();

    entity_factory_ = registry_->Create<EntityFactory>(registry_.get());
    entity_factory_->CreateSystem<DispatcherSystem>();
    entity_factory_->CreateSystem<TransformSystem>();
    render_system_ = entity_factory_->CreateSystem<RenderSystem>();
    text_input_system_ = entity_factory_->CreateSystem<TextInputSystem>();
    entity_factory_->Initialize();

    mock_render_system_ = render_system_->GetImpl();
  }

  std::unique_ptr<Registry> registry_;
  Dispatcher* dispatcher_;
  TextInputSystem* text_input_system_;
  RenderSystem* render_system_;
  RenderSystemImpl* mock_render_system_;
  EntityFactory* entity_factory_;
};

TEST_F(TextInputSystemTest, SetAndGet) {
  Blueprint blueprint;

  TextInputDefT text_input_def;
  text_input_def.activate_immediately = true;
  text_input_def.deactivate_on_accept = false;
  text_input_def.hint = "Type something";
  text_input_def.hint_color = lull::Color4ub(1, 1, 1, 1);
  text_input_def.caret_entity = "";
  text_input_def.is_clipped = false;
  blueprint.Write(&text_input_def);

  auto* entity_factory = registry_->Get<EntityFactory>();
  Entity text_entity = entity_factory->Create(&blueprint);

  // Check initial index values are 0, 0
  size_t start, end;
  text_input_system_->GetComposingIndices(text_entity, &start, &end);
  EXPECT_EQ(start, 0UL);
  EXPECT_EQ(end, 0UL);

  std::string test_string = "This is a test";

  mathfu::vec4 dont_care(1, 1, 1, 1);
  EXPECT_CALL(*mock_render_system_, GetDefaultColor(::testing::_))
      .WillRepeatedly(::testing::ReturnRef(dont_care));
  text_input_system_->SetText(text_entity, test_string);

  // Set and get sane values
  text_input_system_->SetComposingIndices(text_entity, 3, 7);
  text_input_system_->GetComposingIndices(text_entity, &start, &end);
  EXPECT_EQ(start, 3UL);
  EXPECT_EQ(end, 7UL);

  // Set and get out of range values
  text_input_system_->SetComposingIndices(text_entity,
                                          test_string.size() + 1,
                                          test_string.size() + 1);
  text_input_system_->GetComposingIndices(text_entity, &start, &end);
  EXPECT_EQ(start, test_string.size());
  EXPECT_EQ(end, test_string.size());
}

}  // namespace
}  // namespace lull
