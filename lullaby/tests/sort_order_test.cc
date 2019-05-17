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

#include "lullaby/systems/render/detail/sort_order.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/render/detail/render_pool_map.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/tests/portable_test_macros.h"

namespace lull {
namespace {

using ::testing::Eq;
using detail::RenderPoolMap;
using detail::SortOrderManager;
using SortOrder = RenderSortOrder;
using SortOrderOffset = RenderSortOrderOffset;

const SortOrder kDefaultSortOrder = 0;
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

  for (int i = 1; i <= num_entities; ++i) {
    transform_system->Create(i, Sqt());
    if (component_map) {
      component_map->EmplaceComponent(i, RenderPass_Main);
    }
  }

  return transform_system;
}

// Calculates the sort order for a root level entity.
SortOrder RootSortOrderFromOffset(SortOrderOffset offset) {
  const int kRootShift = RenderSortOrder::kRootShift;
  return (static_cast<SortOrder>(offset) << kRootShift);
}

// Tests that the offset for unknown entities is kUseDefaultOffset, as
// documented.
TEST(SortOrderTest, UnknownOffset) {
  Registry registry;
  SortOrderManager manager(&registry);
  EXPECT_THAT(manager.GetOffset(Entity(1)), Eq(kUseDefaultOffset));
}

// Tests that explicitly set offsets are stored and returned.
TEST(SortOrderTest, SetOffset) {
  Registry registry;
  SortOrderManager manager(&registry);

  manager.SetOffset(Entity(1), 1);
  manager.SetOffset(Entity(2), 1);
  manager.SetOffset(Entity(3), -1);
  manager.SetOffset(Entity(4), 1);
  manager.SetOffset(Entity(5), 2);
  manager.SetOffset(Entity(6), 3);
  manager.SetOffset(Entity(7), 4);
  manager.SetOffset(Entity(8), -5);

  EXPECT_THAT(manager.GetOffset(Entity(1)), Eq(1));
  EXPECT_THAT(manager.GetOffset(Entity(2)), Eq(1));
  EXPECT_THAT(manager.GetOffset(Entity(3)), Eq(-1));
  EXPECT_THAT(manager.GetOffset(Entity(4)), Eq(1));
  EXPECT_THAT(manager.GetOffset(Entity(5)), Eq(2));
  EXPECT_THAT(manager.GetOffset(Entity(6)), Eq(3));
  EXPECT_THAT(manager.GetOffset(Entity(7)), Eq(4));
  EXPECT_THAT(manager.GetOffset(Entity(8)), Eq(-5));
}

// Tests that a destroyed entity becomes unknown.
TEST(SortOrderTest, Destroy) {
  Registry registry;
  SortOrderManager manager(&registry);

  manager.SetOffset(Entity(1), 2);
  EXPECT_THAT(manager.GetOffset(Entity(1)), Eq(2));

  manager.Destroy(Entity(1));
  EXPECT_THAT(manager.GetOffset(Entity(1)), Eq(kUseDefaultOffset));
}

// Test that the default offsets for root level entities are not 0 and varying.
TEST(SortOrderTest, DefaultRootLevelOffsets) {
  Registry registry;
  SortOrderManager manager(&registry);

  const int kNumEntities = 17;
  CreateTransformSystemWithEntities(&registry, kNumEntities);

  // Set some offsets to kUseDefaultOffset, which shouldn't change anything.
  manager.SetOffset(Entity(1), kUseDefaultOffset);
  manager.SetOffset(Entity(5), kUseDefaultOffset);

  EXPECT_THAT(manager.CalculateSortOrder(Entity(1)),
              Eq(RootSortOrderFromOffset(1)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(2)),
              Eq(RootSortOrderFromOffset(2)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(3)),
              Eq(RootSortOrderFromOffset(3)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(4)),
              Eq(RootSortOrderFromOffset(4)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(5)),
              Eq(RootSortOrderFromOffset(5)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(6)),
              Eq(RootSortOrderFromOffset(6)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(7)),
              Eq(RootSortOrderFromOffset(7)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(8)),
              Eq(RootSortOrderFromOffset(8)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(9)),
              Eq(RootSortOrderFromOffset(9)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(10)),
              Eq(RootSortOrderFromOffset(10)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(11)),
              Eq(RootSortOrderFromOffset(11)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(12)),
              Eq(RootSortOrderFromOffset(12)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(13)),
              Eq(RootSortOrderFromOffset(13)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(14)),
              Eq(RootSortOrderFromOffset(14)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(15)),
              Eq(RootSortOrderFromOffset(15)));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(16)),
              Eq(RootSortOrderFromOffset(1)));
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

  for (int i = 1; i <= kNumEntities; ++i) {
    EXPECT_THAT(manager.GetOffset(Entity(i)), Eq(kUseDefaultOffset));
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

  manager.SetOffset(Entity(1), 1);
  manager.SetOffset(Entity(2), 1);
  manager.SetOffset(Entity(3), -1);
  manager.SetOffset(Entity(4), 1);
  manager.SetOffset(Entity(5), 2);
  manager.SetOffset(Entity(6), 3);
  manager.SetOffset(Entity(7), 4);
  manager.SetOffset(Entity(8), -5);

  EXPECT_THAT(manager.CalculateSortOrder(Entity(1)).ToHexString(),
              Eq("0x10000000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(2)).ToHexString(),
              Eq("0x11000000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(3)).ToHexString(),
              Eq("0x0F000000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(4)).ToHexString(),
              Eq("0x0F100000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(5)).ToHexString(),
              Eq("0x0F200000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(6)).ToHexString(),
              Eq("0x0F230000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(7)).ToHexString(),
              Eq("0x0F240000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(8)).ToHexString(),
              Eq("0x0F1B0000000000000000000000000000"));
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

  EXPECT_THAT(manager.CalculateSortOrder(Entity(1)).ToHexString(),
              Eq("0x10000000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(2)).ToHexString(),
              Eq("0x11000000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(3)).ToHexString(),
              Eq("0x12000000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(4)).ToHexString(),
              Eq("0x12100000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(5)).ToHexString(),
              Eq("0x12200000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(6)).ToHexString(),
              Eq("0x12210000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(7)).ToHexString(),
              Eq("0x12220000000000000000000000000000"));
  EXPECT_THAT(manager.CalculateSortOrder(Entity(8)).ToHexString(),
              Eq("0x12230000000000000000000000000000"));
}

// Tests that our debug check for exceeding the max depth is caught.
TEST(SortOrderDeathTest, MaxDepth) {
  Registry registry;
  SortOrderManager manager(&registry);

  const int kNumEntities = RenderSortOrder::kMaxDepth + 1;
  auto* transform_system =
      CreateTransformSystemWithEntities(&registry, kNumEntities);

  for (int i = 1; i < kNumEntities; ++i) {
    transform_system->AddChild(i, i + 1);
  }

  PORT_EXPECT_DEBUG_DEATH(manager.CalculateSortOrder(Entity(kNumEntities)),
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
  for (int i = 1; i <= kNumEntities; ++i) {
    EXPECT_THAT(component_map.GetComponent(i)->sort_order,
                Eq(kDefaultSortOrder));
  }

  // Create the hierarchy.
  for (int i = 1; i < kNumHierarchyEntities; ++i) {
    transform_system->AddChild(i, i + 1);
  }

  // Update only a subtree of the hierarchy.
  manager.UpdateSortOrder(Entity(2), [&](detail::EntityIdPair entity_id_pair) {
    return component_map.GetComponent(entity_id_pair.entity);
  });

  // Make sure that the sort orders for the subtree were updated.
  EXPECT_THAT(component_map.GetComponent(2)->sort_order.ToHexString(),
              Eq("0x11000000000000000000000000000000"));
  EXPECT_THAT(component_map.GetComponent(3)->sort_order.ToHexString(),
              Eq("0x11100000000000000000000000000000"));
  EXPECT_THAT(component_map.GetComponent(4)->sort_order.ToHexString(),
              Eq("0x11110000000000000000000000000000"));

  // Check that the entities outside the subtree weren't updated.
  EXPECT_THAT(component_map.GetComponent(1)->sort_order, Eq(kDefaultSortOrder));
  EXPECT_THAT(component_map.GetComponent(5)->sort_order, Eq(kDefaultSortOrder));
}

}  // namespace
}  // namespace lull
