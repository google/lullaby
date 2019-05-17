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

#include "lullaby/modules/manipulator/translation_manipulator.h"

#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"
#include "mathfu/constants.h"

namespace lull {

namespace {
const Aabb kTranslationAabb(mathfu::vec3(-0.04f, -0.05f, 0.15f),
                            mathfu::vec3(0.04f, 0.05f, 0.8f));
}  // namespace
TranslationManipulator::TranslationManipulator(Registry* registry,
                                              const std::string& asset_prefix)
    : Manipulator(registry),
      indicator_transforms_(kNumDirections),
      control_mode_(Manipulator::kGlobal) {
  const Color4ub kStartTints[] = {
      {204, 0, 0, 204},  // red
      {0, 204, 0, 204},  // green
      {0, 0, 204, 204},  // blue
  };

  const Color4ub kEndTints[] = {
      {127, 0, 0, 204},  // darker red
      {0, 127, 0, 204},  // darker green
      {0, 0, 127, 204},  // darker blue
  };
  // Create the arrow meshes.
  for (int i = 0; i < indicator_transforms_.size(); ++i) {
    arrow_meshes_[i] = CreateArrowMeshWithTint(
        /*start_angle*/ 0.f,
        /*delta_angle*/ 2.f * mathfu::kPi / 15.f,
        /*line_length*/ 0.5f,
        /*line_width*/ 0.01f,
        /*line_offset*/ 0.15f,
        /*pointer_height*/ .04f,
        /*pointer_length*/ 0.2f,
        /*start_tint*/ kStartTints[i],
        /*end_tint*/ kEndTints[i]);
  }
  // Load the arrow shader here to prevent spam calls every frame they are
  // rendered.
  auto* render_system = registry_->Get<RenderSystem>();
  const std::string kShapeShader = asset_prefix
      + "shaders/vertex_color.fplshader";
  shape_shader_ = render_system->LoadShader(kShapeShader);
}

void TranslationManipulator::ApplyManipulator(
    Entity entity, const mathfu::vec3& previous_cursor_pos,
    const mathfu::vec3& current_cursor_pos, size_t indicator_index) {
  // Calculate the projection of the delta position onto the axis of translation
  // to move the manipulators and entity in that direction.
  mathfu::vec3 axis_vec;
  switch (indicator_index) {
    case kTranslateX:
      axis_vec = mathfu::kAxisX3f;
      break;
    case kTranslateY:
      axis_vec = mathfu::kAxisY3f;
      break;
    case kTranslateZ:
      axis_vec = mathfu::kAxisZ3f;
      break;
    default:
      return;
  }
  if (control_mode_ == Manipulator::kLocal) {
    axis_vec = entity_rotation_ * axis_vec;
  }
  // Convert the positions to worldspace before applying the translation to
  // account for selected children entities.
  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::vec3 delta_pos = current_cursor_pos - previous_cursor_pos;
  const float projection = mathfu::dot(delta_pos, axis_vec);
  const mathfu::vec3 translation = axis_vec * projection;
  const mathfu::mat4* entity_world_from_object =
      transform_system->GetWorldFromEntityMatrix(entity);
  Sqt sqt = CalculateSqtFromMatrix(entity_world_from_object);
  sqt.translation += translation;
  const mathfu::mat4 new_entity_world_from_object =
      CalculateTransformMatrix(sqt);
  transform_system->SetWorldFromEntityMatrix(entity,
                                             new_entity_world_from_object);

  // Update the indicators.
  UpdateIndicatorsTransform(new_entity_world_from_object);
}

float TranslationManipulator::CheckRayCollidingIndicator(
    const Ray& ray, size_t indicator_index) {
  return CheckRayOBBCollision(ray, indicator_transforms_[indicator_index],
                              kTranslationAabb);
}

void TranslationManipulator::SetupIndicators(Entity entity) {
  if (entity == kNullEntity) {
    return;
  }
  size_t num_indicators = indicator_transforms_.size();
  auto* transform_system = registry_->Get<TransformSystem>();
  // Use the translation vector from the world entity instead of grabbing the
  // local to account for the parent's translation.
  const mathfu::mat4* entity_matrix =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (!entity_matrix) {
    return;
  }
  entity_rotation_ = mathfu::quat::FromMatrix(*entity_matrix);
  for (size_t i = 0; i < num_indicators; ++i) {
    Sqt indicator_sqt;
    indicator_sqt.translation = entity_matrix->TranslationVector3D();
    indicator_sqt.scale = mathfu::kOnes3f;
    // Rotate appropriately. This is relative to an arrow pointing towards
    // the negative Z-Axis (towards the camera).
    mathfu::vec3 rotation = mathfu::kZeros3f;
    switch (i) {
      case kTranslateX:  // X-axis
        rotation = mathfu::vec3(0.f, 90.f * mathfu::kDegreesToRadians, 0.f);
        break;
      case kTranslateY:  // Y-axis
        rotation = mathfu::vec3(-90.f * mathfu::kDegreesToRadians, 0.f, 0.f);
        break;
      case kTranslateZ:  // Z-axis
        rotation = mathfu::vec3(0.f, 0.f, 0.f);
        break;
    }
    initial_rotations_[i] = mathfu::quat::FromEulerAngles(rotation);
    if (control_mode_ == Manipulator::kLocal) {
      indicator_sqt.rotation = entity_rotation_ * initial_rotations_[i];
    } else {
      indicator_sqt.rotation = initial_rotations_[i];
    }
    indicator_transforms_[i] = CalculateTransformMatrix(indicator_sqt);
  }
}

void TranslationManipulator::Render(Span<RenderView> views) {
  if (views.empty()) {
    return;
  }
  auto* render_system = registry_->Get<RenderSystem>();
  size_t num_indicators = indicator_transforms_.size();
  for (size_t view = 0; view < views.size(); ++view) {
    render_system->SetViewport(views[view]);
    for (size_t indicator = 0; indicator < num_indicators; ++indicator) {
      render_system->BindShader(shape_shader_);
      render_system->DrawMesh(arrow_meshes_[indicator],
                              views[view].clip_from_world_matrix *
                                  indicator_transforms_[indicator]);
    }
  }
}

void TranslationManipulator::SetControlMode(ControlMode mode) {
  control_mode_ = mode;
}

void TranslationManipulator::UpdateIndicatorsTransform(
    const mathfu::mat4& transform) {
  // Update indicator position and recalculate their transform matrices.
  const Sqt entity_sqt = CalculateSqtFromMatrix(transform);
  entity_rotation_ = entity_sqt.rotation.Normalized();
  for (int i = 0; i < indicator_transforms_.size(); ++i) {
    Sqt sqt = CalculateSqtFromMatrix(indicator_transforms_[i]);
    sqt.translation = transform.TranslationVector3D();
    sqt.scale = mathfu::kOnes3f;
    if (control_mode_ == Manipulator::kLocal) {
      sqt.rotation = entity_rotation_ * initial_rotations_[i];
    } else {
      sqt.rotation = initial_rotations_[i];
    }
    indicator_transforms_[i] = CalculateTransformMatrix(sqt);
  }
}

mathfu::vec3 TranslationManipulator::GetMovementPlaneNormal(
    size_t indicator_index) const {
  mathfu::vec3 movement_normal = mathfu::kZeros3f;
  switch (indicator_index) {
    // Case kTranslateX should fallthrough kTranslateY since they both have the
    // same movement plane.
    case kTranslateX:
    case kTranslateY:
      movement_normal = mathfu::kAxisZ3f;
      break;
    case kTranslateZ:
      movement_normal = mathfu::kAxisX3f;
      break;
    default:
      LOG(DFATAL)
          << "Grabbing a translation movement plane for an unknown axis.";
      return mathfu::kZeros3f;
  }
  if (control_mode_ == Manipulator::kLocal) {
    movement_normal = entity_rotation_ * movement_normal;
  }
  return movement_normal;
}

mathfu::vec3 TranslationManipulator::GetDummyPosition(
    size_t indicator_index) const {
  return indicator_transforms_[indicator_index].TranslationVector3D();
}

size_t TranslationManipulator::GetNumIndicators() const {
  return indicator_transforms_.size();
}
}  // namespace lull
