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

#ifndef LULLABY_MODULES_MANIPULATOR_MANAGER_H_
#define LULLABY_MODULES_MANIPULATOR_MANAGER_H_

#include <memory>

#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/modules/manipulator/manipulator.h"
#include "lullaby/modules/render/mesh_util.h"
#include "lullaby/modules/render/render_view.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"

namespace lull {

/// The Manipulator class is responsible for managing the manipulator entities
/// that can translate, rotate, and scale other entities with transforms.
///
/// The Manipulator class listens to "Manipulator" global prefixed events
/// through the dispatcher and modified InputProcessor. It also is responsible
/// for properly delegating work to the proper manipulators in response to the
/// user's actions.
class ManipulatorManager {
 public:
  explicit ManipulatorManager(Registry* registry,
                              const std::string& asset_prefix);

  ~ManipulatorManager();

  /// Updates the positions of the manipulator indicators and selected entity.
  void AdvanceFrame(Clock::duration delta_time);

  /// Renders the appropriate manipulator indicators, if any.
  void Render(Span<RenderView> views);

  /// Selects |entity| to be manipulated. Can only be selected if Manipulator
  /// mode is enabled through EnableManipulatorMode();
  void SelectEntity(Entity entity);

  /// Returns currently selected entity.
  Entity GetSelectedEntity() const;

 private:
  // Manipulator types
  enum Type {
    kTranslation,
    kScaling,
    kRotation,
    kNumManipulators,
  };

  // Contains data about the current controller input
  struct InputData {
    // Location of our cursor when we are pressing the primary button.
    mathfu::vec3 press_location = mathfu::kZeros3f;
    // Location of the initial grabbing point.
    mathfu::vec3 origin_location = mathfu::kZeros3f;
    // Initial offset when grabbing the indicator.
    mathfu::vec3 initial_offset = mathfu::kZeros3f;
    // Normal of the collision plane.
    mathfu::vec3 plane_normal = mathfu::kZeros3f;
  };

  // Returns the selected entity to its original state before it was pressed.
  void CancelLastAction();

  // Returns a valid indicator index if the device ray is colliding with a
  // manipulator indicator and sets |distance| to the distance between the
  // primary device and colliding indicator. Returns kNoIndicatorHit if there
  // was no collision.
  size_t CheckRayCollidingManipulatorIndicator(float* distance);

  // Disable the current manipulator indicators at the selected entity
  // entity and resets the current manipulator and direction.
  void DisableIndicators();

  // Enables |manipulator| type indicators around |entity|.
  void EnableIndicators(Type manipulator, Entity entity);

  // Processes the event input when pressed and redirects it based on if it was
  // an entity that was selected or a manipulator indicator entity.
  void OnPress(const EventWrapper& event);

  // Processes the event input when the button was released.
  void OnRelease(const EventWrapper& event);

  // Update the position of the dummy to that of the selected entity
  void UpdateDummyPosition();

  Sqt original_sqt_;

  Entity selected_entity_;
  Entity dummy_entity_;
  InputData selected_entity_input_data_;
  std::shared_ptr<InputProcessor> manipulator_input_processor_ = nullptr;
  Registry* registry_;
  Manipulator::ControlMode control_mode_;

  std::unique_ptr<Manipulator> manipulators_[kNumManipulators];
  Type current_manipulator_;
  size_t selected_indicator_;
};
}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ManipulatorManager);

#endif  // LULLABY_MODULES_MANIPULATOR_MANAGER_H_
