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

#include "lullaby/contrib/grab/spatial_grab_input_system.h"

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/modules/reticle/input_focus_locker.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/spatial_grab_input_def_generated.h"
#include "mathfu/io.h"

namespace lull {

namespace {
const HashValue kSpatialGrabInputDef = ConstHash("SpatialGrabInputDef");
}  // namespace

SpatialGrabInputSystem::SpatialGrabInputSystem(Registry* registry)
    : System(registry) {
  RegisterDependency<GrabSystem>(this);
  RegisterDependency<InputManager>(this);
  RegisterDependency<InputProcessor>(this);
  RegisterDependency<InputFocusLocker>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDef<SpatialGrabInputDefT>(this);
}

void SpatialGrabInputSystem::Create(Entity entity, HashValue type,
                                    const Def* def) {
  if (type != kSpatialGrabInputDef) {
    LOG(DFATAL)
        << "Invalid type passed to Create. Expecting SpatialGrabInputDef!";
    return;
  }

  const auto* data = ConvertDef<SpatialGrabInputDef>(def);
  auto& handler = handlers_[entity];
  handler.break_angle = DegreesToRadians(data->break_angle());
  handler.set_grab_offset_on_start = data->set_grab_offset_on_start();
  Sqt grab_offset;
  MathfuVec3FromFbVec3(data->position(), &grab_offset.translation);
  MathfuQuatFromFbVec3(data->rotation(), &grab_offset.rotation);
  MathfuVec3FromFbVec3(data->scale(), &grab_offset.scale);
  handler.device_from_grabbed = CalculateTransformMatrix(grab_offset).Inverse();

  auto* grab_system = registry_->Get<GrabSystem>();
  grab_system->SetInputHandler(entity, this);
}

void SpatialGrabInputSystem::Destroy(Entity entity) {
  auto* grab_system = registry_->Get<GrabSystem>();
  grab_system->RemoveInputHandler(entity, this);
  handlers_.erase(entity);
}

bool SpatialGrabInputSystem::StartGrab(Entity entity,
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

  const mathfu::mat4 world_from_device =
      registry_->Get<InputManager>()->GetDofWorldFromObjectMatrix(device);

  if (handler.set_grab_offset_on_start) {
    // Store the actual transformation between device and grabbed entity, so
    // that the entity doesn't jump as soon as it is picked up.
    handler.device_from_grabbed =
        world_from_device.Inverse() * (*world_from_grabbed);
  }

  const mathfu::vec3 cursor_offset =
      world_from_grabbed->Inverse() * focus->cursor_position;
  focus_locker->LockOn(device, entity, cursor_offset);

  return true;
}

Sqt SpatialGrabInputSystem::UpdateGrab(Entity entity,
                                       InputManager::DeviceType device,
                                       const Sqt& original_sqt) {
  auto handler_iter = handlers_.find(entity);
  if (handler_iter == handlers_.end()) {
    LOG(DFATAL) << "Handler not found when starting grab.";
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

  const mathfu::mat4 world_from_device =
      registry_->Get<InputManager>()->GetDofWorldFromObjectMatrix(device);

  transform_system->SetWorldFromEntityMatrix(
      entity, world_from_device * handler.device_from_grabbed);
  Sqt new_sqt(*(transform_system->GetSqt(entity)));
  return new_sqt;
}

bool SpatialGrabInputSystem::ShouldCancel(Entity entity,
                                          InputManager::DeviceType device) {
  auto handler_iter = handlers_.find(entity);
  if (handler_iter == handlers_.end()) {
    LOG(DFATAL) << "Handler not found when starting grab.";
    return true;
  }
  auto* transform_system = registry_->Get<TransformSystem>();

  auto* input_processor = registry_->Get<InputProcessor>();
  const InputFocus* focus = input_processor->GetInputFocus(device);

  const mathfu::mat4* world_from_grabbed =
      transform_system->GetWorldFromEntityMatrix(entity);

  if (world_from_grabbed == nullptr) {
    LOG(DFATAL) << "Can't grab an object without a transform.";
    return true;
  }

  const float angle = std::acos(CosAngleFromRay(
      focus->collision_ray, world_from_grabbed->TranslationVector3D()));

  // Return true if the grab should cancel.
  return angle >= handler_iter->second.break_angle;
}

void SpatialGrabInputSystem::EndGrab(Entity entity,
                                     InputManager::DeviceType device) {
  auto* focus_locker = registry_->Get<InputFocusLocker>();
  focus_locker->Unlock(device);
}

}  // namespace lull
