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

#include "lullaby/contrib/grab/planar_grab_input_system.h"

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/modules/reticle/input_focus_locker.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/planar_grab_input_def_generated.h"
#include "mathfu/io.h"

namespace lull {

namespace {
const HashValue kPlanarGrabInputDef = ConstHash("PlanarGrabInputDef");
// The rate which the difference between ideal and actual grab_offset decays.
const float kOffsetDecay = 1.2f;

bool ComputePlaneIntersection(const mathfu::mat4& space, const Ray& ray,
                              const mathfu::vec3& plane_direction,
                              mathfu::vec3* hit) {
  const mathfu::vec3 plane_pos = space.TranslationVector3D();
  const mathfu::vec3 normal =
      ((space * plane_direction) - plane_pos).Normalized();

  Plane plane(plane_pos, normal);
  return ComputeRayPlaneCollision(ray, plane, hit);
}
}  // namespace

PlanarGrabInputSystem::PlanarGrabInputSystem(Registry* registry)
    : System(registry) {
  RegisterDependency<GrabSystem>(this);
  RegisterDependency<InputProcessor>(this);
  RegisterDependency<InputFocusLocker>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDef<PlanarGrabInputDefT>(this);
}

void PlanarGrabInputSystem::Create(Entity entity, HashValue type,
                                   const Def* def) {
  if (type != kPlanarGrabInputDef) {
    LOG(DFATAL)
        << "Invalid type passed to Create. Expecting PlanarGrabInputDef!";
    return;
  }

  const auto* data = ConvertDef<PlanarGrabInputDef>(def);
  auto& handler = handlers_[entity];
  MathfuVec3FromFbVec3(data->normal(), &handler.plane_normal);
  handler.break_angle = DegreesToRadians(data->break_angle());

  auto* grab_system = registry_->Get<GrabSystem>();
  grab_system->SetInputHandler(entity, this);
}

void PlanarGrabInputSystem::Destroy(Entity entity) {
  auto* grab_system = registry_->Get<GrabSystem>();
  grab_system->RemoveInputHandler(entity, this);
  handlers_.erase(entity);
}

bool PlanarGrabInputSystem::StartGrab(Entity entity,
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

  const mathfu::mat4* grabbed_matrix =
      transform_system->GetWorldFromEntityMatrix(entity);

  if (grabbed_matrix == nullptr) {
    LOG(DFATAL) << "Can't grab an object without a transform.";
    handler.grab_offset = mathfu::kZeros3f;
    return false;
  }

  // Cast the reticle ray into the collision plane, to calculate an initial
  // offset.
  mathfu::vec3 grab_pos;
  const bool found_hit = ComputePlaneIntersection(
      *grabbed_matrix, focus->collision_ray, handler.plane_normal, &grab_pos);

  if (!found_hit) {
    // Ray is not pointing in the same hemisphere as the entity, so cancel the
    // grab.
    handler.grab_offset = mathfu::kZeros3f;
    return false;
  }

  const mathfu::mat4 inverse_mat = grabbed_matrix->Inverse();

  // Lock the cursor to the new entity, with an offset to keep the cursor
  // from jumping.
  const mathfu::vec3 local_cursor_pos = inverse_mat * focus->cursor_position;
  focus_locker->LockOn(device, entity, local_cursor_pos);

  // Store the ideal offset from the entity's origin.
  handler.grab_offset = -local_cursor_pos;
  // Make the offset perpendicular to the collision plane normal.
  handler.grab_offset -=
      handler.plane_normal *
      mathfu::vec3::DotProduct(handler.grab_offset, handler.plane_normal);

  // Store the difference between the ideal grab_offset (based on position at
  // press) and the actual grab_offset (based on position at dragStart).
  const mathfu::vec3 local_plane_intersection = inverse_mat * grab_pos * -1.0f;
  handler.initial_offset = local_plane_intersection - handler.grab_offset;
  return true;
}

Sqt PlanarGrabInputSystem::UpdateGrab(Entity entity,
                                      InputManager::DeviceType device,
                                      const Sqt& original_sqt) {
  auto handler_iter = handlers_.find(entity);
  if (handler_iter == handlers_.end()) {
    LOG(DFATAL) << "Handler not found when starting grab.";
    return original_sqt;
  }
  auto& handler = handler_iter->second;

  auto* transform_system = registry_->Get<TransformSystem>();

  const mathfu::mat4* grabbed_matrix =
      transform_system->GetWorldFromEntityMatrix(entity);

  if (grabbed_matrix == nullptr) {
    LOG(DFATAL) << "Can't grab an object without a transform.";
    return original_sqt;
  }

  auto* input_processor = registry_->Get<InputProcessor>();
  const InputFocus* focus = input_processor->GetInputFocus(device);

  // Cast the reticle ray into the collision plane, to get the world space hit
  // position.
  mathfu::vec3 grab_pos;
  bool found_hit = ComputePlaneIntersection(
      *grabbed_matrix, focus->collision_ray, handler.plane_normal, &grab_pos);

  Sqt result(original_sqt);
  if (found_hit) {
    // grab_pos is in world space, convert to local:
    grab_pos = grabbed_matrix->Inverse() * grab_pos;

    // add back in the initial offset so the user is still grabbing the same
    // part of the entity.
    result.translation +=
        grab_pos + handler.grab_offset + handler.initial_offset;
  }

  handler.initial_offset = handler.initial_offset / kOffsetDecay;

  return result;
}

bool PlanarGrabInputSystem::ShouldCancel(Entity entity,
                                         InputManager::DeviceType device) {
  auto handler_iter = handlers_.find(entity);
  if (handler_iter == handlers_.end()) {
    LOG(DFATAL) << "Handler not found when starting grab.";
    return true;
  }
  auto* transform_system = registry_->Get<TransformSystem>();

  auto* input_processor = registry_->Get<InputProcessor>();
  const InputFocus* focus = input_processor->GetInputFocus(device);

  const mathfu::mat4* grabbed_matrix =
      transform_system->GetWorldFromEntityMatrix(entity);

  if (grabbed_matrix == nullptr) {
    LOG(DFATAL) << "Can't grab an object without a transform.";
    return true;
  }

  const float angle = std::acos(CosAngleFromRay(
      focus->collision_ray, grabbed_matrix->TranslationVector3D()));

  // return true if the grab should cancel;
  return angle >= handler_iter->second.break_angle;
}

void PlanarGrabInputSystem::EndGrab(Entity entity,
                                    InputManager::DeviceType device) {
  auto* focus_locker = registry_->Get<InputFocusLocker>();
  focus_locker->Unlock(device);
}

}  // namespace lull
