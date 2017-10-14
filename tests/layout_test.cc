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

#include <memory>

#include "gtest/gtest.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/layout/layout.h"
#include "lullaby/systems/layout/layout_box_system.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {
namespace {

static const float kEpsilon = 0.0001f;
static const Entity kParent = 123;

class LayoutTest : public testing::Test {
 public:
  void SetUp() override {
    registry_.reset(new Registry());
    Dispatcher* dispatcher = new Dispatcher();
    registry_->Register(std::unique_ptr<Dispatcher>(dispatcher));
    entity_factory_ = registry_->Create<EntityFactory>(registry_.get());
    transform_system_ = entity_factory_->CreateSystem<TransformSystem>();
    layout_box_system_ = entity_factory_->CreateSystem<LayoutBoxSystem>();
    set_pos_fn_ = GetDefaultSetLayoutPositionFn(registry_.get());

    root_ = entity_factory_->Create();
    transform_system_->Create(root_, Sqt());

    dispatcher->Connect(this, [this](const DesiredSizeChangedEvent& e) {
      mathfu::vec3 size = layout_box_system_->GetOriginalBox(e.target)->Size();
      if (e.x) {
        size.x = *e.x;
      }
      if (e.y) {
        size.y = *e.y;
      }
      if (e.z) {
        size.z = *e.z;
      }
      Aabb aabb(-size / 2.0f, size / 2.0f);
      layout_box_system_->SetActualBox(e.target, e.source, aabb);
    });
  }

 protected:
  struct DesiredSize {
    Optional<float> x;
    Optional<float> y;
    DesiredSize(const Optional<float>& x, const Optional<float>& y)
        : x(x), y(y) {}
  };

  // Creates the specified number of 1x1 sized children.
  std::vector<Entity> CreateChildren(size_t num) {
    for (size_t i = 0; i < num; ++i) {
      CreateChild(1.0f);
    }
    return *transform_system_->GetChildren(root_);
  }

  Entity CreateChild(float item_size) {
    Aabb aabb;
    aabb.min = mathfu::vec3(-item_size / 2.0f, -item_size / 2.0f, 0.f);
    aabb.max = mathfu::vec3(item_size / 2.0f, item_size / 2.0f, 0.f);

    Entity child = entity_factory_->Create();
    transform_system_->Create(child, Sqt());
    layout_box_system_->SetOriginalBox(child, aabb);
    transform_system_->AddChild(root_, child);
    return child;
  }

  void ResizeEntityX(Entity entity, float size) {
    Aabb aabb = *layout_box_system_->GetOriginalBox(entity);
    aabb.min.x = -size / 2.0f;
    aabb.max.x = size / 2.0f;
    layout_box_system_->SetOriginalBox(entity, aabb);
  }

  void ResizeEntityY(Entity entity, float size) {
    Aabb aabb = *layout_box_system_->GetOriginalBox(entity);
    aabb.min.y = -size / 2.0f;
    aabb.max.y = size / 2.0f;
    layout_box_system_->SetOriginalBox(entity, aabb);
  }

  void CreateElementParams(std::vector<LayoutElement>* elements,
                           const std::vector<Entity>& children) {
    for (Entity child : children) {
      elements->emplace_back(child);
    }
  }

  // Layout num (default 3) children in a row, and then check the expectations
  void LayoutChildrenAndAssertTranslations(const LayoutParams& params,
      const mathfu::vec2* expectations, size_t num = 3) {
    const std::vector<Entity> children = CreateChildren(num);
    LayoutAndAssertTranslations(params, children, expectations);
  }

  void LayoutAndAssertTranslations(const LayoutParams& params,
      const std::vector<Entity>& children, const mathfu::vec2* expectations) {
    ApplyLayout(registry_.get(), params, children);
    AssertTranslations(children, expectations);
  }

  void AssertTranslations(const std::vector<Entity>& children,
                          const mathfu::vec2* expectations) {
    for (size_t i = 0; i < children.size(); ++i) {
      const Sqt* sqt = transform_system_->GetSqt(children[i]);
      EXPECT_NEAR(expectations[i].x, sqt->translation.x, kEpsilon);
      EXPECT_NEAR(expectations[i].y, sqt->translation.y, kEpsilon);
    }
  }

  // If enabled_expectations is null that means check all are enabled.
  void AssertDesiredSizesAndEnabled(const std::vector<Entity>& children,
      const DesiredSize* desired_sizes,
      const bool* enabled_expectations = nullptr) {
    for (size_t i = 0; i < children.size(); ++i) {
      const Optional<float> size_x =
          layout_box_system_->GetDesiredSizeX(children[i]);
      const Optional<float> size_y =
          layout_box_system_->GetDesiredSizeY(children[i]);
      const Optional<float> size_z =
          layout_box_system_->GetDesiredSizeZ(children[i]);
      EXPECT_EQ(desired_sizes[i].x, size_x);
      EXPECT_EQ(desired_sizes[i].y, size_y);
      EXPECT_EQ(kUnchanged, size_z);
      EXPECT_EQ(enabled_expectations ? enabled_expectations[i] : true,
                transform_system_->IsEnabled(children[i]));
    }
  }

  const Optional<float> kUnchanged;
  const DesiredSize kUnchangedSize = DesiredSize(kUnchanged, kUnchanged);

  std::unique_ptr<Registry> registry_;
  EntityFactory* entity_factory_;
  TransformSystem* transform_system_;
  LayoutBoxSystem* layout_box_system_;
  SetLayoutPositionFn set_pos_fn_;
  Entity root_;
};

TEST_F(LayoutTest, SetLayoutPositionFn) {
  std::vector<Entity> children = CreateChildren(3);
  std::vector<LayoutElement> elements;
  CreateElementParams(&elements, children);
  const auto set_pos_fn = [&](Entity entity, const mathfu::vec2& position) {
    EXPECT_EQ(false, children.empty());
    EXPECT_EQ(children.front(), entity);
    children.erase(children.begin());
  };

  ApplyLayout(registry_.get(), LayoutParams(), elements, set_pos_fn);
  EXPECT_EQ(true, children.empty());
}

TEST_F(LayoutTest, Spacing_RightDown) {
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-3.f, 0.f), mathfu::vec2(0.f, 0.f), mathfu::vec2(3.f, 0.f),
  };
  LayoutParams params;
  params.fill_order = LayoutFillOrder_RightDown;
  params.spacing = mathfu::vec2(2.f, 2.f);
  params.canvas_size = mathfu::vec2(7.f, 1.f);
  LayoutChildrenAndAssertTranslations(params, expectations);
}

TEST_F(LayoutTest, Spacing_LeftDown) {
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(3.f, 0.f), mathfu::vec2(0.f, 0.f), mathfu::vec2(-3.f, 0.f),
  };
  LayoutParams params;
  params.fill_order = LayoutFillOrder_LeftDown;
  params.spacing = mathfu::vec2(2.f, 2.f);
  params.canvas_size = mathfu::vec2(7.f, 1.f);
  LayoutChildrenAndAssertTranslations(params, expectations);
}

TEST_F(LayoutTest, Spacing_DownRight) {
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-3.f, 0.f), mathfu::vec2(-3.f, -3.f),
        mathfu::vec2(-3.f, -6.f),
  };
  LayoutParams params;
  params.fill_order = LayoutFillOrder_DownRight;
  params.spacing = mathfu::vec2(0.0f, 2.f);
  params.canvas_size = mathfu::vec2(7.f, 1.f);
  LayoutChildrenAndAssertTranslations(params, expectations);
}

TEST_F(LayoutTest, TopLeftAlignment) {
  // Layout 5 children in a 3x3 grid using TopLeft alignment.  They should be
  // arranged in the following manner.
  //  0 1 2
  //  3 4 -
  //  - - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-1.f, 1.f), mathfu::vec2(0.f, 1.f), mathfu::vec2(1.f, 1.f),
      mathfu::vec2(-1.f, 0.f), mathfu::vec2(0.f, 0.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, TopLeftAlignment_LeftDownFill) {
  // Layout 5 children in a 3x3 grid using TopLeft alignment, but with a
  // LeftDown fill order. They should be arranged in the following manner.
  //  2 1 0
  //  4 3 -
  //  - - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(1.f, 1.f), mathfu::vec2(0.f, 1.f),  mathfu::vec2(-1.f, 1.f),
      mathfu::vec2(0.f, 0.f), mathfu::vec2(-1.f, 0.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_LeftDown;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, TopCenterAlignment) {
  // Layout 5 children in a 3x3 grid using TopCenter alignment.  They should be
  // arranged in the following manner.
  //  0 1 2
  //   3 4
  //  - - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-1.f, 1.f),  mathfu::vec2(0.f, 1.f),  mathfu::vec2(1.f, 1.f),
      mathfu::vec2(-0.5f, 0.f), mathfu::vec2(0.5f, 0.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Center;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, TopRightAlignment) {
  // Layout 5 children in a 3x3 grid using TopRight alignment.  They should be
  // arranged in the following manner.
  //  0 1 2
  //  - 3 4
  //  - - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-1.f, 1.f), mathfu::vec2(0.f, 1.f), mathfu::vec2(1.f, 1.f),
      mathfu::vec2(0.f, 0.f),  mathfu::vec2(1.f, 0.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Right;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, CenterCenterAlignment) {
  // Layout 2 children using a CenterCenter alignment.
  const Entity small_box = CreateChild(1.0f);
  const Entity big_box = CreateChild(2.0f);
  const std::vector<Entity> children{small_box, big_box};

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Center;
  params.horizontal_alignment = LayoutHorizontalAlignment_Center;
  params.row_alignment = LayoutVerticalAlignment_Center;
  params.canvas_size = mathfu::vec2(3.f, 3.f);

  ApplyLayout(registry_.get(), params, children);

  const Sqt* small_sqt = transform_system_->GetSqt(small_box);
  const Sqt* big_sqt = transform_system_->GetSqt(big_box);

  // Both should be centered vertically.
  EXPECT_NEAR(0.0f, big_sqt->translation.y, kEpsilon);
  EXPECT_NEAR(0.0f, small_sqt->translation.y, kEpsilon);

  // The small box moves twice as far, and the distance between centers is half
  // the sum of the sizes.
  EXPECT_NEAR(0.5f, big_sqt->translation.x, kEpsilon);
  EXPECT_NEAR(-1.0f, small_sqt->translation.x, kEpsilon);
}

TEST_F(LayoutTest, TopLeftAlignment_DownRightFill) {
  // Layout 5 children in a 3x3 grid using TopLeft alignment.  They should be
  // arranged in the following manner.
  //  0 3 -
  //  1 4 -
  //  2 - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-1.f, 1.f),  mathfu::vec2(-1.f, 0.f),
      mathfu::vec2(-1.f, -1.f), mathfu::vec2(0.f, 1.f),
      mathfu::vec2(0.f, 0.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_DownRight;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, TopRightAlignment_DownRightFill) {
  // Layout 5 children in a 3x3 grid using TopRight alignment.  They should be
  // arranged in the following manner.
  //  - 0 3
  //  - 1 4
  //  - 2 -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(0.f, 1.f),  mathfu::vec2(0.f, 0.f),
      mathfu::vec2(0.f, -1.f), mathfu::vec2(1.f, 1.f),
      mathfu::vec2(1.f, 0.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Right;
  params.fill_order = LayoutFillOrder_DownRight;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, CanvasBottom_DownRightFill) {
  // Layout 5 children in an uneven 5x3 grid using BottomLeft alignment.
  // They should be arranged in the following manner.
  //  0 - - - -
  //  1 3 - - -
  //  2 4 - - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-2.f, 1.f),  mathfu::vec2(-2.f, 0.f),
      mathfu::vec2(-2.f, -1.f), mathfu::vec2(-1.f, 0.f),
      mathfu::vec2(-1.f, -1.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Bottom;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_DownRight;
  params.canvas_size = mathfu::vec2(5.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, ColAlignmentLeft_DownRightFill) {
  // Layout 2 unequally sized children using DownRight fill.
  // They should be arranged in the following manner.
  //  0 - -
  //  1 1 1
  //  1 1 1
  //  1 1 1
  //  - - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-1.f, 2.f), mathfu::vec2(0.f, 0.f),
  };

  LayoutParams params;
  params.fill_order = LayoutFillOrder_DownRight;
  params.column_alignment = LayoutHorizontalAlignment_Left;
  params.canvas_size = mathfu::vec2(3.f, 5.f);
  params.elements_per_wrap = 2;

  const std::vector<Entity> children = CreateChildren(2);
  ResizeEntityX(children[1], 3.f);
  ResizeEntityY(children[1], 3.f);
  LayoutAndAssertTranslations(params, children, expectations);
}

TEST_F(LayoutTest, ColAlignmentRight_DownRightFill) {
  // Layout 2 unequally sized children using DownRight fill.
  // They should be arranged in the following manner.
  //  - - 0
  //  1 1 1
  //  1 1 1
  //  1 1 1
  //  - - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(1.f, 2.f), mathfu::vec2(0.f, 0.f),
  };

  LayoutParams params;
  params.fill_order = LayoutFillOrder_DownRight;
  params.column_alignment = LayoutHorizontalAlignment_Right;
  params.canvas_size = mathfu::vec2(3.f, 5.f);
  params.elements_per_wrap = 2;

  const std::vector<Entity> children = CreateChildren(2);
  ResizeEntityX(children[1], 3.f);
  ResizeEntityY(children[1], 3.f);
  LayoutAndAssertTranslations(params, children, expectations);
}

TEST_F(LayoutTest, CanvasSize_Empty) {
  // If there are no children, preserve the given canvas size
  mathfu::vec2 canvas_size(3.f, 3.f);

  LayoutParams params;
  params.canvas_size = canvas_size;
  std::vector<Entity> children;

  Aabb aabb = ApplyLayout(registry_.get(), params, children);
  mathfu::vec2 layout = aabb.max.xy() - aabb.min.xy();

  EXPECT_NEAR(canvas_size.x, layout.x, kEpsilon);
  EXPECT_NEAR(canvas_size.y, layout.y, kEpsilon);
}

TEST_F(LayoutTest, CanvasSize_Empty_ShrinkToFit) {
  // But, if there are no children and shrink_to_fit is true, canvas_size will
  // be ignored and we will end up with size 0.
  mathfu::vec2 canvas_size(3.f, 3.f);

  LayoutParams params;
  params.canvas_size = canvas_size;
  params.shrink_to_fit = true;
  std::vector<Entity> children;

  Aabb aabb = ApplyLayout(registry_.get(), params, children);
  mathfu::vec2 layout = aabb.max.xy() - aabb.min.xy();

  EXPECT_NEAR(0, layout.x, kEpsilon);
  EXPECT_NEAR(0, layout.y, kEpsilon);
}

TEST_F(LayoutTest, CanvasSize_NonEmpty_ShrinkToFit) {
  // We have a 4x4 canvas, and three 1x1 entities.
  // They should be arranged as follows:
  // 0 - - -
  // 1 - - -
  // 2 - - -
  // - - - -
  // Here, we want the canvas to shrink around them, into a 1x3 shape.
  mathfu::vec2 canvas_size(4.f, 4.f);

  LayoutParams params;
  params.canvas_size = canvas_size;
  params.shrink_to_fit = true;
  params.fill_order = LayoutFillOrder_DownRight;

  auto children = CreateChildren(3);

  Aabb aabb = ApplyLayout(registry_.get(), params, children);
  EXPECT_NEAR(-2, aabb.min.x, kEpsilon);
  EXPECT_NEAR(-1, aabb.min.y, kEpsilon);

  EXPECT_NEAR(-1, aabb.max.x, kEpsilon);
  EXPECT_NEAR(2, aabb.max.y, kEpsilon);
}

TEST_F(LayoutTest, TopLeftAlignment_DownLeftFill) {
  // Layout 5 children in a 3x3 grid using DownLeft fill.  They should be
  // arranged in the following manner.
  //  3 0 -
  //  4 1 -
  //  - 2 -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(0.f, 1.f),  mathfu::vec2(0.f, 0.f), mathfu::vec2(0.f, -1.f),
      mathfu::vec2(-1.f, 1.f), mathfu::vec2(-1.f, 0.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_DownLeft;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, TopLeftAlignment_RightUpFill) {
  // Layout 5 children in a 3x3 grid using RightUp fill.  They should be
  // arranged in the following manner.
  //  3 4 -
  //  0 1 2
  //  - - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-1.f, 0.f), mathfu::vec2(0.f, 0.f), mathfu::vec2(1.f, 0.f),
      mathfu::vec2(-1.f, 1.f), mathfu::vec2(0.f, 1.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_RightUp;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, TopLeftAlignment_LeftUpFill) {
  // Layout 5 children in a 3x3 grid using LeftUp fill.  They should be
  // arranged in the following manner.
  //  4 3 -
  //  2 1 0
  //  - - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(1.f, 0.f), mathfu::vec2(0.f, 0.f), mathfu::vec2(-1.f, 0.f),
      mathfu::vec2(0.f, 1.f), mathfu::vec2(-1.f, 1.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_LeftUp;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, TopLeftAlignment_UpRightFill) {
  // Layout 5 children in a 3x3 grid using UpRight fill.  They should be
  // arranged in the following manner.
  //  2 4 -
  //  1 3 -
  //  0 - -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-1.f, -1.f), mathfu::vec2(-1.f, 0.f),
      mathfu::vec2(-1.f, 1.f),  mathfu::vec2(0.f, 0.f), mathfu::vec2(0.f, 1.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_UpRight;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, TopLeftAlignment_UpLeftFill) {
  // Layout 5 children in a 3x3 grid using UpLeft fill.  They should be
  // arranged in the following manner.
  //  4 2 -
  //  3 1 -
  //  - 0 -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(0.f, -1.f), mathfu::vec2(0.f, 0.f), mathfu::vec2(0.f, 1.f),
      mathfu::vec2(-1.f, 0.f), mathfu::vec2(-1.f, 1.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_UpLeft;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 3;

  LayoutChildrenAndAssertTranslations(params, expectations, 5);
}

TEST_F(LayoutTest, BottomRightAlignment_UpLeftFill_Overflow) {
  // Layout 13 children in a 3x3 grid using BottomRightAlignment & UpLeft fill.
  // There will be overflow, so they should be arranged in the following manner.
  //    11 7 3
  //    10 6 2
  //     9*5*1
  // 12  8 4 0
  // Cell 5 is at the origin, and cells 12, 11, 7, 3 all are outside the canvas
  // size.
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(1.f, -1.f), mathfu::vec2(1.f, 0.f), mathfu::vec2(1.f, 1.f),
      mathfu::vec2(1.f, 2.f), mathfu::vec2(0.f, -1.f), mathfu::vec2(0.f, 0.f),
      mathfu::vec2(0.f, 1.f), mathfu::vec2(0.f, 2.f), mathfu::vec2(-1.f, -1.f),
      mathfu::vec2(-1.f, 0.f), mathfu::vec2(-1.f, 1.f), mathfu::vec2(-1.f, 2.f),
      mathfu::vec2(-2.f, -1.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Bottom;
  params.horizontal_alignment = LayoutHorizontalAlignment_Right;
  params.fill_order = LayoutFillOrder_UpLeft;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 4;

  LayoutChildrenAndAssertTranslations(params, expectations, 13);
}

TEST_F(LayoutTest, BottomRightAlignment_LeftUpFill_Overflow) {
  // Layout 13 children in a 3x3 grid using BottomRightAlignment & LeftUp fill.
  // There will be overflow, so they should be arranged in the following manner.
  //          12
  // 11 10  9  8
  //  7  6 *5* 4
  //  3  2  1  0
  // Cell 5 is at the origin, and cells 12, 11, 7, 3 all are outside the canvas
  // size.
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(1.f, -1.f), mathfu::vec2(0.f, -1.f),
      mathfu::vec2(-1.f, -1.f), mathfu::vec2(-2.f, -1.f),
      mathfu::vec2(1.f, 0.f), mathfu::vec2(0.f, 0.f), mathfu::vec2(-1.f, 0.f),
      mathfu::vec2(-2.f, 0.f), mathfu::vec2(1.f, 1.f), mathfu::vec2(0.f, 1.f),
      mathfu::vec2(-1.f, 1.f), mathfu::vec2(-2.f, 1.f), mathfu::vec2(1.f, 2.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Bottom;
  params.horizontal_alignment = LayoutHorizontalAlignment_Right;
  params.fill_order = LayoutFillOrder_LeftUp;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.elements_per_wrap = 4;

  LayoutChildrenAndAssertTranslations(params, expectations, 13);
}

TEST_F(LayoutTest, WeightedElements) {
  // Layout 3 children in a 1x7 grid.  The weighted children will expand to
  // fill the unused space in the following manner.
  // 0 0 0 - 1 - 2
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-2.f, 0.f), mathfu::vec2(1.f, 0.f), mathfu::vec2(3.f, 0.f),
  };
  const DesiredSize desired_sizes[] = {
      DesiredSize(3.f, kUnchanged), DesiredSize(1.f, kUnchanged),
      kUnchangedSize,
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_RightDown;
  params.canvas_size = mathfu::vec2(7.f, 1.f);
  params.spacing = mathfu::vec2(1.f, 0.f);
  params.elements_per_wrap = 3;

  const std::vector<Entity> children = CreateChildren(3);
  std::vector<LayoutElement> elements;
  CreateElementParams(&elements, children);
  elements[0].horizontal_weight = 3.0;
  elements[1].horizontal_weight = 1.0;

  ApplyLayout(registry_.get(), params, elements, set_pos_fn_, kParent);

  AssertTranslations(children, expectations);
  AssertDesiredSizesAndEnabled(children, desired_sizes);
}

TEST_F(LayoutTest, WeightedElements_Vertical) {
  // Layout 6 children in a 3x7 grid.  The weighted children will expand to
  // fill the unused space in the following manner.
  // 0 3 -
  // 0 - -
  // 0 4 -
  // - 4 -
  // 1 - -
  // - 5 -
  // 2 5 -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-1.f, 2.f), mathfu::vec2(-1.f, -1.f),
      mathfu::vec2(-1.f, -3.f), mathfu::vec2(0.f, 3.f),
      mathfu::vec2(0.f, 0.5f), mathfu::vec2(0.f, -2.5f),
  };
  const DesiredSize desired_sizes[] = {
      DesiredSize(kUnchanged, 3.f), DesiredSize(kUnchanged, 1.f),
      kUnchangedSize, kUnchangedSize,
      DesiredSize(kUnchanged, 2.f), DesiredSize(kUnchanged, 2.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_DownRight;
  params.canvas_size = mathfu::vec2(3.f, 7.f);
  params.spacing = mathfu::vec2(0.f, 1.f);
  params.elements_per_wrap = 3;

  const std::vector<Entity> children = CreateChildren(6);
  std::vector<LayoutElement> elements;
  CreateElementParams(&elements, children);
  elements[0].vertical_weight = 3.0;
  elements[1].vertical_weight = 1.0;
  elements[4].vertical_weight = 10.0;
  elements[5].vertical_weight = 10.0;

  ApplyLayout(registry_.get(), params, elements, set_pos_fn_, kParent);

  AssertTranslations(children, expectations);
  AssertDesiredSizesAndEnabled(children, desired_sizes);
}

TEST_F(LayoutTest, WeightedElements_Disabled) {
  // Layout 3 children in a 3x7 grid.  The weighted children will expand to
  // fill the unused space in the following manner.
  // 2 2 2 2 2 2 2
  // - - - - - - -
  // - - - - - - -
  // The fixed element is so large it disables all other elements.
  // And, no spacing is left over.

  const DesiredSize desired_sizes[] = {
      kUnchangedSize, kUnchangedSize,
      kUnchangedSize
  };
  const bool enabled_expectations[] = {
      false, false, true,
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_RightDown;
  params.canvas_size = mathfu::vec2(7.f, 3.f);
  params.spacing = mathfu::vec2(1.f, 0.f);
  params.elements_per_wrap = 3;

  const std::vector<Entity> children = CreateChildren(3);
  ResizeEntityX(children[2], 7.f);

  std::vector<LayoutElement> elements;
  CreateElementParams(&elements, children);
  elements[0].horizontal_weight = 3.0;
  elements[1].horizontal_weight = 1.0;

  ApplyLayout(registry_.get(), params, elements, set_pos_fn_, kParent);

  const Sqt* sqt = transform_system_->GetSqt(elements[2].entity);
  EXPECT_NEAR(0.f, sqt->translation.x, kEpsilon);
  EXPECT_NEAR(1.f, sqt->translation.y, kEpsilon);

  AssertDesiredSizesAndEnabled(children, desired_sizes, enabled_expectations);
}

TEST_F(LayoutTest, WeightedElements_OuterWeight) {
  // Layout 8 children in a 3x11 grid.  The children are weighted in the
  // secondary direction and will fill the unused space in the following manner.
  // 0 1 -  (fixed 1) (fixed 1)
  // - - -
  // 2 3 -  (weight 2) (fixed 1)
  // 2 - -
  // - - -
  // 4 5 -  (fixed 2) (weight 1)
  // 4 5 -
  // - - -
  // 6 7 -  (weight 1) (weight 3)
  // 6 7 -
  // 6 7 -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-1.f, 5.f), mathfu::vec2(0.f, 5.f),
      mathfu::vec2(-1.f, 2.5f), mathfu::vec2(0.f, 3.f),
      mathfu::vec2(-1.f, -0.5f), mathfu::vec2(0.f, -0.5f),
      mathfu::vec2(-1.f, -4.f), mathfu::vec2(0.f, -4.f),
  };
  const DesiredSize desired_sizes[] = {
      kUnchangedSize, kUnchangedSize,
      DesiredSize(kUnchanged, 2.f), kUnchangedSize,
      kUnchangedSize, DesiredSize(kUnchanged, 2.f),
      DesiredSize(kUnchanged, 3.f), DesiredSize(kUnchanged, 3.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_RightDown;
  params.canvas_size = mathfu::vec2(3.f, 11.f);
  params.spacing = mathfu::vec2(0.f, 1.f);
  params.elements_per_wrap = 2;

  const std::vector<Entity> children = CreateChildren(8);
  ResizeEntityY(children[4], 2.f);

  std::vector<LayoutElement> elements;
  CreateElementParams(&elements, children);
  elements[2].vertical_weight = 2.0;
  elements[5].vertical_weight = 1.0;
  elements[6].vertical_weight = 1.0;
  elements[7].vertical_weight = 3.0;

  ApplyLayout(registry_.get(), params, elements, set_pos_fn_, kParent);

  AssertTranslations(children, expectations);
  AssertDesiredSizesAndEnabled(children, desired_sizes);
}

TEST_F(LayoutTest, WeightedElements_OuterHidden) {
  // Layout 4 children in a 3x3 grid.  The first two children are weighted in
  // the secondary direction, but the third child is too tall so it will take up
  // all the size instead and hide the first two children.  Also there will be
  // no spacing above the third child.
  // 2 3 -
  // 2 3 -
  // 2 3 -
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(0.f, 0.f), mathfu::vec2(0.f, 0.f),  // both ignored
      mathfu::vec2(-1.f, 0.f), mathfu::vec2(0.f, 0.f),
  };
  const DesiredSize desired_sizes[] = {
      kUnchangedSize, kUnchangedSize,
      kUnchangedSize, DesiredSize(kUnchanged, 3.f),
  };
  const bool enabled_expectations[] = {
      false, false, true, true
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_RightDown;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.spacing = mathfu::vec2(0.f, 1.f);
  params.elements_per_wrap = 2;

  const std::vector<Entity> children = CreateChildren(4);
  ResizeEntityY(children[2], 3.f);

  std::vector<LayoutElement> elements;
  CreateElementParams(&elements, children);
  elements[0].vertical_weight = 1.0;
  elements[1].vertical_weight = 2.0;
  elements[3].vertical_weight = 3.0;

  ApplyLayout(registry_.get(), params, elements, set_pos_fn_, kParent);

  AssertTranslations(children, expectations);
  AssertDesiredSizesAndEnabled(children, desired_sizes, enabled_expectations);
}

TEST_F(LayoutTest, InsertIndex) {
  // Layout 4 children in a 3x3 grid.
  // 0 - 1
  // - - -
  // 2 - 3
  const mathfu::vec2 expectations[] = {
      mathfu::vec2(-1.f, 1.f), mathfu::vec2(1.f, 1.f),
      mathfu::vec2(-1.f, -1.f), mathfu::vec2(1.f, -1.f),
  };

  LayoutParams params;
  params.vertical_alignment = LayoutVerticalAlignment_Top;
  params.horizontal_alignment = LayoutHorizontalAlignment_Left;
  params.fill_order = LayoutFillOrder_RightDown;
  params.canvas_size = mathfu::vec2(3.f, 3.f);
  params.spacing = mathfu::vec2(1.f, 1.f);
  params.elements_per_wrap = 2;
  CachedPositions cached_positions;

  // If layout has not occurred yet, or there were no children, don't fail and
  // just return 0.
  EXPECT_EQ(0u,
      CalculateInsertIndexForPosition(cached_positions, mathfu::kZeros3f));
  ApplyLayout(registry_.get(), params, {}, set_pos_fn_, kParent,
              &cached_positions);
  EXPECT_EQ(0u,
      CalculateInsertIndexForPosition(cached_positions, mathfu::kZeros3f));


  const std::vector<Entity> children = CreateChildren(4);
  std::vector<LayoutElement> elements;
  CreateElementParams(&elements, children);
  ApplyLayout(registry_.get(), params, elements, set_pos_fn_, kParent,
              &cached_positions);
  AssertTranslations(children, expectations);

  // Horizontal first fill orders.
  {
    const mathfu::vec3 positions[] = {
      {-2.5f, 0.1f, 0.0f}, {-0.5f, 0.5f, 0.0f},
      {1.5f, 1.5f, 0.0f}, {-2.5f, -0.1f, 0.0f},
      {0.5f, -1.5f, 0.0f}, {2.0f, -1.0f, 0.0f},
    };
    const std::pair<LayoutFillOrder, std::vector<size_t>> fill_orders[] = {
      {LayoutFillOrder_RightDown, {0, 1, 2, 2, 3, 4}},
      {LayoutFillOrder_LeftDown, {2, 1, 0, 4, 3, 2}},
      {LayoutFillOrder_RightUp, {2, 3, 4, 0, 1, 2}},
      {LayoutFillOrder_LeftUp, {4, 3, 2, 2, 1, 0}},
    };
    for (const auto& fill_order : fill_orders) {
      params.fill_order = fill_order.first;
      ApplyLayout(registry_.get(), params, elements, set_pos_fn_, kParent,
                  &cached_positions);
      for (size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(fill_order.second[i],
            CalculateInsertIndexForPosition(cached_positions, positions[i]));
      }
    }
  }
  // Vertical first fill orders.
  {
    const mathfu::vec3 positions[] = {
      {-0.1f, 2.5f, 0.0f}, {-0.5f, 0.5f, 0.0f},
      {-1.5f, -1.5f, 0.0f}, {0.1f, 2.5f, 0.0f},
      {1.5f, -0.5f, 0.0f}, {1.0f, -2.0f, 0.0f},
    };
    const std::pair<LayoutFillOrder, std::vector<size_t>> fill_orders[] = {
      {LayoutFillOrder_DownRight, {0, 1, 2, 2, 3, 4}},
      {LayoutFillOrder_DownLeft, {2, 3, 4, 0, 1, 2}},
      {LayoutFillOrder_UpRight, {2, 1, 0, 4, 3, 2}},
      {LayoutFillOrder_UpLeft, {4, 3, 2, 2, 1, 0}},
    };
    for (const auto& fill_order : fill_orders) {
      params.fill_order = fill_order.first;
      ApplyLayout(registry_.get(), params, elements, set_pos_fn_, kParent,
                  &cached_positions);
      for (size_t i = 0; i < 6; ++i) {
        EXPECT_EQ(fill_order.second[i],
            CalculateInsertIndexForPosition(cached_positions, positions[i]));
      }
    }
  }
}

}  // namespace
}  // namespace lull
