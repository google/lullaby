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

#ifndef LULLABY_SYSTEMS_SCROLL_SCROLL_SYSTEM_H_
#define LULLABY_SYSTEMS_SCROLL_SCROLL_SYSTEM_H_

#include <deque>

#include "lullaby/events/animation_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// The ScrollSystem manages scroll views.  Scroll views are defined by view
// bounds and content bounds.  The content bounds are the virtual position and
// size of the entire field of content, and the view bounds are the size and
// position of the visible window.  The view position/offset can range from the
// minimum content bounds in x and y to the maximum content bounds in x and y
// less the size of the view.
//
// Scroll views are controlled by touchpad input.
class ScrollSystem : public System {
 public:
  // This is used to indicate where the SnapOffsetFunction is being called from.
  enum SnapCallType { kSetBounds, kSetOffset, kEndTouch };

  enum { kHoverPriority = 0 };

  // Function used for determining positions at which the content view can be
  // set.
  // Arguments are: desired offset, gesture direction, the current content
  // bounds, and the location this is being called from
  using SnapOffsetFn = std::function<mathfu::vec2(
      const mathfu::vec2&, InputManager::GestureDirection, const Aabb&,
      SnapCallType)>;
  // Function used to snap the current view to a new position by a delta over
  // time_ms.
  using SnapByDeltaFn = std::function<Optional<mathfu::vec2>(int delta)>;

  explicit ScrollSystem(Registry* registry);
  ~ScrollSystem() override;

  // Register global event handlers.
  void Initialize() override;

  // Creates a scroll view for |entity| using the specified |def|.
  void Create(Entity entity, HashValue type, const Def* def) override;

  // Creates a scroll view for |entity| without the need for a def.  If |entity|
  // already has a scroll view, this sets its |content_bounds|.
  void Create(Entity entity, const Aabb& content_bounds);

  // Deregisters a scroll view.
  void Destroy(Entity entity) override;

  // Processes touch input to control scrolling.
  void AdvanceFrame(Clock::duration delta_time);

  // Activates scrolling for the |entity| regardless of whether or not it is
  // the current hover target.
  void Activate(Entity entity);

  // Deactivates scrolling for the |entity| unless it is the hover target.
  void Deactivate(Entity entity);

  // Calls the snap_by_delta_fn callback set on the view, if applicable. If
  // time_ms is unset or less than 0 it'll automatically be set to be the same
  // as if a user had swiped by the delta manually.
  void SnapByDelta(Entity entity, int delta, float time_ms);

  // Sets view priority on the given |entity|.  If |priority| is greater than
  // kHoverPriority, the view is eligible to receive input while not hovered.
  void SetPriority(Entity entity, int priority);

  // Sets the touch sensitivity for |entity| for response to touchpad input.
  void SetTouchSensitivity(Entity entity,
                           const mathfu::vec2& touch_sensitivity);

  // Sets the callback function to be used in order to snap the view offset to
  // a predetermined position (eg. snapping to a grid).
  void SetSnapOffsetFn(Entity entity, SnapOffsetFn fn);

  // Sets the callback function to be used in order to snap the view offset to
  // a relative position from the current one.
  void SetSnapByDeltaFn(Entity entity, SnapByDeltaFn fn);

  // Sets the content bounds of the |entity|. The scroll system only considers
  // the x and y values of the AABB.
  void SetContentBounds(Entity entity, const Aabb& bounds);

  // Sets the content bounds of the |entity| as above, but clamps view_offset
  // immediately.
  void ForceContentBounds(Entity entity, const Aabb& bounds);

  // Attempts to set |entity|'s view offset to |offset| over |time|.  Returns
  // true if the target position of the scroll has been set. Note this can be
  // overridden by touch input if the scroll view is considered active.
  bool SetViewOffset(
      Entity entity, const mathfu::vec2& offset,
      Clock::duration time = Clock::duration::zero());

  // Attempts to set |entity|'s view offset to |offset| over |time|.  Returns
  // true if the target position of the scroll has been set. During the
  // transition to the new offset touch input on this view will be ignored.
  bool ForceViewOffset(
      Entity entity, const mathfu::vec2& offset,
      Clock::duration time = Clock::duration::zero());

  // Get |entity|'s current view offset.
  mathfu::vec2 GetViewOffset(Entity entity) const;

  // Get current scroll view's touch sensitivity.
  mathfu::vec2 GetTouchSensitivity(Entity entity) const;

 private:
  struct ScrollView : Component {
    explicit ScrollView(Entity e) : Component(e) {}

    Aabb content_bounds;
    mathfu::vec2 view_offset = mathfu::kZeros2f;
    mathfu::vec2 target_offset = mathfu::kZeros2f;
    mathfu::vec2 touch_sensitivity = mathfu::kZeros2f;
    mathfu::vec2 drag_border = mathfu::kZeros2f;
    Clock::duration momentum_time = Clock::duration::zero();
    Clock::duration drag_momentum_time = Clock::duration::zero();
    SnapOffsetFn snap_offset_fn;
    SnapByDeltaFn snap_by_delta_fn;
    int priority = kHoverPriority;
    Dispatcher::ScopedConnection on_animation_complete;
    AnimationId forced_offset_animation = kNullAnimation;
  };

  struct EntityPriorityTuple {
    EntityPriorityTuple(Entity entity, int priority)
        : entity(entity), priority(priority) {}
    Entity entity = kNullEntity;
    int priority = kHoverPriority;
  };

  using ScrollViewPool = ComponentPool<ScrollView>;
  using InputViewQueue = std::deque<EntityPriorityTuple>;

  bool IsTouchControllerConnected() const;

  ScrollView* GetContainerView(Entity e);
  ScrollView* GetViewForInput(Entity e);
  ScrollView* GetActiveInputView();

  void UpdateHoverView();
  void OnStartHover(Entity entity);
  void OnStopHover(Entity entity);

  void OnEntityDisabled(Entity entity);
  void OnEntityEnabled(Entity entity);
  void OnAnimationComplete(Entity entity, AnimationId animation);

  void ProcessTouch();
  void StartTouch();
  void UpdateTouch();
  void EndTouch();

  void ActuallySetContentBounds(ScrollView* view, const Aabb& bounds);
  void ActuallySetViewOffset(Entity e, const mathfu::vec2& offset);
  mathfu::vec2 ClampOffset(const ScrollView* view,
                           const mathfu::vec2& offset) const;

  AnimationId SetSnappedTargetOffset(ScrollView* view,
                                     const mathfu::vec2& offset,
                                     Clock::duration time);
  AnimationId SetTargetOffset(ScrollView* view, const mathfu::vec2& target,
                              Clock::duration time);

  bool IsInputView(Entity e) const;
  void RemoveInputView(Entity e);

  // Updates |e|'s entry within input_views_ by doing one of the following:
  // - removes if |priority| is kHoverPiroty and |e| isn't hovered; or
  // - seemlessly updates priority if |e| is and will continue to be active; or
  // - updates priority and moves |e|, de/activating if necessary; or
  // - adds |e|, activating if necessary.
  void UpdateInputView(Entity e, int priority);

  ScrollViewPool views_;
  InputViewQueue input_views_;
  Entity current_hover_view_;
  Entity next_hover_view_;

  // Calls GetViewOffset and ActuallySetViewOffset.
  friend class ScrollViewOffsetChannel;

  ScrollSystem(const ScrollSystem&) = delete;
  ScrollSystem& operator=(const ScrollSystem&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScrollSystem);

#endif  // LULLABY_SYSTEMS_SCROLL_SCROLL_SYSTEM_H_
