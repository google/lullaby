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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/systems/render/detail/render_pool.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {
namespace {

using ::testing::Eq;
using ::testing::Ne;
using ::testing::IsNull;
using ::testing::NotNull;
using detail::RenderPool;

constexpr size_t kInitialSize = 8;

struct TestComponent : Component {
  explicit TestComponent(Entity e) : Component(e) {}
};

TEST(RenderPoolTest, StartsEmpty) {
  Registry registry;
  registry.Create<TransformSystem>(&registry);
  RenderPool<TestComponent> pool(&registry, kInitialSize);

  EXPECT_THAT(pool.Size(), Eq(0U));
}

TEST(RenderPoolTest, EmplaceGetDestroy) {
  Registry registry;
  registry.Create<TransformSystem>(&registry);
  RenderPool<TestComponent> pool(&registry, kInitialSize);

  const Entity entity = 1;

  EXPECT_THAT(pool.GetComponent(entity), IsNull());

  pool.EmplaceComponent(entity);
  EXPECT_THAT(pool.Size(), Eq(1U));
  EXPECT_THAT(pool.GetComponent(entity), NotNull());

  pool.DestroyComponent(entity);
  EXPECT_THAT(pool.Size(), Eq(0U));
  EXPECT_THAT(pool.GetComponent(entity), IsNull());
}

TEST(RenderPoolTest, ForEach) {
  Registry registry;
  registry.Create<TransformSystem>(&registry);
  RenderPool<TestComponent> pool(&registry, kInitialSize);

  const uint32_t kNumEntities = 10;
  for (uint32_t e = 1; e <= kNumEntities; ++e) {
    pool.EmplaceComponent(e);
  }
  EXPECT_THAT(pool.Size(), Eq(kNumEntities));

  std::unordered_map<Entity, size_t> count;

  pool.ForEachComponent([&count](const TestComponent& component) {
    ++count[component.GetEntity()];
  });

  for (uint32_t i = 1; i <= kNumEntities; ++i) {
    EXPECT_THAT(count[i], Eq(1U));
  }
}

TEST(RenderPoolTest, GetSetCullMode) {
  Registry registry;
  registry.Create<TransformSystem>(&registry);
  RenderPool<TestComponent> pool(&registry, kInitialSize);

  EXPECT_THAT(pool.GetCullMode(), Eq(RenderCullMode::kNone));
  pool.SetCullMode(RenderCullMode::kVisibleInAnyView);
  EXPECT_THAT(pool.GetCullMode(), Eq(RenderCullMode::kVisibleInAnyView));
}

TEST(RenderPoolTest, GetSetSortMode) {
  Registry registry;
  registry.Create<TransformSystem>(&registry);
  RenderPool<TestComponent> pool(&registry, kInitialSize);

  EXPECT_THAT(pool.GetSortMode(), Eq(SortMode_None));
  pool.SetSortMode(SortMode_AverageSpaceOriginFrontToBack);
  EXPECT_THAT(pool.GetSortMode(), Eq(SortMode_AverageSpaceOriginFrontToBack));
}

TEST(RenderPoolTest, TransformFlags) {
  Registry registry;
  registry.Create<TransformSystem>(&registry);
  RenderPool<TestComponent> pool(&registry, kInitialSize);

  EXPECT_THAT(pool.GetTransformFlag(), Ne(TransformSystem::kInvalidFlag));

  RenderPool<TestComponent> another_pool(&registry, kInitialSize);

  EXPECT_THAT(another_pool.GetTransformFlag(),
              Ne(TransformSystem::kInvalidFlag));
  EXPECT_THAT(pool.GetTransformFlag(), Ne(another_pool.GetTransformFlag()));
}

TEST(RenderPoolTest, Move) {
  Registry registry;
  registry.Create<TransformSystem>(&registry);

  RenderPool<TestComponent> source_pool(&registry, kInitialSize);
  source_pool.EmplaceComponent(1);

  const auto expected_size = source_pool.Size();
  const auto expected_flag = source_pool.GetTransformFlag();

  RenderPool<TestComponent> target_pool = std::move(source_pool);

  EXPECT_THAT(target_pool.Size(), Eq(expected_size));
  EXPECT_THAT(target_pool.GetTransformFlag(), Eq(expected_flag));

  EXPECT_THAT(source_pool.Size(), Eq(0U));
  EXPECT_THAT(source_pool.GetTransformFlag(), Ne(expected_flag));
}

}  // namespace
}  // namespace lull
