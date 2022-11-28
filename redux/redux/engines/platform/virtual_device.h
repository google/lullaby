/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_ENGINES_PLATFORM_VIRTUAL_DEVICE_H_
#define REDUX_ENGINES_PLATFORM_VIRTUAL_DEVICE_H_

#include <functional>
#include <optional>
#include <utility>

#include "absl/time/time.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Virtual devices provide an API for platform-specific code to "push" data
// about a device into DeviceManager. Once all input events have been processed,
// the DeviceManager will make the current state of the devices available to
// the rest of the runtime to query.
class VirtualDevice {
 public:
  virtual ~VirtualDevice() { on_destroy_(); }

  VirtualDevice(const VirtualDevice&) = delete;
  VirtualDevice& operator=(const VirtualDevice&) = delete;

  // The state for a binary trigger inputs (i.e. keys and buttons).
  using TriggerFlag = uint8_t;
  static constexpr TriggerFlag kReleased = 0x01 << 0;
  static constexpr TriggerFlag kPressed = 0x01 << 1;
  static constexpr TriggerFlag kLongPressed = 0x01 << 2;
  static constexpr TriggerFlag kJustReleased = 0x01 << 3;
  static constexpr TriggerFlag kJustPressed = 0x01 << 4;
  static constexpr TriggerFlag kJustLongPressed = 0x01 << 5;
  static constexpr TriggerFlag kRepeat = 0x01 << 6;

  // The state for a binary value.
  struct BooleanState {
    bool active = false;
    absl::Duration toggle_time;
  };

  // The state for a single scalar value.
  struct ScalarState {
    bool active = false;
    float value = 0.f;
    absl::Duration toggle_time;
  };

  // The state for a 2D value.
  struct Vec2State {
    vec2i value = vec2i::Zero();
  };

  // A view provides read-only information about a device's state.
  template <typename Device>
  struct VirtualView {
   protected:
    const Device* GetDevice() const { return getter(); }
    friend class DeviceManager;
    std::function<const Device*()> getter;
  };

  // Returns the TriggerFlag based on current and previous states.
  static const TriggerFlag DetermineTrigger(bool curr, bool prev);
  static const TriggerFlag DetermineTrigger(
      const BooleanState& curr, const BooleanState& prev,
      std::optional<float> long_press_time_ms);

 protected:
  using OnDestroy = std::function<void()>;
  explicit VirtualDevice(OnDestroy on_destroy)
      : on_destroy_(std::move(on_destroy)) {}

  friend class DeviceManager;
  virtual void Apply(absl::Duration delta_time) = 0;

 private:
  OnDestroy on_destroy_;
};
}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_VIRTUAL_DEVICE_H_
