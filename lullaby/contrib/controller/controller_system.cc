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

#include "lullaby/contrib/controller/controller_system.h"

#include "lullaby/generated/controller_def_generated.h"
#include "lullaby/contrib/fade/fade_system.h"
#include "lullaby/events/lifetime_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/modules/reticle/reticle_util.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/time.h"
#include "lullaby/util/trace.h"

namespace lull {

namespace {
const HashValue kControllerDefHash = Hash("ControllerDef");
const HashValue kLaserDefHash = Hash("LaserDef");
const float kControllerFadeCos = 0.94f;
const Clock::duration kControllerFadeTime = std::chrono::milliseconds(250);

const float kLaserBendLimit = 60.0f * kDegreesToRadians;
const float kLaserBendThrow = 0.5f;
const mathfu::vec2 kLaserCornerOffsetRange = mathfu::vec2(0.1f, 0.45f);
const mathfu::vec2 kLaserNearFractionRange = mathfu::vec2(0.66f, 0.75f);
const mathfu::vec2 kLaserFarFractionRange = mathfu::vec2(0.66f, 0.5f);
const mathfu::vec2 kLaserAlphaRange = mathfu::vec2(0.5f, 0.95f);
constexpr const char* kEntityFromWorld = "entity_from_world";
constexpr const char* kControlPoints = "control_points";

}  // namespace

ControllerSystem::Controller::Controller(Entity entity) : Component(entity) {}

ControllerSystem::ControllerSystem(Registry* registry)
    : System(registry), controllers_(4) {
  RegisterDef(this, kControllerDefHash);
  RegisterDef(this, kLaserDefHash);
  RegisterDependency<InputManager>(this);
  RegisterDependency<TransformSystem>(this);

  // Reset controller stabilization counters on resume since data can be flaky.
  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->Connect(this, [this](const OnResumeEvent& event) {
      auto* transform_system = registry_->Get<TransformSystem>();
      controllers_.ForEach([transform_system](Controller& controller) {
        transform_system->Disable(controller.GetEntity());
        controller.stabilization_counter = controller.stabilization_frames;
        controller.is_visible = false;
      });
    });
  }
}

ControllerSystem::~ControllerSystem() {
  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->DisconnectAll(this);
  }
}

void ControllerSystem::AdvanceFrame(const Clock::duration& delta_time) {
  LULLABY_CPU_TRACE_CALL();
  if (show_controls_) {
    HandleControllerTransforms();
  }
}

void ControllerSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type == kControllerDefHash) {
    Controller* controller = controllers_.Emplace(entity);

    auto* data = ConvertDef<ControllerDef>(def);

    controller->enable_events = data->enable_events();
    controller->disable_events = data->disable_events();

    controller->stabilization_frames = data->stabilization_frames();
    controller->stabilization_counter = controller->stabilization_frames;

    if (data->controller_type()) {
      if (data->controller_type() == ControllerType_Controller1) {
        controller->controller_type = InputManager::kController;
      } else if (data->controller_type() == ControllerType_Controller2) {
        controller->controller_type = InputManager::kController2;
      }
    }
  } else if (type == kLaserDefHash) {
    Controller* controller = controllers_.Emplace(entity);
    controller->is_laser = true;
    controller->controller_type = InputManager::kController;

    auto* data = ConvertDef<LaserDef>(def);
    controller->stabilization_frames = data->stabilization_frames();
    controller->stabilization_counter = controller->stabilization_frames;
  } else {
    LOG(DFATAL)
        << "Invalid def passed to Create. Expecting ControllerDef or LaserDef!";
  }
}

void ControllerSystem::Destroy(Entity entity) { controllers_.Destroy(entity); }

void ControllerSystem::HandleControllerTransforms() {
  auto* input = registry_->Get<InputManager>();
  auto* transform_system = registry_->Get<TransformSystem>();
  auto* fade_system = registry_->Get<FadeSystem>();

  controllers_.ForEach([this, input, transform_system,
                        fade_system](Controller& controller) {
    // Retrieve the current sqt from the transform system to maintain scale
    // and default settings.
    const Sqt* controller_sqt =
        transform_system->GetSqt(controller.GetEntity());
    if (!controller_sqt) {
      LOG(DFATAL) << "Missing controller transform. Was it destroyed without "
                     "the controller component being destroyed?";
      return;
    }

    if (controller.is_laser) {
      // Assign the laser to the controller currently used by the cursor.
      const auto* input_processor = registry_->Get<InputProcessor>();
      const auto device = input_processor->GetPrimaryDevice();
      if (device == InputManager::kController ||
          device == InputManager::kController2) {
        controller.controller_type = device;
      }
    }

    Sqt device_sqt = *controller_sqt;
    if (!GetSqtForDevice(registry_, controller.controller_type, &device_sqt)) {
      controller.is_visible = false;
      // Reset the stabilization period since the device will have to reconnect,
      // which may result in a number of frames with bad transforms.
      controller.stabilization_counter = controller.stabilization_frames;
      if (fade_system) {
        fade_system->FadeToEnabledState(controller.GetEntity(), false,
                                        kControllerFadeTime);
      } else {
        transform_system->Disable(controller.GetEntity());
      }

      SendEventDefs(registry_, controller.GetEntity(),
                    controller.disable_events);
      return;
    }

    if (controller.stabilization_counter >= 0) {
      controller.stabilization_counter--;
    }
    // A controller is considered "stable" if it has not missed a device
    // transform for a set number of frames.
    const bool is_stable = controller.stabilization_counter < 0;

    // We check for fading before we offset the laser so that the controller and
    // laser fade at the same time
    if (fade_system) {
      mathfu::vec3 controller_forward = device_sqt.rotation * -mathfu::kAxisZ3f;
      controller_forward.Normalize();
      const float dot_y =
          mathfu::vec3::DotProduct(controller_forward, mathfu::kAxisY3f);
      const bool is_within_angle_threshold = dot_y < kControllerFadeCos;

      const bool is_enabled =
          transform_system->IsLocallyEnabled(controller.GetEntity());
      if (controller.is_visible && !is_within_angle_threshold) {
        controller.is_visible = false;

        SendEventDefs(registry_, controller.GetEntity(),
                      controller.disable_events);
        fade_system->FadeToEnabledState(controller.GetEntity(), false,
                                        kControllerFadeTime);
      } else if ((!controller.is_visible || !is_enabled) &&
                 is_within_angle_threshold && is_stable) {
        controller.is_visible = true;

        SendEventDefs(registry_, controller.GetEntity(),
                      controller.enable_events);
        fade_system->FadeToEnabledState(controller.GetEntity(), true,
                                        kControllerFadeTime);
      }
    } else if (is_stable) {
      transform_system->Enable(controller.GetEntity());
      controller.is_visible = true;
    }

    if (controller.is_laser) {
      device_sqt = AdjustSqtForReticle(registry_, device_sqt);
    }

    transform_system->SetSqt(controller.GetEntity(), device_sqt);

    if (controller.is_laser && controller.is_visible) {
      UpdateBendUniforms(controller);
    }
  });
}

void ControllerSystem::ShowControls(bool hard_enable) {
  show_controls_ = true;
  if (hard_enable) {
    auto* transform_system = registry_->Get<TransformSystem>();
    controllers_.ForEach([this, transform_system](Controller& controller) {
      transform_system->Enable(controller.GetEntity());
    });
  }
}

void ControllerSystem::HideControls() {
  show_controls_ = false;
  auto* transform_system = registry_->Get<TransformSystem>();
  controllers_.ForEach([this, transform_system](Controller& controller) {
    transform_system->Disable(controller.GetEntity());
  });
}

void ControllerSystem::UpdateBendUniforms(Controller& laser) {
  LULLABY_CPU_TRACE_CALL();
  Entity entity = laser.GetEntity();
  auto* input_processor = registry_->Get<InputProcessor>();
  const InputFocus* focus =
      input_processor->GetInputFocus(laser.controller_type);

  if (!focus) {
    DCHECK(false) << "No input focus for device " << laser.controller_type;
    return;
  }

  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::mat4* world_from_entity_mat =
      transform_system->GetWorldFromEntityMatrix(entity);

  if (!world_from_entity_mat) {
    return;
  }

  const mathfu::vec3 laser_ray =
      (focus->no_hit_cursor_position - focus->origin);

  const mathfu::vec3 laser_origin = focus->origin;
  const mathfu::vec3 laser_forward = laser_ray.Normalized();
  const mathfu::vec3 laser_endpoint = focus->cursor_position;
  const mathfu::vec3 origin_to_endpoint = laser_endpoint - laser_origin;
  const float distance = origin_to_endpoint.Length();

  const float laser_cross_length =
      mathfu::vec3::CrossProduct(laser_forward, origin_to_endpoint / distance)
          .Length();
  const float laser_bend_angle =
      std::asin(mathfu::Clamp(laser_cross_length, -1.f, 1.f));
  const float laser_bend_max_angle = kLaserBendLimit;
  const float laser_bend_initial = mathfu::Clamp(
      std::abs(laser_bend_angle) / laser_bend_max_angle, 0.f, 1.f);

  const float laser_bend = std::pow(laser_bend_initial, kLaserBendThrow);

  const float corner_offset_fraction = mathfu::Lerp(
      kLaserCornerOffsetRange.x, kLaserCornerOffsetRange.y, laser_bend);
  const float near_fraction = mathfu::Lerp(
      kLaserNearFractionRange.x, kLaserNearFractionRange.y, laser_bend);
  const float far_fraction = mathfu::Lerp(kLaserFarFractionRange.x,
                                          kLaserFarFractionRange.y, laser_bend);

  const mathfu::vec3 corner_point =
      laser_origin + (laser_forward * corner_offset_fraction * distance);

  const mathfu::vec3_packed curve_points[4] = {
      mathfu::vec3_packed(laser_origin),
      mathfu::vec3_packed(laser_origin +
                          (corner_point - laser_origin) * near_fraction),
      mathfu::vec3_packed(laser_endpoint +
                          (corner_point - laser_endpoint) * far_fraction),
      mathfu::vec3_packed(laser_endpoint)};

  auto* render_system = registry_->Get<RenderSystem>();
  const mathfu::mat4 entity_from_world = world_from_entity_mat->Inverse();
  if (laser.last_bend_fraction != laser_bend) {
    const auto alpha =
        mathfu::Lerp(kLaserAlphaRange.x, kLaserAlphaRange.y, laser_bend);

    mathfu::vec4 color;
    render_system->GetColor(entity, &color);
    color[3] = alpha;
    render_system->SetColor(entity, color);
    laser.last_bend_fraction = laser_bend;
  }

  render_system->SetUniform(entity, kEntityFromWorld, &entity_from_world[0],
                            16);
  render_system->SetUniform(entity, kControlPoints, &curve_points[0].data[0], 3,
                            4);
}

}  // namespace lull
