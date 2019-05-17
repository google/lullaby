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

#ifndef LULLABY_MODULES_MANIPULATOR_SCALING_MANIPULATOR_H_
#define LULLABY_MODULES_MANIPULATOR_SCALING_MANIPULATOR_H_

#include <vector>
#include "lullaby/modules/manipulator/manipulator.h"

namespace lull {
/// Scaling manipulators allow for an entity to be scaled, using indicators
/// positioned in the global X, Y, Z axis, along their X, Y, and Z axis via
/// the dragging of its indicators.
///
/// This class handles managing its own indicator's transforms and applying the
/// proper scaling to the specified entity.
class ScalingManipulator : public Manipulator {
 public:
  ScalingManipulator(Registry* registry);

  void ApplyManipulator(Entity entity, const mathfu::vec3& previous_cursor_pos,
                        const mathfu::vec3& current_cursor_pos,
                        size_t indicator_index) override;

  float CheckRayCollidingIndicator(const Ray& ray,
                                   size_t indicator_index) override;

  void SetupIndicators(Entity entity) override;

  void Render(Span<RenderView> views) override;

  void ResetIndicators() override;

  void UpdateIndicatorsTransform(const mathfu::mat4& transform) override;

  mathfu::vec3 GetMovementPlaneNormal(size_t indicator_index) const override;

  mathfu::vec3 GetDummyPosition(size_t indicator_index) const override;

  size_t GetNumIndicators() const override;

 private:
  enum Direction {
    kScaleX,
    kScaleY,
    kScaleZ,
    kScaleUniform,
    kNumDirections,
  };
  std::vector<mathfu::mat4> indicator_transforms_;
  float indicator_length_offset_[kNumDirections];
  mathfu::vec3 selected_entity_translation_;
  mathfu::quat initial_rotations_[kNumDirections];
  mathfu::quat selected_entity_rotation_;
};
}  // namespace lull
#endif  // LULLABY_MODULES_MANIPULATOR_SCALING_MANIPULATOR_H_
