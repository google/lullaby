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

#ifndef LULLABY_UTIL_LAYOUT_MANAGER_H_
#define LULLABY_UTIL_LAYOUT_MANAGER_H_

#include <functional>
#include <vector>

#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

/// @enum LayoutGroupAlignment
///
/// @brief Alignment of the groups.
///
/// @note: `kTop` and `kLeft`(as well as `kBottom` and
/// `kRight`) are intended to be aliases of one another, as they both
/// express the same thing on their respective axis.
///
/// **Enumerations**:
///
/// * `kTop` or `kLeft` (`1`) - Align along the top (or left, depending on the
///                             axis).
/// * `kCenter` (`2`) - Align along the center of the axis.
/// * `kBottom` or `kRight` (`3`) - Align along the bottom (or right, depending
///                                 on the axis).
enum class LayoutGroupAlignment {
  kTop = 1,
  kLeft = 1,
  kCenter = 2,
  kBottom = 3,
  kRight = 3
};

/// @enum LayoutGroupDirection
///
/// @brief Direction of the groups.
///
/// **Enumerations**:
///
/// * `kHorizontal` (`4`) - The direction of the group is horizontal (x-axis).
/// * `kVertical` (`8`) - The direction of the group is vertical (y-axis).
/// * `kOverlay` (`12`) - The group of elements are placed on top of one another
///                       (along the z-axis).
enum class LayoutGroupDirection {
  kHorizontal = 4,
  kVertical = 8,
  kOverlay = 12
};

/// @var kDefaultLayoutResolution
///
/// @brief The default virtual resolution, if none is set.
const float kDefaultLayoutResolution = 1000.0f;

// This holds the transient state of a group while its layout is being
// calculated / rendered.
class LayoutGroup {
 public:
  LayoutGroup()
      : direction_(LayoutGroupDirection::kHorizontal),
        align_(LayoutGroupAlignment::kTop),
        spacing_(0),
        size_(mathfu::kZeros2i),
        position_(mathfu::kZeros2i),
        element_idx_(0),
        margin_(mathfu::kZeros4i) {}

  LayoutGroup(LayoutGroupDirection direction, LayoutGroupAlignment align,
              int spacing, size_t element_idx)
      : direction_(direction),
        align_(align),
        spacing_(spacing),
        size_(mathfu::kZeros2i),
        position_(mathfu::kZeros2i),
        element_idx_(element_idx),
        margin_(mathfu::kZeros4i) {}

  // Extend this group with the size of a new element, and possibly spacing
  // if it wasn't the first element.
  void Extend(const mathfu::vec2i& extension) {
    switch (direction_) {
      case LayoutGroupDirection::kHorizontal:
        size_ = mathfu::vec2i(size_.x + extension.x + (size_.x ? spacing_ : 0),
                              std::max(size_.y, extension.y));
        break;
      case LayoutGroupDirection::kVertical:
        size_ = mathfu::vec2i(std::max(size_.x, extension.x),
                              size_.y + extension.y + (size_.y ? spacing_ : 0));
        break;
      case LayoutGroupDirection::kOverlay:
        size_ = mathfu::vec2i(std::max(size_.x, extension.x),
                              std::max(size_.y, extension.y));
        break;
    }
  }

  LayoutGroupDirection direction_;
  LayoutGroupAlignment align_;
  int spacing_;
  mathfu::vec2i size_;
  mathfu::vec2i position_;
  size_t element_idx_;
  mathfu::vec4i margin_;
};

// This holds the transient state while a layout is being performed.
// Call Run() on an instance to layout a definition.
class LayoutManager : public LayoutGroup {
 public:
  explicit LayoutManager(const mathfu::vec2i& canvas_size)
      : LayoutGroup(LayoutGroupDirection::kVertical,
                    LayoutGroupAlignment::kLeft, 0, 0),
        layout_pass_(true),
        canvas_size_(canvas_size),
        virtual_resolution_(kDefaultLayoutResolution) {
    SetScale();
  }

  // Changes the virtual resolution (defaults to kDefaultLayoutResolution).
  // All floating point sizes below for elements are in terms of this
  // resolution, which will then be converted to physical (pixel) based integer
  // coordinates during layout.
  void SetVirtualResolution(float virtual_resolution) {
    if (layout_pass_) {
      virtual_resolution_ = virtual_resolution;
      SetScale();
    }
  }

  mathfu::vec2 GetVirtualResolution() const {
    return mathfu::vec2(canvas_size_) / pixel_scale_;
  }

  // Helpers to convert entire vectors between virtual and physical coordinates.
  template <int D>
  mathfu::Vector<int, D> VirtualToPhysical(
      const mathfu::Vector<float, D>& v) const {
    return mathfu::Vector<int, D>(v * pixel_scale_ + 0.5f);
  }

  template <int D>
  mathfu::Vector<float, D> PhysicalToVirtual(
      const mathfu::Vector<int, D>& v) const {
    return mathfu::Vector<float, D>(v) / pixel_scale_;
  }

  // Retrieve the scaling factor for the virtual resolution.
  float GetScale() const { return pixel_scale_; }

  // Determines placement for the UI as a whole inside the available space
  // (screen).
  void PositionGroup(LayoutGroupAlignment horizontal,
                     LayoutGroupAlignment vertical,
                     const mathfu::vec2& offset) {
    if (!layout_pass_) {
      auto space = canvas_size_ - size_;
      position_ = AlignDimension(horizontal, 0, space) +
                  AlignDimension(vertical, 1, space) +
                  VirtualToPhysical(offset);
    }
  }

  // Switch from the layout pass to the second pass (for positioning and
  // rendering etc).
  // Layout happens in two passes, where the first computes the sizes of
  // things, and the second assigns final positions based on that. As such,
  // you define your layout using a function (where you call StartGroup /
  // EndGroup / ELement etc.) which you call once before and once
  // after this function.
  // See the implementation of Run() below.
  bool StartSecondPass() {
    // If you hit this assert, you are missing an EndGroup().
    assert(!group_stack_.size());

    // Do nothing if there is no elements.
    if (elements_.size() == 0) return false;

    // Put in a sentinel element. We'll use this element to point to
    // when a group didn't exist during layout but it does during rendering.
    NewElement(mathfu::kZeros2i);

    position_ = mathfu::kZeros2i;
    size_ = elements_[0].size;

    layout_pass_ = false;
    element_it_ = elements_.begin();

    return true;
  }

  // Set the margin for the current group.
  void SetMargin(const mathfu::vec4& margin) {
    margin_ = VirtualToPhysical(margin);
  }

  // Generic element with user supplied renderer.
  void Element(const mathfu::vec2& virtual_size,
               const std::function<void(const mathfu::vec2i& pos,
                                        const mathfu::vec2i& size)>& renderer) {
    if (layout_pass_) {
      auto size = VirtualToPhysical(virtual_size);
      NewElement(size);
      Extend(size);
    } else {
      auto element = NextElement();
      if (element) {
        const mathfu::vec2i pos = Position(*element);
        if (renderer) {
          renderer(pos, element->size);
        }
        Advance(element->size);
      }
    }
  }

  // An element that has sub-elements.
  void StartGroup(const LayoutGroup& group) {
    StartGroup(group.direction_, group.align_,
               static_cast<float>(group.spacing_));
  }

  // An element that has sub-elements. Tracks its state in an instance of
  // Layout, that is pushed/popped from the stack as needed.
  void StartGroup(LayoutGroupDirection direction, LayoutGroupAlignment align,
                  float spacing) {
    LayoutGroup layout(direction, align, static_cast<int>(spacing),
                       elements_.size());
    group_stack_.push_back(*this);
    if (layout_pass_) {
      NewElement(mathfu::kZeros2i);
    } else {
      auto element = NextElement();
      if (element) {
        layout.position_ = Position(*element);
        layout.size_ = element->size;
        // Make layout refer to element it originates from, iterator points
        // to next element after the current one.
        layout.element_idx_ = element_it_ - elements_.begin() - 1;
      } else {
        // This group did not exist during layout, but since all code inside
        // this group will run, it is important to have a valid element_idx_
        // to refer to, so we point it to our (empty) sentinel element:
        layout.element_idx_ = elements_.size() - 1;
      }
    }
    *static_cast<LayoutGroup*>(this) = layout;
  }

  // Clean up the Group element started by StartGroup()
  void EndGroup() {
    // If you hit this assert, you have one too many EndGroup().
    assert(group_stack_.size());

    auto size = size_;
    auto margin = margin_.xy() + margin_.zw();
    auto element_idx = element_idx_;
    *static_cast<LayoutGroup*>(this) = group_stack_.back();
    group_stack_.pop_back();
    if (layout_pass_) {
      size += margin;
      // Contribute the size of this group to its parent.
      Extend(size);
      // Set the size of this group as the size of the element tracking it.
      elements_[element_idx].size = size;
    } else {
      Advance(elements_[element_idx].size);
    }
  }

  // Return the position for the current enclosing StartGroup/EndGroup.
  const mathfu::vec2i& GroupPosition() const { return position_; }

  // Return the size for the current enclosing StartGroup/EndGroup.
  mathfu::vec2i GroupSize() const {
    return size_ + elements_[element_idx_].extra_size;
  }

  // Run two passes of layout.
  void Run(const std::function<void()>& layout_definition) {
    layout_definition();
    StartSecondPass();
    layout_definition();
  }

 protected:
  // We create one of these per GUI element, so new fields should only be
  // added when absolutely necessary.
  struct UIElement {
    explicit UIElement(const mathfu::vec2i& _size)
        : size(_size), extra_size(mathfu::kZeros2i), interactive(false) {}
    mathfu::vec2i size;  // Minimum on-screen size computed by layout pass.
    mathfu::vec2i extra_size;  // Additional size in a scrolling area (TODO:rem)
    bool interactive;          // Wants to respond to user input.
  };

  // (second pass): retrieve the next corresponding cached element we
  // created in the layout pass.
  UIElement* NextElement() {
    assert(!layout_pass_);
    if (element_it_ != elements_.end()) {
      auto& element = *element_it_;
      ++element_it_;
      return &element;
    }
    return nullptr;
  }

  // (layout pass): create a new element.
  void NewElement(const mathfu::vec2i& size) {
    assert(layout_pass_);
    elements_.push_back(UIElement(size));
  }

  // (second pass): move the group's current position past an element of
  // the given size.
  void Advance(const mathfu::vec2i& size) {
    assert(!layout_pass_);
    switch (direction_) {
      case LayoutGroupDirection::kHorizontal:
        position_ += mathfu::vec2i(size.x + spacing_, 0);
        break;
      case LayoutGroupDirection::kVertical:
        position_ += mathfu::vec2i(0, size.y + spacing_);
        break;
      case LayoutGroupDirection::kOverlay:
        // Keep at starting position.
        break;
    }
  }

  // (second pass): return the top-left position of the current element, as a
  // function of the group's current position and the alignment.
  mathfu::vec2i Position(const UIElement& element) const {
    assert(!layout_pass_);
    auto pos = position_ + margin_.xy();
    auto space = size_ - element.size - margin_.xy() - margin_.zw();
    switch (direction_) {
      case LayoutGroupDirection::kHorizontal:
        pos += AlignDimension(align_, 1, space);
        break;
      case LayoutGroupDirection::kVertical:
        pos += AlignDimension(align_, 0, space);
        break;
      case LayoutGroupDirection::kOverlay:
        pos += AlignDimension(align_, 0, space);
        pos += AlignDimension(align_, 1, space);
        break;
    }
    return pos;
  }

 private:
  // Compute a space offset for a particular alignment for just the x or y
  // dimension.
  static mathfu::vec2i AlignDimension(LayoutGroupAlignment align, int dim,
                                      const mathfu::vec2i& space) {
    mathfu::vec2i dest(0, 0);
    switch (align) {
      case LayoutGroupAlignment::kTop:  // Same as Left.
        break;
      case LayoutGroupAlignment::kCenter:
        dest[dim] += space[dim] / 2;
        break;
      case LayoutGroupAlignment::kBottom:  // Same as Right.
        dest[dim] += space[dim];
        break;
    }
    return dest;
  }

  // Initialize the scaling factor for the virtual resolution.
  void SetScale() {
    mathfu::vec2 scale = mathfu::vec2(canvas_size_) / virtual_resolution_;
    pixel_scale_ = std::min(scale.x, scale.y);
  }

 protected:
  bool layout_pass_;
  std::vector<UIElement> elements_;
  std::vector<UIElement>::iterator element_it_;
  std::vector<LayoutGroup> group_stack_;
  mathfu::vec2i canvas_size_;
  float virtual_resolution_;
  float pixel_scale_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_LAYOUT_MANAGER_H_
