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

#include <math.h>

#include <vector>

#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/render/detail/display_list.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/registry.h"

namespace lull {
namespace {

const int kNumComponents = 100;

const int kRandomNumbers[] = {
    626099, 302018, 335613, 583726, 481191, 427656, 37245,  77980,
    632858, 749417, 823811, 415632, 504611, 562865, 406849, 486223,
    638384, 514559, 573035, 579590, 958397, 62008,  833640, 953109,
    401013, 663000, 891953, 336082, 416915, 695619, 111900, 602672,
    740063, 698323, 582968, 896507, 250982, 793163, 581443, 625006,
};
const size_t kNumRandomNumbers =
    sizeof(kRandomNumbers) / sizeof(kRandomNumbers[0]);

class DisplayListTest : public ::testing::Test {
 protected:
  void SetUp() override {
    registry_.reset(new Registry());

    auto dispatcher = new Dispatcher;
    registry_->Register(std::unique_ptr<Dispatcher>(dispatcher));

    auto entity_factory = registry_->Create<EntityFactory>(registry_.get());
    entity_factory->CreateSystem<TransformSystem>();

    pool_.reset(new RenderPool(registry_.get(), kNumComponents));

    CreateEntities(kNumComponents);
  }

 protected:
  struct RenderComponent : public Component {
    explicit RenderComponent(Entity e)
        : Component(e),
          pass(RenderPass_Main),
          sort_order(0),
          sort_order_offset(0) {}

    RenderPass pass;
    RenderSystem::SortOrder sort_order;
    RenderSystem::SortOrder sort_order_offset;
  };

  using DisplayList = detail::DisplayList<RenderComponent>;
  using RenderPool = detail::RenderPool<RenderComponent>;
  using SortMode = RenderSystem::SortMode;

  uint32_t SortaRandomUint(size_t index, uint32_t min, uint32_t max) {
    CHECK_GT(max, min);
    return (min + (kRandomNumbers[index % kNumRandomNumbers] % (max - min)));
  }

  float SortaRandomFloat(size_t index, float min, float max) {
    const float blend =
        fmodf(static_cast<float>(kRandomNumbers[index % kNumRandomNumbers]) /
                  1000000.0f,
              1.0f);
    return (min + max * blend);
  }

  void CreateEntities(size_t count) {
    auto* entity_factory = registry_->Get<EntityFactory>();
    auto* transform_system = registry_->Get<TransformSystem>();
    Sqt sqt;

    entities_.reserve(count);

    for (size_t i = 0; i < count; ++i) {
      sqt.translation = mathfu::vec3(SortaRandomFloat(i, -99, 99),
                                     SortaRandomFloat(i, -78, 78),
                                     SortaRandomFloat(i, -100, 100));

      Entity e = entity_factory->Create();
      transform_system->Create(e, sqt);

      RenderComponent& component = pool_->EmplaceComponent(e);
      component.sort_order = SortaRandomUint(i, 0, 10000000);
      component.sort_order_offset = 0;

      entities_.push_back(e);
    }
  }

  RenderSystem::View GetDefaultView() {
    const mathfu::mat4 world_from_eye_matrix = mathfu::mat4::Identity();
    const mathfu::mat4 clip_from_eye_matrix = mathfu::mat4::Identity();
    RenderSystem::View view;
    view.viewport = mathfu::vec2i(0, 0);
    view.dimensions = mathfu::vec2i(1, 1);
    view.world_from_eye_matrix = world_from_eye_matrix;
    view.clip_from_eye_matrix = clip_from_eye_matrix;
    view.clip_from_world_matrix =
        clip_from_eye_matrix * world_from_eye_matrix.Inverse();
    return view;
  }

  std::unique_ptr<Registry> registry_;
  std::unique_ptr<RenderPool> pool_;
  std::vector<Entity> entities_;
};

TEST_F(DisplayListTest, SortOrderDecreasing) {
  pool_->SetSortMode(SortMode::kSortOrderDecreasing);

  DisplayList list(registry_.get());
  list.Populate(*pool_, nullptr, 0);

  const std::vector<DisplayList::Entry> contents = *list.GetContents();

  for (size_t i = 1; i < contents.size(); ++i) {
    EXPECT_GE(contents[i - 1].component->sort_order,
              contents[i].component->sort_order);
  }
}

TEST_F(DisplayListTest, SortOrderIncreasing) {
  pool_->SetSortMode(SortMode::kSortOrderIncreasing);

  DisplayList list(registry_.get());
  list.Populate(*pool_, nullptr, 0);

  const std::vector<DisplayList::Entry> contents = *list.GetContents();

  for (size_t i = 1; i < contents.size(); ++i) {
    EXPECT_LE(contents[i - 1].component->sort_order,
              contents[i].component->sort_order);
  }
}

TEST_F(DisplayListTest, AverageSpaceOriginFrontToBack) {
  pool_->SetSortMode(SortMode::kAverageSpaceOriginFrontToBack);

  const RenderSystem::View view = GetDefaultView();

  DisplayList list(registry_.get());
  list.Populate(*pool_, &view, 1);

  const std::vector<DisplayList::Entry> contents = *list.GetContents();
  auto* transform_system = registry_->Get<TransformSystem>();

  for (size_t i = 1; i < contents.size(); ++i) {
    const mathfu::vec3 prev_pos =
        transform_system->GetWorldFromEntityMatrix(contents[i - 1].entity)
            ->TranslationVector3D();
    const mathfu::vec3 cur_pos =
        transform_system->GetWorldFromEntityMatrix(contents[i].entity)
            ->TranslationVector3D();
    EXPECT_GE(prev_pos.z, cur_pos.z);
  }
}

TEST_F(DisplayListTest, AverageSpaceOriginBackToFront) {
  pool_->SetSortMode(SortMode::kAverageSpaceOriginBackToFront);

  const RenderSystem::View view = GetDefaultView();

  DisplayList list(registry_.get());
  list.Populate(*pool_, &view, 1);

  const std::vector<DisplayList::Entry> contents = *list.GetContents();
  auto* transform_system = registry_->Get<TransformSystem>();

  for (size_t i = 1; i < contents.size(); ++i) {
    const mathfu::vec3 prev_pos =
        transform_system->GetWorldFromEntityMatrix(contents[i - 1].entity)
            ->TranslationVector3D();
    const mathfu::vec3 cur_pos =
        transform_system->GetWorldFromEntityMatrix(contents[i].entity)
            ->TranslationVector3D();
    EXPECT_LE(prev_pos.z, cur_pos.z);
  }
}

}  // namespace
}  // namespace lull
