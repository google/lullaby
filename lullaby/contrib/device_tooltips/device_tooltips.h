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

#ifndef LULLABY_CONTRIB_DEVICE_TOOLTIPS_DEVICE_TOOLTIPS_H_
#define LULLABY_CONTRIB_DEVICE_TOOLTIPS_DEVICE_TOOLTIPS_H_

#include <string>

#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/enum_hash.h"
#include "lullaby/util/registry.h"

namespace lull {

/// DeviceTooltips is a helper class which provides methods for showing/hiding
/// tooltips on the device, typically associated with a button or other input
/// mechanism.
class DeviceTooltips {
 public:
  explicit DeviceTooltips(Registry* registry);

  /// Shows the tooltip with the given device & id.
  void ShowTooltip(InputManager::DeviceType device,
                   InputManager::ButtonId button, const std::string& hint_text);

  /// Hides the tooltip with the given device & id.
  void HideTooltip(InputManager::DeviceType device,
                   InputManager::ButtonId button);

  /// Set the blueprints to be used for creating tooltips.
  /// |tooltip_line_blueprint| should handle the a DesiredSizeChangedEvent,
  /// typically using a ninePatchDef.
  /// |tooltip_text_blueprint| should have a TextDef attached, and will have the
  /// text, vertical / horizontal alignment, position, and rotation set.
  /// |tooltip_text_blueprint| will be created as a child of
  /// |tooltip_line_blueprint|.  |tooltip_line_blueprint| should handle
  /// ShowEvent and HideEvent.
  void Setup(const std::string& tooltip_line_blueprint,
             const std::string& tooltip_text_blueprint);

 private:
  using DeviceButtonPair =
      std::pair<InputManager::DeviceType, InputManager::ButtonId>;
  // Hash function to allow DeviceButtonPair to be used as a key in a map.
  struct DeviceButtonPairHash {
    std::size_t operator()(const DeviceButtonPair& pair) const {
      uint64_t combine = pair.first;
      combine = combine << 32;
      combine += pair.second;
      return std::hash<uint64_t>{}(combine);
    }
  };

  struct Tooltip {
    Entity line = kNullEntity;
    Entity text = kNullEntity;
    bool should_show = false;
  };

  void CreateTooltip(const DeviceButtonPair& pair);
  void UpdateTooltip(const DeviceButtonPair& pair,
                     const DeviceProfile* profile);
  void ConditionallyShowTooltip(const DeviceButtonPair& pair,
                                const Tooltip& tooltip,
                                const DeviceProfile* profile, bool is_new);

  std::string tooltip_line_blueprint_;
  std::string tooltip_text_blueprint_;

  std::unordered_map<DeviceButtonPair, Tooltip, DeviceButtonPairHash> tooltips_;

  std::unordered_map<InputManager::DeviceType, Entity, EnumHash> devices_;

  Dispatcher::ScopedConnection device_connnected_connection_;
  Dispatcher::ScopedConnection show_tooltip_connection_;
  Dispatcher::ScopedConnection hide_tooltip_connection_;
  Registry* registry_;
};

// Triggers DeviceTooltips::ShowTooltip
struct ShowTooltipEvent {
  ShowTooltipEvent() {}
  ShowTooltipEvent(InputManager::DeviceType device,
                   InputManager::ButtonId button, const std::string& hint_text)
      : device(device), button(button), hint_text(hint_text) {}
  InputManager::DeviceType device;
  InputManager::ButtonId button;
  std::string hint_text;

  template <typename Archive>
  void Serialize(Archive archive) {
    int device_int = static_cast<int>(device);
    archive(&device_int, ConstHash("device"));
    device = static_cast<InputManager::DeviceType>(device_int);
    int button_int = static_cast<int>(button);
    archive(&button_int, ConstHash("button"));
    button = static_cast<InputManager::ButtonId>(button_int);
    archive(&hint_text, ConstHash("hint_text"));
  }
};

// Triggers DeviceTooltips::HideTooltip
struct HideTooltipEvent {
  HideTooltipEvent() {}
  HideTooltipEvent(InputManager::DeviceType device,
                   InputManager::ButtonId button)
      : device(device), button(button) {}
  InputManager::DeviceType device;
  InputManager::ButtonId button;

  template <typename Archive>
  void Serialize(Archive archive) {
    int device_int = static_cast<int>(device);
    archive(&device_int, ConstHash("device"));
    device = static_cast<InputManager::DeviceType>(device_int);
    int button_int = static_cast<int>(button);
    archive(&button_int, ConstHash("button"));
    button = static_cast<InputManager::ButtonId>(button_int);
  }
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::DeviceTooltips);
LULLABY_SETUP_TYPEID(lull::ShowTooltipEvent);
LULLABY_SETUP_TYPEID(lull::HideTooltipEvent);

#endif  // LULLABY_CONTRIB_DEVICE_TOOLTIPS_DEVICE_TOOLTIPS_H_
