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

#include "lullaby/modules/reticle/standard_input_pipeline.h"

#include <utility>

#include "lullaby/events/input_events.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/modules/reticle/input_focus_locker.h"
#include "lullaby/systems/cursor/cursor_system.h"
#include "lullaby/systems/input_behavior/input_behavior_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/device_util.h"
#include "lullaby/util/math.h"
#include "lullaby/util/trace.h"
#include "mathfu/constants.h"

namespace lull {

StandardInputPipeline::StandardInputPipeline(Registry* registry) {
  registry_ = registry;
  device_preference_ = {InputManager::kController, InputManager::kHmd};
  auto* input_processor = registry_->Get<InputProcessor>();
  if (input_processor) {
    // Set up the standard prefixes for Input events.
    // Make Device events for the controller send with no prefix, i.e.
    // "FocusStartEvent".
    input_processor->SetPrefix(InputManager::kController, "");

    // Make clicks on the controller's primary button send with no prefix.
    // i.e. "ClickEvent".
    input_processor->SetPrefix(InputManager::kController,
                               InputManager::kPrimaryButton, "");

    // Make clicks on the controller's app button send as "Secondary<event>".
    // i.e. "SecondaryClickEvent".
    input_processor->SetPrefix(InputManager::kController,
                               InputManager::kSecondaryButton, "Secondary");
  }
}

void StandardInputPipeline::AdvanceFrame(const Clock::duration& delta_time) {
  LULLABY_CPU_TRACE_CALL();
  auto* input_processor = registry_->Get<InputProcessor>();
  if (input_processor == nullptr) {
    LOG(DFATAL) << "StandardInputPipeline depends on InputProcessor.";
    return;
  }

  auto device = GetPrimaryDevice();
  if (device == InputManager::kMaxNumDeviceTypes) {
    // No active device, just wait for something to connect.
    return;
  }
  input_processor->SetPrimaryDevice(device);

  InputFocus focus = ComputeInputFocus(delta_time, device);

  // Update InputProcessor with the focus and send events.
  input_processor->UpdateDevice(delta_time, focus);
}

InputFocus StandardInputPipeline::ComputeInputFocus(
    const Clock::duration& delta_time, InputManager::DeviceType device) const {
  InputFocus focus;
  focus.device = device;

  auto* input_processor = registry_->Get<InputProcessor>();
  auto* cursor_system = registry_->Get<CursorSystem>();
  if (cursor_system == nullptr || input_processor == nullptr) {
    LOG(DFATAL)
        << "StandardInputPipeline depends on CursorSystem and InputProcessor.";
    return focus;
  }

  const auto* transform_system = registry_->Get<TransformSystem>();
  Entity cursor_entity = cursor_system->GetCursor(device);

  focus.collision_ray = GetDeviceSelectionRay(
      focus.device, transform_system->GetParent(cursor_entity));

  focus.origin = focus.collision_ray.origin;

  // Set cursor position to be a default depth in the direction of its forward
  // vector, and calculate the direction of the collision_ray.
  focus.cursor_position =
      cursor_system->CalculateCursorPosition(device, focus.collision_ray);
  focus.no_hit_cursor_position = focus.cursor_position;

  // Make the collision come from the hmd instead of the controller
  MakeRayComeFromHmd(&focus);

  // Apply focus locking, input behaviors, collision detection, etc
  ApplySystemsToInputFocus(&focus);

  return focus;
}

void StandardInputPipeline::MakeRayComeFromHmd(InputFocus* focus) const {
  if (focus == nullptr) {
    DCHECK(false) << "Focus must not be null.";
    return;
  }

  const auto* input = registry_->Get<InputManager>();
  // Make the collision come from the hmd instead of the controller
  if (input->HasPositionDof(InputManager::kHmd)) {
    focus->collision_ray.origin = input->GetDofPosition(InputManager::kHmd);
    focus->collision_ray.direction =
        (focus->cursor_position - focus->collision_ray.origin).Normalized();
  }
}

void StandardInputPipeline::ApplySystemsToInputFocus(InputFocus* focus) const {
  if (focus == nullptr) {
    DCHECK(false) << "Focus must not be null.";
    return;
  }

  const auto* collision_system = registry_->Get<CollisionSystem>();
  const auto* input_behavior_system = registry_->Get<InputBehaviorSystem>();
  auto* input_focus_locker = registry_->Get<InputFocusLocker>();

  // Check if focus is locked to an entity.
  bool locked = false;
  if (input_focus_locker) {
    locked = input_focus_locker->UpdateInputFocus(focus);
  }

  // If focus isn't locked, try to collide against AABBs in the world.
  if (!locked) {
    ApplyCollisionSystemToInputFocus(focus);
  }

  // Apply input behaviors:
  if (input_behavior_system && focus->target != kNullEntity) {
    input_behavior_system->UpdateInputFocus(focus);
  }

  if (collision_system) {
    focus->interactive = collision_system->IsInteractionEnabled(focus->target);
  }
}

void StandardInputPipeline::ApplyCollisionSystemToInputFocus(
    InputFocus* focus) const {
  if (focus == nullptr) {
    DCHECK(false) << "Focus must not be null.";
    return;
  }

  CollisionSystem::CollisionResult collision = {kNullEntity, kNoHitDistance};
  const auto* collision_system = registry_->Get<CollisionSystem>();
  if (manual_collision_) {
    collision = *manual_collision_;
  } else if (collision_system) {
    collision = collision_system->CheckForCollision(focus->collision_ray);
  }

  if (manual_collision_ || collision.entity != kNullEntity) {
    focus->target = collision.entity;
    focus->cursor_position =
        focus->collision_ray.origin +
        focus->collision_ray.direction * collision.distance;
  }
}

void StandardInputPipeline::StartManualCollision(Entity entity, float depth) {
  manual_collision_ = {entity, depth};
}

void StandardInputPipeline::StopManualCollision() { manual_collision_.reset(); }

InputManager::DeviceType StandardInputPipeline::GetPrimaryDevice() const {
  auto input = registry_->Get<InputManager>();
  for (InputManager::DeviceType device : device_preference_) {
    if (input->IsConnected(device)) {
      return device;
    }
  }
  return InputManager::kMaxNumDeviceTypes;
}

Ray StandardInputPipeline::GetDeviceSelectionRay(
    InputManager::DeviceType device, Entity parent) const {
  const auto* transform_system = registry_->Get<TransformSystem>();
  const auto* input = registry_->Get<InputManager>();
  Ray result(mathfu::kZeros3f, -mathfu::kAxisZ3f);

  // If the device has a default local space ray, use that instead.
  auto selection_ray = input->GetDeviceInfo(device, kSelectionRayHash);

  result = selection_ray.ValueOr(result);

  if (input && input->HasRotationDof(device)) {
    mathfu::quat rotation = input->GetDofRotation(device);
    result.origin = rotation * result.origin;

    if (input->HasPositionDof(device)) {
      result.origin += input->GetDofPosition(device);
    }
    result.direction = rotation * result.direction;
  }

  if (transform_system) {
    // Get world transform from any existing parent transformations.
    const mathfu::mat4* world_from_parent =
        transform_system->GetWorldFromEntityMatrix(parent);
    if (world_from_parent) {
      // Apply any world transform to account for the actual forward direction
      // of the preferred device and the raycast origin. This allows to add
      // the reticle entity as a child to a parent entity and have the reticle
      // behave correctly when the parent entity is moved and rotated in world
      // space.
      result.origin = (*world_from_parent) * result.origin;
      result.direction = ((*world_from_parent) * result.direction).Normalized();
    }
  }
  return result;
}

void StandardInputPipeline::SetDevicePreference(
    Span<InputManager::DeviceType> devices) {
  device_preference_.assign(devices.begin(), devices.end());
}

}  // namespace lull
