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

#include "lullaby/util/device_util.h"

namespace lull {

// Shader Uniform names
const char kControllerButtonUVRectsUniform[] = "button_uv_rects";
const char kControllerButtonColorsUniform[] = "button_colors";
const char kControllerBatteryUVRectUniform[] = "battery_uv_rect";
const char kControllerBatteryUVOffsetUniform[] = "battery_offset";
const char kControllerTouchpadRectUniform[] = "touchpad_rect";
const char kControllerTouchColorUniform[] = "touch_color";
const char kControllerTouchPositionUniform[] = "touch_position";
const char kControllerTouchRadiusSquaredUniform[] = "touch_radius_squared";

DeviceProfile GetDaydreamControllerProfile() {
  DeviceProfile result;

  result.assets.mesh = "meshes/daydream_controller.fplmesh";
  result.assets.unlit_texture = "textures/daydream_controller.webp";

  const mathfu::vec3 ray_direction =
      mathfu::quat::FromAngleAxis(kDaydreamControllerErgoAngleRadians,
                                  mathfu::kAxisX3f) *
      -mathfu::kAxisZ3f;

  result.selection_ray = Ray(mathfu::vec3(0.0f, -0.01f, -0.05f), ray_direction);
  result.selection_sphere = Sphere();
  result.type = DeviceProfile::kThreeButtonController;
  result.handedness = DeviceProfile::kEitherHand;
  result.position_dof = DeviceProfile::kFakeDof;
  result.rotation_dof = DeviceProfile::kRealDof;

  result.touchpads.resize(1);
  result.touchpads[0].tooltip_ray =
      Ray(mathfu::vec3(0.0175f, 0.002f, -0.035f), mathfu::kAxisX3f * 0.0158f);
  result.touchpads[0].uv_coords = mathfu::vec4(0.0f, 0.0f, 0.3f, 0.3f);
  result.touchpads[0].touch_radius = 0.05f;
  result.touchpads[0].has_gestures = true;

  result.buttons.resize(3);
  result.buttons[0].type = DeviceProfile::Button::kTouchpad;
  result.buttons[0].purpose = DeviceProfile::Button::kButton0;
  result.buttons[0].tooltip_ray =
      Ray(mathfu::vec3(0.0175f, 0.002f, -0.035f), mathfu::kAxisX3f * 0.0158f);
  result.buttons[0].bone = 0;
  result.buttons[0].uv_coords = mathfu::vec4(0.0f, 0.0f, 0.3f, 0.3f);
  result.buttons[0].pressed_position = mathfu::kZeros3f;
  result.buttons[0].pressed_rotation = mathfu::quat::identity;

  result.buttons[1].type = DeviceProfile::Button::kStandardButton;
  result.buttons[1].purpose = DeviceProfile::Button::kButton1;
  result.buttons[1].tooltip_ray =
      Ray(mathfu::vec3(0.007f, 0.002f, -0.0083f), mathfu::kAxisX3f * 0.0264f);
  result.buttons[1].bone = 0;
  result.buttons[1].uv_coords = mathfu::vec4(0.0f, 0.3f, 0.2f, 0.5f);
  result.buttons[1].pressed_position = mathfu::kZeros3f;
  result.buttons[1].pressed_rotation = mathfu::quat::identity;

  result.buttons[2].type = DeviceProfile::Button::kStandardButton;
  result.buttons[2].purpose = DeviceProfile::Button::kSystem;
  result.buttons[2].tooltip_ray =
      Ray(mathfu::vec3(0.007f, 0.002f, 0.0083f), mathfu::kAxisX3f * 0.0264f);
  result.buttons[2].bone = 0;
  result.buttons[2].uv_coords = mathfu::vec4(0.0f, 0.5f, 0.2f, 0.7f);
  result.buttons[2].pressed_position = mathfu::kZeros3f;
  result.buttons[2].pressed_rotation = mathfu::quat::identity;

  result.battery.emplace();
  result.battery->uv_coords = mathfu::vec4(0.35f, 0.72f, 0.55f, 0.76f);
  result.battery->charged_offset = mathfu::vec2(0.0f, 0.18f);
  result.battery->critical_offset = mathfu::vec2(0.0f, 0.22f);
  result.battery->critical_percentage = 0.2f;
  result.battery->segments = 5;

  return result;
}


DeviceProfile GetCardboardHeadsetProfile() {
  DeviceProfile result;

  result.selection_ray = Ray(mathfu::kZeros3f, -mathfu::kAxisZ3f);
  result.position_dof = DeviceProfile::kFakeDof;
  result.rotation_dof = DeviceProfile::kRealDof;

  result.buttons.resize(1);
  result.buttons[0].type = DeviceProfile::Button::kStandardButton;
  result.buttons[0].purpose = DeviceProfile::Button::kButton0;

  result.eyes.resize(2);

  return result;
}

DeviceProfile GetDaydreamHeadsetProfile() {
  DeviceProfile result;

  result.position_dof = DeviceProfile::kFakeDof;
  result.rotation_dof = DeviceProfile::kRealDof;

  result.eyes.resize(2);

  return result;
}


DeviceProfile GetARHeadsetProfile() {
  DeviceProfile result;

  result.type = DeviceProfile::kTouchScreen;
  result.position_dof = DeviceProfile::kRealDof;
  result.rotation_dof = DeviceProfile::kRealDof;

  result.eyes.resize(1);

  result.buttons.resize(1);
  result.buttons[0].type = DeviceProfile::Button::kStandardButton;
  result.buttons[0].purpose = DeviceProfile::Button::kButton0;

  result.touchpads.resize(1);

  return result;
}

DeviceProfile GetDeviceProfileForHeadsetModel(const HashValue hash) {

  return GetCardboardHeadsetProfile();
}

}  // namespace lull
