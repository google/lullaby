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

#include "lullaby/contrib/reticle/reticle_system.h"

#include "lullaby/generated/input_behavior_def_generated.h"
#include "lullaby/events/input_events.h"
#include "lullaby/modules/animation_channels/render_channels.h"
#include "lullaby/modules/animation_channels/transform_channels.h"
#include "lullaby/modules/config/config.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/flatbuffers/common_fb_conversions.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/reticle/input_focus_locker.h"
#include "lullaby/modules/reticle/standard_input_pipeline.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/contrib/cursor/cursor_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/contrib/input_behavior/input_behavior_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/bits.h"
#include "lullaby/util/trace.h"
#include "mathfu/constants.h"

#include "mathfu/io.h"

// Enable the HMD reticle fallback in DEBUG and Linux builds.
#if DEBUG || (defined(__linux__) && !defined(__ANDROID__))
#ifndef LULLABY_HMD_RETICLE
#define LULLABY_HMD_RETICLE
#endif
#endif

namespace lull {

const HashValue kReticleDef = ConstHash("ReticleDef");
const HashValue kReticleBehaviourDef = ConstHash("ReticleBehaviourDef");
const HashValue kCursorDef = ConstHash("CursorDef");
const HashValue kEnableHmdFallback =
    ConstHash("lull.Reticle.EnableHmdFallback");

ReticleSystem::Reticle::Reticle(Entity e)
    : Component(e),
      target_entity(kNullEntity),
      pressed_entity(kNullEntity) {}

ReticleSystem::ReticleSystem(Registry* registry) : System(registry) {
  RegisterDef<ReticleDefT>(this);
  RegisterDef<ReticleBehaviourDefT>(this);
  RegisterDependency<CursorSystem>(this);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);

  // Create the classes and systems that replaced reticle_system:
  auto* entity_factory = registry_->Get<EntityFactory>();
  if (entity_factory != nullptr) {
    entity_factory->CreateSystem<CursorSystem>();
    entity_factory->CreateSystem<InputBehaviorSystem>();
  }
  registry_->Create<InputFocusLocker>(registry_);

  // InputProcessor may already have been made with a different event logic
  // option.
  if (registry_->Get<InputProcessor>() == nullptr) {
    registry_->Create<InputProcessor>(registry_,
                                      InputProcessor::kLegacyEventsAndLogic);
  }
  if (registry_->Get<StandardInputPipeline>() == nullptr) {
    registry_->Create<StandardInputPipeline>(registry_);
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
  auto* input_processor = registry_->Get<InputProcessor>();

  std::vector<InputManager::DeviceType> device_preference;
  if (data->device_preference()) {
    for (uint32_t i = 0; i < data->device_preference()->size(); ++i) {
      const auto device = data->device_preference()->GetEnum<DeviceType>(i);
      device_preference.push_back(TranslateInputDeviceType(device));
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
      device_preference.push_back(InputManager::kHmd);
    }
  } else {
    device_preference = {InputManager::kController, InputManager::kHmd};
  }
  auto* pipeline = registry_->Get<StandardInputPipeline>();
  pipeline->SetDevicePreference(device_preference);
  input_processor->SetPrimaryDevice(pipeline->GetPrimaryDevice());

  auto* cursor_system = registry_->Get<CursorSystem>();
  if (cursor_system) {
    CursorDefT cursor;
    cursor.ring_active_diameter = data->ring_active_diameter();
    cursor.ring_inactive_diameter = data->ring_inactive_diameter();
    cursor.no_hit_distance = data->no_hit_distance();
    Color4fFromFbColor(data->hit_color(), &cursor.hit_color);
    Color4fFromFbColor(data->no_hit_color(), &cursor.no_hit_color);
    cursor.inner_hole = data->inner_hole();
    cursor.inner_ring_end = data->inner_ring_end();
    cursor.inner_ring_thickness = data->inner_ring_thickness();
    cursor.mid_ring_end = data->mid_ring_end();
    cursor.mid_ring_opacity = data->mid_ring_opacity();

    cursor_system->CreateComponent(entity, Blueprint(&cursor));
  }
}

void ReticleSystem::CreateReticleBehaviour(Entity entity,
                                           const ReticleBehaviourDef* data) {
  auto* input_behavior_system = registry_->Get<InputBehaviorSystem>();
  if (input_behavior_system != nullptr) {
    InputBehaviorDefT behavior;

    if (data->hover_start_dead_zone()) {
      MathfuVec3FromFbVec3(data->hover_start_dead_zone(),
                           &behavior.focus_start_dead_zone);
    }
    switch (data->collision_behaviour()) {
      case ReticleCollisionBehaviour::ReticleCollisionBehaviour_HandleAlone:
        behavior.behavior_type =
            InputBehaviorType::InputBehaviorType_HandleAlone;
        break;
      case ReticleCollisionBehaviour::ReticleCollisionBehaviour_FindAncestor:
        behavior.behavior_type =
            InputBehaviorType::InputBehaviorType_FindAncestor;
        break;
      case ReticleCollisionBehaviour::
          ReticleCollisionBehaviour_HandleDescendants:
        behavior.behavior_type =
            InputBehaviorType::InputBehaviorType_HandleDescendants;
        break;
    }
    behavior.draggable = data->draggable();
    input_behavior_system->CreateComponent(entity, Blueprint(&behavior));
  } else {
    LOG(DFATAL) << "Tried to create ReticleBehavior, but InputBehaviorSystem "
                   "did not exist.";
  }
}

void ReticleSystem::PostCreateInit(Entity entity, HashValue type,
                                   const Def* def) {
  if (type == kReticleDef) {
    auto* cursor_system = registry_->Get<CursorSystem>();
    if (cursor_system) {
      cursor_system->PostCreateInit(entity, kCursorDef, def);
    }
    return;
  }

  if (type == kReticleBehaviourDef) {
    auto* input_behavior_system = registry_->Get<InputBehaviorSystem>();
    if (input_behavior_system != nullptr) {
      const auto* data = ConvertDef<ReticleBehaviourDef>(def);
      InputBehaviorDefT behavior;
      behavior.interactive_if_handle_descendants =
          data->interactive_if_handle_descendants();
      input_behavior_system->PostCreateComponent(entity, Blueprint(&behavior));
    }
    return;
  }
}

void ReticleSystem::Destroy(Entity entity) {
  if (reticle_ && reticle_->GetEntity() == entity) {
    reticle_.reset();
    auto* input_processor = registry_->Get<InputProcessor>();
    if (input_processor) {
      input_processor->SetPrimaryDevice(InputManager::kMaxNumDeviceTypes);
    }
  }
}

void ReticleSystem::AdvanceFrame(const Clock::duration& delta_time) {
  LULLABY_CPU_TRACE_CALL();
  if (!reticle_) {
    return;
  }
  auto* cursor_system = registry_->Get<CursorSystem>();
  auto* input_processor = registry_->Get<InputProcessor>();

  auto* pipeline = registry_->Get<StandardInputPipeline>();
  auto device = pipeline->GetPrimaryDevice();
  input_processor->SetPrimaryDevice(device);

  if (device == InputManager::kMaxNumDeviceTypes) {
    cursor_system->AdvanceFrame(delta_time);
    return;
  }

  // Determine the focused entity.
  InputFocus focus;
  focus.device = device;

  // Optionally use movement_fn and smoothing_fn to calc collision_ray.
  CalculateFocusPositions(reticle_->GetEntity(), delta_time, &focus);

  // Set cursor position to be a default depth in the direction of its forward
  // vector, and calculate the direction of the collision_ray.
  focus.cursor_position =
      cursor_system->CalculateCursorPosition(device, focus.collision_ray);
  focus.no_hit_cursor_position = focus.cursor_position;

  // Make the collision come from the hmd instead of the controller under some
  // circumstances.
  pipeline->MaybeMakeRayComeFromHmd(&focus);

  pipeline->ApplySystemsToInputFocus(&focus);

  // Send Events.
  input_processor->UpdateDevice(delta_time, focus);

  // Update Cursor rendering and placement
  cursor_system->SetDevice(reticle_->GetEntity(), device);
  cursor_system->AdvanceFrame(delta_time);
}

void ReticleSystem::CalculateFocusPositions(Entity reticle_entity,
                                            const Clock::duration& delta_time,
                                            InputFocus* focus) {
  if (reticle_->movement_fn) {
    focus->collision_ray = reticle_->movement_fn(focus->device);
  } else {
    const auto* transform_system = registry_->Get<TransformSystem>();
    focus->collision_ray =
        registry_->Get<StandardInputPipeline>()->GetDeviceSelectionRay(
            focus->device, transform_system->GetParent(reticle_entity));
  }

  if (reticle_->smoothing_fn) {
    focus->collision_ray.direction =
        reticle_->smoothing_fn(focus->collision_ray.direction, delta_time);
  }

  focus->origin = focus->collision_ray.origin;
}

Entity ReticleSystem::GetReticle() const {
  if (reticle_) {
    return reticle_->GetEntity();
  }
  return kNullEntity;
}

Entity ReticleSystem::GetTarget() const {
  auto device = GetActiveDevice();
  auto input_processor = registry_->Get<InputProcessor>();
  const InputFocus* focus = input_processor->GetInputFocus(device);
  if (focus) {
    return focus->target;
  }
  return kNullEntity;
}

Ray ReticleSystem::GetCollisionRay() const {
  auto device = GetActiveDevice();
  auto input_processor = registry_->Get<InputProcessor>();
  const InputFocus* focus = input_processor->GetInputFocus(device);
  if (focus) {
    return focus->collision_ray;
  }
  // Default to pointing forward.
  return Ray(mathfu::kZeros3f, -mathfu::kAxisZ3f);
}

void ReticleSystem::SetNoHitDistance(float distance) {
  if (reticle_) {
    auto* cursor_system = registry_->Get<CursorSystem>();
    cursor_system->SetNoHitDistance(reticle_->GetEntity(), distance);
  }
}

InputManager::DeviceType ReticleSystem::GetActiveDevice() const {
  if (reticle_) {
    auto* input_processor = registry_->Get<InputProcessor>();
    return input_processor->GetPrimaryDevice();
  } else {
    return InputManager::kMaxNumDeviceTypes;
  }
}

void ReticleSystem::SetReticleMovementFn(ReticleMovementFn fn) {
  if (reticle_) {
    reticle_->movement_fn = std::move(fn);
  }
}

void ReticleSystem::SetReticleSmoothingFn(ReticleSmoothingFn fn) {
  if (reticle_) {
    reticle_->smoothing_fn = std::move(fn);
  }
}

void ReticleSystem::LockOn(Entity entity, const mathfu::vec3& offset) {
  auto* input_focus_locker = registry_->Get<InputFocusLocker>();
  input_focus_locker->LockOn(GetActiveDevice(), entity, offset);
}
}  // namespace lull
