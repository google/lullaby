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

#include "lullaby/contrib/device_tooltips/device_tooltips.h"

#include "lullaby/events/input_events.h"
#include "lullaby/events/layout_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/contrib/layout/layout_box_system.h"
#include "lullaby/systems/text/text_system.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

namespace {
const char* const kDefaultLineBlueprint = "device-tooltip-line";
const char* const kDefaultTextBlueprint = "device-tooltip-text";
constexpr HashValue kShowEventHash = ConstHash("Show");
constexpr HashValue kHideEventHash = ConstHash("Hide");
constexpr HashValue kHideNowThenShowEventHash = ConstHash("HideNowThenShow");
constexpr HashValue kHideNowEventHash = ConstHash("HideNow");
const float kLineWidth = 0.002f;
const float kLineMargin = 0.007f;

// Split the circle into 8 sections, each centered around a direction (left, top
// left, etc).  Then return a horizontal and vertical alignment that will put
// the text anchor at the end of the line.
void GetAlignmentFromDirection(const float angle,
                               HorizontalAlignment* out_h_align,
                               VerticalAlignment* out_v_align) {
  // angle = 0 is (1,0).  8 or -8 for (0,1).
  // Note that +z is 'down', so positive octants are in the lower half.
  const float octant = angle / kPi * 8.0f;

  if (octant <= 7.0f && octant >= 1.0f) {
    // octant is in bottom area
    *out_v_align = VerticalAlignment_Top;
  } else if (octant <= -1.0f && octant >= -7.0f) {
    // octant is in top area
    *out_v_align = VerticalAlignment_Bottom;
  } else {
    *out_v_align = VerticalAlignment_Center;
  }

  if (octant <= 3.0f && octant >= -3.0f) {
    // octant is in right area.
    *out_h_align = HorizontalAlignment_Left;
  } else if (octant < 5.0f && octant > -5.0f) {
    // octant is center area.
    *out_h_align = HorizontalAlignment_Center;
  } else {
    *out_h_align = HorizontalAlignment_Right;
  }
}
}  // namespace

DeviceTooltips::DeviceTooltips(Registry* registry) : registry_(registry) {
  tooltip_line_blueprint_ = kDefaultLineBlueprint;
  tooltip_text_blueprint_ = kDefaultTextBlueprint;
  auto* dispatcher = registry_->Get<Dispatcher>();
  show_tooltip_connection_ =
      dispatcher->Connect([this](const ShowTooltipEvent& event) {
        ShowTooltip(event.device, event.button, event.hint_text);
      });
  hide_tooltip_connection_ =
      dispatcher->Connect([this](const HideTooltipEvent& event) {
        HideTooltip(event.device, event.button);
      });
  device_connnected_connection_ =
      dispatcher->Connect([this](const DeviceConnectedEvent& event) {
        auto* input_manager = registry_->Get<InputManager>();
        // This will only display tooltips on the last entity to send the event.
        devices_[event.device] = event.display_entity;

        const DeviceProfile* profile =
            input_manager->GetDeviceProfile(event.device);
        // Update tooltips.
        for (const auto& iter : tooltips_) {
          const DeviceButtonPair pair = iter.first;
          const Tooltip& tooltip = iter.second;
          UpdateTooltip(pair, profile);
          // Show a tooltip if it should be showing but was disabled
          ConditionallyShowTooltip(pair, tooltip, profile, false);
        }
      });
}

void DeviceTooltips::Setup(const std::string& tooltip_line_blueprint,
                           const std::string& tooltip_text_blueprint) {
  tooltip_line_blueprint_ = tooltip_line_blueprint;
  tooltip_text_blueprint_ = tooltip_text_blueprint;
}

void DeviceTooltips::ShowTooltip(InputManager::DeviceType device,
                                 InputManager::ButtonId button,
                                 const std::string& hint_text) {
  auto* input_manager = registry_->Get<InputManager>();
  const DeviceProfile* profile = input_manager->GetDeviceProfile(device);

  auto pair = std::make_pair(device, button);
  auto search = tooltips_.find(pair);
  bool is_new = false;
  if (search == tooltips_.end()) {
    CreateTooltip(pair);
    UpdateTooltip(pair, profile);
    is_new = true;
  }

  Tooltip& tooltip = tooltips_[pair];
  tooltip.should_show = true;

  auto* text_system = registry_->Get<TextSystem>();
  text_system->SetText(tooltip.text, hint_text);

  if (!profile || !input_manager->IsConnected(device)) {
    // Device isn't setup yet, so just store that the tooltip should be
    // showing.
    return;
  }
  if (button >= profile->buttons.size()) {
    // Device profile doesn't have |button|.
    LOG(WARNING) << "Connected device doesn't support tooltips for " << button;
    return;
  }

  ConditionallyShowTooltip(pair, tooltip, profile, is_new);
}

void DeviceTooltips::HideTooltip(InputManager::DeviceType device,
                                 InputManager::ButtonId button) {
  auto pair = std::make_pair(device, button);
  auto search = tooltips_.find(pair);
  if (search != tooltips_.end()) {
    search->second.should_show = false;
    auto* dispatcher_system = registry_->Get<DispatcherSystem>();
    dispatcher_system->Send(search->second.line, EventWrapper(kHideEventHash));
  }
}

void DeviceTooltips::CreateTooltip(const DeviceButtonPair& pair) {
  auto* transform_system = registry_->Get<TransformSystem>();
  const Entity device_entity = devices_[pair.first];

  auto& tooltip = tooltips_[pair];
  if (device_entity != kNullEntity) {
    tooltip.line =
        transform_system->CreateChild(device_entity, tooltip_line_blueprint_);
  } else {
    auto* entity_factory = registry_->Get<EntityFactory>();
    tooltip.line = entity_factory->Create(tooltip_line_blueprint_);
  }
  tooltip.text =
      transform_system->CreateChild(tooltip.line, tooltip_text_blueprint_);
}

void DeviceTooltips::UpdateTooltip(const DeviceButtonPair& pair,
                                   const DeviceProfile* profile) {
  if (!profile || pair.second >= profile->buttons.size()) {
    // If no profile, wait for the DeviceConnectedEvent. If button >=
    // profile->buttons.size(), the device doesn't support this button.
    return;
  }

  const Tooltip& tooltip = tooltips_[pair];

  Entity text_entity = tooltip.text;
  Entity line_entity = tooltip.line;

  auto* text_system = registry_->Get<TextSystem>();
  auto* layout_box_system = registry_->Get<LayoutBoxSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();

  const Sqt* text_sqt = transform_system->GetSqt(text_entity);
  if (text_sqt == nullptr) {
    LOG(WARNING) << "Tooltip text entity is missing a transform.";
    return;
  }

  const Ray& ray = profile->buttons[pair.second].tooltip_ray;
  const float angle = atan2f(ray.direction.z, ray.direction.x);

  if (std::abs(ray.direction.y) > 0.00001) {
    LOG(WARNING)
        << "DeviceProfile tooltip_ray direction should be in the x,z plane.";
  }

  const mathfu::vec2 line_size(ray.direction.Length(), kLineWidth);
  const mathfu::vec3 line_pos = ray.origin + 0.5f * ray.direction;
  const mathfu::quat line_rot =
      mathfu::quat::FromEulerAngles(mathfu::vec3(-kPi / 2.0f, -angle, 0.0f));
  const mathfu::vec3 text_pos =
      (line_size.x * 0.5f + kLineMargin) * mathfu::kAxisX3f;
  const mathfu::quat text_rot =
      mathfu::quat::FromAngleAxis(angle, mathfu::kAxisZ3f);

  if (devices_[pair.first] != kNullEntity &&
      devices_[pair.first] != transform_system->GetParent(line_entity)) {
    transform_system->AddChild(devices_[pair.first], line_entity);
  }
  transform_system->SetSqt(line_entity,
                           Sqt(line_pos, line_rot, mathfu::kOnes3f));
  transform_system->SetSqt(text_entity,
                           Sqt(text_pos, text_rot, text_sqt->scale));

  HorizontalAlignment h_align = HorizontalAlignment_Left;
  VerticalAlignment v_align = VerticalAlignment_Center;
  GetAlignmentFromDirection(angle, &h_align, &v_align);
  text_system->SetHorizontalAlignment(text_entity, h_align);
  text_system->SetVerticalAlignment(text_entity, v_align);

  layout_box_system->SetDesiredSize(line_entity, kNullEntity, line_size.x,
                                    line_size.y, Optional<float>());
}

void DeviceTooltips::ConditionallyShowTooltip(const DeviceButtonPair& pair,
                                              const Tooltip& tooltip,
                                              const DeviceProfile* profile,
                                              bool is_new) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  auto* input_manager = registry_->Get<InputManager>();
  if (!profile || pair.second >= profile->buttons.size() ||
      devices_[pair.first] == kNullEntity ||
      !input_manager->IsConnected(pair.first)) {
    dispatcher_system->Send(tooltip.line, EventWrapper(kHideNowEventHash));
  } else if (tooltip.should_show) {
    if (is_new) {
      dispatcher_system->Send(tooltip.line,
                              EventWrapper(kHideNowThenShowEventHash));
    } else {
      dispatcher_system->Send(tooltip.line, EventWrapper(kShowEventHash));
    }
  }
}

}  // namespace lull
