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

#ifndef LULLABY_MODULES_LAYOUT_LAYOUT_H_
#define LULLABY_MODULES_LAYOUT_LAYOUT_H_

#include <vector>

#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "lullaby/generated/common_generated.h"
#include "lullaby/modules/ecs/entity.h"
#include "lullaby/util/registry.h"
#include "lullaby/util/math.h"

namespace lull {

// Parameters used by ApplyLayout to determine how to layout Entities.
struct LayoutParams {
  // The virtual "canvas" on which the entities will be positioned.
  mathfu::vec2 canvas_size = mathfu::kZeros2f;

  // The spacing between entities.
  mathfu::vec2 spacing = mathfu::kZeros2f;

  // The order in which elements will be added to the canvas.
  LayoutFillOrder fill_order = LayoutFillOrder_RightDown;

  // The anchor at which the entities will be aligned relative to the canvas
  // horizontally.
  LayoutHorizontalAlignment horizontal_alignment =
      LayoutHorizontalAlignment_Left;
  // The anchor at which the entities will be aligned relative to the canvas
  // vertically.
  LayoutVerticalAlignment vertical_alignment = LayoutVerticalAlignment_Top;
  // The anchor at which entities will be aligned relative to their row within
  // the layout.
  LayoutVerticalAlignment row_alignment = LayoutVerticalAlignment_Top;
  // The anchor at which entities will be aligned relative to their col within
  // the layout.
  LayoutHorizontalAlignment column_alignment = LayoutHorizontalAlignment_Left;

  // The number of elements in the orders primary direction before wrapping.
  size_t elements_per_wrap = 0;

  // If true then the AABB of the layout will be set to the extent of the
  // layout's children. If false then it will be set to the size of the canvas.
  bool shrink_to_fit = false;
};

// Parameters used by ApplyRadialLayout
struct RadialLayoutParams {
  // The distance around the circumference between each entity.
  float degrees_per_element = 30.f;

  // The vector defining the major axis of the ellipse to place entities on.
  // An Entity at 0 degrees will be at |major_axis|, and at 180 degrees will be
  // - |major_axis|
  mathfu::vec3 major_axis = mathfu::kAxisX3f;
  // The vector defining the minor axis of the ellipse to place entities on.
  // An Entity at 90 degrees will be at |minor_axis|, and at 270 degrees will be
  // - |minor_axis|
  mathfu::vec3 minor_axis = mathfu::kAxisY3f;
};

// Data for each child element of a layout.  Elements without weight will
// will get their stated size.  The remaining unused size will get distributed
// proportionally to all elements with weight.  If there is no unused size all
// weighted children are disabled.
struct LayoutElement {
  explicit LayoutElement(Entity e) : entity(e) {}

  Entity entity = kNullEntity;

  // If horizontal_weight is non-zero, then this element will fill up available
  // space proportional to the total weight of all other weighted elements
  // horizontally up to canvas_size.x if non-zero.
  float horizontal_weight = 0.0f;

  // If vertical_weight is non-zero, then this element will fill up available
  // space proportional to the total weight of all other weighted elements
  // vertically up to canvas_size.y if non-zero.
  float vertical_weight = 0.0f;
};

// Data saved in ApplyLayout() that can be used to
// CalculateInsertIndexForPosition().
struct CachedPositions {
  bool is_horizontal_first = true;
  bool is_inner_forward = true;
  // Boundaries between rows in the secondary direction.  There will be one
  // fewer element in this list compared to the actual number of rows.
  std::vector<float> secondary_positions;

  // The |positions| of each element in the primary direction and |index| among
  // the original |elements| from ApplyLayout().
  using PositionIndex = std::pair<float, size_t>;
  // A row or column of elements, based on the primary direction of the
  // fill_order.
  using RowOfPositions = std::vector<PositionIndex>;
  using GridOfRows = std::vector<RowOfPositions>;

  // There will be exactly the same amount of IndexPositions as |elements| in
  // ApplyLayout().
  GridOfRows primary_positions;
};

// DEPRECATED
// Update the positions of the |entities| based on the |params|.  Returns
// the total AABB that is filled up by the entities.
Aabb ApplyLayout(Registry* registry, const LayoutParams& params,
                 const std::vector<Entity>& entities);

// Update the positions of the entities in |infos| based on the |params|.
// If |desired_source| is set it will also SetDesiredSize() in LayoutBoxSystem
// with that source.
// Returns the total AABB that is filled up by the entities.
Aabb ApplyLayout(Registry* registry, const LayoutParams& params,
                 const std::vector<LayoutElement>& elements,
                 Entity desired_source = kNullEntity,
                 CachedPositions* cached_positions = nullptr);

// Update the positions of the |entities| based on the ellipse defined by the
// params.
void ApplyRadialLayout(Registry* registry, const std::vector<Entity>& entities,
                       const RadialLayoutParams& params);

// Given the |cached_positions| from a previous ApplyLayout(), return the index
// within |elements| that a new element at |local_position| should be inserted
// to.
size_t CalculateInsertIndexForPosition(const CachedPositions& cached_positions,
                                       const mathfu::vec3& local_position);

}  // namespace lull

#endif  // LULLABY_MODULES_LAYOUT_LAYOUT_H_
