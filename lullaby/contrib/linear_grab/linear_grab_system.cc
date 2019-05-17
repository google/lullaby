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

#include "lullaby/contrib/linear_grab/linear_grab_system.h"

#include "lullaby/generated/linear_grabbable_def_generated.h"
#include "lullaby/events/input_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/contrib/input_behavior/input_behavior_system.h"
#include "lullaby/contrib/reticle/reticle_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "mathfu/io.h"

namespace lull {

namespace {
const HashValue kLinearGrabbableDefHash = ConstHash("LinearGrabbableDef");
}  // namespace

LinearGrabSystem::LinearGrabSystem(Registry* registry)
    : System(registry), grabbables_(4) {
  RegisterDef<LinearGrabbableDefT>(this);
  RegisterDependency<DispatcherSystem>(this);
  RegisterDependency<ReticleSystem>(this);
}

void LinearGrabSystem::Create(Entity entity, HashValue type, const Def* def) {
  Grabbable grabbable(entity);

  auto* data = ConvertDef<LinearGrabbableDef>(def);
  MathfuVec3FromFbVec3(data->direction(), &grabbable.line_direction);
  grabbable.local_orientation = data->local_orientation();
  grabbables_.emplace(entity, grabbable);

  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Connect(
      entity, kDragStartEventHash, this,
      [this, entity](const EventWrapper& event) { OnGrab(event); });
  dispatcher_system->Connect(
      entity, kDragStopEventHash, this,
      [this, entity](const EventWrapper& event) { OnGrabReleased(event); });
  dispatcher_system->Connect(
      entity, kCancelEventHash, this,
      [this, entity](const EventWrapper& event) { OnGrabReleased(event); });

  auto* input_behavior_system = registry_->Get<InputBehaviorSystem>();
  if (input_behavior_system) {
    input_behavior_system->SetDraggable(entity, true);
  }
}

void LinearGrabSystem::Destroy(Entity entity) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Disconnect<ClickEvent>(entity, this);
  dispatcher_system->Disconnect<ClickReleasedEvent>(entity, this);
  grabbables_.erase(entity);
  grabbed_.erase(entity);
}

void LinearGrabSystem::Enable(Entity entity) {
  auto found = grabbables_.find(entity);
  if (found == grabbables_.end()) {
    return;
  }

  found->second.enabled = true;
}

void LinearGrabSystem::Disable(Entity entity) {
  auto found = grabbables_.find(entity);
  if (found == grabbables_.end()) {
    return;
  }

  Release(entity);
  found->second.enabled = false;
}

Optional<Line> LinearGrabSystem::GetGrabLine(Entity entity) {
  auto data = grabbed_.find(entity);
  if (data != grabbed_.end()) {
    return data->second.line;
  }
  return Optional<Line>();  // entity not found
}

void LinearGrabSystem::AdvanceFrame(const Clock::duration& delta_time) {
  // Determine sqt of controller this frame.
  auto* transform_system = registry_->Get<TransformSystem>();
  auto* reticle_system = registry_->Get<ReticleSystem>();

  const Ray controller_ray = reticle_system->GetCollisionRay();
  const Line controller_line(controller_ray.origin, controller_ray.direction);

  for (auto& grabbed : grabbed_) {
    const Grabbable& grabbable = grabbables_.at(grabbed.first);
    GrabData& data = grabbed.second;

    // Get the entity's current worldspace pose.
    const mathfu::mat4* world_from_object_matrix =
        transform_system->GetWorldFromEntityMatrix(data.entity);
    if (world_from_object_matrix == nullptr) {
      continue;
    }

    // Update the line constraint to account for object's current pose.
    //  - origin should be at object's current position + local grab offset.
    //  - origin & normal should be converted into world-space.
    const mathfu::vec3 line_position =
        *world_from_object_matrix * data.grab_local_offset;
    mathfu::vec3 line_direction = grabbable.line_direction;
    if (grabbable.local_orientation) {
      line_direction =
          (*world_from_object_matrix * mathfu::vec4(line_direction, 0.0f))
              .xyz();
    }
    data.line = Line(line_position, line_direction);

    // Get the world-space hit point of the controller ray & this line.
    mathfu::vec3 hit;
    if (!ComputeClosestPointBetweenLines(data.line, controller_line, &hit,
                                         nullptr)) {
      continue;
    }

    // Translate to the hit point.
    mathfu::mat4 updated_world_from_object_matrix(*world_from_object_matrix);
    updated_world_from_object_matrix.GetColumn(3) = mathfu::vec4(hit, 1.0);

    // Account for the offset in local object coordinates of the original click
    // point.
    const mathfu::mat4 local_offset =
        mathfu::mat4::FromTranslationVector(-1.0f * data.grab_local_offset);
    updated_world_from_object_matrix =
        updated_world_from_object_matrix * local_offset;

    // Update the worldspace pose of the entity (local sqt will be
    // re-calculated by transform_system).
    transform_system->SetWorldFromEntityMatrix(
        data.entity, updated_world_from_object_matrix);
  }
}

void LinearGrabSystem::OnGrab(const EventWrapper& event) {
  Entity target = event.GetValueWithDefault<Entity>(kEntityHash, kNullEntity);
  auto device = event.GetValueWithDefault<InputManager::DeviceType>(
      kDeviceHash, InputManager::kMaxNumDeviceTypes);
  const Grabbable& grabbable = grabbables_.at(target);

  if (!grabbable.enabled || device == InputManager::kMaxNumDeviceTypes) {
    return;
  }

  const mathfu::vec3 press_location =
      event.GetValueWithDefault<mathfu::vec3>(kLocationHash, mathfu::kZeros3f);

  auto* reticle_system = registry_->Get<ReticleSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::mat4* world_from_object_matrix =
      transform_system->GetWorldFromEntityMatrix(target);
  if (world_from_object_matrix == nullptr) {
    return;
  }

  const mathfu::vec3 line_position = *world_from_object_matrix * press_location;
  mathfu::vec3 line_direction = grabbable.line_direction;
  if (grabbable.local_orientation) {
    line_direction =
        (*world_from_object_matrix * mathfu::vec4(line_direction, 0.0f)).xyz();
  }
  Line drag_line(line_position, line_direction);

  const Ray controller_ray = reticle_system->GetCollisionRay();
  const Line controller_line(controller_ray.origin, controller_ray.direction);

  // Get the world-space hit point of the controller ray & this line.
  mathfu::vec3 world_grab_offset;
  if (!ComputeClosestPointBetweenLines(drag_line, controller_line,
                                       &world_grab_offset, nullptr)) {
    return;
  }

  const mathfu::vec3 local_offset =
      world_from_object_matrix->Inverse() * world_grab_offset;

  GrabData data(target, local_offset, world_grab_offset, drag_line, device);

  grabbed_.emplace(target, data);

  const LinearGrabEvent grab_event(target, local_offset);
  SendEvent(registry_, target, grab_event);

  auto* focus_locker = registry_->Get<InputFocusLocker>();
  focus_locker->LockOn(device, target, press_location);
}

void LinearGrabSystem::OnGrabReleased(const EventWrapper& event) {
  Entity target = event.GetValueWithDefault<Entity>(kEntityHash, kNullEntity);
  Release(target);
}

void LinearGrabSystem::Release(Entity entity) {
  auto data = grabbed_.find(entity);
  if (data == grabbed_.end()) {
    return;
  }
  const LinearGrabReleasedEvent grab_event(entity);
  SendEvent(registry_, entity, grab_event);
  auto* focus_locker = registry_->Get<InputFocusLocker>();
  focus_locker->Unlock(data->second.device);

  grabbed_.erase(entity);
}
}  // namespace lull
