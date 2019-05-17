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

#ifndef LULLABY_MODULES_MANIPULATOR_H_
#define LULLABY_MODULES_MANIPULATOR_H_

#include <vector>

#include "lullaby/modules/render/render_view.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"

namespace lull {

/// A general class for each type of manipulator with these functions.
/// In general, these manipulator classes are more meant to store extra
/// information of the manipulator indicators (besides it's sqt and collision
/// information).
class Manipulator {
 public:
  enum ControlMode {
    kLocal,
    kGlobal,
  };

  Manipulator(Registry* registry) : registry_(registry) {
    if (!registry) {
      LOG(DFATAL) << "Attempted to pass a nullptr registry";
    }
  }

  virtual ~Manipulator() {}

  /// Applies the manipulator's actions on the entity based on the starting
  /// and ending vector.
  virtual void ApplyManipulator(Entity entity,
                                const mathfu::vec3& previous_cursor_pos,
                                const mathfu::vec3& current_cursor_pos,
                                size_t indicator_index) = 0;

  /// Checks if the ray is colliding against the specified indicator.
  virtual float CheckRayCollidingIndicator(const Ray& ray,
                                           size_t indicator_index) = 0;

  /// Sets up the indicators around |entity| every time the manipulator is
  /// selected.
  virtual void SetupIndicators(Entity entity) = 0;

  /// Renders the indicators every frame.
  virtual void Render(Span<RenderView> views) = 0;

  /// Reset the indicators whenever the user cancels the current action or
  /// releases the primary button.
  virtual void ResetIndicators() = 0;

  /// Sets the manipulator to local mode.
  virtual void SetControlMode(ControlMode mode) {}

  /// Updates the indicator's transform every frame.
  virtual void UpdateIndicatorsTransform(const mathfu::mat4& transform) = 0;

  /// Returns the normal for the plane the cursor will collide against for
  /// applying the manipulator.
  virtual mathfu::vec3 GetMovementPlaneNormal(size_t indicator_index) const = 0;

  /// Get the number of indicators.
  virtual size_t GetNumIndicators() const = 0;

  /// Get the position the dummy should be when |indicator_index| is selected.
  virtual mathfu::vec3 GetDummyPosition(size_t indicator_index) const = 0;

 protected:
  Registry* registry_;
};
}  // namespace lull

#endif  // LULLABY_MODULES_MANIPULATOR_MANIPULATOR_H_
