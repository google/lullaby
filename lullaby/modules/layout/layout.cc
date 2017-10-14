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

#include "lullaby/modules/layout/layout.h"

#include <algorithm>

#include "flatui/internal/flatui_layout.h"
#include "lullaby/systems/layout/layout_box_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"

namespace lull {

namespace {

// Converts a lull::LayoutHorizontalAlignment to it's flatui equivalent.
flatui::Alignment HorizontalAlignmentToFlatUi(LayoutHorizontalAlignment align) {
  switch (align) {
    case LayoutHorizontalAlignment_Left:
      return flatui::kAlignLeft;
      break;
    case LayoutHorizontalAlignment_Center:
      return flatui::kAlignCenter;
      break;
    case LayoutHorizontalAlignment_Right:
      return flatui::kAlignRight;
      break;
  }

  return flatui::kAlignLeft;
}

// Converts a lull::LayoutVerticalAlignment to it's flatui equivalent.
flatui::Alignment VerticalAlignmentToFlatUi(LayoutVerticalAlignment align) {
  switch (align) {
    case LayoutVerticalAlignment_Top:
      return flatui::kAlignTop;
    case LayoutVerticalAlignment_Center:
      return flatui::kAlignCenter;
    case LayoutVerticalAlignment_Bottom:
      return flatui::kAlignBottom;
  }

  return flatui::kAlignTop;
}

size_t CalculateElementsPerWrap(size_t elements_per_wrap, size_t total_count) {
  return elements_per_wrap > 0 ? elements_per_wrap : total_count;
}

// Computes the number of groups needed when a |total_count| of elements is
// divided into |count_per_group| elements.
size_t CalculateNumberOfGroups(size_t total_count, size_t count_per_group) {
  if (count_per_group == 0) {
    return total_count;
  }
  const size_t rounded_toward_zero = total_count / count_per_group;
  const size_t intermediate_product = rounded_toward_zero * count_per_group;
  if (intermediate_product < total_count) {
    return rounded_toward_zero + 1;
  } else {
    return rounded_toward_zero;
  }
}

// Primary direction is forward, eg. Right* or Down*
bool IsInnerForwardFillOrder(LayoutFillOrder fill_order) {
  switch (fill_order) {
    case LayoutFillOrder_RightDown:
    case LayoutFillOrder_RightUp:
    case LayoutFillOrder_DownRight:
    case LayoutFillOrder_DownLeft:
      return true;
    default:
      return false;
  }
}

// Primary direction is backward, eg. Left* or Up*
bool IsInnerBackwardFillOrder(LayoutFillOrder fill_order) {
  switch (fill_order) {
    case LayoutFillOrder_LeftDown:
    case LayoutFillOrder_LeftUp:
    case LayoutFillOrder_UpRight:
    case LayoutFillOrder_UpLeft:
      return true;
    default:
      return false;
  }
}

// Layout supports 8 total types of fill_orders, which have a primary direction
// and an orthogonal secondary direction.
// We need to account for our orientation when using FlatUI, which is origined
// at the TopLeft corner, traversing forward toward Right and Down.

// A structure to encapsulate the traversal of the primary row of child
// elements.
struct InnerIndexRange {
  LayoutFillOrder fill_order;
  size_t start = 0;  // Always the minimum index of a row
  size_t size = 0;  // Size of a row, will be capped by total_size

  // outer_idx is the index in the secondary direction
  // outer_count is the count of primary rows in secondary direction
  // elements_per_wrap is the length of the primary rows
  // total_size is the count of all the elements
  InnerIndexRange(LayoutFillOrder fill_order_, size_t outer_idx,
                  size_t outer_count, size_t elements_per_wrap,
                  size_t total_size) : fill_order(fill_order_) {
    size_t index;
    if (IsOuterForward()) {
      index = outer_idx;
    } else if (IsOuterBackward()) {
      index = ReverseIndex(outer_idx, outer_count);
    } else {
      LOG(DFATAL) << "Invalid fill order: " << fill_order;
      return;
    }

    // Make sure the last index doesn't exceed the total_size.
    start = index * elements_per_wrap;
    size = std::min((index + 1) * elements_per_wrap, total_size) - start;
  }

  // i is the index within: [0, size - 1]
  size_t GetInnerIndex(size_t i) const {
    // We iterate each group forward or backwards based on the fill_order.
    if (IsInnerForwardFillOrder(fill_order)) {
      return start + i;
    } else if (IsInnerBackwardFillOrder(fill_order)) {
      return start + ReverseIndex(i, size);
    } else {
      return 0;
    }
  }

  // Get the index of an array if iterating in reverse
  size_t ReverseIndex(size_t index, size_t size_) const {
    if (index >= size_) {
      LOG(DFATAL) << "Invalid index: " << index << " and size: " << size_;
      return 0;
    }
    return size_ - index - 1;
  }

  // Secondary direction is forward, eg. *Down or *Right
  bool IsOuterForward() const {
    switch (fill_order) {
      case LayoutFillOrder_RightDown:
      case LayoutFillOrder_LeftDown:
      case LayoutFillOrder_DownRight:
      case LayoutFillOrder_UpRight:
        return true;
      default:
        return false;
    }
  }

  // Secondary direction is backward, eg. *Left or *Up
  bool IsOuterBackward() const {
    switch (fill_order) {
      case LayoutFillOrder_DownLeft:
      case LayoutFillOrder_UpLeft:
      case LayoutFillOrder_RightUp:
      case LayoutFillOrder_LeftUp:
        return true;
      default:
        return false;
    }
  }
};

struct DesiredSize {
  Optional<float> x;
  Optional<float> y;
  Optional<float> z;
  bool IsHidden() const {
    return (x && *x == 0) || (y && *y == 0) || (z && *z == 0);
  }
  bool IsChanged() const {
    return x || y || z;
  }
};

// A class to determine the inner size and weight based on fill_order axis.
// It can access the final desired_size that will be set.
class InnerElement {
 public:
  InnerElement(bool horizontal_first, const LayoutElement& element,
               const LayoutBoxSystem* layout_box_system,
               DesiredSize* desired_size)
      : horizontal_first_(horizontal_first), desired_size_(desired_size) {
    const mathfu::vec3 entity_size =
      layout_box_system->GetOriginalBox(element.entity)->Size();

    if (horizontal_first_) {
      weight_ = element.horizontal_weight;
      outer_weight_ = element.vertical_weight;
      size_ = entity_size.x;
      outer_size_ = entity_size.y;
    } else {
      weight_ = element.vertical_weight;
      outer_weight_ = element.horizontal_weight;
      size_ = entity_size.y;
      outer_size_ = entity_size.x;
    }
  }

  // Weight of an element in the primary direction
  float GetWeight() const { return weight_; }

  // Size of an element in the primary direction
  float GetSize() const { return size_; }

  // Used by OuterElement, these are values in the orthogonal secondary
  // direction.
  float GetOuterWeight() const { return outer_weight_; }
  float GetOuterSize() const { return outer_size_; }

  void SetInnerDesiredSize(float inner_size) {
    if (horizontal_first_) {
      desired_size_->x = inner_size;
    } else {
      desired_size_->y = inner_size;
    }
  }

  void SetOuterDesiredSize(float outer_size) {
    if (horizontal_first_) {
      desired_size_->y = outer_size;
    } else {
      desired_size_->x = outer_size;
    }
  }

 private:
  float weight_ = 0.0f;
  float size_ = 0.0f;
  float outer_weight_ = 0.0f;
  float outer_size_ = 0.0f;
  const bool horizontal_first_;
  DesiredSize* desired_size_;
};

// A class to help calculate the size and weight in the secondary direction.
class OuterElement {
 public:
  // Calculations just use the max of all values in the secondary direction
  // of elements in the inner_group row.
  void UpdateFromInnerElement(const InnerElement& inner_element) {
    if (inner_element.GetOuterWeight() > 0) {
      weight_ = std::max(weight_, inner_element.GetOuterWeight());
    } else {
      size_ = std::max(size_, inner_element.GetOuterSize());
    }
  }

  // Max weight of one row in the secondary direction
  void SetWeight(float weight) { weight_ = weight; }

  float GetWeight() const { return weight_; }

  // Max size of one row in the secondary direction
  float GetSize() const { return size_; }

  // Store the calculated size based on weight.
  void SetWeightedSize(float weighted_size) { weighted_size_ = weighted_size; }

  // The desired size is the max of size_ or weighted_size_, in case the
  // weight was relatively low and superceded by another element in the row.
  float GetDesiredSize() const {
    return std::max(size_, weighted_size_);
  }

  // If all of the elements in a row are hidden by weight, don't create the
  // inner_group at all so there isn't extra spacing.
  void SetAllHiddenByWeight(bool all_hidden_by_weight) {
    all_hidden_by_weight_ = all_hidden_by_weight;
  }

  bool GetAllHiddenByWeight() const { return all_hidden_by_weight_; }

 private:
  float weight_ = 0.0f;
  float size_ = 0.0f;
  float weighted_size_ = 0.0f;
  bool all_hidden_by_weight_ = false;
};

// A class to help traversal of elements by creating InnerIndexRanges and
// InnerElements.
class ApplyLayoutContext {
 public:
  ApplyLayoutContext(const LayoutParams &params,
                     const std::vector<LayoutElement>& elements,
                     LayoutBoxSystem* layout_box_system,
                     std::vector<DesiredSize>* desired_sizes)
      : fill_order_(params.fill_order), elements_(elements),
        layout_box_system_(layout_box_system), desired_sizes_(desired_sizes) {
    if (IsHorizontalFirstFillOrder()) {
      horizontal_first_ = true;
    } else if (IsVerticalFirstFillOrder()) {
      horizontal_first_ = false;
    } else {
      LOG(DFATAL) << "Invalid fill order: " << fill_order_;
    }

    elements_per_wrap_ =
        CalculateElementsPerWrap(params.elements_per_wrap, elements_.size());
    outer_count_ =
        CalculateNumberOfGroups(elements_.size(), elements_per_wrap_);
  }

  bool IsHorizontalFirst() const { return horizontal_first_; }

  size_t GetElementsPerWrap() const { return elements_per_wrap_; }

  size_t GetOuterCount() const { return outer_count_; }

  InnerIndexRange GetRangeForOuter(size_t outer_idx) const {
    return InnerIndexRange(fill_order_, outer_idx, outer_count_,
                           elements_per_wrap_, elements_.size());
  }

  InnerElement GetInnerElementInRange(const InnerIndexRange& range, size_t i) {
    const size_t index = range.GetInnerIndex(i);
    const LayoutElement& element = elements_[index];
    DesiredSize* desired_size = &desired_sizes_->at(index);
    return InnerElement(horizontal_first_, element, layout_box_system_,
                        desired_size);
  }

 private:
  // Primary direction is horizontal, eg. Right* or Left*
  bool IsHorizontalFirstFillOrder() const {
    switch (fill_order_) {
      case LayoutFillOrder_RightDown:
      case LayoutFillOrder_LeftDown:
      case LayoutFillOrder_RightUp:
      case LayoutFillOrder_LeftUp:
        return true;
      default:
        return false;
    }
  }

  // Primary direction is vertical, eg. Down* or Up*
  bool IsVerticalFirstFillOrder() const {
    switch (fill_order_) {
      case LayoutFillOrder_DownRight:
      case LayoutFillOrder_DownLeft:
      case LayoutFillOrder_UpRight:
      case LayoutFillOrder_UpLeft:
        return true;
      default:
        return false;
    }
  }

  const LayoutFillOrder fill_order_;
  const std::vector<LayoutElement>& elements_;
  bool horizontal_first_;
  size_t elements_per_wrap_;
  size_t outer_count_;
  LayoutBoxSystem* layout_box_system_;
  std::vector<DesiredSize>* desired_sizes_;
};

// If it has weight, sum it up.  Otherwise, it is unresizable and should
// account for used size.
void UpdateWeightAndSize(float weight, float size,
                         float* total_weight, float* used_size) {
  if (weight > 0) {
    *total_weight += weight;
  } else {
    *used_size += size;
  }
}

float CalculateFreeSizePerWeight(float used_size, float total_weight,
                                 float spacing, float canvas_size,
                                 size_t count) {
  // Add spacing as part of used size.
  used_size += spacing * static_cast<float>(count - 1);
  // Minimum free size is 0.
  return std::max(0.0f, canvas_size - used_size) / total_weight;
}

float CalculateChildSizeFromWeight(float weight, float size_per_weight) {
  return weight * size_per_weight;
}

// Calculate the primary direction weight and sizes.
void ApplyLayoutInnerDesired(const float inner_spacing,
                             const float inner_canvas_size,
                             ApplyLayoutContext* context,
                             std::vector<OuterElement>* outer_elements) {
  for (size_t outer_idx  = 0; outer_idx < outer_elements->size(); ++outer_idx) {
    const InnerIndexRange range = context->GetRangeForOuter(outer_idx);
    if (range.size == 0) {
      return;
    }

    float total_inner_weight = 0.0f;
    float used_inner_size = 0.0f;
    for (size_t i = 0; i < range.size; ++i) {
      const InnerElement inner_element =
          context->GetInnerElementInRange(range, i);
      // Inner_element will have the values for the outer direction, and we will
      // only use the max for outer direction calculations.
      outer_elements->at(outer_idx).UpdateFromInnerElement(inner_element);
      UpdateWeightAndSize(inner_element.GetWeight(), inner_element.GetSize(),
                          &total_inner_weight, &used_inner_size);
    }

    if (total_inner_weight > 0) {
      const float free_size_per_weight = CalculateFreeSizePerWeight(
          used_inner_size, total_inner_weight, inner_spacing, inner_canvas_size,
          range.size);
      // Apply desired_size to all children
      for (size_t i = 0; i < range.size; ++i) {
        InnerElement inner_element = context->GetInnerElementInRange(range, i);

        if (inner_element.GetWeight() > 0) {
          const float child_inner_size =
              CalculateChildSizeFromWeight(inner_element.GetWeight(),
                                           free_size_per_weight);
          inner_element.SetInnerDesiredSize(child_inner_size);
        }
      }
    }
  }
}

// Calculate the secondary direction weight and sizes.
// Each iteration will either succeed by assigning a size to the weight
// increment that works, or remove a weight from a inner_group row and
// restart.  Eventually there will be no weights and it terminates, or it
// works meaning that the assigned size based on weight is greater than or
// equal to any fixed size elements in that row.
void ApplyLayoutOuterDesired(const float outer_spacing,
                             const float outer_canvas_size,
                             ApplyLayoutContext* context,
                             std::vector<OuterElement>* outer_elements) {
  // There should be at most one iteration per element, since each iteration
  // will remove the weight from at least one element if it doesnt exit.
  for (size_t iter = 0; iter <= outer_elements->size(); ++iter) {
    if (iter == outer_elements->size()) {
      LOG(DFATAL) << "Exceeded maximum iterations for resizing outer elements: "
                  << outer_elements->size();
      break;
    }

    float total_outer_weight = 0.0f;
    float used_outer_size = 0.0f;
    for (size_t outer_idx = 0; outer_idx < outer_elements->size();
         ++outer_idx) {
      OuterElement& outer_element = outer_elements->at(outer_idx);
      UpdateWeightAndSize(outer_element.GetWeight(), outer_element.GetSize(),
                          &total_outer_weight, &used_outer_size);
    }

    // There is no weight to calculate, done.
    if (total_outer_weight == 0) {
      break;
    }

    const float free_outer_size_per_weight = CalculateFreeSizePerWeight(
        used_outer_size, total_outer_weight, outer_spacing, outer_canvas_size,
        outer_elements->size());

    // Check that the calculated size is greater than or equal to any
    // fixed size.  If any aren't, ignore those weights and restart.
    // But, only restart if there is more weight to calculate anyway.
    bool undersized = false;
    bool more_weight = false;
    for (size_t outer_idx = 0; outer_idx < outer_elements->size();
         ++outer_idx) {
      OuterElement& outer_element = outer_elements->at(outer_idx);
      if (outer_element.GetWeight() > 0) {
        const float child_outer_size =
            CalculateChildSizeFromWeight(outer_element.GetWeight(),
                                         free_outer_size_per_weight);
        if (child_outer_size < outer_element.GetSize()) {
          // Don't use weight, just leave as original max_size.
          outer_element.SetWeight(0.0f);
          outer_element.SetWeightedSize(outer_element.GetSize());
          undersized = true;
        } else {
          outer_element.SetWeightedSize(child_outer_size);
          more_weight = true;
        }
      }
    }

    // No children are undersized, we are done.
    if (!undersized) {
      break;
    }

    // There is no more weight to process, we are done.
    if (!more_weight) {
      break;
    }

    // Some children are undersized and there is more weight left, so restart
    // with some weight removed.
  }

  // Now go and update all weighted children with their calculated size.
  // Note that any weighted children in the same row will get the same size,
  // based on max_weight, no matter their original weight because they'll
  // just fill available space.
  // Also, if max_size is greater, use that instead.
  for (size_t outer_idx = 0; outer_idx < outer_elements->size(); ++outer_idx) {
    const InnerIndexRange range = context->GetRangeForOuter(outer_idx);
    if (range.size == 0) {
      continue;
    }
    OuterElement& outer_element = outer_elements->at(outer_idx);

    // If any are showing, then the row is not all_hidden_by_weight.
    bool any_showing = false;
    for (size_t i = 0; i < range.size; ++i) {
      InnerElement inner_element = context->GetInnerElementInRange(range, i);
      // Apply weighted size to any element with outer_weight.
      if (inner_element.GetOuterWeight() > 0) {
        const float desired_size = outer_element.GetDesiredSize();
        inner_element.SetOuterDesiredSize(desired_size);
        if (desired_size > 0) {
          any_showing = true;
        }
      } else {
        any_showing = true;  // If no weight it counts as showing.
      }
    }
    // If are none showing, then the row is all_hidden_by_weight.
    outer_element.SetAllHiddenByWeight(!any_showing);
  }
  // We successfully allocated sizes to weighted children, done.
}

void ApplyLayoutSetDesired(const std::vector<LayoutElement>& elements,
                           const std::vector<DesiredSize>& desired_sizes,
                           Entity desired_source,
                           TransformSystem* transform_system,
                           LayoutBoxSystem* layout_box_system) {
  for (size_t i = 0; i < elements.size(); ++i) {
    const LayoutElement& element = elements[i];
    const Entity entity = element.entity;
    const DesiredSize& desired_size = desired_sizes[i];
    if (desired_size.IsHidden()) {
      transform_system->Disable(entity);
    } else if (element.horizontal_weight > 0 || element.vertical_weight > 0) {
      transform_system->Enable(entity);
      if (desired_size.IsChanged()) {
        layout_box_system->SetDesiredSize(entity, desired_source,
                                          desired_size.x, desired_size.y,
                                          desired_size.z);
      }
    }
  }
}

// A helper class to calculate the correct data to store in CachedPositions
// for CalculateInsertIndexForPosition().
class CachedPositionsCalculator {
 public:
  CachedPositionsCalculator(bool is_horizontal_first, bool is_inner_forward,
      size_t outer_count, size_t elements_per_wrap,
      CachedPositions* cached_positions)
      : is_horizontal_first_(is_horizontal_first),
        cached_positions_(cached_positions), outer_idx_set_(false),
        current_outer_idx_(0), min_(mathfu::kZeros2f), max_(mathfu::kZeros2f) {
    if (!cached_positions_) {
      return;
    }
    cached_positions_->secondary_positions.clear();
    cached_positions_->secondary_positions.reserve(outer_count);
    cached_positions_->primary_positions.resize(outer_count);
    for (auto& row : cached_positions_->primary_positions) {
      row.clear();
      row.reserve(elements_per_wrap);
    }
    cached_positions_->is_horizontal_first = is_horizontal_first;
    cached_positions_->is_inner_forward = is_inner_forward;
  }

  // This should be called once per Element with the final bounds in the root
  // Layout's coordinate space.  It is expected to be called in the same order
  // which FlatUI processes elements, top-left to bottom-right.
  void UpdateWithPositions(size_t outer_idx, size_t index,
      const mathfu::vec2& entity_min, const mathfu::vec2& entity_max) {
    if (!cached_positions_) {
      return;
    }

    if (outer_idx >= cached_positions_->primary_positions.size()) {
      LOG(DFATAL) << "Exceeded rows in primary_positions: " << outer_idx;
      return;
    }
    const mathfu::vec2 position = (entity_max + entity_min) / 2.0f;
    cached_positions_->primary_positions[outer_idx].emplace_back(
        is_horizontal_first_ ? position.x : position.y, index);

    // We have encountered a new row, so insert a boundary.  In the y direction,
    // the boundary is at the bottom (min), whereas in the x direction, the
    // boundary is at the right (max).
    if (outer_idx != current_outer_idx_) {
      cached_positions_->secondary_positions.push_back(
          is_horizontal_first_ ? min_.y : max_.x);
      // Reset the min/max of the new row.
      min_ = entity_min;
      max_ = entity_max;
    } else if (outer_idx_set_) {
      // Otherwise, we are still in the same row, so keep track of the min/max.
      min_ = mathfu::vec2::Min(min_, entity_min);
      max_ = mathfu::vec2::Min(max_, entity_max);
    } else {
      // This is the very first element, so just keep it.
      min_ = entity_min;
      max_ = entity_max;
    }
    outer_idx_set_ = true;
    current_outer_idx_ = outer_idx;
  }

  // Call this once after all Elements are processed to correct offsets and
  // ordering.
  void Finalize(const mathfu::vec2& spacing) {
    if (!cached_positions_) {
      return;
    }

    // Add half spacing so the boundary is in the middle of the spacing between
    // rows.
    const mathfu::vec2 half_spacing = spacing / 2.f;
    for (auto& secondary_position : cached_positions_->secondary_positions) {
      if (is_horizontal_first_) {
        secondary_position -= half_spacing.y;
      } else {
        secondary_position += half_spacing.x;
      }
    }

    // Flatui always creates elements top-down, which in lullaby's coordinate
    // system is y positive to negative.  Reverse the y axis so that both x
    // and y are ordered increasing and we can use the same comparison for
    // lookup.
    if (is_horizontal_first_) {
      // The secondary direction is y.
      std::reverse(cached_positions_->secondary_positions.begin(),
                   cached_positions_->secondary_positions.end());
      std::reverse(cached_positions_->primary_positions.begin(),
                   cached_positions_->primary_positions.end());
    } else {
      // The primary direction is y.
      for (auto& row : cached_positions_->primary_positions) {
        std::reverse(row.begin(), row.end());
      }
      cached_positions_->is_inner_forward =
          !cached_positions_->is_inner_forward;
    }
  }

 private:
  bool is_horizontal_first_;
  CachedPositions* cached_positions_;
  bool outer_idx_set_;
  size_t current_outer_idx_;
  mathfu::vec2 min_;
  mathfu::vec2 max_;
};

// Alias for update function used to set elements' position.
using UpdateFunction =
    std::function<void (Entity entity, size_t outer_idx, size_t index,
                        mathfu::vec2 pos, mathfu::vec2 size)>;
// Function to move all children from flatui.
UpdateFunction ApplyLayoutUpdateFunction(
    const SetLayoutPositionFn& set_pos_fn,
    const LayoutBoxSystem* layout_box_system, const LayoutParams* params,
    bool horizontal_first, const mathfu::vec2* root_pos, bool* min_max_set,
    mathfu::vec2* min, mathfu::vec2* max,
    CachedPositionsCalculator* calculator) {
  return [=](Entity entity, size_t outer_idx, size_t index, mathfu::vec2 pos,
             mathfu::vec2 size) {
    mathfu::vec2 new_pos(pos.x, -pos.y);

    // Flatui element positions are in the top-left corner, but Lullaby
    // positions are centered in the aabb, so adjust accordingly.
    const mathfu::vec2 half_size = size * 0.5f;
    new_pos.x += half_size.x;
    new_pos.y -= half_size.y;

    // Similarly, child entities are centered in the middle of their parent's
    // Aabb, so move them up to the top-left.
    new_pos.x -= params->canvas_size.x * 0.5f;
    new_pos.y += params->canvas_size.y * 0.5f;

    // Enforce alignment by using the "empty" groups as true origin.
    new_pos.x -= root_pos->x;
    new_pos.y += root_pos->y;

    // Adjust for the extra spacing caused by the "empty" fill element.
    if (horizontal_first) {
      new_pos.y += params->spacing.y;
    } else {
      new_pos.x -= params->spacing.x;
    }

    const mathfu::vec2 entity_xy = new_pos;
    const mathfu::vec2 entity_min = entity_xy - half_size;
    const mathfu::vec2 entity_max = entity_xy + half_size;
    calculator->UpdateWithPositions(outer_idx, index, entity_min, entity_max);

    if (*min_max_set) {
      *min = mathfu::vec2::Min(*min, entity_min);
      *max = mathfu::vec2::Max(*max, entity_max);
    } else {
      // We haven't yet set our min and max to meaningful values yet, so we
      // should always replace them.
      *min = entity_min;
      *max = entity_max;

      *min_max_set = true;
    }

    // Finally, adjust for an AABB that is not centered around the origin.
    const Aabb* aabb = layout_box_system->GetActualBox(entity);
    if (aabb) {
      new_pos.x -= 0.5f * (aabb->min.x + aabb->max.x);
      new_pos.y -= 0.5f * (aabb->min.y + aabb->max.y);
    }

    set_pos_fn(entity, new_pos);
  };
}

}  // namespace

// Backwards compatibility
Aabb ApplyLayout(Registry* registry, const LayoutParams& params,
                 const std::vector<Entity>& entities) {
  std::vector<LayoutElement> elements;
  elements.reserve(entities.size());
  for (Entity e : entities) {
    elements.emplace_back(e);
  }
  return ApplyLayout(registry, params, elements,
                     GetDefaultSetLayoutPositionFn(registry));
}

// Uses the flatui::LayoutManager to arrange the specified entities in
// |elements| based on the Layout |params|.
Aabb ApplyLayout(Registry* registry, const LayoutParams& params,
                 const std::vector<LayoutElement>& elements,
                 const SetLayoutPositionFn& set_pos_fn, Entity desired_source,
                 CachedPositions* cached_positions) {
  // The minimum and maximum points making up the area used for this layout.
  mathfu::vec2 min = mathfu::kZeros2f;
  mathfu::vec2 max = mathfu::kZeros2f;
  // If the min and max have been set to a non-default value yet.
  bool min_max_set = false;

  if (!params.shrink_to_fit) {
    // Normally, the parent's aabb will be at least the canvas_size,
    // but could be bigger if the children overflow.
    min = -0.5f * params.canvas_size;
    max = 0.5f * params.canvas_size;
    min_max_set = true;
  }

  if (elements.empty()) {
    return Aabb(mathfu::vec3(min, 0.0f), mathfu::vec3(max, 0.0f));
  }

  // Internally, flatui uses ints for sizes/positions (in pixels) but Lullaby
  // uses floats (in meters).  This scale factor allows for a reasonable
  // mapping as most Lullaby elements are not smaller than mm scale.
  static const float kScaleFactor = 100000.f;

  // Setup the flatui::LayoutManager using the specified canvas size.
  const mathfu::vec2i canvas_size(
      static_cast<int>(kScaleFactor),
      static_cast<int>(kScaleFactor));
  flatui::LayoutManager layout(canvas_size);

  const mathfu::vec2i spacing =
      layout.VirtualToPhysical(params.spacing * kScaleFactor);

  // flatui layouts are done using two types of components: groups and elements.
  // The LayoutManager basically takes element sizes (width + height) and
  // returns a position (x, y) for those elements.  Groups determine the
  // direction (horizontal or vertical) in which sub-elements will be arranged,
  // as well as the margin, alignment, and spacing of those elements.
  //
  // To acheive a grid-like layout, an "outer" group is used to arrange "inner"
  // groups in the secondary order axis.  The "inner" groups are used to
  // arrange elements in the primary order axis.

  flatui::Group inner_group;
  flatui::Group outer_group;
  flatui::Group outermost_group;
  float inner_canvas_size;
  float outer_canvas_size;
  float inner_spacing;
  float outer_spacing;

  auto* transform_system = registry->Get<TransformSystem>();
  auto* layout_box_system = registry->Get<LayoutBoxSystem>();
  std::vector<DesiredSize> desired_sizes(elements.size());
  ApplyLayoutContext context(params, elements, layout_box_system,
                             &desired_sizes);
  std::vector<OuterElement> outer_elements(context.GetOuterCount());

  if (context.IsHorizontalFirst()) {
    inner_group.direction_ = flatui::kDirHorizontal;
    outer_group.direction_ = flatui::kDirVertical;
    outermost_group.direction_ = flatui::kDirHorizontal;

    inner_group.spacing_ = spacing.x;
    outer_group.spacing_ = spacing.y;

    // Set the horizontal alignment of the entire layout collectively.
    outer_group.align_ =
        HorizontalAlignmentToFlatUi(params.horizontal_alignment);
    // Set the vertical alignment of the entire layout collectively.
    outermost_group.align_ =
        VerticalAlignmentToFlatUi(params.vertical_alignment);
    // Set the vertical alignment of entities within a row.
    inner_group.align_ = VerticalAlignmentToFlatUi(params.row_alignment);

    inner_canvas_size = params.canvas_size.x;
    outer_canvas_size = params.canvas_size.y;
    inner_spacing = params.spacing.x;
    outer_spacing = params.spacing.y;
  } else {
    inner_group.direction_ = flatui::kDirVertical;
    outer_group.direction_ = flatui::kDirHorizontal;
    outermost_group.direction_ = flatui::kDirVertical;

    inner_group.spacing_ = spacing.y;
    outer_group.spacing_ = spacing.x;

    // // Set the horizontal alignment of the entire layout collectively.
    outer_group.align_ = VerticalAlignmentToFlatUi(params.vertical_alignment);
    // // Set the vertical alignment of the entire layout collectively.
    outermost_group.align_ =
      HorizontalAlignmentToFlatUi(params.horizontal_alignment);
    // Set the horizontal alignment of entities within a column.
    inner_group.align_ = HorizontalAlignmentToFlatUi(params.column_alignment);

    inner_canvas_size = params.canvas_size.y;
    outer_canvas_size = params.canvas_size.x;
    inner_spacing = params.spacing.y;
    outer_spacing = params.spacing.x;
  }

  if (desired_source != kNullEntity) {
    // If any elements are weighted, we need to resize remaining space
    // proportional to the weight.
    ApplyLayoutInnerDesired(inner_spacing, inner_canvas_size, &context,
                           &outer_elements);
    ApplyLayoutOuterDesired(outer_spacing, outer_canvas_size, &context,
                            &outer_elements);

    // Apply all the desired_sizes. SetDesired is immediate, and if the client
    // Systems respond in this frame, the new actual_boxes will be ready for
    // flatui below in this same frame. Otherwise, layout will have to run again
    // after the actual_box changes for real.  The subsequent iteration will
    // have desired_source = kNullEntity so no infinite loop occurs.
    ApplyLayoutSetDesired(elements, desired_sizes, desired_source,
                          transform_system, layout_box_system);
  }

  // We will keep track of 2 "empty" groups that will each hold one canvas_size
  // dimension.  They will be aligned by flatui in our desired alignment.
  // Afterwards, we will re-origin our children based on these "empty" groups
  // in case our children have overflowed in a direction.
  // An assumption is made here that FlatUI processes elements in the order
  // we give the tree of elements to it, because we need these offsets first in
  // order to apply them on all the children afterward.
  mathfu::vec2 root_pos = mathfu::kZeros2f;
  const auto update_root_x_fn = [&](const mathfu::vec2i& pos,
                                    const mathfu::vec2i& size) {
    const mathfu::vec2 virtual_pos =
        layout.PhysicalToVirtual(pos) / kScaleFactor;
    root_pos.x = virtual_pos.x;
  };
  const auto update_root_y_fn = [&](const mathfu::vec2i& pos,
                                    const mathfu::vec2i& size) {
    const mathfu::vec2 virtual_pos =
        layout.PhysicalToVirtual(pos) / kScaleFactor;
    root_pos.y = virtual_pos.y;
  };

  CachedPositionsCalculator calculator(context.IsHorizontalFirst(),
      IsInnerForwardFillOrder(params.fill_order), context.GetOuterCount(),
      context.GetElementsPerWrap(), cached_positions);
  const auto update_fn = ApplyLayoutUpdateFunction(
      set_pos_fn, layout_box_system, &params, context.IsHorizontalFirst(),
      &root_pos, &min_max_set, &min, &max, &calculator);

  // Break the elements into groups based on the wrapping.
  layout.Run([&]() {
    // Start the group that handles the collective vertical alignment.
    layout.StartGroup(outermost_group, kNullEntity);
    // Add an "empty" group that fills the entire height of the canvas so rows
    // will be aligned correctly within it.
    // Also, we will save the final position of the "empty" group to offset
    // all the children with it if they overflow
    layout.StartGroup(outer_group, kNullEntity);
    if (context.IsHorizontalFirst()) {
      layout.Element(mathfu::vec2(0, params.canvas_size.y) * kScaleFactor,
                     kNullEntity, update_root_y_fn);
    } else {
      layout.Element(mathfu::vec2(params.canvas_size.x, 0) * kScaleFactor,
                     kNullEntity, update_root_x_fn);
    }
    layout.EndGroup();

    // Start the outer group that expands in the "secondary" fill direction.
    layout.StartGroup(outer_group, kNullEntity);

    // Add an "empty" inner group that fills the extents of the canvas so that
    // elements will be aligned to the canvas rather than to themselves.
    // Also, we will save the final position of the "empty" group to offset
    // all the children with it if they overflow
    layout.StartGroup(inner_group, kNullEntity);
    if (context.IsHorizontalFirst()) {
      layout.Element(mathfu::vec2(params.canvas_size.x, 0) * kScaleFactor,
                     kNullEntity, update_root_x_fn);
    } else {
      layout.Element(mathfu::vec2(0, params.canvas_size.y) * kScaleFactor,
                     kNullEntity, update_root_y_fn);
    }
    layout.EndGroup();

    // Add entities as elements.
    for (size_t outer_idx = 0; outer_idx < outer_elements.size();
         ++outer_idx) {
      const InnerIndexRange range = context.GetRangeForOuter(outer_idx);
      if (range.size == 0) {
        continue;
      }

      // If the whole row is hidden by weight, don't show it so there isn't
      // extra spacing.
      if (outer_elements[outer_idx].GetAllHiddenByWeight()) {
        continue;
      }

      // Start the actual inner group for the elements that expands in the
      // "primary" fill direction.
      layout.StartGroup(inner_group, kNullEntity);

      // Use i = [0, size-1] for inner group indexes
      for (size_t i = 0; i < range.size; ++i) {
        const size_t index = range.GetInnerIndex(i);
        const Entity entity = elements[index].entity;

        // Get the 2D size of the entity.
        mathfu::vec2 size;
        const Aabb* aabb = layout_box_system->GetActualBox(entity);
        if (aabb) {
          size.x = (aabb->max.x - aabb->min.x);
          size.y = (aabb->max.y - aabb->min.y);
          size *= kScaleFactor;
        }

        // If the element should be hidden, don't add it so there's no extra
        // spacing.
        if (desired_sizes[index].IsHidden()) {
          continue;
        }

        // Add the entity as an element to the layout.
        layout.Element(size, entity,
            [&](mathfu::vec2i pos, mathfu::vec2i size) {
          const mathfu::vec2 virtual_pos =
              layout.PhysicalToVirtual(pos) / kScaleFactor;
          const mathfu::vec2 virtual_size =
              layout.PhysicalToVirtual(size) / kScaleFactor;
          // Update the position of the entity.
          update_fn(entity, outer_idx, index, virtual_pos, virtual_size);
        });
      }

      // End inner group.
      layout.EndGroup();
    }

    // End outer group.
    layout.EndGroup();
    // End outermost_group.
    layout.EndGroup();
  });
  calculator.Finalize(params.spacing);

  return Aabb(mathfu::vec3(min, 0.0f), mathfu::vec3(max, 0.0f));
}

// This needs to by after ApplyLayout due to compilation ambiguities.
SetLayoutPositionFn GetDefaultSetLayoutPositionFn(Registry* registry) {
  return [=](Entity entity, const mathfu::vec2& position) {
    auto* transform_system = registry->Get<TransformSystem>();
    // Preserve the z, only change xy.
    const mathfu::vec3 translation(
        position, transform_system->GetLocalTranslation(entity).z);
    transform_system->SetLocalTranslation(entity, translation);
  };
}

void ApplyRadialLayout(Registry* registry, const std::vector<Entity>& entities,
                       const RadialLayoutParams& params) {
  auto* transform_system = registry->Get<TransformSystem>();
  float ind = 0.0f;
  for (Entity child : entities) {
    const float angle = ind * params.degrees_per_element * kDegreesToRadians;

    Sqt sqt = *transform_system->GetSqt(child);
    sqt.translation = params.major_axis * std::cos(angle) +
                      params.minor_axis * std::sin(angle);
    transform_system->SetSqt(child, sqt);
    ++ind;
  }
}

size_t CalculateInsertIndexForPosition(const CachedPositions& cached_positions,
                                       const mathfu::vec3& local_position) {
  const float outer_position = cached_positions.is_horizontal_first ?
      local_position.y : local_position.x;
  const float inner_position = cached_positions.is_horizontal_first ?
      local_position.x : local_position.y;
  if (cached_positions.primary_positions.empty()) {
    // Layout has no children, just return 0.
    return 0;
  }

  // Find the correct row in the secondary direction.
  size_t outer_idx = 0;
  while (outer_idx < cached_positions.secondary_positions.size() &&
         outer_position > cached_positions.secondary_positions[outer_idx]) {
    ++outer_idx;
  }
  if (outer_idx >= cached_positions.primary_positions.size()) {
    LOG(DFATAL) << "Invalid cached positions, more secondary positions than "
                << "rows of primary positions.";
    return 0;
  }

  // Find the first existing element we are less than.
  bool found = false;
  size_t index = 0;
  for (const auto& primary_position :
       cached_positions.primary_positions[outer_idx]) {
    index = primary_position.second;
    if (inner_position < primary_position.first) {
      found = true;
      break;
    }
  }

  // If we are not less than any element, then we are past the last one, so
  // increment the index.
  // If we are less than an element, but the fill_order is backwards, also
  // increment the index.
  if ((!found && cached_positions.is_inner_forward) ||
      (found && !cached_positions.is_inner_forward)) {
    ++index;
  }
  return index;
}

}  // namespace lull
