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

#ifndef LULLABY_SYSTEMS_LAYOUT_LAYOUT_BOX_SYSTEM_H_
#define LULLABY_SYSTEMS_LAYOUT_LAYOUT_BOX_SYSTEM_H_

#include <unordered_map>

#include "lullaby/events/layout_events.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/math.h"
#include "lullaby/util/optional.h"

namespace lull {

/// Stores data for separate steps of the Layout process so that other systems
/// such as Text or NinePatch can resize without getting stuck in an infinite
/// loop.
///
/// - When a Client system such as Text or NinePatch creates a new mesh or
///   detects a change in the params used to generate their mesh, they should
///   call SetOriginalBox() to signal their requested maximum box.
///
/// - Layouts such as LayoutSystem respond to the OriginalBoxChangedEvent and
///   perform calculations, then call SetDesiredSize() if resizing is necessary.
///   They use the requested original_box for these calculations and put
///   themselves as the source parameter.
///
/// - Client systems respond to DesiredSizeChangedEvent by calling
///   SetActualBox() with their final box, which should be smaller than or equal
///   to desired_size.  They should use the source from the Event to pass into
///   the SetActualBox() method.
///
/// - Layouts can recalculate after ActualBoxChangedEvent with the most updated
///   actual_box. The Layout should save the result as SetActualBox() and keep
///   the same source. However, if they are source of the event, then they can
///   save their result as a SetOriginalBox() instead.
///
/// IMPORTANT:
/// - Clients cannot SetOriginalSize() in response to a DesiredSizeChangedEvent.
/// - Clients must remember the source and send it back in SetActualBox().
/// - Layouts cannot SetDesiredSize() in response to a ActualBoxChangedEvent.
/// - Layouts, if they are not the source of a Desired or ActualChangedEvent,
///   need to keep the same source for any new events they create.
/// - Layouts can use SetOriginalBox() instead of SetActualBox() if they are the
///   source.
/// Maintaining these rules will ensure resizing occurs without triggering
/// infinite loops. Also, the initial Layout spawns "jobs" to Clients with
/// itself as the source in SetDesiredSize().  When the responses come back
/// from those Clients, and the ActualEvent has the Layout as the source, the
/// Layout can now finish its calculations with its own SetOriginalBox().
///
/// Layouts should use GetOriginalBox() before resizing, but only GetActualBox()
/// for final positioning calculations.  SetOriginalBox() will also set
/// actual_box in case Clients don't know how to handle DesiredSizeChangedEvent.
///
/// DesiredSizeChangedEvent is sent *immediately* so that fast Clients can fix
/// their mesh in the same frame before Layouts perform positioning
/// calculations.
///
/// For backwards compatibility, if OriginalBox or ActualBox have not been Set,
/// Get will still return TransformSystem's Aabb as a fallback.

class LayoutBoxSystem : public System {
 public:
  explicit LayoutBoxSystem(Registry* registry);

  ~LayoutBoxSystem() override;

  // Disassociates all layout data from the Entity.
  void Destroy(Entity e) override;

  // Triggers OriginalBoxChangedEvent on next frame.
  // Also sets actual_box in case a Client doesn't support
  // DesiredSizeChangedEvent.
  void SetOriginalBox(Entity e, const Aabb& original_box);

  // Gets the original_box for |e|, or transform's Aabb if not previously set.
  const Aabb* GetOriginalBox(Entity e) const;

  // Triggers DesiredSizeChangeEvent *immediately*. The original Layout
  // initiating this call will put themselves as the source.
  void SetDesiredSize(Entity e, Entity source, const Optional<float>& x,
                      const Optional<float>& y, const Optional<float>& z);

  // Gets each dimension of desired_size independently for |e| if its been set.
  Optional<float> GetDesiredSizeX(Entity e) const;
  Optional<float> GetDesiredSizeY(Entity e) const;
  Optional<float> GetDesiredSizeZ(Entity e) const;

  // Triggers ActualBoxChangedEvent on next frame. Clients responding to a
  // DesiredSizeChangedEvent should pass the source from that event into this
  // method.
  void SetActualBox(Entity e, Entity source, const Aabb& actual_box);

  // Gets the actual_box for |e|, or transform's Aabb if not previously set.
  const Aabb* GetActualBox(Entity e) const;

 private:
  struct LayoutBox {
    Optional<Aabb> original_box;
    Optional<float> desired_size_x;
    Optional<float> desired_size_y;
    Optional<float> desired_size_z;
    Optional<Aabb> actual_box;
  };

  // If the box doesn't exist yet, but the transform does, we will create one.
  LayoutBox* GetOrCreateLayoutBox(Entity e);
  // This won't create a new LayoutBox if it doesn't already exist.
  const LayoutBox* GetLayoutBox(Entity e) const;

  std::unordered_map<Entity, LayoutBox> layout_boxes_;

  LayoutBoxSystem(const LayoutBoxSystem&) = delete;
  LayoutBoxSystem& operator=(const LayoutBoxSystem&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::LayoutBoxSystem);

#endif  // LULLABY_SYSTEMS_LAYOUT_LAYOUT_BOX_SYSTEM_H_
