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

#include "lullaby/contrib/controller/controller_system.h"

#include "lullaby/generated/controller_def_generated.h"
#include "lullaby/contrib/fade/fade_system.h"
#include "lullaby/contrib/fpl_mesh/fpl_mesh_system.h"
#include "lullaby/events/controller_events.h"
#include "lullaby/events/input_events.h"
#include "lullaby/events/lifetime_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/flatbuffers/common_fb_conversions.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/modules/reticle/reticle_util.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/device_util.h"
#include "lullaby/util/interpolation.h"
#include "lullaby/util/time.h"
#include "lullaby/util/trace.h"

namespace lull {

namespace {
const HashValue kControllerDefHash = ConstHash("ControllerDef");
const HashValue kLaserDefHash = ConstHash("LaserDef");
const Clock::duration kControllerFadeTime = std::chrono::milliseconds(250);

// Laser Constants
const float kLaserBendLimit = 60.0f * kDegreesToRadians;
const float kLaserBendThrow = 0.5f;
const mathfu::vec2 kLaserCornerOffsetRange = mathfu::vec2(0.1f, 0.45f);
const mathfu::vec2 kLaserNearFractionRange = mathfu::vec2(0.66f, 0.75f);
const mathfu::vec2 kLaserFarFractionRange = mathfu::vec2(0.66f, 0.5f);
const mathfu::vec2 kLaserAlphaRange = mathfu::vec2(0.5f, 0.95f);
constexpr const char* kEntityFromWorld = "entity_from_world";
constexpr const char* kControlPoints = "control_points";

// Controller Constants
const Clock::duration kButtonAnimationDuration = std::chrono::milliseconds(100);
const mathfu::vec4 kButtonPressColor = mathfu::vec4(0.161f, 0.475f, 1.0f, 1.0f);
const mathfu::vec4 kTouchColor = mathfu::vec4(0.161f, 0.475f, 1.0f, 1.0f);
}  // namespace

ControllerSystem::Controller::Controller(Entity entity) : Component(entity) {}

ControllerSystem::ControllerSystem(Registry* registry)
    : System(registry), controllers_(4) {
  RegisterDef<ControllerDefT>(this);
  RegisterDef<LaserDefT>(this);
  RegisterDependency<InputManager>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<Dispatcher>(this);
  auto* dispatcher = registry->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->Connect(this, [this](const ShowControllerModelEvent&) {
      ShowControls(/* hard_enable= */ false);
    });
    dispatcher->Connect(
        this, [this](const HideControllerModelEvent&) { HideControls(); });
    dispatcher->Connect(this, [this](const ShowControllerLaserEvent& event) {
      ShowLaser(event.controller_type);
    });
    dispatcher->Connect(this, [this](const HideControllerLaserEvent& event) {
      HideLaser(event.controller_type);
    });
    dispatcher->Connect(this, [this](const SetLaserFadePointsEvent& event) {
      SetLaserFadePoints(event.controller_type, event.fade_points);
    });
    dispatcher->Connect(this, [this](const ResetLaserFadePointsEvent& event) {
      ResetLaserFadePoints(event.controller_type);
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
  auto* input = registry_->Get<InputManager>();
  for (auto& controller : controllers_) {
    if (controller.should_hide) {
      continue;
    }
    const bool connected = input->IsConnected(controller.controller_type);
    // If a controller is disconnected & reconnected with a new DeviceProfile
    // on the same frame, the connection status won't change. We check the
    // DeviceProfile's name against our last frame's name to handle this case.
    const DeviceProfile* profile =
        input->GetDeviceProfile(controller.controller_type);
    const HashValue profile_name = profile ? profile->name : 0;
    const bool profile_changed = profile_name != controller.device_profile_name;
    if (connected != controller.connected || profile_changed) {
      controller.device_profile_name = profile_name;
      if (connected) {
        OnControllerConnected(&controller);
      } else {
        OnControllerDisconnected(&controller);
      }
    }
    HandleControllerTransforms(delta_time, &controller);
    if (controller.is_visible) {
      if (controller.is_laser) {
        UpdateBendUniforms(&controller);
      } else {
        UpdateControllerUniforms(delta_time, &controller);
      }
    }
  }
}

void ControllerSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type == kControllerDefHash) {
    Controller* controller = controllers_.Emplace(entity);

    auto* data = ConvertDef<ControllerDef>(def);
    controller->enable_events = data->enable_events();
    controller->disable_events = data->disable_events();

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
    auto* data = ConvertDef<LaserDef>(def);
    controller->controller_type =
        data->controller_type() == ControllerType_Controller2
            ? InputManager::kController2
            : InputManager::kController;
    auto* render_system = registry_->Get<RenderSystem>();
    render_system->GetUniform(controller->GetEntity(), "fade_points", 4,
                              &controller->default_fade_points_[0]);
  } else {
    LOG(DFATAL)
        << "Invalid def passed to Create. Expecting ControllerDef or LaserDef!";
  }
}

void ControllerSystem::Destroy(Entity entity) { controllers_.Destroy(entity); }

void ControllerSystem::HandleControllerTransforms(
    const Clock::duration& delta_time, Controller* controller) {
  auto* transform_system = registry_->Get<TransformSystem>();

  // Retrieve the current sqt from the transform system to maintain scale
  // and default settings.
  const Sqt* controller_sqt = transform_system->GetSqt(controller->GetEntity());
  if (!controller_sqt) {
    LOG(DFATAL) << "Missing controller transform. Was it destroyed without "
                   "the controller component being destroyed?";
    return;
  }

  // If the controller is not enabled then it is definitely not visible. The
  // converse is not true. It could be enabled but in the process of fading out,
  // in which case it is still considered not visible.
  const bool is_enabled =
      transform_system->IsLocallyEnabled(controller->GetEntity());
  if (!is_enabled) {
    controller->is_visible = false;
  }

  Sqt device_sqt = *controller_sqt;
  if (!GetSqtForDevice(registry_, controller->controller_type, &device_sqt)) {
    // If we cannot get the SQT because the controller is not connected than
    // disable it immediately and wait for valid data.
    if (is_enabled) {
      SendEventDefs(registry_, controller->GetEntity(),
                    controller->disable_events);
      // The is_visible flag will be set to false next time time through.
      transform_system->Disable(controller->GetEntity());
    }
    return;
  }

  // If the controller is too close to the head then it should be disabled/faded
  // out so that it doesn't block the entire view.
  auto* input = registry_->Get<InputManager>();
  if (input->IsConnected(InputManager::kHmd)) {
    mathfu::vec3 head_pos = input->GetDofPosition(InputManager::kHmd);
    if (!controller->should_hide &&
        (head_pos - device_sqt.translation).LengthSquared() >
            controller_fade_distance_) {
      MaybeFadeInController(controller);
    } else {
      MaybeFadeOutController(controller);
    }
  }

  transform_system->SetSqt(controller->GetEntity(),
                           controller->is_laser
                               ? AdjustSqtForReticle(registry_, device_sqt)
                               : device_sqt);
}

void ControllerSystem::MaybeFadeInController(Controller* controller) {
  if (controller->is_visible) {
    return;
  }

  controller->is_visible = true;
  SendEventDefs(registry_, controller->GetEntity(), controller->enable_events);

  auto* fade_system = registry_->Get<FadeSystem>();
  if (fade_system) {
    fade_system->FadeToEnabledState(controller->GetEntity(), true,
                                    kControllerFadeTime);
  } else {
    registry_->Get<TransformSystem>()->Enable(controller->GetEntity());
  }
}

void ControllerSystem::MaybeFadeOutController(Controller* controller) {
  if (!controller->is_visible) {
    return;
  }

  controller->is_visible = false;
  SendEventDefs(registry_, controller->GetEntity(), controller->disable_events);

  auto* fade_system = registry_->Get<FadeSystem>();
  if (fade_system) {
    fade_system->FadeToEnabledState(controller->GetEntity(), false,
                                    kControllerFadeTime);
  } else {
    registry_->Get<TransformSystem>()->Disable(controller->GetEntity());
  }
}

void ControllerSystem::ShowControls(bool hard_enable) {
  auto* transform_system = registry_->Get<TransformSystem>();
  controllers_.ForEach([transform_system, hard_enable](Controller& controller) {
    controller.should_hide = false;
    if (hard_enable) {
      transform_system->Enable(controller.GetEntity());
    }
  });
}

void ControllerSystem::HideControls() {
  auto* transform_system = registry_->Get<TransformSystem>();
  controllers_.ForEach([transform_system](Controller& controller) {
    controller.should_hide = true;
    transform_system->Disable(controller.GetEntity());
  });
}

void ControllerSystem::ShowLaser(InputManager::DeviceType controller_type) {
  for (auto& controller : controllers_) {
    if (controller.controller_type == controller_type && controller.is_laser) {
      controller.should_hide = false;
    }
  }
}

void ControllerSystem::HideLaser(InputManager::DeviceType controller_type) {
  for (auto& controller : controllers_) {
    if (controller.controller_type == controller_type && controller.is_laser) {
      controller.should_hide = true;
      auto* transform_system = registry_->Get<TransformSystem>();
      transform_system->Disable(controller.GetEntity());
    }
  }
}

bool ControllerSystem::IsLaserHidden(InputManager::DeviceType controller_type) {
  for (const auto& controller : controllers_) {
    if (controller.controller_type == controller_type && controller.is_laser) {
      return controller.should_hide;
    }
  }
  // There is no laser model bound with the given controller, return true by
  // default.
  return true;
}

bool ControllerSystem::SetLaserFadePoints(
    InputManager::DeviceType controller_type, const mathfu::vec4& fade_points) {
  for (const auto& controller : controllers_) {
    if (controller.controller_type == controller_type && controller.is_laser) {
      auto* render_system = registry_->Get<RenderSystem>();
      render_system->SetUniform(controller.GetEntity(), "fade_points",
                                &fade_points[0], 4);
      return true;
    }
  }
  // Return false if there is no laser model bound with the given controller.
  return false;
}

bool ControllerSystem::ResetLaserFadePoints(
    InputManager::DeviceType controller_type) {
  for (const auto& controller : controllers_) {
    if (controller.controller_type == controller_type && controller.is_laser) {
      auto* render_system = registry_->Get<RenderSystem>();
      render_system->SetUniform(controller.GetEntity(), "fade_points",
                                &controller.default_fade_points_[0], 4);
      return true;
    }
  }
  // Return false if there is no laser model bound with the given controller.
  return false;
}

void ControllerSystem::OnControllerConnected(Controller* controller) {
  controller->connected = true;
  if (!controller->is_laser) {
    auto* input = registry_->Get<InputManager>();
    auto* render_system = registry_->Get<RenderSystem>();
    auto* dispatcher = registry_->Get<Dispatcher>();

    const Entity entity = controller->GetEntity();
    const InputManager::DeviceType device = controller->controller_type;
    const size_t num_buttons = input->GetNumButtons(device);
    controller->buttons.resize(num_buttons);

    const DeviceProfile* profile = input->GetDeviceProfile(device);
    if (profile) {
      auto* fpl_mesh_system = registry_->Get<FplMeshSystem>();
      if (fpl_mesh_system) {
        fpl_mesh_system->CreateMesh(
            controller->GetEntity(),
            Hash("Opaque"),  // RenderSystem::kDefaultPass,
            profile->assets.mesh);
      } else {
        render_system->SetMesh(controller->GetEntity(), profile->assets.mesh);
      }
      render_system->SetTexture(controller->GetEntity(), 0,
                                profile->assets.unlit_texture);

      // TODO remove this when all no apps are still using the old
      // controller model.
      input->SetDeviceInfo(InputManager::kController, kSelectionRayHash,
                           profile->selection_ray);
    }

    // Button init.
    const float button_zeros[kControllerMaxColoredButtons * 4] = {0.0f};
    render_system->SetUniform(entity, kControllerButtonUVRectsUniform,
                              &button_zeros[0], 4,
                              kControllerMaxColoredButtons);
    render_system->SetUniform(entity, kControllerButtonColorsUniform,
                              &button_zeros[0], 4,
                              kControllerMaxColoredButtons);

    // Touchpad init.
    const float radius_squared = 0.0f;
    render_system->SetUniform(entity, kControllerTouchRadiusSquaredUniform,
                              &radius_squared, 1, 1);
    render_system->SetUniform(entity, kControllerTouchColorUniform,
                              &kTouchColor[0], 4, 1);

    if (profile && !profile->touchpads.empty()) {
      const mathfu::vec4 uv_rect = profile->touchpads[0].uv_coords;
      render_system->SetUniform(entity, kControllerTouchpadRectUniform,
                                &uv_rect[0], 4, 1);
    }

    // Battery init.
    controller->last_battery_level = InputManager::kInvalidBatteryCharge;
    render_system->SetUniform(entity, kControllerBatteryUVRectUniform,
                              &mathfu::kZeros4f[0], 4, 1);

    dispatcher->Send(DeviceConnectedEvent(controller->controller_type,
                                          controller->GetEntity()));
  }
}

void ControllerSystem::OnControllerDisconnected(Controller* controller) {
  controller->connected = false;
  if (!controller->is_laser) {
    controller->buttons.resize(0);
  }
}

void ControllerSystem::UpdateBendUniforms(Controller* laser) {
  LULLABY_CPU_TRACE_CALL();
  Entity entity = laser->GetEntity();
  auto* input_processor = registry_->Get<InputProcessor>();
  const InputFocus* focus =
      input_processor->GetInputFocus(laser->controller_type);

  if (!focus) {
    DCHECK(false) << "No input focus for device " << laser->controller_type;
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
  if (laser->last_bend_fraction != laser_bend) {
    const auto alpha =
        mathfu::Lerp(kLaserAlphaRange.x, kLaserAlphaRange.y, laser_bend);

    mathfu::vec4 color;
    render_system->GetColor(entity, &color);
    color[3] = alpha;
    render_system->SetColor(entity, color);
    laser->last_bend_fraction = laser_bend;
  }

  render_system->SetUniform(entity, kEntityFromWorld, &entity_from_world[0],
                            16);
  render_system->SetUniform(entity, kControlPoints, &curve_points[0].data_[0],
                            3, 4);
}

void ControllerSystem::UpdateControllerUniforms(
    const Clock::duration& delta_time, Controller* controller) {
  UpdateControllerButtonUniforms(delta_time, controller);
  UpdateControllerTouchpadUniforms(delta_time, controller);
  UpdateControllerBatteryUniforms(delta_time, controller);
}

void ControllerSystem::UpdateControllerButtonUniforms(
    const Clock::duration& delta_time, Controller* controller) {
  auto* input = registry_->Get<InputManager>();
  const InputManager::DeviceType device = controller->controller_type;
  const DeviceProfile* profile = input->GetDeviceProfile(device);
  if (!profile) {
    return;
  }

  const size_t num_buttons = input->GetNumButtons(device);
  const std::vector<DeviceProfile::Button>& buttons = profile->buttons;

  bool update_uniform = false;
  for (uint8_t i = 0; i < num_buttons; ++i) {
    const InputManager::ButtonState state = input->GetButtonState(device, i);
    const bool pressed = CheckBit(state, InputManager::kPressed);
    Button& button = controller->buttons[i];
    if (pressed != button.was_pressed) {
      button.was_pressed = pressed;
      button.anim_start_alpha = button.current_alpha;
      button.anim_time_left = kButtonAnimationDuration;
      if (pressed) {
        button.target_alpha = 1.0f;
      } else {
        button.target_alpha = 0.0f;
      }
    }

    if (button.anim_time_left > Clock::duration::zero()) {
      button.anim_time_left -= delta_time;
      update_uniform = true;
      if (button.anim_time_left < Clock::duration::zero()) {
        button.anim_time_left = Clock::duration::zero();
        button.current_alpha = button.target_alpha;
      } else {
        const float percent =
            1.0f - SecondsFromDuration(button.anim_time_left) * 1.0f /
                       SecondsFromDuration(kButtonAnimationDuration);
        button.current_alpha = QuadraticEaseOut(button.anim_start_alpha,
                                                button.target_alpha, percent);
      }
    }
  }

  if (update_uniform) {
    auto* render_system = registry_->Get<RenderSystem>();
    const Entity entity = controller->GetEntity();

    float button_uv_rects[kControllerMaxColoredButtons * 4] = {0.0f};
    float button_colors[kControllerMaxColoredButtons * 4] = {0.0f};
    size_t num_colored = 0;
    controller->touchpad_button_pressed = false;
    for (uint8_t i = 0; i < num_buttons && i < buttons.size() &&
                        num_colored < kControllerMaxColoredButtons;
         ++i) {
      // TODO update bone transforms
      if (controller->buttons[i].current_alpha > 0.0f) {
        if (buttons[i].type == DeviceProfile::Button::kTouchpad) {
          // Grow the touch indicator instead of using a button press.
          controller->touchpad_button_pressed = true;
          controller->touch_ripple_factor =
              controller->buttons[i].current_alpha;
        } else {
          button_colors[num_colored * 4 + 0] = kButtonPressColor[0];
          button_colors[num_colored * 4 + 1] = kButtonPressColor[1];
          button_colors[num_colored * 4 + 2] = kButtonPressColor[2];
          button_colors[num_colored * 4 + 3] =
              kButtonPressColor[3] * controller->buttons[i].current_alpha;

          button_uv_rects[num_colored * 4 + 0] = buttons[i].uv_coords[0];
          button_uv_rects[num_colored * 4 + 1] = buttons[i].uv_coords[1];
          button_uv_rects[num_colored * 4 + 2] = buttons[i].uv_coords[2];
          button_uv_rects[num_colored * 4 + 3] = buttons[i].uv_coords[3];
          ++num_colored;
        }
      }
    }
    render_system->SetUniform(entity, kControllerButtonUVRectsUniform,
                              &button_uv_rects[0], 4,
                              kControllerMaxColoredButtons);
    render_system->SetUniform(entity, kControllerButtonColorsUniform,
                              &button_colors[0], 4,
                              kControllerMaxColoredButtons);
  }
}

void ControllerSystem::UpdateControllerTouchpadUniforms(
    const Clock::duration& delta_time, Controller* controller) {
  auto* input = registry_->Get<InputManager>();
  const InputManager::DeviceType device = controller->controller_type;
  const DeviceProfile* profile = input->GetDeviceProfile(device);
  if (!profile || !input->HasTouchpad(device)) {
    return;
  }

  auto* render_system = registry_->Get<RenderSystem>();
  const Entity entity = controller->GetEntity();
  if (input->IsValidTouch(device) || controller->touchpad_button_pressed) {
    const DeviceProfile::Touchpad& touchpad = profile->touchpads[0];
    controller->was_touched = true;
    mathfu::vec2 pos = mathfu::kOnes2f * 0.5f;
    if (input->IsValidTouch(device)) {
      pos = input->GetTouchLocation(device);
    }
    const mathfu::vec4 uv_rect = touchpad.uv_coords;
    pos.x = uv_rect[0] + pos.x * (uv_rect[2] - uv_rect[0]);
    pos.y = uv_rect[1] + pos.y * (uv_rect[3] - uv_rect[1]);
    const float largest_dimension =
        std::max((uv_rect[2] - uv_rect[0]), (uv_rect[3] - uv_rect[1]));
    const float radius =
        std::max(touchpad.touch_radius,
                 controller->touch_ripple_factor * largest_dimension);
    const float radius_squared = radius * radius;

    render_system->SetUniform(entity, kControllerTouchPositionUniform, &pos[0],
                              2, 1);
    render_system->SetUniform(entity, kControllerTouchRadiusSquaredUniform,
                              &radius_squared, 1, 1);
  } else if (controller->was_touched) {
    controller->was_touched = false;
    const float radius_squared = 0.0f;
    render_system->SetUniform(entity, kControllerTouchRadiusSquaredUniform,
                              &radius_squared, 1, 1);
  }
}

void ControllerSystem::UpdateControllerBatteryUniforms(
    const Clock::duration& delta_time, Controller* controller) {
  auto* input = registry_->Get<InputManager>();
  const InputManager::DeviceType device = controller->controller_type;
  const DeviceProfile* profile = input->GetDeviceProfile(device);
  if (!profile || !input->HasBattery(device) || !profile->battery) {
    return;
  }

  auto* render_system = registry_->Get<RenderSystem>();
  const Entity entity = controller->GetEntity();
  const DeviceProfile::Battery* battery = profile->battery.get();
  const uint8_t battery_level_percent = input->GetBatteryCharge(device);

  if (controller->last_battery_level != battery_level_percent) {
    controller->last_battery_level = battery_level_percent;
    const float segments = static_cast<float>(battery->segments);
    const float battery_level =
        std::round(battery_level_percent * segments / 100.0f) / segments;
    mathfu::vec4 uv_rect = battery->uv_coords;
    uv_rect[2] = mathfu::Lerp(uv_rect[0], uv_rect[2], battery_level);
    const mathfu::vec2 offset = battery_level > battery->critical_percentage
                                    ? battery->charged_offset
                                    : battery->critical_offset;
    render_system->SetUniform(entity, kControllerBatteryUVRectUniform,
                              &uv_rect[0], 4, 1);
    render_system->SetUniform(entity, kControllerBatteryUVOffsetUniform,
                              &offset[0], 2, 1);
  }
}

}  // namespace lull
