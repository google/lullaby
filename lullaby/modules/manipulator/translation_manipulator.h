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

#ifndef LULLABY_MODULES_MANIPULATOR_TRANSLATION_MANIPULATOR_H_
#define LULLABY_MODULES_MANIPULATOR_TRANSLATION_MANIPULATOR_H_

#include "lullaby/modules/manipulator/manipulator.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/modules/render/mesh_util.h"
#include "lullaby/systems/render/shader.h"

namespace lull {

/// Translation manipulators allow for an entity to be translated along the X,Y
/// and Z axis via the dragging of its indicators.
///
/// This class handles managing its indicators in response to the incoming input
/// (handled by the manipulator manager class). This includes applying its
/// translation effects to the specified entity, as well as ensuring the
/// indicators are in the right position every frame.
class TranslationManipulator : public Manipulator {
 public:
  TranslationManipulator(Registry* registry, const std::string& asset_prefix);

  /// Translate the |entity| along the axis corresponding to the
  /// |indicator_index| based on the change in cursor position.
  void ApplyManipulator(Entity entity, const mathfu::vec3& previous_cursor_pos,
                        const mathfu::vec3& current_cursor_pos,
                        size_t indicator_index) override;

  /// Checks to see if the ray is colliding with the specified indicator aabb.
  float CheckRayCollidingIndicator(const Ray& ray,
                                   size_t indicator_index) override;

  /// Set ups the indicators in the proper positions around |entity|.
  void SetupIndicators(Entity entity) override;

  void Render(Span<RenderView> views) override;

  /// Reset the indicators.
  void ResetIndicators() override {}

  void SetControlMode(ControlMode control_mode) override;

  /// Update the indicator's transform to that of |transform|.
  void UpdateIndicatorsTransform(const mathfu::mat4& transform) override;

  /// Get the movement plane normal for the indicator indicated by
  /// |indicator_index|.
  mathfu::vec3 GetMovementPlaneNormal(size_t indicator_index) const override;

  /// Obtain the dummy position for the selected |indicator_index|.
  mathfu::vec3 GetDummyPosition(size_t indicator_index) const override;

  /// Returns the number of indicators.
  size_t GetNumIndicators() const override;

 private:
  enum Direction {
    kTranslateX,
    kTranslateY,
    kTranslateZ,
    kNumDirections,
  };
  std::vector<mathfu::mat4> indicator_transforms_;
  mathfu::quat initial_rotations_[kNumDirections];
  mathfu::quat entity_rotation_;
  MeshData arrow_meshes_[kNumDirections];
  Manipulator::ControlMode control_mode_;
  ShaderPtr shape_shader_;
};
}  // namespace lull

#endif  // LULLABY_MODULES_MANIPULATOR_TRANSLATION_MANIPULATOR_H_
