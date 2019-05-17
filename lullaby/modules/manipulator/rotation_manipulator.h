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

#ifndef LULLABY_MODULES_MANIPULATOR_ROTATION_MANIPULATOR_H_
#define LULLABY_MODULES_MANIPULATOR_ROTATION_MANIPULATOR_H_

#include <vector>

#include "lullaby/modules/manipulator/manipulator.h"
#include "lullaby/modules/render/mesh_data.h"
#include "lullaby/systems/render/shader.h"

namespace lull {
/// Rotation manipulators rotate the entity around the X, Y, and Z axis in
/// worldspace.
///
/// This class handles managing its own indicator's transforms and applying the
/// proper rotation to the specified entity.
class RotationManipulator : public Manipulator {
 public:
  RotationManipulator(Registry* registry, const std::string& asset_prefix);

  void ApplyManipulator(Entity entity, const mathfu::vec3& previous_cursor_pos,
                        const mathfu::vec3& current_cursor_pos,
                        size_t indicator_index) override;

  float CheckRayCollidingIndicator(const Ray& ray,
                                   size_t indicator_index) override;

  void SetupIndicators(Entity entity) override;

  void Render(Span<RenderView> views) override;

  void ResetIndicators() override;

  void SetControlMode(ControlMode mode) override;

  void UpdateIndicatorsTransform(const mathfu::mat4& transform) override;

  mathfu::vec3 GetMovementPlaneNormal(size_t indicator_index) const override;

  mathfu::vec3 GetDummyPosition(size_t indicator_index) const override;

  size_t GetNumIndicators() const override;

 private:
  // Each direction represents the axis on which the entity is rotated about.
  enum RotateDirection {
    kRotateX,
    kRotateY,
    kRotateZ,
    kNumDirections,
  };

  enum BackSideFlags {
    kIncludeBackSide,
    kHideBackSide,
  };

  mathfu::mat4 CalculateMatrixForRingFacingCamera();

  void ComputeCameraPosition(const Span<RenderView>& views);

  // Returns true if |point| on the ring with |center| is in front of the camera
  // plane, otherwise false.
  bool CheckPointIsInFrontCameraPlane(const mathfu::vec3& point,
                                      const mathfu::vec3& ring_center);

  // Generates the meshdata needed to display the indicator.
  MeshData GenerateRingMesh(Color4ub color, size_t indicator_index,
                            BackSideFlags back_side_flag);

  // Computes the vector tangent to the circle at where the |collision_pos|
  // ocurred on the indicator indicated by |indicator_index|.
  void ComputeTangentVector(const mathfu::vec3& collision_pos,
                            size_t indicator_index);

  mathfu::vec3 GetClosestPointOnTangentFromRing(const mathfu::vec3 current_pos,
                                                size_t indicator_index);

  mathfu::vec3 GetRingPlaneNormal(size_t indicator_index) const;

  std::vector<mathfu::vec3> ring_verts_;
  mathfu::mat4 indicator_transforms_[kNumDirections];
  mathfu::quat initial_rotations_[kNumDirections];
  mathfu::quat selected_entity_rotation_;
  ShaderPtr shape_shader_;
  mathfu::vec3 camera_pos_;

  // Variables about the tangent line on the currently selected ring.
  bool dragging_;
  bool local_mode_;
  mathfu::vec3 current_point_on_tangent_;
  mathfu::vec3 initial_collision_pos_;
  mathfu::vec3 tangent_vector_;
};
}  // namespace lull

#endif  // LULLABY_MODULES_MANIPULATOR_ROTATION_MANIPULATOR_H_
