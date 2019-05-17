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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/render/detail/render_pool_map.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::IsNull;
using ::testing::NotNull;
using detail::RenderPoolMap;

struct MyRenderComponent : Component {
  explicit MyRenderComponent(Entity e) : Component(e) {}

  std::string name;
};

TEST(RenderPoolMapTest, ReturnsExistingPool) {
  Registry registry;
  RenderPoolMap<MyRenderComponent> render_pool_map(&registry);
  auto* pool = &render_pool_map.GetPool(RenderPass_Main);
  EXPECT_THAT(&render_pool_map.GetPool(RenderPass_Main), Eq(pool));

  // Make sure custom render passes are not created by default.
  EXPECT_THAT(render_pool_map.GetExistingPool(RenderPass_AppIdStart),
              Eq(nullptr));
}

TEST(RenderPoolMapTest, ReturnsComponentInPool) {
  Registry registry;
  RenderPoolMap<MyRenderComponent> render_pool_map(&registry);
  EXPECT_THAT(render_pool_map.GetComponent(1), IsNull());
  EXPECT_THAT(render_pool_map.GetComponent(2), IsNull());
  EXPECT_THAT(render_pool_map.GetComponent(3), IsNull());

  auto& component1 = render_pool_map.EmplaceComponent(1, RenderPass_Main);
  auto& component2 = render_pool_map.EmplaceComponent(2, RenderPass_Main);
  auto& component3 = render_pool_map.EmplaceComponent(3, RenderPass_Opaque);

  EXPECT_THAT(render_pool_map.GetComponent(1), Eq(&component1));
  EXPECT_THAT(render_pool_map.GetComponent(2), Eq(&component2));
  EXPECT_THAT(render_pool_map.GetComponent(3), Eq(&component3));

  auto& pool1 = render_pool_map.GetPool(RenderPass_Main);
  EXPECT_THAT(pool1.GetComponent(1), Eq(&component1));
  EXPECT_THAT(pool1.GetComponent(2), Eq(&component2));

  auto& pool2 = render_pool_map.GetPool(RenderPass_Opaque);
  EXPECT_THAT(pool2.GetComponent(3), Eq(&component3));
}

TEST(RenderPoolMapTest, DestroysComponentInPool) {
  Registry registry;
  RenderPoolMap<MyRenderComponent> render_pool_map(&registry);

  auto& pool = render_pool_map.GetPool(RenderPass_Main);
  auto& component = render_pool_map.EmplaceComponent(1, RenderPass_Main);
  EXPECT_THAT(render_pool_map.GetComponent(1), Eq(&component));
  EXPECT_THAT(pool.GetComponent(1), Eq(&component));

  render_pool_map.DestroyComponent(1);
  EXPECT_THAT(render_pool_map.GetComponent(1), IsNull());
  EXPECT_THAT(pool.GetComponent(1), IsNull());
}

TEST(RenderPoolMapTest, SwapsComponentToPool) {
  Registry registry;
  RenderPoolMap<MyRenderComponent> render_pool_map(&registry);

  auto& pool1 = render_pool_map.GetPool(RenderPass_Main);
  auto& pool2 = render_pool_map.GetPool(RenderPass_Opaque);
  auto& component = render_pool_map.EmplaceComponent(1, RenderPass_Main);
  component.name = "entity1_component";
  EXPECT_THAT(pool1.GetComponent(1), Eq(&component));
  EXPECT_THAT(pool2.GetComponent(1), IsNull());

  render_pool_map.MoveToPool(1, RenderPass_Opaque);
  EXPECT_THAT(pool1.GetComponent(1), IsNull());

  // MoveToPool destroys the original object so verify that the component data
  // has been swapped instead.
  auto* copied_component = pool2.GetComponent(1);
  ASSERT_THAT(copied_component, NotNull());
  EXPECT_THAT(copied_component->name, Eq("entity1_component"));
}

TEST(RenderPoolMapTest, IgnoresUnknownComponent) {
  Registry registry;
  RenderPoolMap<MyRenderComponent> render_pool_map(&registry);
  EXPECT_THAT(render_pool_map.GetComponent(1), IsNull());

  // These should be silently ignored.
  render_pool_map.DestroyComponent(1);
  render_pool_map.MoveToPool(1, RenderPass_Main);
}

}  // namespace
}  // namespace lull
