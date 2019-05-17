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

#include "lullaby/modules/manipulator/scaling_manipulator.h"
#include "lullaby/modules/debug/debug_render.h"
#include "lullaby/modules/debug/log_tag.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "mathfu/constants.h"
#include "mathfu/io.h"

namespace lull {

namespace {
const float kIndicatorLengthOffsetInit = 0.8f;
const Aabb kScaleAabb(mathfu::vec3(-0.04f, -0.04f, 0.f),
                      mathfu::vec3(0.04f, 0.04f, 0.1f));
}  // namespace

ScalingManipulator::ScalingManipulator(Registry* registry)
    : Manipulator(registry), indicator_transforms_(kNumDirections) {
  for (int i = 0; i < kNumDirections; ++i) {
    indicator_length_offset_[i] = 0.f;
  }
}

void ScalingManipulator::ApplyManipulator(
    Entity entity, const mathfu::vec3& previous_cursor_pos,
    const mathfu::vec3& current_cursor_pos, size_t indicator_index) {
  // Calculate the projection of the delta position onto the vector of scaling
  // to scale the entity in that direction.
  mathfu::vec3 axis_vec;
  mathfu::vec3 offset_vec = mathfu::kZeros3f;
  switch (indicator_index) {
    case kScaleX:
      axis_vec = mathfu::kAxisX3f;
      break;
    case kScaleY:
      axis_vec = mathfu::kAxisY3f;
      break;
    case kScaleZ:
      axis_vec = mathfu::kAxisZ3f;
      break;
    case kScaleUniform:
      axis_vec = mathfu::vec3(1.f, 1.f, 0.f).Normalized();
      // This is to apply a proportional scaling among the z-axis.
      offset_vec.z = axis_vec[0];
      break;
    default:
      return;
  }
  const mathfu::vec3 movement_vector = selected_entity_rotation_ * axis_vec;
  // Scale the projection onto the scaling vector.
  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::vec3 delta_pos = current_cursor_pos - previous_cursor_pos;
  const float projection = mathfu::dot(delta_pos, movement_vector);
  const mathfu::vec3 scale = (axis_vec + offset_vec) * projection;
  const mathfu::vec3 entity_scale = transform_system->GetLocalScale(entity);
  transform_system->SetLocalScale(entity, entity_scale + scale);
  // Update the length offset of the currently selected indicator.
  indicator_length_offset_[indicator_index] += projection;
}

float ScalingManipulator::CheckRayCollidingIndicator(const Ray& ray,
                                                     size_t indicator_index) {
  return CheckRayOBBCollision(ray, indicator_transforms_[indicator_index],
                              kScaleAabb);
}

void ScalingManipulator::SetupIndicators(Entity entity) {
  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::mat4* entity_matrix =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (!entity_matrix) {
    LOG(INFO)
        << "Unable to setup scaling indicators due to invalid enitity matrix";
    return;
  }
  size_t num_indicators = indicator_transforms_.size();
  const Sqt entity_sqt = CalculateSqtFromMatrix(entity_matrix);
  selected_entity_translation_ = entity_sqt.translation;
  selected_entity_rotation_ = entity_sqt.rotation;
  for (size_t i = 0; i < num_indicators; ++i) {
    // Setup the indicators for scaling
    mathfu::vec3 rotation;
    switch (i) {
      case kScaleX:
        rotation = mathfu::vec3(0.f, 90.f * mathfu::kDegreesToRadians, 0.f);
        break;
      case kScaleY:
        rotation = mathfu::vec3(-90.f * mathfu::kDegreesToRadians, 0.f, 0.f);
        break;
      case kScaleZ:
        rotation = mathfu::vec3(0.f, 0.f, 0.f);
        break;
      case kScaleUniform:
        rotation = mathfu::vec3(-90.f * mathfu::kDegreesToRadians,
                                -45.f * mathfu::kDegreesToRadians,
                                -45.f * mathfu::kDegreesToRadians);
    }
    Sqt indicator_sqt;
    initial_rotations_[i] = mathfu::quat::FromEulerAngles(rotation);
    indicator_sqt.rotation = selected_entity_rotation_ * initial_rotations_[i];
    // Rotate the indicators and offset them from the entity
    indicator_length_offset_[i] = kIndicatorLengthOffsetInit;
    indicator_sqt.translation = selected_entity_translation_ +
                                indicator_length_offset_[i] *
                                    (indicator_sqt.rotation * mathfu::kAxisZ3f);
    indicator_transforms_[i] = CalculateTransformMatrix(indicator_sqt);
  }
}

void ScalingManipulator::Render(Span<RenderView> views) {
  size_t num_indicators = indicator_transforms_.size();
  const Color4ub colors[] = {
      {220, 0, 0, 255},  // red
      {0, 220, 0, 255},  // green
      {0, 0, 220, 255},  // blue
      {0, 0, 0, 255},    // black
  };
  for (size_t i = 0; i < num_indicators; ++i) {
    debug::DrawLine("lull.Manipulator.Scale.Line", selected_entity_translation_,
                    indicator_transforms_[i].TranslationVector3D(), colors[i]);
    debug::DrawBox3D("lull.Manipulator.Scale.Box", indicator_transforms_[i],
                     kScaleAabb, colors[i]);
    debug::EnableBranch("lull.Manipulator.Scale");
  }
}

void ScalingManipulator::ResetIndicators() {
  size_t num_indicators = indicator_transforms_.size();
  // Reset the scaling offsets to the initial value.
  for (size_t i = 0; i < num_indicators; ++i) {
    indicator_length_offset_[i] = kIndicatorLengthOffsetInit;
  }
}

void ScalingManipulator::UpdateIndicatorsTransform(
    const mathfu::mat4& transform) {
  size_t num_indicators = indicator_transforms_.size();
  const Sqt entity_sqt = CalculateSqtFromMatrix(transform);
  mathfu::vec3 selected_entity_translation = entity_sqt.translation;
  selected_entity_rotation_ = entity_sqt.rotation;
  for (size_t i = 0; i < num_indicators; ++i) {
    Sqt indicator_sqt = CalculateSqtFromMatrix(indicator_transforms_[i]);
    indicator_sqt.rotation = selected_entity_rotation_ * initial_rotations_[i];
    indicator_sqt.translation = selected_entity_translation +
                                indicator_length_offset_[i] *
                                    (indicator_sqt.rotation * mathfu::kAxisZ3f);
    indicator_sqt.scale = mathfu::kOnes3f;
    indicator_transforms_[i] = CalculateTransformMatrix(indicator_sqt);
  }
}

mathfu::vec3 ScalingManipulator::GetMovementPlaneNormal(
    size_t indicator_index) const {
  mathfu::vec3 movement_normal = mathfu::kZeros3f;
  switch (indicator_index) {
    // All three of these are in the same plane so their movement normal
    // is the same.
    case kScaleX:
    case kScaleY:
    case kScaleUniform:
      movement_normal = mathfu::kAxisZ3f;
      break;
    case kScaleZ:
      movement_normal = mathfu::kAxisX3f;
      break;
    default:
      LOG(DFATAL) << "Grabbing a scaling movement plane for an unknown axis.";
      return mathfu::kZeros3f;
  }
  movement_normal = selected_entity_rotation_ * movement_normal;
  return movement_normal;
}

mathfu::vec3 ScalingManipulator::GetDummyPosition(
    size_t indicator_index) const {
  // The dummy should be located the same relative distance and direction away
  // from the selected entity as the initial indicators are to the current
  // indicators.
  const float relative_offset =
      indicator_length_offset_[indicator_index] - kIndicatorLengthOffsetInit;
  const mathfu::vec3 direction_vector =
      mathfu::mat4::ToRotationMatrix(indicator_transforms_[indicator_index]) *
      mathfu::kAxisZ3f;
  return selected_entity_translation_ + (relative_offset * direction_vector);
}

size_t ScalingManipulator::GetNumIndicators() const {
  return indicator_transforms_.size();
}

}  // namespace lull
