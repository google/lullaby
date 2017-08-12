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

#ifndef LULLABY_SYSTEMS_LAYOUT_LAYOUT_SYSTEM_H_
#define LULLABY_SYSTEMS_LAYOUT_LAYOUT_SYSTEM_H_

#include <queue>
#include <unordered_set>

#include "lullaby/events/entity_events.h"
#include "lullaby/events/layout_events.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/layout/layout.h"

namespace lull {

// Controls all children's translation using the Layout utility.
class LayoutSystem : public System {
 public:
  explicit LayoutSystem(Registry* registry);

  ~LayoutSystem() override;

  // Specifies a specific entity as the root of a layout.
  void Create(Entity e, HashValue type, const Def* def) override;

  // Creates a layout component for an Entity with the provided params.
  void Create(Entity e, const LayoutParams& params);

  // Disassociates all layout data from the Entity.
  void Destroy(Entity e) override;

  // Sets the LayoutParams for the specified layout.
  void SetLayoutParams(Entity e, const LayoutParams& params);

  // Sets individual values within the LayoutParams for the specified layout.
  void SetCanvasSizeX(Entity e, float x);
  void SetCanvasSizeY(Entity e, float y);
  void SetSpacingX(Entity e, float x);
  void SetSpacingY(Entity e, float y);
  void SetFillOrder(Entity e, LayoutFillOrder fill_order);
  void SetHorizontalAlignment(Entity e,
                              LayoutHorizontalAlignment horizontal_alignment);
  void SetVerticalAlignment(Entity e,
                            LayoutVerticalAlignment vertical_alignment);
  void SetRowAlignment(Entity e, LayoutVerticalAlignment row_alignment);
  void SetColumnAlignment(Entity e, LayoutHorizontalAlignment column_alignment);
  void SetElementsPerWrap(Entity e, int elements_per_wrap);
  void SetMaxElements(Entity e, int max_elements);

  // Returns the index among this layout's children that a new element at
  // |world_position| should be inserted to.
  size_t GetInsertIndexForPosition(Entity entity,
                                   const mathfu::vec3& world_position);

  // Updates the layout of the specified entity.  Only call this if you
  // need the entity's children to be updated immediately, as they will
  // automatically be updated on the next AdvanceFrame.
  void Layout(Entity e);

 private:
  struct LayoutComponent : Component {
    explicit LayoutComponent(Entity e);
    std::unique_ptr<LayoutParams> layout = nullptr;
    std::unique_ptr<RadialLayoutParams> radial_layout = nullptr;
    size_t max_elements = 0;
    std::string empty_blueprint = "";
    std::queue<Entity> empty_placeholders;
    CachedPositions cached_positions;
  };

  // The processing done by the LayoutSystem is catagorized into different
  // Layout Passes, which define how it interacts with the LayoutBoxSystem.
  // If multiple passes affect the same layout in the same frame, only the
  // higher priority pass is processed (kOriginal is highest).
  enum LayoutPass {
    // Update the layout's original_box in LayoutBoxSystem using canvas_size.
    // Update the layout's weighted children's desired_size with resize logic.
    kOriginal = 2,

    // Update the layout's actual_box using its desired_size.
    // Update the layout's weighted children's desired_size with resize logic.
    kDesired = 1,

    // Update the layout's actual_box using its desired_size.
    // Do not update the layout's weighted children's desired_size.
    kActual = 0,
    // However, if the layout itself is the source of a ActualBoxChangedEvent,
    // it will SetOriginalBox() instead of SetActualBox(), since that event was
    // a response to one of the layout's previous kOriginal passes.  It still
    // uses its desired_size instead of canvas_size.
  };

  // DirtyLayout represents a layout to be processed.
  class DirtyLayout {
   public:
    explicit DirtyLayout(Entity layout, LayoutPass pass,
                         Entity source = kNullEntity);

    Entity GetLayout() const { return layout_; }

    // Aggregate multiple passes and sources on the same layout to find the
    // highest priority pass and closest source.
    void Update(Registry* registry, LayoutPass new_pass, Entity source);

    // Returns either |old_source| or |new_source|, whichever is closest to
    // the layout, which could be the layout itself.
    Entity FindClosestParent(Registry* registry, Entity old_source,
                             Entity new_source) const;

    // In kDesired and kActual passes, do not use the layout's canvas_size,
    // instead use the assigned desired_size if it's been set.
    bool ShouldUseDesiredSize() const;

    // Usually in kDesired and kActual passes, SetActualBox() is used, but if
    // the source of the triggering event is the layout itself, use
    // SetOriginalBox() instead.
    bool ShouldSetActualBox() const;

    // This is the desired_source that will be sent to the layout's children in
    // SetDesiredSize().
    Entity GetChildrensDesiredSource() const;

    Entity GetActualSource() const { return actual_source_; }

   private:
    const Entity layout_;
    LayoutPass pass_;
    Entity desired_source_ = kNullEntity;
    Entity actual_source_ = kNullEntity;
  };

  void LayoutImpl(const DirtyLayout& dirty_layout);
  LayoutElement GetLayoutElement(Entity e);
  void ProcessDirty();
  void SetDirty(Entity e, LayoutPass pass, Entity source = kNullEntity);
  void SetParentDirty(Entity e, LayoutPass pass, Entity source = kNullEntity);

  // All of these events can trigger passes, which are labeled alongside.
  // Calling Layout() or any SetLayoutParams()            LayoutPass::kOriginal
  void OnParentChanged(const ParentChangedEvent& ev);  // LayoutPass::kOriginal
  void OnOriginalBoxChanged(Entity e);                 // LayoutPass::kOriginal
  void OnDesiredSizeChanged(
      const DesiredSizeChangedEvent& event);           // LayoutPass::kDesired
  void OnAabbChanged(Entity e);                        // LayoutPass::kActual
  void OnActualBoxChanged(
      const ActualBoxChangedEvent& event);             // LayoutPass::kActual

  ComponentPool<LayoutComponent> layouts_;
  std::unordered_map<Entity, LayoutElement> layout_elements_;
  std::unordered_map<Entity, DirtyLayout> dirty_layouts_;

  LayoutSystem(const LayoutSystem&) = delete;
  LayoutSystem& operator=(const LayoutSystem&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::LayoutSystem);

#endif  // LULLABY_SYSTEMS_LAYOUT_LAYOUT_SYSTEM_H_
