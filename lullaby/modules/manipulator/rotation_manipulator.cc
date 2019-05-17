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

#include "lullaby/modules/manipulator/rotation_manipulator.h"

#include <cmath>
#include "lullaby/modules/debug/debug_render.h"
#include "lullaby/modules/debug/log_tag.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input/input_manager_util.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/io.h"

namespace lull {
namespace {
const size_t kNumCircleSegments = 180;
const float kRingRadius = 0.4f;
}  //  namespace

RotationManipulator::RotationManipulator(Registry* registry,
                                         const std::string& asset_prefix)
    : Manipulator(registry), dragging_(false), local_mode_(false) {
  // Create the ring vertices.
  const float end_angle = 2.f * mathfu::kPi;
  const float delta_angle = end_angle / static_cast<float>(kNumCircleSegments);
  ring_verts_.reserve(kNumCircleSegments + 1);
  for (float current_angle = 0.f; current_angle <= end_angle;
       current_angle += delta_angle) {
    mathfu::vec3 point(kRingRadius * cosf(current_angle),
                       kRingRadius * sinf(current_angle), 0.f);
    ring_verts_.emplace_back(point);
  }
  // Load the shader once here to prevent a spam of error calls every time
  // the rings are rendered.
  auto* render_system = registry_->Get<RenderSystem>();
  const std::string kShapeShader =
      asset_prefix + "shaders/vertex_color.fplshader";
  shape_shader_ = render_system->LoadShader(kShapeShader);
}

mathfu::mat4 RotationManipulator::CalculateMatrixForRingFacingCamera() {
  const mathfu::vec3 translation =
      camera_pos_ - indicator_transforms_[0].TranslationVector3D();
  const mathfu::quat camera_rotation =
      mathfu::quat::RotateFromTo(mathfu::kAxisZ3f, translation);
  const Sqt sqt(indicator_transforms_[0].TranslationVector3D(), camera_rotation,
                mathfu::kOnes3f);
  return CalculateTransformMatrix(sqt);
}

void RotationManipulator::ComputeCameraPosition(const Span<RenderView>& views) {
  // Take the average of the views to get a camera position to use for other
  // calculations in this manipulator.
  camera_pos_ = mathfu::kZeros3f;
  const size_t num_views = views.size();
  for (size_t i = 0; i < num_views; ++i) {
    camera_pos_ += views[i].world_from_eye_matrix.TranslationVector3D();
  }
  camera_pos_ /= static_cast<float>(num_views);
}

MeshData RotationManipulator::GenerateRingMesh(Color4ub color,
                                               size_t indicator_index,
                                               BackSideFlags back_side_flag) {
  MeshData ring_mesh(
      MeshData::PrimitiveType::kLines, VertexPC::kFormat,
      DataContainer::CreateHeapDataContainer(
          2 * VertexPC::kFormat.GetVertexSize() * ring_verts_.size()));
  mathfu::vec3 head_diff(mathfu::kZeros3f);
  if (back_side_flag == kHideBackSide) {
    // Find the vector pointing away from the camera in object space.
    head_diff = indicator_transforms_[indicator_index].Inverse() * camera_pos_;
  }
  for (size_t i = 0; i < ring_verts_.size() - 1; ++i) {
    if (back_side_flag == kHideBackSide &&
        mathfu::dot(head_diff, ring_verts_[i]) < 0.f) {
      // Skip these vertices if they are behind the plane of the other vertices.
      continue;
    }
    ring_mesh.AddVertex<VertexPC>(ring_verts_[i].x, ring_verts_[i].y,
                                  ring_verts_[i].z, color);
    ring_mesh.AddVertex<VertexPC>(ring_verts_[i + 1].x, ring_verts_[i + 1].y,
                                  ring_verts_[i + 1].z, color);
  }
  return ring_mesh;
}

void RotationManipulator::ApplyManipulator(
    Entity entity, const mathfu::vec3& previous_cursor_pos,
    const mathfu::vec3& current_cursor_pos, size_t indicator_index) {
  auto* transform_system = registry_->Get<TransformSystem>();
  if (!dragging_) {
    ComputeTangentVector(current_cursor_pos, indicator_index);
    dragging_ = true;
  }

  // If the current position of the cursor is way too far from the object,
  // then it should not apply anymore rotation than beyond the limit,
  // otherwise it might spin.
  const float distance_from_collision_squared =
      (initial_collision_pos_ - current_cursor_pos).LengthSquared();
  const mathfu::vec3 delta_pos = current_cursor_pos - previous_cursor_pos;
  const float kMaxDistanceSquared = 16.f;
  if (distance_from_collision_squared >= kMaxDistanceSquared ||
      IsNearlyZero(delta_pos.LengthSquared())) {
    return;
  }

  // Calculate the amount of rotation based on the projection of the delta
  // position along the tangent vector.
  const float projection = mathfu::dot(delta_pos, tangent_vector_);
  const mathfu::vec3 ring_plane_normal = GetRingPlaneNormal(indicator_index);
  const mathfu::vec3 rotation = ring_plane_normal * projection;
  const mathfu::quat entity_rotation =
      transform_system->GetLocalRotation(entity);
  const mathfu::quat delta_rotation = mathfu::quat::FromEulerAngles(rotation);
  transform_system->SetLocalRotation(entity, delta_rotation * entity_rotation);

  current_point_on_tangent_ =
      GetClosestPointOnTangentFromRing(current_cursor_pos, indicator_index);
}

mathfu::vec3 RotationManipulator::GetDummyPosition(
    size_t indicator_index) const {
  return (dragging_)
             ? current_point_on_tangent_
             : indicator_transforms_[indicator_index].TranslationVector3D();
}

bool RotationManipulator::CheckPointIsInFrontCameraPlane(
    const mathfu::vec3& point, const mathfu::vec3& ring_center) {
  // Check that the collision ocurred with the part of the ring that appears
  // in front of the camera.
  const mathfu::vec3 head_ring_center_diff = camera_pos_ - ring_center;
  const mathfu::vec3 radial_vec = point - ring_center;
  const float head_radius_dot = mathfu::dot(head_ring_center_diff, radial_vec);
  if (head_radius_dot >= 0.f || IsNearlyZero(head_radius_dot)) {
    return true;
  }
  return false;
}

void RotationManipulator::ComputeTangentVector(
    const mathfu::vec3& collision_pos, size_t indicator_index) {
  const mathfu::vec3 radius_vector =
      collision_pos -
      indicator_transforms_[indicator_index].TranslationVector3D();
  const mathfu::vec3 ring_plane_normal = GetRingPlaneNormal(indicator_index);
  const mathfu::vec3 camera_plane_normal =
      GetMovementPlaneNormal(indicator_index);
  tangent_vector_ =
      mathfu::cross(ring_plane_normal, radius_vector).Normalized();

  // Project into camera plane to make it easier to rotate along with the
  // cursor.
  tangent_vector_ =
      tangent_vector_ -
      mathfu::dot(tangent_vector_, camera_plane_normal) * camera_plane_normal;
  tangent_vector_ = tangent_vector_.Normalized();

  current_point_on_tangent_ =
      indicator_transforms_[indicator_index].TranslationVector3D();
  initial_collision_pos_ = collision_pos;
}

float RotationManipulator::CheckRayCollidingIndicator(const Ray& ray,
                                                      size_t indicator_index) {
  // Check if the ray is colliding with the ring and also is in front of the
  // camera plane since the backface of the ring is not displayed.
  const mathfu::vec3 ring_center =
      indicator_transforms_[indicator_index] * mathfu::kZeros3f;
  const mathfu::vec3 ring_normal = GetRingPlaneNormal(indicator_index);
  const float kRingThickness = 0.01f;
  mathfu::vec3 collision_point = mathfu::kZeros3f;
  Plane collision_plane(ring_center, ring_normal);
  // Compute a collision against the plane the ring is in and compare the
  // collision point to the radius to check if it is within its thickness.
  if (ComputeRayPlaneCollision(ray, collision_plane, &collision_point)) {
    // Measure distance from the ring's center to the collision point
    const float distance_to_ring_center_from_collision =
        (collision_point - ring_center).Length();
    const float collision_radius_difference =
        distance_to_ring_center_from_collision - kRingRadius;
    if (std::abs(collision_radius_difference) <= kRingThickness &&
        CheckPointIsInFrontCameraPlane(collision_point, ring_center)) {
      return (ray.origin - collision_point).Length();
    }
  }
  // If no collision detected with the previous method, then use an algorithm to
  // check a collision against a sphere with the same radius and center as the
  // ring and check if it is within its thickness.
  if (ComputeRaySphereCollision(ray, ring_center, kRingRadius,
                                &collision_point)) {
    const mathfu::vec3 radius_vector = collision_point - ring_center;
    // Compute the vector from the radius vector to the ring plane and use
    // its distance to decide if it is colliding with the ring.
    const mathfu::vec3 radius_ring_normal_projection =
        mathfu::dot(radius_vector, ring_normal) * ring_normal;
    const float radius_ring_normal_projection_distance =
        radius_ring_normal_projection.Length();
    if (radius_ring_normal_projection_distance <= kRingThickness &&
        CheckPointIsInFrontCameraPlane(collision_point, ring_center)) {
      return (ray.origin - collision_point).Length();
    }
  }
  return kNoHitDistance;
}

void RotationManipulator::SetupIndicators(Entity entity) {
  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::mat4* entity_matrix =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (!entity_matrix) {
    LOG(DFATAL) << "No world from entity matrix found when setting up rotation"
                   " indicators!";
    return;
  }
  Sqt entity_sqt = CalculateSqtFromMatrix(entity_matrix);
  mathfu::vec3 entity_pos = entity_sqt.translation;
  selected_entity_rotation_ = entity_sqt.rotation;
  for (int i = 0; i < kNumDirections; ++i) {
    Sqt indicator_sqt;
    indicator_sqt.translation = entity_pos;
    mathfu::vec3 rotation;
    switch (i) {
      case kRotateX:
        rotation = mathfu::vec3(0.f, 90.f * mathfu::kDegreesToRadians, 0.f);
        break;
      case kRotateY:
        rotation = mathfu::vec3(-90.f * mathfu::kDegreesToRadians, 0.f, 0.f);
        break;
      case kRotateZ:
        rotation = mathfu::vec3(0.f, 0.f, 0.f);
        break;
    }
    initial_rotations_[i] = mathfu::quat::FromEulerAngles(rotation);
    indicator_sqt.rotation = initial_rotations_[i];
    indicator_transforms_[i] = CalculateTransformMatrix(indicator_sqt);
  }
}

void RotationManipulator::Render(Span<RenderView> views) {
  ComputeCameraPosition(views);
  const mathfu::mat4 screen_mat = CalculateMatrixForRingFacingCamera();

  const Color4ub kColors[] = {
      {255, 0, 0, 255},  // red
      {0, 255, 0, 255},  // green
      {0, 0, 255, 255},  // blue
      {0, 0, 0, 255},    // black
  };
  auto* render_system = registry_->Get<RenderSystem>();
  for (size_t view = 0; view < views.size(); ++view) {
    render_system->SetViewport(views[view]);
    for (size_t indicator = 0; indicator < kNumDirections; ++indicator) {
      render_system->BindShader(shape_shader_);
      render_system->DrawMesh(
          GenerateRingMesh(kColors[indicator], indicator, kHideBackSide),
          views[view].clip_from_world_matrix *
              indicator_transforms_[indicator]);
    }
    // Draw the black ring that faces the camera and indicates the border
    // between the back and front side of rotation manipulators.
    render_system->BindShader(shape_shader_);
    render_system->DrawMesh(GenerateRingMesh(kColors[3], 0, kIncludeBackSide),
                            views[view].clip_from_world_matrix * screen_mat);
  }

  if (dragging_) {
    // Display a line indicating the tangent line to drag the cursor along
    // to rotate the entity to give the user a visual cue on how to move the
    // cursor.
    lull::debug::DrawLine("lull.Manipulator.Tangent.Line",
                          (50.f * tangent_vector_) + initial_collision_pos_,
                          (-50.f * tangent_vector_) + initial_collision_pos_,
                          Color4ub(255, 255, 255, 255));
    lull::debug::Enable("lull.Manipulator.Tangent.Line");
  }
}

void RotationManipulator::ResetIndicators() { dragging_ = false; }

void RotationManipulator::SetControlMode(ControlMode mode) {
  if (mode == Manipulator::kLocal) {
    local_mode_ = true;
  } else {
    local_mode_ = false;
  }
}

void RotationManipulator::UpdateIndicatorsTransform(
    const mathfu::mat4& transform) {
  const Sqt entity_sqt = CalculateSqtFromMatrix(transform);
  selected_entity_rotation_ = entity_sqt.rotation;
  for (size_t indicator = 0; indicator < kNumDirections; ++indicator) {
    Sqt indicator_sqt =
        CalculateSqtFromMatrix(indicator_transforms_[indicator]);
    indicator_sqt.translation = entity_sqt.translation;
    if (local_mode_) {
      indicator_sqt.rotation = selected_entity_rotation_
          * initial_rotations_[indicator];
    } else {
      indicator_sqt.rotation = initial_rotations_[indicator];
    }
    indicator_transforms_[indicator] = CalculateTransformMatrix(indicator_sqt);
  }
}

mathfu::vec3 RotationManipulator::GetMovementPlaneNormal(
    size_t indicator_index) const {
  // Return a plane normal that is facing the camera.
  const mathfu::vec3 indicator_pos =
      indicator_transforms_[indicator_index].TranslationVector3D();
  return -1.f * (camera_pos_ - indicator_pos).Normalized();
}

size_t RotationManipulator::GetNumIndicators() const { return kNumDirections; }

mathfu::vec3 RotationManipulator::GetClosestPointOnTangentFromRing(
    const mathfu::vec3 current_pos, size_t indicator_index) {
  const mathfu::vec3 change_from_initial_pos =
      current_pos - initial_collision_pos_;
  const float change_from_initial_pos_tangent_dot =
      mathfu::dot(change_from_initial_pos, tangent_vector_);
  const mathfu::vec3 change_tangent_projection =
      change_from_initial_pos_tangent_dot * tangent_vector_;
  return indicator_transforms_[indicator_index].TranslationVector3D() +
         change_tangent_projection;
}

mathfu::vec3 RotationManipulator::GetRingPlaneNormal(
    size_t indicator_index) const {
  mathfu::vec3 ring_normal = mathfu::kZeros3f;
  switch (indicator_index) {
    case kRotateX:
      ring_normal = mathfu::kAxisX3f;
      break;
    case kRotateY:
      ring_normal = mathfu::kAxisY3f;
      break;
    case kRotateZ:
      ring_normal = mathfu::kAxisZ3f;
      break;
    default:
      LOG(DFATAL) << "Invalid indicator index for getting normal!";
      return ring_normal;
  }
  if (local_mode_) {
    ring_normal = selected_entity_rotation_ * ring_normal;
  }
  return ring_normal;
}
}  // namespace lull
