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

#include "lullaby/events/input_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/reticle/reticle_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"

namespace lull {
namespace {

static const float kMinFloat = std::numeric_limits<float>::lowest();
static const float kMaxFloat = std::numeric_limits<float>::max();
static const float kDefaultControllerHeight = -0.6f;

const HashValue kReticleBoundedMovementDefHash = Hash("ReticleBoundaryDef");
const lull::InputManager::DeviceType kDevice = lull::InputManager::kController;
const int kDefaultStabilizationFrames = 20;

float GetDeltaPositionFromAngle(float delta_angle, float current_position,
                                float distance) {
  // Calculate the delta position from delta angle, current position and the
  // distance.
  // Denote:
  //   a = current angle,
  //   da = delta angle,
  //   p = current position, which could stand for either x or y value.
  //   dp = delta position,
  //   z = depth.
  // Then the delta position can be calculated by following:
  // a = arctan(p / z),
  // (p + dp) / z = tan(a + da)
  // dp = z * tan(a + da) - p
  //    = z * tan(arctan(p / z) + da) - p
  return distance *
             std::tan(std::atan(current_position / distance) + delta_angle) -
         current_position;
}

mathfu::vec2 GetDeltaPositionFromOrientation(
    const mathfu::vec2& delta_orientation, const mathfu::vec2& current_position,
    const float z) {
  mathfu::vec2 delta_position(
      GetDeltaPositionFromAngle(-delta_orientation.x, current_position.x, z),
      GetDeltaPositionFromAngle(delta_orientation.y, current_position.y, z));
  return delta_position;
}

mathfu::vec2 ClampToBoundary(const mathfu::vec2& position,
                             const mathfu::vec2& horizontal,
                             const mathfu::vec2& vertical) {
  return mathfu::vec2(mathfu::Clamp(position.x, horizontal.x, horizontal.y),
                      mathfu::Clamp(position.y, vertical.x, vertical.y));
}

}  // namespace

ReticleBoundedMovementSystem::ReticleBoundedMovement::ReticleBoundedMovement(
    Entity entity)
    : Component(entity),
      horizontal(kMinFloat, kMaxFloat),
      vertical(kMinFloat, kMaxFloat),
      is_horizontal_only(false),
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
  MathfuVec2FromFbVec2(data->horizontal(), &(iter->second.horizontal));

  if (data->vertical()) {
    MathfuVec2FromFbVec2(data->vertical(), &(iter->second.vertical));
    iter->second.is_horizontal_only = false;
  } else {
    iter->second.is_horizontal_only = true;
  }
}

void ReticleBoundedMovementSystem::Destroy(Entity entity) {
  const size_t num_removed = reticle_movement_map_.erase(entity);
  if (num_removed > 0) {
    registry_->Get<ReticleSystem>()->SetReticleMovementFn(nullptr);
  }
}

void ReticleBoundedMovementSystem::Enable(Entity entity) {
  ResetReticlePosition(entity);
  ResetStabilizationCounter();

  auto* reticle_system = registry_->Get<ReticleSystem>();
  auto fn = [this, entity, reticle_system](
                const lull::InputManager::DeviceType input_device) -> Sqt {
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
      // If no connected HMD device, input manager returns (0, 0, 0).
      const mathfu::vec3 camera_position =
          input->GetDofPosition(InputManager::kHmd);

      // If the reticle is during stabilization, do not update reticle position.
      if (stabilization_counter_ > 0) {
        stabilization_counter_--;
      } else {
        mathfu::vec2 delta_orientation =
            input_orientation - bounded_reticle->input_orientation_last_frame;
        // Clamp delta yaw value into [-pi, pi] to avoid reticle jumping
        // across boundary.
        while (delta_orientation.x > kPi) {
          delta_orientation.x -= kPi * 2;
        }
        while (delta_orientation.x < -kPi) {
          delta_orientation.x += kPi * 2;
        }

        // As the boundary is defined on X-Y plane, it should always be
        // vertical. Therefore we only take x and z values when calculating the
        // depth of the screen by setting y value the same as the camera.
        mathfu::vec3 entity_position_xz =
            transform_system->GetWorldFromEntityMatrix(entity)
                ->TranslationVector3D();
        entity_position_xz.y = camera_position.y;
        const float depth = (entity_position_xz - camera_position).Length();
        const mathfu::vec2 delta_position = GetDeltaPositionFromOrientation(
            delta_orientation, bounded_reticle->reticle_2d_position_last_frame,
            depth);
        reticle_position = ClampToBoundary(reticle_position + delta_position,
                                           bounded_reticle->horizontal,
                                           bounded_reticle->vertical);
        bounded_reticle->reticle_2d_position_last_frame = reticle_position;
      }

      bounded_reticle->input_orientation_last_frame = input_orientation;

      // Calculate collision ray from  controller origin to reticle position.
      sqt.translation = mathfu::vec3(0, kDefaultControllerHeight, 0);
      const mathfu::vec3 reticle_position_in_world_space =
          *(transform_system->GetWorldFromEntityMatrix(entity)) *
          mathfu::vec3(reticle_position, 0);
      mathfu::vec3 direction =
          (reticle_position_in_world_space - sqt.translation).Normalized();

      if (bounded_reticle->is_horizontal_only) {
        // Abadon the relative movement in vertical direction.
        direction.y = 0.0f;
        // First rotate around x axis using the absolute pitch value in vertical
        // direction, then rotate around y axis using the relative movement in
        // horizontal direction.
        sqt.rotation = mathfu::quat::RotateFromToWithAxis(
                           -mathfu::kAxisZ3f, direction, mathfu::kAxisY3f) *
                       mathfu::quat::FromAngleAxis(
                           input_orientation.y +
                               reticle_system->GetReticleErgoAngleOffset(),
                           mathfu::kAxisX3f);
      } else {
        sqt.rotation = mathfu::quat::RotateFromTo(-mathfu::kAxisZ3f, direction);
      }
    } else {
      ResetStabilizationCounter();
    }
    return sqt;
  };
  reticle_system->SetReticleMovementFn(fn);
}

void ReticleBoundedMovementSystem::Disable() {
  auto* reticle_system = registry_->Get<ReticleSystem>();
  reticle_system->SetReticleMovementFn(nullptr);
}

void ReticleBoundedMovementSystem::SetReticleHorizontalBoundary(
    Entity entity, const mathfu::vec2& horizontal) {
  auto iter = reticle_movement_map_.find(entity);
  if (iter != reticle_movement_map_.end()) {
    iter->second.horizontal = horizontal;
  }
}

void ReticleBoundedMovementSystem::SetReticleVerticalBoundary(
    Entity entity, const mathfu::vec2& vertical) {
  auto iter = reticle_movement_map_.find(entity);
  if (iter != reticle_movement_map_.end()) {
    iter->second.vertical = vertical;
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
