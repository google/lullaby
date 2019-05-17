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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/entity.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/render/shader.h"
#include "lullaby/systems/render/testing/mock_render_system_impl.h"
#include "lullaby/systems/render/texture.h"
#include "lullaby/util/registry.h"

namespace lull {

class Shader {};
class Texture {};

namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::Return;
using ::testing::_;

TEST(MockRenderSystemTest, WithNoExpectations) {
  Registry registry;
  auto& entity_factory = *registry.Create<EntityFactory>(&registry);
  auto& render_system = *entity_factory.CreateSystem<RenderSystem>();

  EXPECT_CALL(*render_system.GetImpl(), LoadShader(_))
      .WillRepeatedly(Return(ShaderPtr()));
  EXPECT_CALL(*render_system.GetImpl(), LoadTexture(_, _))
      .WillRepeatedly(Return(TexturePtr()));

  EXPECT_THAT(render_system.LoadShader("random_shader_path").get(), IsNull());
  EXPECT_THAT(render_system.LoadTexture("random_texture_path").get(), IsNull());

  render_system.ProcessTasks();
  render_system.Render(nullptr /* views */, 0 /* num_views */);
}

TEST(MockRenderSystemTest, WithExpectations) {
  Registry registry;
  auto& entity_factory = *registry.Create<EntityFactory>(&registry);
  auto& render_system = *entity_factory.CreateSystem<RenderSystem>();

  EXPECT_CALL(*render_system.GetImpl(), LoadShader(_))
      .WillRepeatedly(Return(ShaderPtr()));

  ShaderPtr shader(new Shader);
  EXPECT_CALL(*render_system.GetImpl(), LoadShader("special_path"))
      .WillRepeatedly(Return(shader));

  EXPECT_THAT(render_system.LoadShader("random_path").get(), IsNull());
  EXPECT_THAT(render_system.LoadShader("special_path").get(), Eq(shader.get()));
}

}  // namespace
}  // namespace lull
