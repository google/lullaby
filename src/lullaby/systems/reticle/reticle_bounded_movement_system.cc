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

#include "lullaby/systems/reticle/reticle_bounded_movement_system.h"

#include "lullaby/base/dispatcher.h"
#include "lullaby/base/input_manager.h"
#include "lullaby/events/input_events.h"
#include "lullaby/systems/reticle/reticle_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/util/mathfu_fb_conversions.h"

namespace lull {
namespace {
const HashValue kReticleBoundedMovementDefHash = Hash("ReticleBoundaryDef");
const lull::InputManager::DeviceType kDevice = lull::InputManager::kController;
const int kDefaultStabilizationFrames = 20;
const mathfu::vec2 kDefaultLowerBound = mathfu::vec2(-1.0f, -1.0f);
const mathfu::vec2 kDefaultUpperBound = mathfu::vec2(1.0f, 1.0f);

mathfu::vec2 GetDeltaPositionFromOrientation(
    const mathfu::vec2& delta_orientation) {
  // TODO(b/62788965): Figure out the math to make reticle move evenly. Now the
  // reticle is moving faster in the center than near the boundary. Temporarily,
  // delta position values is simply set by doubling the delta degree values to
  // make the reticle more sensitive.
  const mathfu::vec2 kFactor = mathfu::vec2(-2.0f, 2.0f);
  mathfu::vec2 delta_position = kFactor * delta_orientation;
  return delta_position;
}

mathfu::vec2 ClampToBoundary(const mathfu::vec2& position,
                             const mathfu::vec2& lower_left_bound,
                             const mathfu::vec2& upper_right_bound) {
  return mathfu::vec2(
      mathfu::Clamp(position.x, lower_left_bound.x, upper_right_bound.x),
      mathfu::Clamp(position.y, lower_left_bound.y, upper_right_bound.y));
}

}  // namespace

ReticleBoundedMovementSystem::ReticleBoundedMovement::ReticleBoundedMovement(
    Entity entity)
    : Component(entity),
      lower_left_bound(kDefaultLowerBound),
      upper_right_bound(kDefaultUpperBound),
      reticle_2d_position_last_frame(mathfu::kZeros2f),
      input_orientation_last_frame(mathfu::kZeros2f) {}

ReticleBoundedMovementSystem::ReticleBoundedMovementSystem(Registry* registry)
    : System(registry),
      stabilization_frames_(kDefaultStabilizationFrames) {
  RegisterDef(this, kReticleBoundedMovementDefHash);
  RegisterDependency<ReticleSystem>(this);
  auto* dispatcher = registry->Get<lull::Dispatcher>();
  dispatcher->Connect(this, [this](const lull::GlobalRecenteredEvent& event) {
    for (auto it = reticle_movement_map_.begin();
         it != reticle_movement_map_.end(); ++it) {
      ResetReticlePosition(it->first);
    }
    ResetStabilizationCounter();
  });
}

void ReticleBoundedMovementSystem::PostCreateInit(Entity entity, HashValue type,
                                                  const Def* def) {
  if (type != kReticleBoundedMovementDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting "
                   "ReticleBoundedMovementDef!";
    return;
  }

  const auto* data = ConvertDef<ReticleBoundaryDef>(def);
  auto iter =
      reticle_movement_map_.emplace(entity, ReticleBoundedMovement(entity))
          .first;
  if (data->lower_left_bound()) {
    MathfuVec2FromFbVec2(data->lower_left_bound(),
                         &(iter->second.lower_left_bound));
  }

  if (data->upper_right_bound()) {
    MathfuVec2FromFbVec2(data->upper_right_bound(),
                         &(iter->second.upper_right_bound));
  }
}

void ReticleBoundedMovementSystem::Destroy(Entity entity) {
  reticle_movement_map_.erase(entity);
  registry_->Get<ReticleSystem>()->SetReticleMovementFn(nullptr);
}

void ReticleBoundedMovementSystem::Enable(Entity entity) {
  ResetReticlePosition(entity);
  ResetStabilizationCounter();

  auto fn = [this,
             entity](const lull::InputManager::DeviceType input_device) -> Sqt {
    Sqt sqt;
    auto iter = reticle_movement_map_.find(entity);
    if (iter == reticle_movement_map_.end()) {
      LOG(WARNING) << "No defined bounded movement for reticle " << entity
                   << " found.";
      return sqt;
    }

    ReticleBoundedMovement* bounded_reticle = &(iter->second);
    // Update 2D reticle position.
    auto input = registry_->Get<InputManager>();
    auto transform_system = registry_->Get<TransformSystem>();
    mathfu::vec2 reticle_position =
        bounded_reticle->reticle_2d_position_last_frame;
    if (input->IsConnected(input_device)) {
      const mathfu::quat controller_quat = input->GetDofRotation(input_device);
      const mathfu::vec2 input_orientation =
          mathfu::vec2(lull::GetHeadingRadians(controller_quat),
                       lull::GetPitchRadians(controller_quat));

      // If the reticle is during stabilization, do not update reticle position.
      if (stabilization_counter_ > 0) {
        stabilization_counter_--;
      } else {
        const mathfu::vec2 delta_orientation =
            input_orientation - bounded_reticle->input_orientation_last_frame;
        const mathfu::vec2 delta_position =
            GetDeltaPositionFromOrientation(delta_orientation);
        reticle_position = ClampToBoundary(reticle_position + delta_position,
                                           bounded_reticle->lower_left_bound,
                                           bounded_reticle->upper_right_bound);
        bounded_reticle->reticle_2d_position_last_frame = reticle_position;
      }

      bounded_reticle->input_orientation_last_frame = input_orientation;
    } else {
      ResetStabilizationCounter();
    }

    // Calculate collision ray from camera position to reticle position.
    const mathfu::vec3 reticle_position_in_world_space =
        *(transform_system->GetWorldFromEntityMatrix(entity)) *
        mathfu::vec3(reticle_position, 0);
    // If no connected HMD device, input manager returns (0, 0, 0).
    const mathfu::vec3 camera_position =
        input->GetDofPosition(InputManager::kHmd);
    const mathfu::vec3 direction =
        (reticle_position_in_world_space - camera_position).Normalized();

    sqt.translation = camera_position;
    sqt.rotation = mathfu::quat::RotateFromTo(-mathfu::kAxisZ3f, direction);
    return sqt;
  };
  auto* reticle_system = registry_->Get<ReticleSystem>();
  reticle_system->SetReticleMovementFn(fn);
}

void ReticleBoundedMovementSystem::Disable() {
  auto* reticle_system = registry_->Get<ReticleSystem>();
  reticle_system->SetReticleMovementFn(nullptr);
}

void ReticleBoundedMovementSystem::SetReticleBoundary(
    Entity entity, const mathfu::vec2& lower_left_bound,
    const mathfu::vec2& upper_right_bound) {
  auto iter = reticle_movement_map_.find(entity);
  if (iter != reticle_movement_map_.end()) {
    iter->second.lower_left_bound = lower_left_bound;
    iter->second.upper_right_bound = upper_right_bound;
  }
}

void ReticleBoundedMovementSystem::ResetReticlePosition(Entity entity) {
  auto iter = reticle_movement_map_.find(entity);
  if (iter == reticle_movement_map_.end()) {
    LOG(WARNING) << "No defined bounded movement for reticle " << entity
                 << " found.";
    return;
  }
  ReticleBoundedMovement* bounded_reticle = &(iter->second);
  bounded_reticle->reticle_2d_position_last_frame = mathfu::vec2(0, 0);
}

void ReticleBoundedMovementSystem::ResetStabilizationCounter() {
  stabilization_counter_ = stabilization_frames_;
}

void ReticleBoundedMovementSystem::SetStabilizationFrames(int frames) {
  stabilization_frames_ = frames;
}

}  // namespace lull
