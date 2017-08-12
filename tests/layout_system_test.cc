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

#include "lullaby/systems/layout/layout_system.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/generated/layout_def_generated.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/events/layout_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/dispatcher/queued_dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/layout/layout_box_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

static const float kEpsilon = 0.0001f;

class LayoutSystemTest : public testing::Test {
 public:
  void SetUp() override {
    registry_.reset(new Registry());
    std::unique_ptr<Dispatcher> dispatcher = CreateDispatcher();
    dispatcher_ = dispatcher.get();
    registry_->Register(std::move(dispatcher));

    registry_->Create<AssetLoader>([&](const char* filename, std::string* out)
          -> bool {
      return true;
    });

    entity_factory_ = registry_->Create<EntityFactory>(registry_.get());
    layout_system_ = entity_factory_->CreateSystem<LayoutSystem>();
    layout_box_system_ = entity_factory_->CreateSystem<LayoutBoxSystem>();
    transform_system_ = entity_factory_->CreateSystem<TransformSystem>();
    dispatcher_system_ = entity_factory_->CreateSystem<DispatcherSystem>();
    entity_factory_->Initialize();
  }

 protected:
  virtual std::unique_ptr<Dispatcher> CreateDispatcher() {
    return std::unique_ptr<Dispatcher>(new Dispatcher());
  }

  Entity CreateParent() {
    TransformDefT transform;
    LayoutDefT layout;
    layout.canvas_size = mathfu::vec2(2, 2);
    layout.shrink_to_fit = false;
    layout.elements_per_wrap = 2;
    layout.max_elements = 4;

    Blueprint blueprint;
    blueprint.Write(&transform);
    blueprint.Write(&layout);
    return entity_factory_->Create(&blueprint);
  }

  Entity CreateChild(Entity parent, float weight = 0.f,
                     bool add_layout = false) {
    Blueprint blueprint;
    TransformDefT transform;
    blueprint.Write(&transform);
    if (weight != 0) {
      LayoutElementDefT layout_element;
      layout_element.horizontal_weight = weight;
      blueprint.Write(&layout_element);
    }
    if (add_layout) {
      LayoutDefT layout;
      blueprint.Write(&layout);
    }

    const Entity child = entity_factory_->Create(&blueprint);
    transform_system_->AddChild(parent, child);
    return child;
  }

  void ConnectLayoutChangedListener(const lull::Entity& entity,
      bool* layout_changed) {
    dispatcher_system_->Connect(entity, this, [layout_changed](
        const LayoutChangedEvent& event) {
      *layout_changed = true;
    });
  }

  void AssertTranslationsAndSizes(int num_children, const Entity* children,
      const mathfu::vec2* expectations, const mathfu::vec2* size_expectations) {
    for (int i = 0; i < num_children; ++i) {
      const Sqt* sqt = transform_system_->GetSqt(children[i]);
      EXPECT_NEAR(expectations[i].x, sqt->translation.x, kEpsilon);
      EXPECT_NEAR(expectations[i].y, sqt->translation.y, kEpsilon);
      const Aabb* aabb = layout_box_system_->GetActualBox(children[i]);
      mathfu::vec2 size = aabb->max.xy() - aabb->min.xy();
      EXPECT_NEAR(size_expectations[i].x, size.x, kEpsilon);
      EXPECT_NEAR(size_expectations[i].y, size.y, kEpsilon);
    }
  }

  std::unique_ptr<Registry> registry_;
  Dispatcher* dispatcher_;
  EntityFactory* entity_factory_;
  TransformSystem* transform_system_;
  LayoutSystem* layout_system_;
  LayoutBoxSystem* layout_box_system_;
  DispatcherSystem* dispatcher_system_;
};

class QueuedLayoutSystemTest : public LayoutSystemTest {
 protected:
  void SetUp() override {
    LayoutSystemTest::SetUp();
    ConnectListeners();
  }

  std::unique_ptr<Dispatcher> CreateDispatcher() override {
    return std::unique_ptr<Dispatcher>(new QueuedDispatcher());
  }

  void ConnectListeners() {
    dispatcher_->Connect(this, [this](const LayoutChangedEvent& e) {
      UpdateListener(&layouts_changed_, e.target);
    });
    dispatcher_->Connect(this, [this](const OriginalBoxChangedEvent& e) {
      UpdateListener(&original_boxes_, e.target);
    });
    dispatcher_->Connect(this, [this](const DesiredSizeChangedEvent& e) {
      UpdateListener(&desired_sizes_, e.target);
      UpdateSources(&desired_sources_, e.target, e.source);
    });
    dispatcher_->Connect(this, [this](const ActualBoxChangedEvent& e) {
      UpdateListener(&actual_boxes_, e.target);
      UpdateSources(&actual_sources_, e.target, e.source);
    });
  }

  void UpdateListener(std::unordered_map<Entity, int>* map, Entity e) {
    auto iter = map->find(e);
    if (iter == map->end()) {
      map->emplace(e, 1);
    } else {
      ++iter->second;
    }
  }

  void UpdateSources(std::unordered_map<Entity, Entity>* sources, Entity e,
                    Entity source) {
    (*sources)[e] = source;
  }

  void ClearListeners() {
    layouts_changed_.clear();
    original_boxes_.clear();
    desired_sizes_.clear();
    actual_boxes_.clear();
    desired_sources_.clear();
    actual_sources_.clear();
  }

  void AssertListenersEmpty() {
    AssertListenerEmpty(layouts_changed_);
    AssertListenerEmpty(original_boxes_);
    AssertListenerEmpty(desired_sizes_);
    AssertListenerEmpty(actual_boxes_);
  }

  void AssertListenerEmpty(const std::unordered_map<Entity, int>& listener) {
    EXPECT_EQ(0u, listener.size());
  }

  void AssertListenerMatch(const std::unordered_map<Entity, int>& expectations,
                           const std::unordered_map<Entity, int>& listener) {
    EXPECT_EQ(expectations, listener);
  }

  void AssertSourcesMatch(const std::unordered_map<Entity,
                          Entity>& expectations,
                          const std::unordered_map<Entity, Entity>& sources) {
    EXPECT_EQ(expectations, sources);
  }

  std::unordered_map<Entity, int> layouts_changed_;
  std::unordered_map<Entity, int> original_boxes_;
  std::unordered_map<Entity, int> desired_sizes_;
  std::unordered_map<Entity, int> actual_boxes_;
  std::unordered_map<Entity, Entity> desired_sources_;
  std::unordered_map<Entity, Entity> actual_sources_;
};

TEST_F(LayoutSystemTest, LayoutInvalidEntity) {
  Entity entity = entity_factory_->Create();
  bool layout_changed = false;
  ConnectLayoutChangedListener(entity, &layout_changed);

  // Should return if called on an entity without a layout with no errors
  layout_system_->Layout(entity_factory_->Create());
  EXPECT_THAT(layout_changed, false);
}

TEST_F(LayoutSystemTest, LayoutEntityNoTransformComponent) {
  LayoutDefT layout;
  layout.canvas_size = mathfu::vec2(2, 2);
  layout.shrink_to_fit = false;
  layout.elements_per_wrap = 2;
  layout.max_elements = 4;

  Blueprint blueprint;
  blueprint.Write(&layout);
  Entity parent = entity_factory_->Create(&blueprint);
  bool layout_changed = false;
  ConnectLayoutChangedListener(parent, &layout_changed);

  // Should return if called on an entity without children with no errors.
  layout_system_->Layout(parent);
  EXPECT_THAT(layout_changed, false);
}

// Test that LayoutSystem handles setting the sqt of multiple child components
// when the entities are created via the entity factory and the layout is linear
// instead of radial.
TEST_F(LayoutSystemTest, CreateLayout) {
  const int num_children = 5;
  const Entity parent = CreateParent();
  Entity children[num_children];
  for (int i = 0; i < num_children; ++i) {
    children[i] = CreateChild(parent);
  }

  for (int i = 0; i < num_children; ++i) {
    const Sqt* sqt = transform_system_->GetSqt(children[i]);
    EXPECT_NEAR(-1, sqt->translation.x, kEpsilon);
    EXPECT_NEAR(1, sqt->translation.y, kEpsilon);
  }
}

TEST_F(LayoutSystemTest, RadialLayout_Circle) {
  TransformDefT transform;
  RadialLayoutDefT layout;
  layout.major_axis = mathfu::vec3(1.f, 0.f, 0.f);
  layout.minor_axis = mathfu::vec3(0.f, 1.f, 0.f);
  layout.degrees_per_element = 45.f;

  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&layout);
  const Entity parent = entity_factory_->Create(&blueprint);

  const int num_children = 9;
  Entity children[num_children];
  for (int i = 0; i < num_children; ++i) {
    children[i] = CreateChild(parent);
  }

  for (int i = 0; i < num_children; ++i) {
    float angle =
        static_cast<float>(i) * layout.degrees_per_element * kDegreesToRadians;
    const Sqt* sqt = transform_system_->GetSqt(children[i]);
    EXPECT_NEAR(
        layout.major_axis.x * cosf(angle) + layout.minor_axis.x * sinf(angle),
        sqt->translation.x, kEpsilon);
    EXPECT_NEAR(
        layout.major_axis.y * cosf(angle) + layout.minor_axis.y * sinf(angle),
        sqt->translation.y, kEpsilon);
    EXPECT_NEAR(
        layout.major_axis.z * cosf(angle) + layout.minor_axis.z * sinf(angle),
        sqt->translation.z, kEpsilon);
  }
}

TEST_F(LayoutSystemTest, RadialLayout_Ellipse) {
  TransformDefT transform;
  RadialLayoutDefT layout;
  layout.major_axis = mathfu::vec3(1.f, 0.f, 0.f);
  layout.minor_axis = mathfu::vec3(0.f, 1.f, 0.f);
  layout.degrees_per_element = 30.f;

  Blueprint blueprint;
  blueprint.Write(&transform);
  blueprint.Write(&layout);
  const Entity parent = entity_factory_->Create(&blueprint);

  const int num_children = 13;
  Entity children[num_children];
  for (int i = 0; i < num_children; ++i) {
    children[i] = CreateChild(parent);
  }

  for (int i = 0; i < num_children; ++i) {
    const float angle =
        static_cast<float>(i) * layout.degrees_per_element * kDegreesToRadians;
    const Sqt* sqt = transform_system_->GetSqt(children[i]);
    EXPECT_NEAR(
        layout.major_axis.x * cosf(angle) + layout.minor_axis.x * sinf(angle),
        sqt->translation.x, kEpsilon);
    EXPECT_NEAR(
        layout.major_axis.y * cosf(angle) + layout.minor_axis.y * sinf(angle),
        sqt->translation.y, kEpsilon);
    EXPECT_NEAR(
        layout.major_axis.z * cosf(angle) + layout.minor_axis.z * sinf(angle),
        sqt->translation.z, kEpsilon);
  }
}

TEST_F(LayoutSystemTest, Destroy) {
  const Entity parent = CreateParent();

  bool layout_changed = false;
  ConnectLayoutChangedListener(parent, &layout_changed);

  Entity child = CreateChild(parent);

  EXPECT_THAT(layout_changed, true);
  layout_changed = false;

  entity_factory_->Destroy(child);
  EXPECT_THAT(layout_changed, true);
  layout_changed = false;

  entity_factory_->Destroy(parent);

  EXPECT_THAT(layout_changed, false);
  layout_system_->Layout(parent);
  EXPECT_THAT(layout_changed, false);  // No change, since entity deleted.
}

// Test that LayoutSystem will resize weighted elements and does not infinite
// loop.
TEST_F(LayoutSystemTest, CreateLayout_WeightedElements) {
  // Canvas_size = (2,2) and elements_per_wrap = 2, so all the weighted children
  // will be 1 wide into 2 columns.
  // They should be arranged in the following manner.
  //  0/2 1/3  (no height)
  //   -   -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-0.5f, 1.f), mathfu::vec2(0.5f, 1.f),
      mathfu::vec2(-0.5f, 1.f), mathfu::vec2(0.5f, 1.f),
  };
  const mathfu::vec2 size_expectations[] = {
      mathfu::vec2(1.f, 0.f), mathfu::vec2(1.f, 0.f),
      mathfu::vec2(1.f, 0.f), mathfu::vec2(1.f, 0.f),
  };

  const Entity parent = CreateParent();

  int parent_layout_times = 0;
  dispatcher_system_->Connect(parent, this, [&parent_layout_times](
      const LayoutChangedEvent& event) {
    ++parent_layout_times;
  });

  const int num_children = 4;
  Entity children[num_children];
  for (int i = 0; i < num_children; ++i) {
    // Give all children weight.
    // The LayoutDef will change the child's size.
    children[i] = CreateChild(parent, 1.0f, true);
  }

  // Resulting children will be all 1 wide into 2 columns.
  AssertTranslationsAndSizes(num_children, children, expectations,
                             size_expectations);

  // The total number of times parent will re-layout, due to non-queued
  // dispatcher:
  // 1: Add child 0.
  // 2x1: Respond to child 0 resize twice, set Aabb & ActualBox.
  // 1: Add child 1.
  // 2x2: Respond to child 0,1 resize twice, set Aabb & ActualBox.
  // 1: Add child 2.
  // 3x2: Respond to child 0,1,2 resize twice, set Aabb & ActualBox.
  // 1: Add child 3.
  // 4x2: Respond to child 0,1,2,3 resize twice, set Aabb & ActualBox.
  EXPECT_EQ(24, parent_layout_times);
  // Queued dispatcher will be much fewer, down to 1 + 1, since
  // all children are added on the same frame, and then hopefully resized on the
  // same frame.  However, if resizing requires complex operations, such as text
  // rendering, that may result in re-layout on multiple frames.
}

// Test that LayoutSystem will resize weighted elements, including other
// Layouts that will resize their children.
// Parent canvas_size = (2,2)
//   Child [0], size = (1,0)
//   Child [1], weighted, becomes size = (1,0)
//     Grandchild [2], weighted, becomes size = (0.5,0)
//     Grandchild [3], weighted, also becomes size = (0.5,0)
TEST_F(LayoutSystemTest, CreateLayout_WeightedElements_Nested) {
  // They should be arranged in the following manner in 2 columns.
  //  0   1  (no height, so 2/3 are inside 1 at the same y)
  //     2 3
  //  -   -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-0.5f, 1.f), mathfu::vec2(0.5f, 1.f),
      // [2] and [3] are relative to [1]'s coordinates.
      mathfu::vec2(-0.25f, 0.f), mathfu::vec2(0.25f, 0.f),
  };
  const mathfu::vec2 size_expectations[] = {
      mathfu::vec2(1.f, 0.f), mathfu::vec2(1.f, 0.f),
      mathfu::vec2(0.5f, 0.f), mathfu::vec2(0.5f, 0.f),
  };

  const int num_children = 4;
  const Entity parent = CreateParent();
  Entity children[num_children];

  // Child [0], fixed size (1,0) in left column.
  children[0] = CreateChild(parent);
  Aabb aabb;
  aabb.min = mathfu::vec3(-0.5f, 0.f, 0.f);
  aabb.max = mathfu::vec3(0.5f, 0.f, 0.f);
  layout_box_system_->SetOriginalBox(children[0], aabb);

  // Child [1], has weight, will weighted to (1,0) on the right column.
  // The LayoutDef will change the child's size.
  children[1] = CreateChild(parent, 1.0f, true);

  // Grandchild [2] and [3] of [1], have weight, will be both weighted to
  // (0.5,0) inside of [1].
  for (int i = 2; i < num_children; ++i) {
  // The LayoutDef will change the child's size.
    children[i] = CreateChild(children[1], 1.0f, true);
  }

  AssertTranslationsAndSizes(num_children, children, expectations,
                             size_expectations);

  // In addition, if there was an asynchronous mesh generator that SetActualBox
  // in a later frame, the Layouts should still use their previously set
  // desired_size.
  layout_box_system_->SetActualBox(children[2], kNullEntity,
                                   Aabb({-0.25f, 0.f, 0.f}, {0.25f, 0.f, 0.f}));
  AssertTranslationsAndSizes(num_children, children, expectations,
                             size_expectations);
}

// Test that LayoutSystem will disable weighted elements when there is no space
// and does not infinite loop.  The disabled child will be a LayoutDef with its
// own children, which will also be disabled as well.
// Parent canvas_size = (2,2)
//   Child [0], size = (2,2)
//   Child [1], weighted, disabled, has a Layout.
//     Grandchild [2] of [1], weighted, disabled by [1]
// They should be arranged in the following manner.
//   0 0
//   0 0
TEST_F(LayoutSystemTest, CreateLayout_WeightedElements_Disabled) {
  const int num_children = 3;
  const Entity parent = CreateParent();
  Entity children[num_children];

  // Child [0], fixed size (2,2)
  children[0] = CreateChild(parent);
  Aabb aabb;
  aabb.min = mathfu::vec3(-1.f, -1.f, 0.f);
  aabb.max = mathfu::vec3(1.f, 1.f, 0.f);
  layout_box_system_->SetOriginalBox(children[0], aabb);

  // Child [1], has weight, will be disabled
  children[1] = CreateChild(parent, 1.0f, true);

  // Grandchild [2] of [1], has weight, will be disabled
  children[2] = CreateChild(children[1]);

  // Child [0] didnt move or change size
  const Sqt* sqt = transform_system_->GetSqt(children[0]);
  EXPECT_NEAR(0.f, sqt->translation.x, kEpsilon);
  EXPECT_NEAR(0.f, sqt->translation.y, kEpsilon);
  const Aabb* aabb_0 = layout_box_system_->GetActualBox(children[0]);
  mathfu::vec2 size = aabb_0->max.xy() - aabb_0->min.xy();
  EXPECT_NEAR(2.f, size.x, kEpsilon);
  EXPECT_NEAR(2.f, size.y, kEpsilon);

  // Children [1] and [2] should be disabled
  EXPECT_EQ(true, transform_system_->IsEnabled(children[0]));
  EXPECT_EQ(false, transform_system_->IsEnabled(children[1]));
  EXPECT_EQ(false, transform_system_->IsEnabled(children[2]));
}

// Test that LayoutSystem will aggregate events from multiple children and only
// ApplyLayout once in a frame if there isn't any resizing.
TEST_F(QueuedLayoutSystemTest, AggregateEvents) {
  const int num_children = 4;
  Entity children[num_children];
  const Entity parent = CreateParent();

  for (int i = 0; i < num_children; ++i) {
    children[i] = CreateChild(parent);
  }

  AssertListenersEmpty();
  dispatcher_->Dispatch();
  AssertListenerMatch({ {parent, 1} }, layouts_changed_);
}

// Test that the LayoutSystem will SetOriginalBox and children SetDesiredSize
// in response to a ParentChangedEvent.
TEST_F(QueuedLayoutSystemTest, ParentChanged) {
  const Entity parent = CreateParent();
  const Entity child = CreateChild(parent, 1.0f);

  AssertListenersEmpty();
  dispatcher_->Dispatch();
  AssertListenerMatch({ {parent, 1} }, original_boxes_);
  AssertListenerMatch({ {child, 1} }, desired_sizes_);
  AssertListenerEmpty(actual_boxes_);
}

// Test that the LayoutSystem will SetOriginalBox and children SetDesiredSize
// in response to a DesiredSizeChangedEvent.
TEST_F(QueuedLayoutSystemTest, OriginalBox) {
  const Entity parent = CreateParent();
  const Entity child = CreateChild(parent, 1.0f);

  dispatcher_->Dispatch();
  ClearListeners();

  layout_box_system_->SetOriginalBox(child, Aabb({-1.f, -1.f, 0.f},
                                                  {1.f, 1.f, 0.f}));
  AssertListenersEmpty();

  dispatcher_->Dispatch();
  AssertListenerMatch({ {parent, 1}, {child, 1} }, original_boxes_);
  AssertListenerMatch({ {child, 1} }, desired_sizes_);
  AssertListenerEmpty(actual_boxes_);
}

// Test that the LayoutSystem will SetActualBox and children SetDesiredSize
// in response to a DesiredSizeChangedEvent.
// The source is not known so the parent uses SetActualBox() as normal.
TEST_F(QueuedLayoutSystemTest, DesiredBox) {
  const Entity parent = CreateParent();
  const Entity child = CreateChild(parent, 1.0f);

  dispatcher_->Dispatch();
  ClearListeners();

  // SetDesiredSize triggers immediately, but Layout aggregates dirty_layouts_
  // until ProcessDirty.
  layout_box_system_->SetDesiredSize(parent, 123, 1.f, 1.f, Optional<float>());
  AssertListenerEmpty(original_boxes_);
  AssertListenerMatch({ {parent, 1} }, desired_sizes_);
  AssertSourcesMatch({ {parent, 123} }, desired_sources_);;
  AssertListenerEmpty(actual_boxes_);

  dispatcher_->Dispatch();
  AssertListenerEmpty(original_boxes_);
  AssertListenerMatch({ {parent, 1}, {child, 1} }, desired_sizes_);
  AssertListenerMatch({ {parent, 1} }, actual_boxes_);
  AssertSourcesMatch({ {parent, 123}, {child, 123} }, desired_sources_);
  AssertSourcesMatch({ {parent, 123} }, actual_sources_);
}

// Test that the LayoutSystem will SetActualBox and no children SetDesiredSize
// in response to a ActualBoxChangedEvent.
// The source is not known so the parent uses SetActualBox() as normal.
TEST_F(QueuedLayoutSystemTest, ActualBox) {
  const Entity parent = CreateParent();
  const Entity child = CreateChild(parent, 1.0f);

  dispatcher_->Dispatch();
  ClearListeners();

  layout_box_system_->SetActualBox(child, 123, Aabb({-1.f, -1.f, 0.f},
                                                    {1.f, 1.f, 0.f}));
  AssertListenersEmpty();

  dispatcher_->Dispatch();
  AssertListenerEmpty(original_boxes_);
  AssertListenerEmpty(desired_sizes_);
  AssertListenerMatch({ {parent, 1}, {child, 1} }, actual_boxes_);
  AssertSourcesMatch({ {parent, 123}, {child, 123} }, actual_sources_);
}

// Test that the LayoutSystem will SetOriginalBox and no children SetDesiredSize
// in response to a ActualBoxChangedEvent with source == self.
TEST_F(QueuedLayoutSystemTest, ActualBoxSameSource) {
  const Entity parent = CreateParent();
  const Entity child = CreateChild(parent, 1.0f);

  dispatcher_->Dispatch();
  ClearListeners();

  layout_box_system_->SetActualBox(child, parent, Aabb({-1.f, -1.f, 0.f},
                                                       {1.f, 1.f, 0.f}));
  AssertListenersEmpty();

  dispatcher_->Dispatch();
  AssertListenerMatch({ {parent, 1} }, original_boxes_);
  AssertListenerEmpty(desired_sizes_);
  AssertListenerMatch({ {child, 1} }, actual_boxes_);
  AssertSourcesMatch({ {child, parent} }, actual_sources_);
}

// Test that if the LayoutSystem receives both a OriginalBoxChangedEvent and a
// ActualBoxChangedEvent it will only calculate the higher priority pass.
TEST_F(QueuedLayoutSystemTest, AggregateOriginalBox) {
  const Entity parent = CreateParent();
  const Entity child = CreateChild(parent, 1.0f);

  dispatcher_->Dispatch();
  ClearListeners();

  layout_box_system_->SetActualBox(child, kNullEntity, Aabb({-1.f, -1.f, 0.f},
                                                            {1.f, 1.f, 0.f}));
  layout_box_system_->SetOriginalBox(child, Aabb({-1.f, -1.f, 0.f},
                                                  {1.f, 1.f, 0.f}));
  AssertListenersEmpty();

  dispatcher_->Dispatch();
  AssertListenerMatch({ {parent, 1} }, layouts_changed_);
  AssertListenerMatch({ {parent, 1}, {child, 1} }, original_boxes_);
  AssertListenerMatch({ {child, 1} }, desired_sizes_);
  AssertListenerMatch({ {child, 1} }, actual_boxes_);
}

// Test that the LayoutSystem will use the previously set DesiredSize even in
// subsequent ActualBoxChangedEvents, supporting asynchronous mesh generators.
TEST_F(QueuedLayoutSystemTest, DesiredBox_Repeated) {
  const Entity parent = CreateParent();
  const Entity child = CreateChild(parent, 1.0f);

  dispatcher_->Dispatch();
  const Aabb* aabb = layout_box_system_->GetActualBox(parent);
  EXPECT_NEAR(-1.f, aabb->min.x, kEpsilon);
  EXPECT_NEAR(-1.f, aabb->min.y, kEpsilon);
  EXPECT_NEAR(1.f, aabb->max.x, kEpsilon);
  EXPECT_NEAR(1.f, aabb->max.y, kEpsilon);

  layout_box_system_->SetDesiredSize(parent, kNullEntity, 1.f, 1.f,
                                     Optional<float>());
  dispatcher_->Dispatch();
  aabb = layout_box_system_->GetActualBox(parent);
  EXPECT_NEAR(-0.5f, aabb->min.x, kEpsilon);
  EXPECT_NEAR(-0.5f, aabb->min.y, kEpsilon);
  EXPECT_NEAR(0.5f, aabb->max.x, kEpsilon);
  EXPECT_NEAR(0.5f, aabb->max.y, kEpsilon);

  layout_box_system_->SetActualBox(child, kNullEntity, Aabb({0.1f, 0.1f, 0.f},
                                                            {0.1f, 0.1f, 0.f}));
  dispatcher_->Dispatch();
  aabb = layout_box_system_->GetActualBox(parent);
  EXPECT_NEAR(-0.5f, aabb->min.x, kEpsilon);
  EXPECT_NEAR(-0.5f, aabb->min.y, kEpsilon);
  EXPECT_NEAR(0.5f, aabb->max.x, kEpsilon);
  EXPECT_NEAR(0.5f, aabb->max.y, kEpsilon);
}

// Test that if the LayoutSystem receives multiple ActualBoxChangedEvents, it
// keeps the closest parent's source.
TEST_F(QueuedLayoutSystemTest, ActualBoxClosestSource_First) {
  const Entity grandgrandparent = CreateParent();
  const Entity grandparent = CreateChild(grandgrandparent, 1.0f);
  const Entity parent = CreateChild(grandparent, 1.0f, true);
  const Entity child = CreateChild(parent, 1.0f);

  dispatcher_->Dispatch();
  ClearListeners();

  layout_box_system_->SetActualBox(child, grandparent,
                                   Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));
  layout_box_system_->SetActualBox(child, grandgrandparent,
                                   Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));
  AssertListenersEmpty();

  dispatcher_->Dispatch();
  AssertListenerEmpty(original_boxes_);
  AssertListenerEmpty(desired_sizes_);
  AssertListenerMatch({ {parent, 1}, {child, 2} }, actual_boxes_);
  AssertSourcesMatch({ {parent, grandparent}, {child, grandgrandparent} },
                     actual_sources_);
}

// Same as above, but closer source is second.
TEST_F(QueuedLayoutSystemTest, ActualBoxClosestSource_Second) {
  const Entity grandgrandparent = CreateParent();
  const Entity grandparent = CreateChild(grandgrandparent, 1.0f);
  const Entity parent = CreateChild(grandparent, 1.0f, true);
  const Entity child = CreateChild(parent, 1.0f);

  dispatcher_->Dispatch();
  ClearListeners();

  layout_box_system_->SetActualBox(child, grandgrandparent,
                                   Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));
  layout_box_system_->SetActualBox(child, grandparent,
                                   Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));
  AssertListenersEmpty();

  dispatcher_->Dispatch();
  AssertListenerEmpty(original_boxes_);
  AssertListenerEmpty(desired_sizes_);
  AssertListenerMatch({ {parent, 1}, {child, 2} }, actual_boxes_);
  AssertSourcesMatch({ {parent, grandparent}, {child, grandparent} },
                     actual_sources_);
}

}  // namespace
}  // namespace lull
