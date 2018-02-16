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

#ifndef LULLABY_CONTRIB_CONTROLLER_CONTROLLER_SYSTEM_H_
#define LULLABY_CONTRIB_CONTROLLER_CONTROLLER_SYSTEM_H_

#include <memory>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"

namespace lull {

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

 private:
  struct Controller : Component {
    explicit Controller(Entity entity);
    InputManager::DeviceType controller_type = InputManager::kController;
    bool is_laser = false;
    bool is_visible = false;

    // The number of frames to let device data stabilize to avoid eye strain
    // from the controller / laser models jumping around. This many frames are
    // ignored before enabling the controller. The counter will be reset when
    // the device disconnects.
    int stabilization_frames = 0;

    // The number of frames that still need to be ignored before allowing the
    // controller to be enabled.
    int stabilization_counter = 0;
    const EventDefArray* enable_events = nullptr;
    const EventDefArray* disable_events = nullptr;

    float last_bend_fraction = 0.0f;
  };

  void HandleControllerTransforms();
  void UpdateBendUniforms(Controller& laser);

  ComponentPool<Controller> controllers_;
  bool show_controls_ = true;
};

}  // namespace lull
LULLABY_SETUP_TYPEID(lull::ControllerSystem);

#endif  // LULLABY_CONTRIB_CONTROLLER_CONTROLLER_SYSTEM_H_
