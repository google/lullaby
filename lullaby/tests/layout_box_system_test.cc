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

#include "lullaby/systems/layout/layout_box_system.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/queued_dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/file/asset_loader.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/tests/mathfu_matchers.h"
#include "lullaby/generated/transform_def_generated.h"

namespace lull {
namespace {

using ::testing::IsNull;
using ::testing::NotNull;
using testing::EqualsMathfuVec3;

class LayoutBoxSystemTest : public ::testing::Test {
 public:
  void SetUp() override {
    registry_.reset(new Registry());
    dispatcher_ = new QueuedDispatcher;
    registry_->Register(std::unique_ptr<Dispatcher>(dispatcher_));

    entity_factory_ = registry_->Create<EntityFactory>(registry_.get());
    transform_system_ = entity_factory_->CreateSystem<TransformSystem>();
    layout_box_system_ = entity_factory_->CreateSystem<LayoutBoxSystem>();
    entity_factory_->Initialize();
  }

 protected:
  void ConnectOriginalBoxChangedListener(bool* changed) {
    dispatcher_->Connect(this, [changed](const OriginalBoxChangedEvent& e) {
      *changed = true;
    });
  }

  void ConnectDesiredSizeChangedListener(bool* changed, Entity* source,
                                         Optional<float>* x, Optional<float>* y,
                                         Optional<float>* z) {
    dispatcher_->Connect(this,
        [changed, source, x, y, z](const DesiredSizeChangedEvent& e) {
      *changed = true;
      *source = e.source;
      *x = e.x;
      *y = e.y;
      *z = e.z;
    });
  }

  void ConnectActualBoxChangedListener(bool* changed, Entity* source) {
    dispatcher_->Connect(this,
                         [changed, source](const ActualBoxChangedEvent& e) {
      *changed = true;
      *source = e.source;
    });
  }

  void AssertAabbEq(const Aabb* system_aabb, const Aabb& expected_aabb) {
    EXPECT_THAT(system_aabb, NotNull());
    EXPECT_THAT(system_aabb->min, EqualsMathfuVec3(expected_aabb.min));
    EXPECT_THAT(system_aabb->max, EqualsMathfuVec3(expected_aabb.max));
  }

  Entity CreateEntity() {
    Blueprint blueprint;
    TransformDefT transform;
    blueprint.Write(&transform);
    const Entity entity = entity_factory_->Create(&blueprint);

    // Clear out queued dispatcher.
    dispatcher_->Dispatch();

    return entity;
  }

  std::unique_ptr<Registry> registry_;
  Dispatcher* dispatcher_;
  EntityFactory* entity_factory_;
  TransformSystem* transform_system_;
  LayoutBoxSystem* layout_box_system_;
};

TEST_F(LayoutBoxSystemTest, SetOriginalBox) {
  Entity e = CreateEntity();
  bool changed = false;
  ConnectOriginalBoxChangedListener(&changed);

  layout_box_system_->SetOriginalBox(e,
      Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));

  AssertAabbEq(layout_box_system_->GetOriginalBox(e),
               Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));
  AssertAabbEq(layout_box_system_->GetActualBox(e),
               Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));
  EXPECT_EQ(false, changed);

  dispatcher_->Dispatch();
  EXPECT_EQ(true, changed);
}

TEST_F(LayoutBoxSystemTest, SetActualBox) {
  Entity e = CreateEntity();
  bool changed = false;
  Entity source = kNullEntity;
  ConnectActualBoxChangedListener(&changed, &source);

  layout_box_system_->SetActualBox(e, 123,
      Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));

  AssertAabbEq(layout_box_system_->GetActualBox(e),
               Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));
  EXPECT_EQ(false, changed);

  dispatcher_->Dispatch();
  EXPECT_EQ(true, changed);
  EXPECT_EQ(123u, source);
}

TEST_F(LayoutBoxSystemTest, SetDesiredSize) {
  Entity e = CreateEntity();
  bool changed = false;
  Entity source = kNullEntity;
  const Optional<float> unset;
  Optional<float> event_x;
  Optional<float> event_y;
  Optional<float> event_z;
  ConnectDesiredSizeChangedListener(&changed, &source, &event_x, &event_y,
                                    &event_z);

  Optional<float> get_x = layout_box_system_->GetDesiredSizeX(e);
  Optional<float> get_y = layout_box_system_->GetDesiredSizeY(e);
  Optional<float> get_z = layout_box_system_->GetDesiredSizeZ(e);
  EXPECT_EQ(event_x, unset);
  EXPECT_EQ(event_y, unset);
  EXPECT_EQ(event_z, unset);
  EXPECT_EQ(get_x, unset);
  EXPECT_EQ(get_y, unset);
  EXPECT_EQ(get_z, unset);

  layout_box_system_->SetDesiredSize(e, 123, 4.f, 5.f, Optional<float>());
  get_x = layout_box_system_->GetDesiredSizeX(e);
  get_y = layout_box_system_->GetDesiredSizeY(e);
  get_z = layout_box_system_->GetDesiredSizeZ(e);
  EXPECT_EQ(event_x, Optional<float>(4.f));
  EXPECT_EQ(event_y, Optional<float>(5.f));
  EXPECT_EQ(event_z, unset);
  EXPECT_EQ(get_x, Optional<float>(4.f));
  EXPECT_EQ(get_y, Optional<float>(5.f));
  EXPECT_EQ(get_z, unset);

  // DesiredSizeChangedEvent is sent immediately
  EXPECT_EQ(true, changed);
  EXPECT_EQ(123u, source);
}

// Even if OriginalBox or ActualBox is set, desired_size will be null until it
// is manually set.
TEST_F(LayoutBoxSystemTest, GetDesiredSize) {
  Entity e = CreateEntity();
  layout_box_system_->SetOriginalBox(e,
      Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));
  layout_box_system_->SetActualBox(e, 123,
      Aabb({-1.f, -1.f, 0.f}, {1.f, 1.f, 0.f}));
  dispatcher_->Dispatch();

  const Optional<float> unset;
  const Optional<float> get_x = layout_box_system_->GetDesiredSizeX(e);
  const Optional<float> get_y = layout_box_system_->GetDesiredSizeY(e);
  const Optional<float> get_z = layout_box_system_->GetDesiredSizeZ(e);
  EXPECT_EQ(get_x, unset);
  EXPECT_EQ(get_y, unset);
  EXPECT_EQ(get_z, unset);
}

// If there is no component from Set, the System will default to reading from
// transform's aabb.
TEST_F(LayoutBoxSystemTest, NoSetOnlyTransformAabb) {
  Entity e = CreateEntity();
  bool changed = false;
  Entity source = kNullEntity;
  ConnectOriginalBoxChangedListener(&changed);
  ConnectActualBoxChangedListener(&changed, &source);

  float f;
  int count;
  for (f = 1.f, count = 1; f < 5.f; ++f, ++count) {
    changed = false;
    transform_system_->SetAabb(e, Aabb({-f, -f, 0.f}, {f, f, 0.f}));

    EXPECT_EQ(false, changed);
    dispatcher_->Dispatch();
    EXPECT_EQ(false, changed);

    AssertAabbEq(layout_box_system_->GetOriginalBox(e),
                 Aabb({-f, -f, 0.f}, {f, f, 0.f}));
    AssertAabbEq(layout_box_system_->GetActualBox(e),
                 Aabb({-f, -f, 0.f}, {f, f, 0.f}));
  }
}

// If we set desired_size only, the System will default to reading from
// transform's aabb for OriginalBox and AcutalBox.
TEST_F(LayoutBoxSystemTest, SetDesiredAndTransformAabb) {
  Entity e = CreateEntity();
  bool changed = false;
  Entity source = kNullEntity;
  ConnectOriginalBoxChangedListener(&changed);
  ConnectActualBoxChangedListener(&changed, &source);

  float f;
  int count;
  for (f = 1.f, count = 1; f < 5.f; ++f, ++count) {
    changed = false;
    layout_box_system_->SetDesiredSize(e, 123, 4.f, 5.f, 6.f);
    transform_system_->SetAabb(e, Aabb({-f, -f, 0.f}, {f, f, 0.f}));

    EXPECT_EQ(false, changed);
    dispatcher_->Dispatch();
    EXPECT_EQ(false, changed);

    AssertAabbEq(layout_box_system_->GetOriginalBox(e),
                 Aabb({-f, -f, 0.f}, {f, f, 0.f}));
    AssertAabbEq(layout_box_system_->GetActualBox(e),
                 Aabb({-f, -f, 0.f}, {f, f, 0.f}));
  }
}

}  // namespace
}  // namespace lull
