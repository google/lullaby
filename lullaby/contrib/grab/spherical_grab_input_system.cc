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

#include "lullaby/contrib/grab/spherical_grab_input_system.h"

#include "lullaby/contrib/controller/controller_system.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/modules/reticle/input_focus_locker.h"
#include "lullaby/contrib/cursor/cursor_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/spherical_grab_input_def_generated.h"
#include "mathfu/io.h"

namespace lull {

namespace {
const HashValue kSphericalGrabInputDef = Hash("SphericalGrabInputDef");
}  // namespace

SphericalGrabInputSystem::SphericalGrabInputSystem(Registry* registry)
    : System(registry) {
  RegisterDependency<ControllerSystem>(this);
  RegisterDependency<CursorSystem>(this);
  RegisterDependency<GrabSystem>(this);
  RegisterDependency<InputProcessor>(this);
  RegisterDependency<InputFocusLocker>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDef<SphericalGrabInputDefT>(this);
}

void SphericalGrabInputSystem::Create(Entity entity, HashValue type,
                                      const Def* def) {
  if (type != kSphericalGrabInputDef) {
    LOG(DFATAL)
        << "Invalid type passed to Create. Expecting SphericalGrabInputDef!";
    return;
  }

  const auto* data = ConvertDef<SphericalGrabInputDef>(def);
  auto& handler = handlers_[entity];
  MathfuVec3FromFbVec3(data->sphere_center(), &handler.grab_sphere.position);
  handler.keep_grab_offset = data->keep_grab_offset();
  handler.move_with_hmd = data->move_with_hmd();
  handler.hide_cursor = data->hide_cursor();
  handler.hide_laser = data->hide_laser();

  auto* grab_system = registry_->Get<GrabSystem>();
  grab_system->SetInputHandler(entity, this);
}

void SphericalGrabInputSystem::Destroy(Entity entity) {
  auto* grab_system = registry_->Get<GrabSystem>();
  grab_system->RemoveInputHandler(entity, this);
  handlers_.erase(entity);
}

bool SphericalGrabInputSystem::StartGrab(Entity entity,
                                         InputManager::DeviceType device) {
  auto handler_iter = handlers_.find(entity);
  if (handler_iter == handlers_.end()) {
    LOG(DFATAL) << "Handler not found when starting grab.";
    return false;
  }

  auto& handler = handler_iter->second;
  auto* transform_system = registry_->Get<TransformSystem>();
  auto* input_processor = registry_->Get<InputProcessor>();
  auto* focus_locker = registry_->Get<InputFocusLocker>();
  const InputFocus* focus = input_processor->GetInputFocus(device);

  const mathfu::mat4* world_from_grabbed =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (world_from_grabbed == nullptr) {
    LOG(DFATAL) << "Can't grab an object without a transform.";
    return false;
  }

  if (handler.move_with_hmd) {
    handler.grab_sphere.position =
        registry_->Get<InputManager>()->GetDofPosition(
            InputManager::DeviceType::kHmd);
  }

  // Set the grabbing radius as the distance from the entity's origin to the
  // sphere center at starting.
  handler.grab_sphere.radius =
      (world_from_grabbed->TranslationVector3D() - handler.grab_sphere.position)
          .Length();

  if (handler.keep_grab_offset) {
    ComputeRaySphereCollision(
        focus->collision_ray, handler.grab_sphere.position,
        handler.grab_sphere.radius, &handler.last_collision_position);
  }

  if (handler.hide_cursor) {
    Entity cursor = registry_->Get<CursorSystem>()->GetCursor(device);
    handler.cursor_enabled_before_grab = false;
    if (cursor != kNullEntity) {
      handler.cursor_enabled_before_grab = transform_system->IsEnabled(cursor);
      transform_system->Disable(cursor);
    }
  }

  if (handler.hide_laser) {
    auto* controller_system = registry_->Get<ControllerSystem>();
    handler.laser_enabled_before_grab =
        !controller_system->IsLaserHidden(device);
    controller_system->HideLaser(device);
  }

  // If any of cursor or laser is not hidden, lock cursor on the entity to avoid
  // cursor or laser drift.
  if (!handler.hide_laser || !handler.hide_cursor) {
    const mathfu::vec3 cursor_local_position =
        world_from_grabbed->Inverse() * focus->cursor_position;
    focus_locker->LockOn(device, entity, cursor_local_position);
  }

  return true;
}

Sqt SphericalGrabInputSystem::UpdateGrab(Entity entity,
                                         InputManager::DeviceType device,
                                         const Sqt& original_sqt) {
  auto handler_iter = handlers_.find(entity);
  if (handler_iter == handlers_.end()) {
    LOG(DFATAL) << "Handler not found when updating grab.";
    return original_sqt;
  }
  auto& handler = handler_iter->second;

  auto* transform_system = registry_->Get<TransformSystem>();

  const mathfu::mat4* world_from_grabbed =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (world_from_grabbed == nullptr) {
    LOG(DFATAL) << "Can't grab an object without a transform.";
    return original_sqt;
  }
  auto* input_processor = registry_->Get<InputProcessor>();
  const InputFocus* focus = input_processor->GetInputFocus(device);

  if (handler.move_with_hmd) {
    handler.grab_sphere.position =
        registry_->Get<InputManager>()->GetDofPosition(
            InputManager::DeviceType::kHmd);
  }

  mathfu::vec3 entity_target_position;
  if (handler.keep_grab_offset) {
    // Here the system maintains a fixed rotation from the collision ray to the
    // entity ray during grabbing. Both rays are originated from the current
    // grab sphere center.
    mathfu::vec3 new_collision_position;
    ComputeRaySphereCollision(
        focus->collision_ray, handler.grab_sphere.position,
        handler.grab_sphere.radius, &new_collision_position);
    const mathfu::quat rotate_from_collision_to_entity =
        mathfu::quat::RotateFromTo(
            handler.last_collision_position - handler.grab_sphere.position,
            world_from_grabbed->TranslationVector3D() -
                handler.grab_sphere.position);
    const mathfu::vec3 entity_ray_direction =
        rotate_from_collision_to_entity *
        (new_collision_position - handler.grab_sphere.position).Normalized();

    entity_target_position = handler.grab_sphere.position +
                             entity_ray_direction * handler.grab_sphere.radius;

    handler.last_collision_position = new_collision_position;
  } else {
    ComputeRaySphereCollision(
        focus->collision_ray, handler.grab_sphere.position,
        handler.grab_sphere.radius, &entity_target_position);
  }

  // Keep the original rotation and scale.
  Sqt sqt = original_sqt;

  // Put the entity at the target position to get its local translation.
  transform_system->SetWorldFromEntityMatrix(
      entity, mathfu::mat4::FromTranslationVector(entity_target_position));
  sqt.translation = transform_system->GetLocalTranslation(entity);

  return sqt;
}

bool SphericalGrabInputSystem::ShouldCancel(Entity entity,
                                            InputManager::DeviceType device) {
  // Cancel grabbing if the entity is disabled.
  if (!registry_->Get<TransformSystem>()->IsEnabled(entity)) {
    return true;
  }
  // Cancel grabbing if the entity's handler is not found.
  if (handlers_.find(entity) == handlers_.end()) {
    return true;
  }

  return false;
}

void SphericalGrabInputSystem::EndGrab(Entity entity,
                                       InputManager::DeviceType device) {
  auto* focus_locker = registry_->Get<InputFocusLocker>();
  focus_locker->Unlock(device);

  auto handler_iter = handlers_.find(entity);
  if (handler_iter == handlers_.end()) {
    LOG(DFATAL) << "Handler not found when ending grab.";
    return;
  }
  auto& handler = handler_iter->second;
  // Reset cursor to its previous state.
  if (handler.hide_cursor && handler.cursor_enabled_before_grab) {
    Entity cursor = registry_->Get<CursorSystem>()->GetCursor(device);
    registry_->Get<TransformSystem>()->Enable(cursor);
  }
  if (handler.hide_laser && handler.laser_enabled_before_grab) {
    registry_->Get<ControllerSystem>()->ShowLaser(device);
  }
}

}  // namespace lull
