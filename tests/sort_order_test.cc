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

#include "lullaby/systems/render/detail/sort_order.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/render/detail/render_pool_map.h"
#include "lullaby/systems/transform/transform_system.h"
#include "tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::Eq;
using detail::RenderPoolMap;
using detail::SortOrderManager;
using SortOrder = RenderSortOrder;
using SortOrderOffset = RenderSortOrderOffset;

constexpr SortOrder kDefaultSortOrder = 0;
constexpr SortOrderOffset kUseDefaultOffset =
    SortOrderManager::kUseDefaultOffset;

struct TestComponent : Component {
  explicit TestComponent(Entity e) : Component(e) {}

  SortOrder sort_order = kDefaultSortOrder;
};

// Creates a transform system and |num_entities| entities starting at 1.
// TestComponents for each entity are added to |component_map| if not null.
TransformSystem* CreateTransformSystemWithEntities(
    Registry* registry, int num_entities,
    RenderPoolMap<TestComponent>* component_map = nullptr) {
  registry->Create<EntityFactory>(registry)->CreateSystem<TransformSystem>();
  auto* transform_system = registry->Get<TransformSystem>();

  for (Entity i = 1; i <= static_cast<Entity>(num_entities); ++i) {
    transform_system->Create(i, Sqt());
    if (component_map) {
      component_map->EmplaceComponent(i, RenderPass_Main);
    }
  }

  return transform_system;
}

// Calculates the sort order for a root level entity.
SortOrder RootSortOrderFromOffset(SortOrderOffset offset) {
  const int kRootShift = 60;
  return (static_cast<SortOrder>(offset) << kRootShift);
}

// Tests that the offset for unknown entities is kUseDefaultOffset, as
// documented.
TEST(SortOrderTest, UnknownOffset) {
  Registry registry;
  SortOrderManager manager(&registry);
  EXPECT_THAT(manager.GetOffset(1), Eq(kUseDefaultOffset));
}

// Tests that explicitly set offsets are stored and returned.
TEST(SortOrderTest, SetOffset) {
  Registry registry;
  SortOrderManager manager(&registry);

  manager.SetOffset(1, 1);
  manager.SetOffset(2, 1);
  manager.SetOffset(3, -1);
  manager.SetOffset(4, 1);
  manager.SetOffset(5, 2);
  manager.SetOffset(6, 3);
  manager.SetOffset(7, 4);
  manager.SetOffset(8, -5);

  EXPECT_THAT(manager.GetOffset(1), Eq(1));
  EXPECT_THAT(manager.GetOffset(2), Eq(1));
  EXPECT_THAT(manager.GetOffset(3), Eq(-1));
  EXPECT_THAT(manager.GetOffset(4), Eq(1));
  EXPECT_THAT(manager.GetOffset(5), Eq(2));
  EXPECT_THAT(manager.GetOffset(6), Eq(3));
  EXPECT_THAT(manager.GetOffset(7), Eq(4));
  EXPECT_THAT(manager.GetOffset(8), Eq(-5));
}

// Tests that a destroyed entity becomes unknown.
TEST(SortOrderTest, Destroy) {
  Registry registry;
  SortOrderManager manager(&registry);

  manager.SetOffset(1, 2);
  EXPECT_THAT(manager.GetOffset(1), Eq(2));

  manager.Destroy(1);
  EXPECT_THAT(manager.GetOffset(1), Eq(kUseDefaultOffset));
}

// Test that the default offsets for root level entities are not 0 and varying.
TEST(SortOrderTest, DefaultRootLevelOffsets) {
  Registry registry;
  SortOrderManager manager(&registry);

  const int kNumEntities = 17;
  CreateTransformSystemWithEntities(&registry, kNumEntities);

  // Set some offsets to kUseDefaultOffset, which shouldn't change anything.
  manager.SetOffset(1, kUseDefaultOffset);
  manager.SetOffset(5, kUseDefaultOffset);

  EXPECT_THAT(manager.CalculateSortOrder(1), Eq(RootSortOrderFromOffset(1)));
  EXPECT_THAT(manager.CalculateSortOrder(2), Eq(RootSortOrderFromOffset(2)));
  EXPECT_THAT(manager.CalculateSortOrder(3), Eq(RootSortOrderFromOffset(3)));
  EXPECT_THAT(manager.CalculateSortOrder(4), Eq(RootSortOrderFromOffset(4)));
  EXPECT_THAT(manager.CalculateSortOrder(5), Eq(RootSortOrderFromOffset(5)));
  EXPECT_THAT(manager.CalculateSortOrder(6), Eq(RootSortOrderFromOffset(6)));
  EXPECT_THAT(manager.CalculateSortOrder(7), Eq(RootSortOrderFromOffset(7)));
  EXPECT_THAT(manager.CalculateSortOrder(8), Eq(RootSortOrderFromOffset(8)));
  EXPECT_THAT(manager.CalculateSortOrder(9), Eq(RootSortOrderFromOffset(9)));
  EXPECT_THAT(manager.CalculateSortOrder(10), Eq(RootSortOrderFromOffset(10)));
  EXPECT_THAT(manager.CalculateSortOrder(11), Eq(RootSortOrderFromOffset(11)));
  EXPECT_THAT(manager.CalculateSortOrder(12), Eq(RootSortOrderFromOffset(12)));
  EXPECT_THAT(manager.CalculateSortOrder(13), Eq(RootSortOrderFromOffset(13)));
  EXPECT_THAT(manager.CalculateSortOrder(14), Eq(RootSortOrderFromOffset(14)));
  EXPECT_THAT(manager.CalculateSortOrder(15), Eq(RootSortOrderFromOffset(15)));
  EXPECT_THAT(manager.CalculateSortOrder(16), Eq(RootSortOrderFromOffset(1)));
}

// Tests that the default offsets are not visible via GetOffset.
TEST(SortOrderTest, DefaultOffsetsNotVisible) {
  Registry registry;
  SortOrderManager manager(&registry);

  const int kNumEntities = 4;
  auto* transform_system =
      CreateTransformSystemWithEntities(&registry, kNumEntities);

  // Create a simple hierarchy so we're testing both root level and sibling
  // offsets.
  transform_system->AddChild(1, 2);
  transform_system->AddChild(1, 3);

  for (Entity i = 1; i <= kNumEntities; ++i) {
    EXPECT_THAT(manager.GetOffset(i), Eq(kUseDefaultOffset));
  }
}

// Tests that the sort orders of an entire hierarchy with explicit offsets are
// calculated as expected.
TEST(SortOrderTest, SimpleHierarchyOrder) {
  Registry registry;
  SortOrderManager manager(&registry);

  auto* transform_system = CreateTransformSystemWithEntities(&registry, 8);

  // Create a hierarchy (with offsets) like so:
  //
  // hierarchy   | offsets
  //   1         |   1
  // 2    3      | 1   -1
  //    4    5   |    1    2
  //       6 7 8 |       3 4 -5
  transform_system->AddChild(1, 2);
  transform_system->AddChild(1, 3);
  transform_system->AddChild(3, 4);
  transform_system->AddChild(3, 5);
  transform_system->AddChild(5, 6);
  transform_system->AddChild(5, 7);
  transform_system->AddChild(5, 8);

  manager.SetOffset(1, 1);
  manager.SetOffset(2, 1);
  manager.SetOffset(3, -1);
  manager.SetOffset(4, 1);
  manager.SetOffset(5, 2);
  manager.SetOffset(6, 3);
  manager.SetOffset(7, 4);
  manager.SetOffset(8, -5);

  EXPECT_THAT(manager.CalculateSortOrder(1), Eq(0x1000000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(2), Eq(0x1100000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(3), Eq(0x0F00000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(4), Eq(0x0F10000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(5), Eq(0x0F20000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(6), Eq(0x0F23000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(7), Eq(0x0F24000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(8), Eq(0x0F1B000000000000U));
}

// Tests that the sort orders of an entire hierarchy without explicit offsets
// are calculated as expected.
TEST(SortOrderTest, SiblingOrder) {
  Registry registry;
  SortOrderManager manager(&registry);

  auto* transform_system = CreateTransformSystemWithEntities(&registry, 8);

  // Create a hierarchy without offsets like so:
  //   1
  // 2    3
  //    4    5
  //       6 7 8
  transform_system->AddChild(1, 2);
  transform_system->AddChild(1, 3);
  transform_system->AddChild(3, 4);
  transform_system->AddChild(3, 5);
  transform_system->AddChild(5, 6);
  transform_system->AddChild(5, 7);
  transform_system->AddChild(5, 8);

  EXPECT_THAT(manager.CalculateSortOrder(1), Eq(0x1000000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(2), Eq(0x1100000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(3), Eq(0x1200000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(4), Eq(0x1210000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(5), Eq(0x1220000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(6), Eq(0x1221000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(7), Eq(0x1222000000000000U));
  EXPECT_THAT(manager.CalculateSortOrder(8), Eq(0x1223000000000000U));
}

// Tests that our debug check for exceeding the max depth is caught.
TEST(SortOrderDeathTest, MaxDepth) {
  Registry registry;
  SortOrderManager manager(&registry);

  const int kNumEntities = 17;
  auto* transform_system =
      CreateTransformSystemWithEntities(&registry, kNumEntities);

  for (Entity i = 1; i < kNumEntities; ++i) {
    transform_system->AddChild(i, i + 1);
  }

  PORT_EXPECT_DEBUG_DEATH(manager.CalculateSortOrder(kNumEntities),
                          "Cannot exceed max depth");
}

// Tests that a subtree is updated by a call to UpdateSortOrder.
TEST(SortOrderTest, UpdateSortOrder) {
  Registry registry;
  SortOrderManager manager(&registry);
  RenderPoolMap<TestComponent> component_map(&registry);

  const int kNumHierarchyEntities = 4;
  // Leave one entity outside the hierarchy completely.
  const int kNumEntities = kNumHierarchyEntities + 1;
  auto* transform_system = CreateTransformSystemWithEntities(
      &registry, kNumEntities, &component_map);

  // Make sure all sort orders start at kDefaultSortOrder.
  for (Entity i = 1; i <= kNumEntities; ++i) {
    EXPECT_THAT(component_map.GetComponent(i)->sort_order,
                Eq(kDefaultSortOrder));
  }

  // Create the hierarchy.
  for (Entity i = 1; i < kNumHierarchyEntities; ++i) {
    transform_system->AddChild(i, i + 1);
  }

  // Update only a subtree of the hierarchy.
  manager.UpdateSortOrder(2, [&](detail::EntityIdPair entity_id_pair) {
    return component_map.GetComponent(entity_id_pair.entity);
  });

  // Make sure that the sort orders for the subtree were updated.
  EXPECT_THAT(component_map.GetComponent(2)->sort_order,
              Eq(0x1100000000000000U));
  EXPECT_THAT(component_map.GetComponent(3)->sort_order,
              Eq(0x1110000000000000U));
  EXPECT_THAT(component_map.GetComponent(4)->sort_order,
              Eq(0x1111000000000000U));

  // Check that the entities outside the subtree weren't updated.
  EXPECT_THAT(component_map.GetComponent(1)->sort_order, Eq(kDefaultSortOrder));
  EXPECT_THAT(component_map.GetComponent(5)->sort_order, Eq(kDefaultSortOrder));
}

}  // namespace
}  // namespace lull
