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

#ifndef LULLABY_CONTRIB_CONTROLLER_CONTROLLER_SYSTEM_H_
#define LULLABY_CONTRIB_CONTROLLER_CONTROLLER_SYSTEM_H_

#include <memory>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"

namespace lull {

namespace {
const float kControllerFadeDistance = 0.10f;
}  // namespace

// The controller system handles the visual representation of the controller,
// tracking the controller's position in a similar manner as the TrackHmdSystem
// tracks the head.
class ControllerSystem : public System {
 public:
  explicit ControllerSystem(Registry* registry);
  ~ControllerSystem() override;

  // Note: This should be called after the InputFocus is updated for the frame.
  // That is usually done by ReticleSystem or StandardInputPipeline.
  void AdvanceFrame(const Clock::duration& delta_time);

  void Create(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;

  /// Show all controller and laser models. If |hard_enable| is true, the
  /// controller entity will be immediately enabled rather than waiting for the
  /// tracking to resume and fading in gently.
  void ShowControls(bool hard_enable = true);
  /// Hide all controller and laser models.
  void HideControls();

  /// Show the laser model of |controller_type|.
  void ShowLaser(InputManager::DeviceType controller_type);
  /// Hide the laser model of |controller_type|.
  void HideLaser(InputManager::DeviceType controller_type);
  /// Retuns whether the laser model of |controller_type| is hidden by request.
  bool IsLaserHidden(InputManager::DeviceType controller_type);
  /// Sets the fading points of the laser to a given value. Returns true if
  /// succeeded.
  bool SetLaserFadePoints(InputManager::DeviceType controller_type,
                          const mathfu::vec4& fade_points);
  /// Resets the fading points of the laser to the default value. Returns true
  /// if succeeded.
  bool ResetLaserFadePoints(InputManager::DeviceType controller_type);

  /// Sets the controller fading distance from the head. The value is used to
  /// fade the controller when it's too close to the head so that it doesn't
  /// block the entire view.
  void SetControllerFadeDistance(float controller_fade_distance) {
    controller_fade_distance_ = controller_fade_distance;
  }

  /// Resets the controller fading distance from the head.
  void ResetControllerFadeDistance() {
    controller_fade_distance_ = kControllerFadeDistance;
  }

 private:
  struct Button {
    bool was_pressed = false;

    float anim_start_alpha = 0.0f;
    float target_alpha = 0.0f;
    float current_alpha = 0.0f;
    Clock::duration anim_time_left = Clock::duration::zero();
  };

  struct Controller : Component {
    explicit Controller(Entity entity);
    InputManager::DeviceType controller_type = InputManager::kController;
    bool is_laser = false;
    bool is_visible = false;
    bool connected = false;
    // The name of the last DeviceProfile this controller was rendered with.
    // This is used to detect when the profile changed without the connection
    // status changing.
    HashValue device_profile_name = 0;
    // Indicates whether this model should hide. Note should_hide == false does
    // not necessarily mean it should show.
    bool should_hide = false;

    const EventDefArray* enable_events = nullptr;
    const EventDefArray* disable_events = nullptr;

    // Laser specific variables.
    float last_bend_fraction = 0.0f;
    mathfu::vec4 default_fade_points_;

    // Controller specific variables;
    std::vector<Button> buttons = std::vector<Button>(3);
    float touch_ripple_factor = 0.0f;
    bool was_touched = false;
    bool touchpad_button_pressed = false;
    uint8_t last_battery_level = InputManager::kInvalidBatteryCharge;
  };

  void OnControllerConnected(Controller* controller);
  void OnControllerDisconnected(Controller* controller);

  void HandleControllerTransforms(const Clock::duration& delta_time,
                                  Controller* controller);
  void MaybeFadeInController(Controller* controller);
  void MaybeFadeOutController(Controller* controller);

  void UpdateBendUniforms(Controller* laser);
  void UpdateControllerUniforms(const Clock::duration& delta_time,
                                Controller* controller);
  void UpdateControllerButtonUniforms(const Clock::duration& delta_time,
                                      Controller* controller);
  void UpdateControllerTouchpadUniforms(const Clock::duration& delta_time,
                                        Controller* controller);
  void UpdateControllerBatteryUniforms(const Clock::duration& delta_time,
                                       Controller* controller);

  ComponentPool<Controller> controllers_;
  float controller_fade_distance_ = kControllerFadeDistance;
};

}  // namespace lull
LULLABY_SETUP_TYPEID(lull::ControllerSystem);

#endif  // LULLABY_CONTRIB_CONTROLLER_CONTROLLER_SYSTEM_H_
