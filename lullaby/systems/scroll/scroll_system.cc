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

#include "lullaby/systems/scroll/scroll_system.h"

#include "lullaby/generated/scroll_def_generated.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/events/input_events.h"
#include "lullaby/events/lifetime_events.h"
#include "lullaby/events/scroll_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/scroll/scroll_channels.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/bits.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/time.h"
#include "lullaby/util/trace.h"

namespace lull {
namespace {

const HashValue kScrollDefHash = Hash("ScrollDef");
constexpr float kResistanceExponent = 8.0f;

static float GetDragTarget(float target, float offset, float min, float max,
                           float border) {
  if (border > 0.0f) {
    if (target < min) {
      const float scale =
          powf(1.0f - (min - target) / border, kResistanceExponent);
      const float start = std::min(min, offset);
      return (start + scale * (target - start));
    }
    if (target > max) {
      const float scale =
          powf(1.0f - (target - max) / border, kResistanceExponent);
      const float start = std::max(max, offset);
      return (start + scale * (target - start));
    }
  }
  return target;
}

static mathfu::vec2 GetDragTarget(const mathfu::vec2& target,
                                  const mathfu::vec2& offset,
                                  const mathfu::vec2& min,
                                  const mathfu::vec2& max,
                                  const mathfu::vec2& border) {
  return mathfu::vec2(
      GetDragTarget(target.x, offset.x, min.x, max.x, border.x),
      GetDragTarget(target.y, offset.y, min.y, max.y, border.y));
}

}  // namespace

ScrollSystem::ScrollSystem(Registry* registry)
    : System(registry),
      views_(8),
      current_hover_view_(kNullEntity),
      next_hover_view_(kNullEntity) {
  RegisterDef(this, kScrollDefHash);

  RegisterDependency<AnimationSystem>(this);
  RegisterDependency<DispatcherSystem>(this);
  RegisterDependency<TransformSystem>(this);

  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(
      this, [this](const StartHoverEvent& ev) { OnStartHover(ev.target); });
  dispatcher->Connect(
      this, [this](const StopHoverEvent& ev) { OnStopHover(ev.target); });
  dispatcher->Connect(
      this, [this](const OnEnabledEvent& ev) { OnEntityEnabled(ev.target); });
  dispatcher->Connect(
      this, [this](const OnDisabledEvent& ev) { OnEntityDisabled(ev.target); });
  dispatcher->Connect(this, [this](const OnResumeEvent& ev) {
    // Snap partially scrolled views back to their last snapped point on resume,
    // if applicable. This prevents the user from partially scrolling a view,
    // leaving an app, and then returning to see the view still off-center.
    for (const auto& view : views_) {
      SnapByDelta(view.GetEntity(), 0, 0);
    }
  });
  dispatcher->Connect(this, [this](const OnInteractionEnabledEvent& ev) {
    OnEntityEnabled(ev.entity);
  });
  dispatcher->Connect(this, [this](const OnInteractionDisabledEvent& ev) {
    OnEntityDisabled(ev.entity);
  });
  dispatcher->Connect(this, [this](const ScrollActivateEvent& ev) {
    Activate(ev.entity);
  });
  dispatcher->Connect(this, [this](const ScrollDeactivateEvent& ev) {
    Deactivate(ev.entity);
  });
  dispatcher->Connect(this, [this](const ScrollSnapByDelta& ev) {
    SnapByDelta(ev.entity, ev.delta, ev.time_ms);
  });
  dispatcher->Connect(this, [this](const ScrollSetViewOffsetEvent& ev) {
    SetViewOffset(ev.entity, ev.offset, DurationFromMilliseconds(ev.time_ms));
  });
}

ScrollSystem::~ScrollSystem() {
  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->DisconnectAll(this);
}

void ScrollSystem::Initialize() {
  ScrollViewOffsetChannel::Setup(registry_, 8);
}

void ScrollSystem::Create(Entity e, HashValue type, const Def* def) {
  if (type != kScrollDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting ScrollDef!";
    return;
  }
  const auto* data = ConvertDef<ScrollDef>(def);

  ScrollView* view = views_.Emplace(e);
  MathfuVec2FromFbVec2(data->touch_sensitivity(), &view->touch_sensitivity);
  AabbFromFbAabb(data->content_bounds(), &view->content_bounds);
  MathfuVec2FromFbVec2(data->drag_border(), &view->drag_border);

  view->momentum_time = std::chrono::milliseconds(data->touch_momentum_ms());
  if (view->momentum_time < std::chrono::milliseconds::zero()) {
    LOG(DFATAL) << "Cannot have negative momentum time!";
    view->momentum_time = std::chrono::milliseconds::zero();
  }
  view->drag_momentum_time =
      std::chrono::milliseconds(data->drag_momentum_ms());
  if (view->drag_momentum_time < std::chrono::milliseconds::zero()) {
    LOG(DFATAL) << "Cannot have negative drag momentum time!";
    view->drag_momentum_time = std::chrono::milliseconds::zero();
  }

  view->priority = data->active_priority();
  DCHECK_GE(view->priority, kHoverPriority);
  if (data->active_priority() > kHoverPriority) {
    UpdateInputView(e, view->priority);
  }

  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  view->on_animation_complete =
      dispatcher_system->Connect(e, [this](const AnimationCompleteEvent& ev) {
        OnAnimationComplete(ev.target, ev.id);
      });
}

void ScrollSystem::Create(Entity entity, const Aabb& content_bounds) {
  ScrollView* view = views_.Get(entity);
  if (!view) {
    view = views_.Emplace(entity);
  }

  view->view_offset = mathfu::kZeros2f;
  view->target_offset = mathfu::kZeros2f;
  view->content_bounds = content_bounds;
}

void ScrollSystem::Destroy(Entity entity) {
  RemoveInputView(entity);
  views_.Destroy(entity);
}

void ScrollSystem::SetTouchSensitivity(Entity entity,
                                       const mathfu::vec2& touch_sensitivity) {
  ScrollView* view = views_.Get(entity);
  if (view) {
    view->touch_sensitivity = touch_sensitivity;
  }
}

void ScrollSystem::SetSnapOffsetFn(Entity entity, SnapOffsetFn fn) {
  ScrollView* view = views_.Get(entity);
  if (view) {
    view->snap_offset_fn = std::move(fn);
  }
}

void ScrollSystem::SetSnapByDeltaFn(Entity entity, SnapByDeltaFn fn) {
  ScrollView* view = views_.Get(entity);
  if (view) {
    view->snap_by_delta_fn = std::move(fn);
  }
}

void ScrollSystem::Activate(Entity entity) {
  ScrollView* view = views_.Get(entity);
  if (view) {
    UpdateInputView(entity, view->priority);
  }
}

void ScrollSystem::Deactivate(Entity entity) {
  ScrollView* view = views_.Get(entity);
  if (view) {
    RemoveInputView(entity);
  }
}

void ScrollSystem::SnapByDelta(Entity entity, int delta, float time_ms = -1) {
  ScrollView* view = views_.Get(entity);
  if (view && view->snap_by_delta_fn) {
    Optional<mathfu::vec2> xy = view->snap_by_delta_fn(delta);
    if (!xy) {
      return;
    }
    if (time_ms < 0) {
      time_ms = static_cast<float>(std::abs(delta)) *
                MillisecondsFromDuration(view->momentum_time);
    }
    Clock::duration time =
        std::chrono::duration_cast<Clock::duration>(
            Millisecondsf(time_ms));
    ForceViewOffset(entity, *xy, time);
  }
}

void ScrollSystem::SetPriority(Entity entity, int priority) {
  DCHECK_GE(priority, kHoverPriority) << "Invalid priority " << priority;

  ScrollView* view = views_.Get(entity);
  if (view && view->priority != priority) {
    view->priority = priority;
    UpdateInputView(entity, view->priority);
  }
}

void ScrollSystem::SetContentBounds(Entity entity, const Aabb& bounds) {
  ScrollView* view = views_.Get(entity);
  if (view) {
    ActuallySetContentBounds(view, bounds);
    SetViewOffset(entity, view->target_offset);
  }
}

void ScrollSystem::ForceContentBounds(Entity entity, const Aabb& bounds) {
  ScrollView* view = views_.Get(entity);
  if (view) {
    ActuallySetContentBounds(view, bounds);
    ForceViewOffset(entity, view->target_offset);
  }
}

void ScrollSystem::ActuallySetContentBounds(ScrollView* view,
                                            const Aabb& bounds) {
  view->content_bounds = bounds;
  if (view->snap_offset_fn) {
    mathfu::vec2 min = view->snap_offset_fn(
        view->content_bounds.min.xy(), InputManager::GestureDirection::kNone,
        view->content_bounds, kSetBounds);
    view->content_bounds.min.x = min.x;
    view->content_bounds.min.y = min.y;
    mathfu::vec2 max = view->snap_offset_fn(
        view->content_bounds.max.xy(), InputManager::GestureDirection::kNone,
        view->content_bounds, kSetBounds);
    view->content_bounds.max.x = max.x;
    view->content_bounds.max.y = max.y;
  }
}

mathfu::vec2 ScrollSystem::ClampOffset(const ScrollView* view,
                                       const mathfu::vec2& offset) const {
  const mathfu::vec2 min = view->content_bounds.min.xy() - view->drag_border;
  const mathfu::vec2 max = view->content_bounds.max.xy() + view->drag_border;
  return mathfu::vec2::Max(min, mathfu::vec2::Min(offset, max));
}

mathfu::vec2 ScrollSystem::GetViewOffset(Entity entity) const {
  const ScrollView* view = views_.Get(entity);
  return view ? view->view_offset : mathfu::kZeros2f;
}

AnimationId ScrollSystem::SetTargetOffset(ScrollView* view,
                                          const mathfu::vec2& target,
                                          Clock::duration time) {
  const mathfu::vec2 delta = target - view->target_offset;
  if (IsNearlyZero(delta.LengthSquared(), kDefaultEpsilonSqr)) {
    return kNullAnimation;
  }

  view->target_offset += delta;

  auto* animation_system = registry_->Get<AnimationSystem>();
  return animation_system->SetTarget(view->GetEntity(),
                                     ScrollViewOffsetChannel::kChannelName,
                                     &view->target_offset[0], 2, time);
}

AnimationId ScrollSystem::SetSnappedTargetOffset(
    ScrollView* view, const mathfu::vec2& requested_offset,
    Clock::duration time) {
  mathfu::vec2 offset = requested_offset;
  if (view->snap_offset_fn) {
    offset = view->snap_offset_fn(offset, InputManager::GestureDirection::kNone,
                                  view->content_bounds, kSetOffset);
  }
  return SetTargetOffset(view, ClampOffset(view, offset), time);
}

bool ScrollSystem::SetViewOffset(Entity entity,
                                 const mathfu::vec2& requested_offset,
                                 Clock::duration time) {
  ScrollView* view = views_.Get(entity);
  if (!view) {
    return false;
  }
  const AnimationId animation =
      SetSnappedTargetOffset(view, requested_offset, time);
  return animation != kNullAnimation;
}

bool ScrollSystem::ForceViewOffset(Entity entity,
                                   const mathfu::vec2& requested_offset,
                                   Clock::duration time) {
  ScrollView* view = views_.Get(entity);
  if (!view) {
    return false;
  }

  const AnimationId animation =
      SetSnappedTargetOffset(view, requested_offset, time);
  if (animation == kNullAnimation) {
    return false;
  }

  view->forced_offset_animation = animation;

  // This will block other scroll animations anyway, so it is safe to
  // immediately set the view offset if a 0 duration animation was requested.
  if (time == Clock::duration(0)) {
    ActuallySetViewOffset(entity, requested_offset);
  }
  return true;
}

void ScrollSystem::ActuallySetViewOffset(Entity entity,
                                         const mathfu::vec2& offset) {
  ScrollView* view = views_.Get(entity);
  if (!view) {
    return;
  }

  const mathfu::vec2 old_offset = view->view_offset;
  const mathfu::vec2 new_offset = ClampOffset(view, offset);
  const ScrollOffsetChanged event(entity, old_offset, new_offset);
  SendEvent(registry_, entity, event);

  TransformSystem* transform_system = registry_->Get<TransformSystem>();
  const std::vector<Entity>* children = transform_system->GetChildren(entity);
  if (children) {
    Sqt sqt;
    sqt.translation = mathfu::vec3(view->view_offset - new_offset, 0);

    for (Entity child : *children) {
      transform_system->ApplySqt(child, sqt);
    }
  }
  view->view_offset = new_offset;
}

ScrollSystem::ScrollView* ScrollSystem::GetContainerView(Entity entity) {
  TransformSystem* transform_system = registry_->Get<TransformSystem>();
  Entity parent = transform_system->GetParent(entity);
  while (parent != kNullEntity) {
    ScrollView* view = views_.Get(parent);
    if (view) {
      return view;
    }
    parent = transform_system->GetParent(parent);
  }
  return nullptr;
}

ScrollSystem::ScrollView* ScrollSystem::GetViewForInput(Entity entity) {
  ScrollView* view = views_.Get(entity);
  if (view) {
    return view;
  }
  return GetContainerView(entity);
}

void ScrollSystem::UpdateHoverView() {
  if (current_hover_view_ == next_hover_view_) {
    return;
  }

  if (current_hover_view_ != kNullEntity) {
    const ScrollView* old_view = views_.Get(current_hover_view_);
    if (old_view && old_view->priority == kHoverPriority) {
      RemoveInputView(current_hover_view_);
    }
  }

  current_hover_view_ = next_hover_view_;
  if (current_hover_view_ != kNullEntity) {
    UpdateInputView(current_hover_view_, kHoverPriority);
  }
}

void ScrollSystem::OnStartHover(Entity entity) {
  const ScrollView* target_view = GetViewForInput(entity);
  next_hover_view_ = target_view ? target_view->GetEntity() : kNullEntity;
}

void ScrollSystem::OnStopHover(Entity entity) {
  next_hover_view_ = kNullEntity;
}

ScrollSystem::ScrollView* ScrollSystem::GetActiveInputView() {
  if (input_views_.empty()) {
    return nullptr;
  }
  const Entity entity = input_views_.back().entity;
  return views_.Get(entity);
}

bool ScrollSystem::IsTouchControllerConnected() const {
  const InputManager* input = registry_->Get<InputManager>();
  return input->IsConnected(InputManager::kController) &&
         input->HasTouchpad(InputManager::kController);
}

void ScrollSystem::UpdateTouch() {
  ScrollView* view = GetActiveInputView();
  if (!view) {
    return;
  }
  if (!IsTouchControllerConnected()) {
    return;
  }

  const InputManager* input = registry_->Get<InputManager>();
  const mathfu::vec2 delta = mathfu::vec2(-1.f, 1.f) *
                             input->GetTouchDelta(InputManager::kController) *
                             view->touch_sensitivity;
  const mathfu::vec2 target =
      GetDragTarget(view->target_offset + delta, view->target_offset,
                    view->content_bounds.min.xy(),
                    view->content_bounds.max.xy(), view->drag_border);

  // Set target directly without snapping to grid while touch is active.
  SetTargetOffset(view, target, view->drag_momentum_time);
}

void ScrollSystem::EndTouch() {
  if (input_views_.empty()) {
    return;
  }

  ScrollView* view = GetActiveInputView();
  if (!view) {
    return;
  }
  if (!IsTouchControllerConnected()) {
    return;
  }

  auto* input = registry_->Get<InputManager>();
  mathfu::vec2 offset = view->target_offset;
  if (view->snap_offset_fn) {
    const InputManager::GestureDirection gesture =
        input->GetTouchGestureDirection(InputManager::kController);
    offset =
        view->snap_offset_fn(offset, gesture, view->content_bounds, kEndTouch);
  } else {
    // Convert the touch velocity to velocity on the offset.
    const mathfu::vec2 velocity =
        mathfu::vec2(-1.0f, 1.0f) *
        input->GetTouchVelocity(InputManager::kController);
    const mathfu::vec2 delta = SecondsFromDuration(view->momentum_time) *
                               velocity * view->touch_sensitivity;
    offset += delta;
  }

  const Entity entity = input_views_.back().entity;
  SetViewOffset(entity, offset, view->momentum_time);
}

void ScrollSystem::AdvanceFrame(Clock::duration delta_time) {
  LULLABY_CPU_TRACE_CALL();
  // Update hover view first since it can modify input_views_.
  UpdateHoverView();
  ProcessTouch();
}

void ScrollSystem::ProcessTouch() {
  ScrollView* view = GetActiveInputView();

  // Skip processing if there is no active view, we are still completing a
  // forced scroll, or there is no controller connected.
  if (!view || view->forced_offset_animation != kNullAnimation ||
      !IsTouchControllerConnected()) {
    return;
  }

  const InputManager* input = registry_->Get<InputManager>();
  const auto state = input->GetTouchState(InputManager::kController);
  if (CheckBit(state, InputManager::kPressed) &&
      !CheckBit(state, InputManager::kJustPressed)) {
    UpdateTouch();
  } else if (CheckBit(state, InputManager::kJustReleased)) {
    EndTouch();
  }
}

void ScrollSystem::OnEntityEnabled(Entity entity) {
  ScrollView* view = views_.Get(entity);
  if (view && view->priority > kHoverPriority) {
    UpdateInputView(entity, view->priority);
  }
}

void ScrollSystem::OnEntityDisabled(Entity entity) { RemoveInputView(entity); }

void ScrollSystem::OnAnimationComplete(Entity entity, AnimationId animation) {
  ScrollView* view = views_.Get(entity);
  if (view && view->forced_offset_animation == animation) {
    view->forced_offset_animation = kNullAnimation;
  }
}

bool ScrollSystem::IsInputView(Entity entity) const {
  auto iter = std::find_if(input_views_.begin(), input_views_.end(),
                           [entity](const EntityPriorityTuple& entry) {
                             return entry.entity == entity;
                           });
  return iter != input_views_.end();
}

void ScrollSystem::UpdateInputView(Entity entity, int priority) {
  const bool is_hovered =
      entity == current_hover_view_ || entity == next_hover_view_;
  if (priority == kHoverPriority && !is_hovered) {
    RemoveInputView(entity);
    return;
  }

  auto iter = std::find_if(input_views_.begin(), input_views_.end(),
                           [entity](const EntityPriorityTuple& entry) {
                             return entry.entity == entity;
                           });

  // If there's an existing entry, do one of two things:
  // - if entity is active and will remain active, just update priority;
  // - otherwise, remove & deactivate entity, then continue to re-add it.
  if (iter != input_views_.end()) {
    auto higher_priority_iter = std::find_if(
        input_views_.begin(), input_views_.end(),
        [entity, priority](const EntityPriorityTuple& entry) {
          return entry.entity != entity && entry.priority > priority;
        });
    const bool will_be_active = higher_priority_iter == input_views_.end();
    const bool is_active = iter + 1 == input_views_.end();

    if (is_active && will_be_active) {
      iter->priority = priority;
      return;
    }

    RemoveInputView(entity);
  }

  // At this point, we know that entity isn't in input_views_.
  const EntityPriorityTuple entry(entity, priority);

  // If there was no active input view (e.g. the viewer was not pointing at a
  // scrollview), send a ScrollViewTargeted signal and just add the entry.
  if (input_views_.empty()) {
    Dispatcher* dispatcher = registry_->Get<Dispatcher>();
    dispatcher->Send(ScrollViewTargeted());

    input_views_.emplace_back(entry);
    return;
  }

  // input_views_ isn't empty, so find where to insert based on priority.
  auto it = input_views_.rbegin();
  for (; it != input_views_.rend(); ++it) {
    if (it->priority <= priority) {
      input_views_.insert(it.base(), entry);
      return;
    }
  }

  input_views_.insert(it.base(), entry);
}

void ScrollSystem::RemoveInputView(Entity entity) {
  if (current_hover_view_ == entity) {
    current_hover_view_ = kNullEntity;
  }
  if (input_views_.empty()) {
    // Do nothing.
  } else if (input_views_.back().entity == entity) {
    EndTouch();
    input_views_.pop_back();
  } else {
    auto iter = std::find_if(input_views_.begin(), input_views_.end(),
                             [entity](const EntityPriorityTuple& entry) {
                               return entry.entity == entity;
                             });
    if (iter != input_views_.end()) {
      input_views_.erase(iter);
    }
  }
}

mathfu::vec2 ScrollSystem::GetTouchSensitivity(Entity entity) const {
  const ScrollView* view = views_.Get(entity);
  return view ? view->touch_sensitivity : mathfu::kZeros2f;
}

}  // namespace lull
