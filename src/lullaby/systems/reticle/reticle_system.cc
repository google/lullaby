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

#include "lullaby/systems/reticle/reticle_system.h"

#include "mathfu/constants.h"
#include "lullaby/base/dispatcher.h"
#include "lullaby/base/input_manager.h"
#include "lullaby/events/input_events.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/reticle/reticle_system_reticle_provider.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/bits.h"
#include "lullaby/util/common_fb_conversions.h"
#include "lullaby/util/config.h"
#include "lullaby/util/mathfu_fb_conversions.h"
#include "lullaby/util/render_channels.h"
#include "lullaby/util/reticle_provider.h"
#include "lullaby/util/trace.h"
#include "lullaby/util/transform_channels.h"

// Enable the HMD reticle fallback in DEBUG and Linux builds.
#if DEBUG || (defined(__linux__) && !defined(__ANDROID__))
#ifndef LULLABY_HMD_RETICLE
#define LULLABY_HMD_RETICLE
#endif
#endif

namespace lull {

const HashValue kRingDiamaterChannelName = Hash("ring-diameter");
const HashValue kReticleDef = Hash("ReticleDef");
const HashValue kReticleBehaviourDef = Hash("ReticleBehaviourDef");
const HashValue kEnableHmdFallback = Hash("lull.Reticle.EnableHmdFallback");

const int kNumVerticesPerTrailQuad = 4;
const int kNumIndicesPerTrailQuad = 6;

ReticleSystem::Reticle::Reticle(Entity e)
    : Component(e),
      target_entity(kNullEntity),
      pressed_entity(kNullEntity),
      collision_ray(mathfu::kZeros3f, -mathfu::kAxisZ3f),
      no_hit_distance(kDefaultNoHitDistance),
      ergo_angle_offset(0.f),
      ring_active_diameter(0.f),
      ring_inactive_diameter(0.f),
      hit_color(mathfu::kZeros4f),
      no_hit_color(mathfu::kZeros4f),
      use_eye_collision_ray(true) {}

ReticleSystem::ReticleSystem(Registry* registry)
    : System(registry), reticle_behaviours_(16) {
  registry->Register(
      std::unique_ptr<ReticleProvider>(new ReticleSystemReticleProvider(this)));
  RegisterDef(this, kReticleDef);
  RegisterDef(this, kReticleBehaviourDef);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
}

void ReticleSystem::Initialize() {
  // Only attempt to setup the channel if it will succeed. This lets this
  // system's tests function without the AnimationSystem.
  if (registry_->Get<AnimationSystem>() && registry_->Get<RenderSystem>()) {
    UniformChannel::Setup(registry_, 2, kRingDiamaterChannelName,
                                "ring_diameter", 1);
  } else {
    LOG(ERROR) << "Failed to set up the ring_diameter channel due to missing "
                  "Animation or Render system.";
  }
}

void ReticleSystem::Create(Entity entity, HashValue type, const Def* def) {
  CHECK_NE(def, nullptr);

  if (type == kReticleDef) {
    const auto* data = ConvertDef<ReticleDef>(def);
    CreateReticle(entity, data);
  } else if (type == kReticleBehaviourDef) {
    const auto* data = ConvertDef<ReticleBehaviourDef>(def);
    CreateReticleBehaviour(entity, data);
  } else {
    DCHECK(false) << "Unsupported ComponentDef type: " << type;
  }
}

void ReticleSystem::CreateReticle(Entity entity, const ReticleDef* data) {
  reticle_.reset(new Reticle(entity));

  if (data->ring_active_diameter() != 0) {
    reticle_->ring_active_diameter = data->ring_active_diameter();
  }
  if (data->ring_inactive_diameter() != 0) {
    reticle_->ring_inactive_diameter = data->ring_inactive_diameter();
  }
  if (data->no_hit_distance() != 0) {
    reticle_->no_hit_distance = data->no_hit_distance();
  }
  reticle_->ergo_angle_offset = data->ergo_angle_offset();

  if (data->device_preference()) {
    for (uint32_t i = 0; i < data->device_preference()->size(); ++i) {
      const auto device = data->device_preference()->GetEnum<DeviceType>(i);
      reticle_->device_preference.push_back(TranslateInputDeviceType(device));
    }
  } else {
    reticle_->device_preference = {InputManager::kController,
                                   InputManager::kHmd};
  }

#if defined(LULLABY_HMD_RETICLE)
  bool hmd_fallback = true;
#else
  bool hmd_fallback = false;
#endif
  auto* config = registry_->Get<Config>();
  if (config) {
    hmd_fallback = config->Get(kEnableHmdFallback, hmd_fallback);
  }
  if (hmd_fallback) {
    reticle_->device_preference.push_back(InputManager::kHmd);
  }

  reticle_->use_eye_collision_ray = data->use_eye_collision_ray();
  MathfuVec4FromFbColor(data->hit_color(), &reticle_->hit_color);
  MathfuVec4FromFbColor(data->no_hit_color(), &reticle_->no_hit_color);

  const float inner_hole = data->inner_hole();
  const float inner_ring_end = data->inner_ring_end();
  const float inner_ring_thickness = data->inner_ring_thickness();
  const float mid_ring_end = data->mid_ring_end();
  const float mid_ring_opacity = data->mid_ring_opacity();

  // Set some initial uniform values.
  auto* render_system = registry_->Get<RenderSystem>();
  if (render_system) {
    mathfu::vec4 reticle_color = reticle_->no_hit_color;
    render_system->SetUniform(entity, "color", &reticle_color[0], 4);
    render_system->SetUniform(entity, "ring_diameter",
                              &reticle_->ring_inactive_diameter, 1);
    render_system->SetUniform(entity, "inner_hole", &inner_hole, 1);
    render_system->SetUniform(entity, "inner_ring_end", &inner_ring_end, 1);
    render_system->SetUniform(entity, "inner_ring_thickness",
                              &inner_ring_thickness, 1);
    render_system->SetUniform(entity, "mid_ring_end", &mid_ring_end, 1);
    render_system->SetUniform(entity, "mid_ring_opacity", &mid_ring_opacity, 1);
  }
}

void ReticleSystem::CreateReticleBehaviour(Entity entity,
                                           const ReticleBehaviourDef* data) {
  auto* behaviour = reticle_behaviours_.Emplace(entity);
  if (data->hover_start_dead_zone()) {
    mathfu::vec3 out_vec;
    MathfuVec3FromFbVec3(data->hover_start_dead_zone(), &out_vec);
    behaviour->hover_start_dead_zone = out_vec;
  }
  behaviour->collision_behaviour = data->collision_behaviour();
}

void ReticleSystem::PostCreateInit(Entity entity, HashValue type,
                                   const Def* def) {
  if (type != kReticleBehaviourDef) {
    return;
  }

  const auto* data = ConvertDef<ReticleBehaviourDef>(def);
  auto* behaviour = reticle_behaviours_.Get(entity);

  if (behaviour->collision_behaviour ==
          ReticleCollisionBehaviour::
              ReticleCollisionBehaviour_HandleDescendants &&
      data->interactive_if_handle_descendants()) {
    auto* collision_system = registry_->Get<CollisionSystem>();
    if (collision_system) {
      collision_system->EnableInteraction(entity);
      collision_system->EnableDefaultInteraction(entity);
    }
  }
}

void ReticleSystem::Destroy(Entity entity) {
  if (reticle_ && reticle_->GetEntity() == entity) {
    reticle_.reset();
  }

  reticle_behaviours_.Destroy(entity);
}

void ReticleSystem::AdvanceFrame(const Clock::duration& delta_time) {
  LULLABY_CPU_TRACE_CALL();
  if (!reticle_) {
    return;
  }

  const Entity entity = reticle_->GetEntity();
  auto input = registry_->Get<InputManager>();
  auto transform_system = registry_->Get<TransformSystem>();
  auto animation_system = registry_->Get<AnimationSystem>();

  auto device = GetActiveDevice();

  Sqt sqt;

  if (device == InputManager::kMaxNumDeviceTypes) {
    // No valid connected input device.
    sqt.scale = mathfu::kZeros3f;
    transform_system->SetWorldFromEntityMatrix(entity,
                                               CalculateTransformMatrix(sqt));
    return;
  }

  // Getting the rotation from the input device, check for collisions in the
  // direction of the rotation, then adjust the position of the reticle using
  // the rotation and the hit distance.

  sqt.rotation = input->GetDofRotation(device);
  sqt.rotation =
      sqt.rotation * mathfu::quat::FromAngleAxis(reticle_->ergo_angle_offset,
                                                 mathfu::kAxisX3f);

  if (input->HasPositionDof(device)) {
    sqt.translation = input->GetDofPosition(device);
  }

  // Get world transform from any existing parent transformations.
  const mathfu::mat4* world_from_parent =
      transform_system->GetWorldFromEntityMatrix(
          transform_system->GetParent(entity));
  if (world_from_parent) {
    // Apply any world transform to account for the actual forward direction of
    // the preferred device and the raycast origin. This allows to add the
    // reticle entity as a child to a parent entity and have the reticle behave
    // correctly when the parent entity is moved and rotated in world space.
    Sqt world_xform = CalculateSqtFromMatrix(*world_from_parent);
    sqt.rotation = world_xform.rotation * sqt.rotation;
    sqt.translation += world_xform.translation;
  }

  // Calculate the forward vector of reticle given its rotation
  auto forward = sqt.rotation * -mathfu::kAxisZ3f;

  mathfu::vec3 start_collision_ray = sqt.translation;
  mathfu::vec3 collision_ray_direction = forward;

  // Set reticle position to be a default depth in the direction of its
  // forward vector
  sqt.translation += reticle_->no_hit_distance * forward;

  auto collision_system = registry_->Get<CollisionSystem>();
  if (!collision_system) {
    // If there is no collision system, this is the reticle's final position.
    transform_system->SetWorldFromEntityMatrix(entity,
                                               CalculateTransformMatrix(sqt));
    return;
  }

  // If use_eye_collision_ray is true, then shoot a ray from the eye to the
  // reticle's current position to check for collision. We do this because we
  // want to know when the drawn reticle, from the user's eye perspective,
  // passes over an element.

  if (reticle_->use_eye_collision_ray) {
    // Get camera position if there is one
    mathfu::vec3 camera_position = mathfu::kZeros3f;

    if (input->HasPositionDof(InputManager::kHmd)) {
      camera_position = input->GetDofPosition(InputManager::kHmd);
    }

    start_collision_ray = camera_position;
    collision_ray_direction = (sqt.translation - camera_position).Normalized();
  }

  reticle_->collision_ray = Ray(start_collision_ray, collision_ray_direction);
  const auto collision =
      collision_system->CheckForCollision(reticle_->collision_ray);

  float depth = collision.distance;
  Entity targeted_entity = collision.entity;

  // Compute where the reticle should reside before attempting to find a reticle
  // collision group.
  if (targeted_entity != kNullEntity) {
    sqt.translation = start_collision_ray + (depth * collision_ray_direction);
    sqt.scale *= depth / reticle_->no_hit_distance;
  }

  transform_system->SetWorldFromEntityMatrix(entity,
                                             CalculateTransformMatrix(sqt));

  // If specified, attempt to find an ancestor that is designated to handle
  // reticle events for its descendants. If no such entity is found, the
  // collision will be handled by the original entity.
  auto* behaviour = reticle_behaviours_.Get(targeted_entity);
  if (behaviour &&
      behaviour->collision_behaviour ==
          ReticleCollisionBehaviour::ReticleCollisionBehaviour_FindAncestor) {
    Entity parent = transform_system->GetParent(targeted_entity);
    while (parent != kNullEntity) {
      behaviour = reticle_behaviours_.Get(parent);
      if (behaviour && behaviour->collision_behaviour ==
                           ReticleCollisionBehaviour::
                               ReticleCollisionBehaviour_HandleDescendants) {
        targeted_entity = parent;
        break;
      }
      parent = transform_system->GetParent(parent);
    }
    if (parent == kNullEntity) {
      LOG(DFATAL) << "Entity specified with FindAncestor collision behaviour, "
                     "but no ancestor with HandleDescendants found.";
    }
  }

  const bool is_interactive =
      collision_system->IsInteractionEnabled(targeted_entity);

  // Target is changing, send events and update the reticle color.
  if (reticle_->target_entity != targeted_entity || !is_interactive) {
    auto dispatcher = registry_->Get<Dispatcher>();
    auto dispatcher_system = registry_->Get<DispatcherSystem>();
    auto render_system = registry_->Get<RenderSystem>();

    if (reticle_->target_entity != kNullEntity) {
      dispatcher->Send(StopHoverEvent(reticle_->target_entity));
      if (dispatcher_system) {
        dispatcher_system->Send(reticle_->target_entity,
                                StopHoverEvent(reticle_->target_entity));
      }
    }
    // Dead zone checks should be performed on the original collided entity, not
    // the entity that takes the targetting (of one exists).
    reticle_->target_entity =
        (is_interactive && !IsInsideEntityDeadZone(collision.entity))
            ? targeted_entity
            : kNullEntity;
    if (reticle_->target_entity != kNullEntity) {
      dispatcher->Send(StartHoverEvent(reticle_->target_entity));
      if (dispatcher_system) {
        dispatcher_system->Send(reticle_->target_entity,
                                StartHoverEvent(reticle_->target_entity));
      }
    }

    const float ring_diameter = is_interactive
                                    ? reticle_->ring_active_diameter
                                    : reticle_->ring_inactive_diameter;
    if (animation_system) {
      const auto kAnimTime = std::chrono::milliseconds(250);
      animation_system->SetTarget(entity, kRingDiamaterChannelName,
                                  &ring_diameter, 1, kAnimTime);
    } else {
      render_system->SetUniform(entity, "ring_diameter", &ring_diameter, 1);
    }

    const float* color_data =
        is_interactive ? &reticle_->hit_color[0] : &reticle_->no_hit_color[0];
    render_system->SetUniform(entity, "color", color_data, 4);
  }

  reticle_->ms_since_press +=
      std::chrono::duration_cast<std::chrono::milliseconds>(delta_time).count();
  const auto button =
      input->GetButtonState(device, InputManager::kPrimaryButton);
  if (CheckBit(button, InputManager::kJustPressed)) {
    reticle_->ms_since_press = 0;
    auto dispatcher = registry_->Get<Dispatcher>();
    auto dispatcher_system = registry_->Get<DispatcherSystem>();

    reticle_->pressed_entity =
        is_interactive ? reticle_->target_entity : kNullEntity;

    mathfu::vec3 local_collision_point;
    if (reticle_->pressed_entity) {
      const mathfu::vec3 global_collision_point =
          reticle_->collision_ray.origin +
          reticle_->collision_ray.direction * collision.distance;
      local_collision_point =
          transform_system->GetWorldFromEntityMatrix(reticle_->pressed_entity)
              ->Inverse() *
          global_collision_point;
    }
    const ClickEvent event(reticle_->pressed_entity, local_collision_point);
    // Send a global ClickEvent dispatch, even if no target was hit.
    dispatcher->Send(event);
    // If some entity was hit, send it a ClickEvent.
    if (reticle_->pressed_entity != kNullEntity && dispatcher_system) {
      dispatcher_system->Send(reticle_->pressed_entity, event);
    }
  } else if (CheckBit(button, InputManager::kJustReleased)) {
    auto dispatcher = registry_->Get<Dispatcher>();
    auto dispatcher_system = registry_->Get<DispatcherSystem>();

    const Entity released_entity =
        is_interactive ? reticle_->target_entity : kNullEntity;
    const ClickReleasedEvent event(reticle_->pressed_entity, released_entity);
    // Send a global ClickReleasedEvent dispatch, even if there is no target.
    dispatcher->Send(event);
    // If there was an entity, send it a ClickReleasedEvent.
    if (reticle_->pressed_entity != kNullEntity && dispatcher_system) {
      dispatcher_system->Send(reticle_->pressed_entity, event);
    }
    // Also send a ClickReleasedEvent to the current target (released) entity,
    // but only if it's different than the pressed_entity
    if (released_entity != kNullEntity && dispatcher_system &&
        reticle_->pressed_entity != released_entity) {
      dispatcher_system->Send(released_entity, event);
    }

    if (released_entity == reticle_->pressed_entity) {
      const ClickPressedAndReleasedEvent click_press_and_release_event(
          released_entity, reticle_->ms_since_press);
      dispatcher_system->Send(released_entity, click_press_and_release_event);
      dispatcher->Send(click_press_and_release_event);
    }

    reticle_->pressed_entity = kNullEntity;
  }
}

bool ReticleSystem::IsInsideEntityDeadZone(Entity collided_entity) const {
  auto* behaviour = reticle_behaviours_.Get(collided_entity);
  if (!behaviour) {
    return false;
  }
  if (behaviour->hover_start_dead_zone == mathfu::kZeros3f) {
    // Entity has no artificial shrinking of the hover-start Aabb.
    return false;
  }

  const auto* transform_system = registry_->Get<TransformSystem>();
  const Aabb* aabb = transform_system->GetAabb(collided_entity);
  if (!aabb) {
    LOG(DFATAL) << "Collided entity must have an Aabb.";
    return false;
  }

  const mathfu::mat4* world_from_collided =
      transform_system->GetWorldFromEntityMatrix(collided_entity);
  if (!world_from_collided) {
    LOG(DFATAL) << "Collided entity should have a world matrix.";
    return false;
  }

  const Aabb modified_aabb(aabb->min + behaviour->hover_start_dead_zone,
                           aabb->max - behaviour->hover_start_dead_zone);
  return !ComputeLocalRayOBBCollision(reticle_->collision_ray,
                                      *world_from_collided, modified_aabb);
}
Entity ReticleSystem::GetReticle() const {
  if (reticle_) {
    return reticle_->GetEntity();
  }
  return kNullEntity;
}

Entity ReticleSystem::GetTarget() const {
  if (reticle_) {
    return reticle_->target_entity;
  }
  return kNullEntity;
}

Ray ReticleSystem::GetCollisionRay() const {
  if (reticle_) {
    return reticle_->collision_ray;
  }
  // Default to pointing forward.
  return Ray(mathfu::kZeros3f, -mathfu::kAxisZ3f);
}

void ReticleSystem::SetNoHitDistance(float distance) {
  if (reticle_) {
    reticle_->no_hit_distance = distance;
  }
}

InputManager::DeviceType ReticleSystem::GetActiveDevice() const {
  auto input = registry_->Get<InputManager>();
  if (!reticle_) {
    return InputManager::kMaxNumDeviceTypes;
  }
  for (InputManager::DeviceType device : reticle_->device_preference) {
    if (input->IsConnected(device) && input->HasRotationDof(device)) {
      return device;
    }
  }
  return InputManager::kMaxNumDeviceTypes;
}

float ReticleSystem::GetReticleErgoAngleOffset() const {
  if (reticle_) {
    return reticle_->ergo_angle_offset;
  } else {
    return .0;
  }
}

}  // namespace lull
