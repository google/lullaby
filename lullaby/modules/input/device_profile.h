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

#ifndef LULLABY_MODULES_INPUT_DEVICE_PROFILE_H_
#define LULLABY_MODULES_INPUT_DEVICE_PROFILE_H_

#include "lullaby/util/clock.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/math.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/typeid.h"

namespace lull {
extern const Clock::duration kDefaultLongPressTime;

/// Information about how to handle a specific controller.  All units are in
/// meters, and all values after sensor_to_mesh are in mesh space.
struct DeviceProfile {
  /// What button ordering standard this device follows.
  // NOTE: These are placeholder values pending product / design / platform
  // deciding on a standard.
  enum ControllerType {
    /// Minimum Daydream Controller
    /// Button 0 = primary button
    /// Button 1 = secondary button
    /// Button 2 = system button
    kThreeButtonController,

    /// Device is a smartphone or similar, where a touchpad position needs to be
    /// projected into real space.
    kTouchScreen,

    /// Not a standard controller type.
    kCustomController,
  };

  /// How the information for a degree of freedom is generated.  If using an
  /// elbow model, has_position should be set to kFakeDof.
  enum DofType { kUnavailableDof, kFakeDof, kRealDof };

  /// What hand a controller can be held in.  If Either, code can attempt to
  /// guess the actual hand.
  enum Handedness { kEitherHand, kLeftHand, kRightHand };

  /// Information about how to handle and render a specific button.
  struct Button {
    /// The setting for buttons that do not have associated bones.
    static const uint8_t kInvalidBone = std::numeric_limits<uint8_t>::max();

    /// Describes the intended purpose of the button
    // NOTE: These are placeholder values pending product / design / platform
    // deciding on a standard.
    enum Purpose {
      kUnspecified,
      kButton0,  // should be primary 'click' button
      kButton1,  // should be 'right click' / 'app click'
      kButton2,
      kButton3,
      kSystem,  // recenter or go to dashboard
      kVolumeDown,
      kVolumeUp,
    };

    /// Describes the physical form of the button
    // NOTE: These are placeholder values pending product / design / platform
    // deciding on a standard.
    enum Type {
      kStandardButton,
      kTouchpad,
      kShoulder,
      kShoulderAnalog,
      kGrip,
      kDpadDown,
      kDpadUp,
      kDpadLeft,
      kDpadRight,
      kStick,
      kLeftStick,
      kRightStick,
      /// A merged button is a special "virtual" button that doesn't necessarily
      /// correspond to a physical button on the device. This button can merge
      /// one or more physical buttons into a single action with button states
      /// ("pressed", "just released", etc.).
      /// For example, a "primary" button could be a merged button that
      /// corresponds to both the touchpad and trigger of the device.
      kMergedButton,
      kOther,
    };

    /// What type of button this is.
    Type type = kOther;
    /// What this button should be used for.
    Purpose purpose = kUnspecified;

    /// Origin point for a tooltip line, and the direction that line should go.
    /// Tooltip text should be past the end of that line.
    Ray tooltip_ray;

    /// Bone for animated buttons.
    /// numbers >= MAX_BONES will be unused / ignored.
    uint8_t bone = kInvalidBone;
    /// UV rect to color on press. (xmin, ymin, xmax, ymax)
    mathfu::vec4 uv_coords = mathfu::kZeros4f;

    /// The transform for the |bone| when the button is pressed.  This should be
    /// combined with the bone's default pose, not replace it.
    mathfu::vec3 pressed_position = mathfu::kZeros3f;
    mathfu::quat pressed_rotation = mathfu::quat::identity;
  };

  /// Information about how to handle and render a touchpad.
  struct Touchpad {
    /// Origin point for a tooltip line, and the direction that line should go.
    /// Tooltip text should be past the end of that line.
    Ray tooltip_ray;

    /// UV rect for displaying the touch. (xmin, ymin, xmax, ymax)
    mathfu::vec4 uv_coords = mathfu::kZeros4f;
    /// Radius of the touch indicator in uv space, 0-1 range.
    float touch_radius = 0.0f;

    /// True if the device includes gesture detection for this touchpad.
    bool has_gestures = false;
  };

  /// Information about how to render the battery level.
  struct Battery {
    /// UV space of the 'empty' battery texture. (xmin, ymin, xmax, ymax)
    mathfu::vec4 uv_coords = mathfu::kZeros4f;
    /// UV offset to go from 'empty' to 'charged'.
    mathfu::vec2 charged_offset = mathfu::kZeros2f;
    /// UV offset to go from 'empty' to 'critical'.
    mathfu::vec2 critical_offset = mathfu::kZeros2f;
    /// If charge <= critical_percentage, use the critical offset instead of the
    /// charged offset.  Range 0.0f to 1.0f.
    float critical_percentage = 0.2f;
    /// How many divisions are supported by the texture (5 for 5 dots, 100 for a
    /// smooth bar, etc).
    uint8_t segments = 5;
  };

  // The assets that can be used to render this controller.
  struct Assets {
    /// Path to mesh.
    std::string mesh = "";
    /// Path to a texture to use with an unlit shader.
    std::string unlit_texture = "";

    // TODO include PBR assets
  };

  /// For now these are just used to count how many a device has, but in the
  /// future they will contain information.
  struct Joystick {};
  struct Eye {};
  struct ScrollWheel {};

  // An optional name to uniquely identify this profile from other profiles.
  HashValue name = 0;

  /// The assets to use for rendering this controller.
  Assets assets;

  /// Ray that should be used to select distance objects
  Ray selection_ray;

  /// Sphere that represents the volumetric collision region for the tip of the
  /// controller.
  Sphere selection_sphere;

  /// Which standard this controller follows
  ControllerType type = kCustomController;
  /// If the device can only be used for a single hand.
  Handedness handedness = kEitherHand;

  /// What type of position information does this device report.
  DofType position_dof = kUnavailableDof;
  /// What type of rotation information does this device report.
  DofType rotation_dof = kUnavailableDof;

  std::vector<Joystick> joysticks;
  std::vector<Button> buttons;
  std::vector<Button> analog_buttons;
  std::vector<Eye> eyes;
  std::vector<Touchpad> touchpads;
  std::vector<ScrollWheel> scroll_wheels;
  Optional<Battery> battery;

  /// The long press time for buttons on this device.
  Clock::duration long_press_time = kDefaultLongPressTime;
};

}  // namespace lull

#endif  // LULLABY_MODULES_INPUT_DEVICE_PROFILE_H_
