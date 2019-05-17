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

#include "lullaby/contrib/planar_grab/planar_grab_system.h"

#include "lullaby/contrib/planar_grab/planar_grab_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/generated/planar_grabbable_def_generated.h"
#include "mathfu/io.h"

namespace lull {

namespace {
const HashValue kPlanarGrabbableDefHash = ConstHash("PlanarGrabbableDef");
}  // namespace

PlanarGrabSystem::PlanarGrabSystem(Registry* registry)
    : System(registry), grabbables_(4) {
  RegisterDef<PlanarGrabbableDefT>(this);
  RegisterDependency<DispatcherSystem>(this);
  RegisterDependency<InputProcessor>(this);
}

void PlanarGrabSystem::Create(Entity entity, HashValue type, const Def* def) {
  Grabbable grabbable(entity);

  auto* data = ConvertDef<PlanarGrabbableDef>(def);
  MathfuVec3FromFbVec3(data->normal(), &grabbable.plane_normal);
  grabbable.local_orientation = data->local_orientation();
  grabbables_.emplace(entity, grabbable);

  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Connect(
      entity, this, [this, entity](const ClickEvent& event) { OnGrab(event); });
  dispatcher_system->Connect(entity, this,
                             [this, entity](const ClickReleasedEvent& event) {
                               OnGrabReleased(event);
                             });
}

void PlanarGrabSystem::Destroy(Entity entity) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Disconnect<ClickEvent>(entity, this);
  dispatcher_system->Disconnect<ClickReleasedEvent>(entity, this);
  grabbables_.erase(entity);
  grabbed_.erase(entity);
}

Optional<mathfu::vec3> PlanarGrabSystem::GetGrabOrigin(Entity entity) {
  auto data = grabbed_.find(entity);
  if (data != grabbed_.end()) {
    return data->second.grab_origin;
  }
  return Optional<mathfu::vec3>();  // entity not found
}

Optional<Plane> PlanarGrabSystem::GetGrabPlane(Entity entity) {
  auto data = grabbed_.find(entity);
  if (data != grabbed_.end()) {
    return data->second.plane;
  }
  return Optional<Plane>();  // entity not found
}

void PlanarGrabSystem::AdvanceFrame(const Clock::duration& delta_time) {
  // Determine sqt of controller this frame.
  auto* transform_system = registry_->Get<TransformSystem>();
  auto* input_processor = registry_->Get<InputProcessor>();

  const InputFocus* focus =
      input_processor->GetInputFocus(input_processor->GetPrimaryDevice());
  const Ray controller_ray = focus->collision_ray;

  for (auto& g : grabbed_) {
    const Grabbable& grabbable = grabbables_.at(g.first);
    GrabData& data = g.second;

    // Get the entity's current worldspace pose.
    const mathfu::mat4* world_from_object_matrix =
        transform_system->GetWorldFromEntityMatrix(data.entity);
    if (world_from_object_matrix == nullptr) {
      continue;
    }

    // Update the plane constraint to account for object's current pose.
    //  - origin should be at object's current position.
    //  - origin & normal should be converted into world-space.
    const mathfu::vec3 plane_position =
        (*world_from_object_matrix * mathfu::vec4(data.grab_local_offset, 1.0f))
            .xyz();
    mathfu::vec3 plane_direction = grabbable.plane_normal;
    if (grabbable.local_orientation) {
      plane_direction =
          (*world_from_object_matrix * mathfu::vec4(plane_direction, 0.0f))
              .xyz().Normalized();
    }
    data.plane = Plane(plane_position, plane_direction);

    // Get the world-space hit point of the controller ray & this plane.
    mathfu::vec3 hit;
    if (!ComputeRayPlaneCollision(controller_ray, data.plane, &hit)) {
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

void PlanarGrabSystem::OnGrab(const ClickEvent& event) {
  const Grabbable& grabbable = grabbables_.at(event.target);

  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::mat4* world_from_object_matrix =
      transform_system->GetWorldFromEntityMatrix(event.target);
  const mathfu::vec3 grab_location_world_space =
      (*world_from_object_matrix) * event.location;

  mathfu::vec3 plane_normal = grabbable.plane_normal;
  // If the plane constraint's normal is defined in the object's local space,
  // transform it into world coordinates.
  if (grabbable.local_orientation) {
    const mathfu::vec4 local_normal(grabbable.plane_normal, 0.0f);
    plane_normal =
        ((*world_from_object_matrix) * local_normal).xyz().Normalized();
  }
  const Plane p(grab_location_world_space, plane_normal);

  GrabData data(event.target, event.location, grab_location_world_space, p);

  grabbed_.emplace(event.target, data);

  const PlanarGrabEvent grab_event(event.target, event.location);
  SendEvent(registry_, event.target, grab_event);
}

void PlanarGrabSystem::OnGrabReleased(const ClickReleasedEvent& event) {
  const Entity entity = event.pressed_entity;
  auto data = grabbed_.find(entity);
  if (data == grabbed_.end()) {
    return;
  }

  const PlanarGrabReleasedEvent grab_event(entity);
  SendEvent(registry_, entity, grab_event);

  grabbed_.erase(entity);
}

}  // namespace lull
