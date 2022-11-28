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

#ifndef REDUX_ENGINES_PLATFORM_DEVICE_PROFILES_H_
#define REDUX_ENGINES_PLATFORM_DEVICE_PROFILES_H_

#include <optional>

#include "absl/container/flat_hash_map.h"
#include "redux/modules/math/vector.h"

namespace redux {

// The different types of devices supported.
enum class DeviceType {
  kDisplay,
  kMouse,
  kKeyboard,
  kSpeaker,
  kTouchpad,
  kController,
  kHmd,
  kHand,
};

// Enum specifying how to handle degrees-of-freedom.
enum class DofProfile {
  kNoDof,    // Device does not provide dof.
  kFakeDof,  // dof values are emulated in software.
  kRealDof,  // dof values are obtained directly from hardware.
};

// Profile for a display device.
struct DisplayProfile {
  static constexpr DeviceType kType = DeviceType::kDisplay;

  void* native_window = nullptr;
  vec2i pixel_size = vec2i::Zero();
  vec2i display_size = vec2i::Zero();
  bool is_touchscreen = false;
  bool supports_gestures = false;
};

// Profile for a mouse device.
struct MouseProfile {
  static constexpr DeviceType kType = DeviceType::kMouse;

  enum Button {
    kLeftButton,
    kRightButton,
    kMiddleButton,
    kBackButton,
    kForwardButton
  };

  size_t num_buttons = 0;
  bool has_scroll_wheel = false;
  std::optional<float> long_press_time_ms;

  // An optional mapping between the button type and the button index. If not
  // specified, we will use the enumeration value itself as the mapping.
  absl::flat_hash_map<Button, size_t> button_map;
};

// Profile for a speaker device.
struct SpeakerProfile {
  static constexpr DeviceType kType = DeviceType::kSpeaker;

  int sample_rate_hz = 0;
  size_t num_channels = 0;
  size_t frames_per_buffer = 0;
};

// Profile for a keyboard device.
struct KeyboardProfile {
  static constexpr DeviceType kType = DeviceType::kKeyboard;
};

// Profile for a touchpad device.
struct TouchpadProfile {
  static constexpr DeviceType kType = DeviceType::kTouchpad;

  bool supports_gestures = false;
};

// Profile for a hand tracking sensor.
struct HandProfile {
  static constexpr DeviceType kType = DeviceType::kHand;

  enum Hand { kLeftHand, kRightHand, kEitherHand };

  Hand hand = kEitherHand;
  DofProfile rotation_dof = DofProfile::kNoDof;
  DofProfile translation_dof = DofProfile::kNoDof;
};

// Profile for a head-mounted display (HMD) sensor.
struct HmdProfile {
  static constexpr DeviceType kType = DeviceType::kHmd;

  enum Eye { kLeftEye, kRightEye };

  size_t num_eyes = 2;
  DofProfile rotation_dof = DofProfile::kNoDof;
  DofProfile translation_dof = DofProfile::kNoDof;

  // An optional mapping between the eye type and the eye index. If not
  // specified, we will use the enumeration value itself as the mapping.
  absl::flat_hash_map<Eye, size_t> eye_map;
};

// Profile for a generic controller; a collection of buttons (1D values ranging
// from 0 to 1) and sticks (2D values ranging from [-1,-1] to [1,1]).
struct ControllerProfile {
  static constexpr DeviceType kType = DeviceType::kController;

  enum Button {
    kPrimaryButton,
    kSecondaryButton,
    kTertiaryButton,
    kCancelButton,
    kLeftShoulder,
    kRightShoulder,
    kSecondaryLeftShoulder,
    kSecondaryRightShoulder,
  };

  enum Stick {
    kPrimaryStick,
    kSecondaryStick,
    kPrimaryDpad,
    kSecondaryDpad,
  };

  size_t num_sticks = 0;
  size_t num_buttons = 0;
  absl::flat_hash_map<Stick, size_t> stick_map;
  absl::flat_hash_map<Button, size_t> button_map;
  std::optional<float> long_press_time_ms;
  std::optional<float> button_threshold;
  std::optional<float> dead_zone;
};
}  // namespace redux

#endif  // REDUX_ENGINES_PLATFORM_DEVICE_PROFILES_H_
