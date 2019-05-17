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

#include "lullaby/modules/manipulator/manipulator_manager.h"
#include <limits>
#include <memory>

#include "lullaby/generated/collision_def_generated.h"
#include "lullaby/modules/debug/debug_render.h"
#include "lullaby/modules/debug/log_tag.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/manipulator/rotation_manipulator.h"
#include "lullaby/modules/manipulator/scaling_manipulator.h"
#include "lullaby/modules/manipulator/translation_manipulator.h"
#include "lullaby/modules/render/mesh_util.h"
#include "lullaby/modules/reticle/standard_input_pipeline.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/entity.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/transform_def_generated.h"
#include "mathfu/io.h"

namespace lull {

static const size_t kNoIndicatorSelected = std::numeric_limits<size_t>::max();

ManipulatorManager::ManipulatorManager(Registry* registry,
                                       const std::string& asset_prefix)
    : selected_entity_(kNullEntity),
      dummy_entity_(kNullEntity),
      registry_(registry),
      control_mode_(Manipulator::kGlobal),
      current_manipulator_(kNumManipulators),
      selected_indicator_(kNoIndicatorSelected) {
  if (!registry_) {
    LOG(DFATAL) << "Attempted to pass a null registry pointer";
  }
  auto* transform_system = registry_->Get<TransformSystem>();
  if (!transform_system) {
    LOG(DFATAL) << "No transform system, unable to create manipulators";
  }

  manipulators_[kTranslation] =
      MakeUnique<TranslationManipulator>(registry_, asset_prefix);
  manipulators_[kScaling] = MakeUnique<ScalingManipulator>(registry_);
  manipulators_[kRotation] =
      MakeUnique<RotationManipulator>(registry_, asset_prefix);

  // Setup prefixed events listeners for manipulators.
  auto* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(ConstHash("ManipulatorPressEvent"), this,
                      [this](const EventWrapper& e) { OnPress(e); });
  dispatcher->Connect(ConstHash("ManipulatorReleaseEvent"), this,
                      [this](const EventWrapper& e) { OnRelease(e); });
  dispatcher->Connect(ConstHash("ManipulatorCancelEvent"), this,
                      [this](const EventWrapper& e) { CancelLastAction(); });
  dispatcher->Connect(ConstHash("ManipulatorSecondaryClickEvent"), this,
                      [this](const EventWrapper& e) {
                        // Ensure all actions are finalized.
                        OnRelease(e);
                        // Swap to the next manipulator.
                        switch (current_manipulator_) {
                          case kTranslation:
                            EnableIndicators(kScaling, selected_entity_);
                            break;
                          case kScaling:
                            EnableIndicators(kRotation, selected_entity_);
                            break;
                          case kRotation:
                            EnableIndicators(kTranslation, selected_entity_);
                            break;
                          case kNumManipulators:
                            break;
                        }
                      });
  dispatcher->Connect(
      ConstHash("ManipulatorSecondaryLongPressEvent"), this,
      [this](const EventWrapper& e) {
        if (current_manipulator_ != kNumManipulators) {
          if (control_mode_ == Manipulator::kGlobal) {
            control_mode_ = Manipulator::kLocal;
          } else {
            control_mode_ = Manipulator::kGlobal;
          }
          manipulators_[current_manipulator_]->SetControlMode(control_mode_);
        }
      });
  // Add override processor to the input stack.
  auto* input = registry_->Get<InputProcessor>();
  if (!input) {
    LOG(DFATAL) << "There is no InputProcessor instance, Unable to create "
                   "manipulators.";
  }
  manipulator_input_processor_ = std::make_shared<InputProcessor>(registry_);
  manipulator_input_processor_->SetPrefix(
      InputManager::kController, InputManager::kPrimaryButton, "Manipulator");
  manipulator_input_processor_->SetPrefix(InputManager::kController,
                                          InputManager::kSecondaryButton,
                                          "ManipulatorSecondary");
  input->AddOverrideProcessor(manipulator_input_processor_);

  // The dummy entity is to allow for manual collisions for the cursor.
  // This will be changed once standard input pipeline is refactored to
  // allow custom collisions without the need of an entity.
  auto* entity_factory = registry_->Get<EntityFactory>();
  if (!entity_factory) {
    LOG(DFATAL) << "No entity factory, unable to create manipulators";
  }
  // Create the dummy entity to allow for action cancelling.
  dummy_entity_ = entity_factory->Create();
  transform_system->Create(dummy_entity_, Sqt());
  // Enable Interaction flag to allow for cancel events
  auto* collision_system = registry_->Get<CollisionSystem>();
  if (collision_system) {
    collision_system->EnableInteraction(dummy_entity_);
  } else {
    LOG(INFO) << "No collision system detected, manipulators will be"
                 "unable to cancel actions.";
  }
}

ManipulatorManager::~ManipulatorManager() {
  auto* input = registry_->Get<InputProcessor>();
  if (input) {
    // Renable normal input events and collision handling in InputProcessor.
    input->RemoveOverrideProcessor(manipulator_input_processor_);
  }
  auto* standard_input_pipeline = registry_->Get<StandardInputPipeline>();
  if (standard_input_pipeline) {
    standard_input_pipeline->StopManualCollision();
  }
  auto* entity_factory = registry_->Get<EntityFactory>();
  if (entity_factory) {
    entity_factory->Destroy(dummy_entity_);
  }
}

void ManipulatorManager::CancelLastAction() {
  if (selected_entity_ == kNullEntity) {
    return;
  }
  TransformSystem* transform_system = registry_->Get<TransformSystem>();
  transform_system->SetSqt(selected_entity_, original_sqt_);
  manipulators_[current_manipulator_]->ResetIndicators();
  manipulators_[current_manipulator_]->UpdateIndicatorsTransform(
      *transform_system->GetWorldFromEntityMatrix(selected_entity_));
  UpdateDummyPosition();
  selected_indicator_ = kNoIndicatorSelected;
}

size_t ManipulatorManager::CheckRayCollidingManipulatorIndicator(
    float* distance) {
  if (selected_entity_ == kNullEntity ||
      current_manipulator_ == kNumManipulators) {
    return kNoIndicatorSelected;
  }
  auto* input_processor = registry_->Get<InputProcessor>();
  auto* input_focus =
      input_processor->GetInputFocus(input_processor->GetPrimaryDevice());
  const Ray& focus_collision_ray = input_focus->collision_ray;
  float closest_distance = std::numeric_limits<float>::max();  // max value
  size_t indicator_index = kNoIndicatorSelected;
  for (size_t i = 0;
       i < manipulators_[current_manipulator_]->GetNumIndicators(); ++i) {
    const float current_distance =
        manipulators_[current_manipulator_]->CheckRayCollidingIndicator(
            focus_collision_ray, i);
    if (current_distance != kNoHitDistance &&
        current_distance < closest_distance) {
      if (distance) {
        *distance = current_distance;
      }
      indicator_index = i;
      closest_distance = current_distance;
    }
  }
  return indicator_index;
}

void ManipulatorManager::DisableIndicators() {
  current_manipulator_ = kNumManipulators;
  selected_indicator_ = kNoIndicatorSelected;
}

void ManipulatorManager::EnableIndicators(Type manipulator, Entity entity) {
  // Setup the indicators proper positions relative to |entity|.
  manipulators_[manipulator]->SetControlMode(control_mode_);
  manipulators_[manipulator]->SetupIndicators(entity);
  current_manipulator_ = manipulator;
  selected_indicator_ = kNoIndicatorSelected;

  // Move dummy to the entity.
  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::vec3 entity_pos =
      transform_system->GetWorldFromEntityMatrix(entity)->TranslationVector3D();
  transform_system->SetLocalTranslation(dummy_entity_, entity_pos);
}

void ManipulatorManager::OnPress(const EventWrapper& event) {
  Entity target = event.GetValueWithDefault(kTargetHash, kNullEntity);
  size_t indicator_index = CheckRayCollidingManipulatorIndicator(nullptr);

  if (indicator_index != kNoIndicatorSelected) {
    auto* transform_system = registry_->Get<TransformSystem>();
    original_sqt_ = *transform_system->GetSqt(selected_entity_);
    // Create a plane of movement for the cursor if it's a different indicator
    // that was selected in the worldspace.
    auto* input_processor = registry_->Get<InputProcessor>();
    auto* input_focus =
        input_processor->GetInputFocus(input_processor->GetPrimaryDevice());
    selected_entity_input_data_.origin_location = input_focus->cursor_position;
    selected_entity_input_data_.plane_normal =
        manipulators_[current_manipulator_]->GetMovementPlaneNormal(
            indicator_index);
    selected_indicator_ = indicator_index;

    // Calculate the initial offset to prevent the entity from jumping.
    const Ray& focus_collision_ray = input_focus->collision_ray;
    const Plane plane(selected_entity_input_data_.origin_location,
                      selected_entity_input_data_.plane_normal);
    ComputeRayPlaneCollision(focus_collision_ray, plane,
                             &selected_entity_input_data_.press_location);
    UpdateDummyPosition();
  } else {
    // If it is not an indicator that was selected, then run it though if an
    // appropriate entity is selected.
    SelectEntity(target);
  }
}

void ManipulatorManager::OnRelease(const EventWrapper& event) {
  if (selected_indicator_ == kNoIndicatorSelected) {
    return;
  }
  manipulators_[current_manipulator_]->ResetIndicators();
  auto* transform_system = registry_->Get<TransformSystem>();
  // Update the newest state of the entity.
  original_sqt_ = *transform_system->GetSqt(selected_entity_);
  manipulators_[current_manipulator_]->UpdateIndicatorsTransform(
      *transform_system->GetWorldFromEntityMatrix(selected_entity_));
  UpdateDummyPosition();
  selected_indicator_ = kNoIndicatorSelected;
}

void ManipulatorManager::SelectEntity(Entity entity) {
  if (entity == kNullEntity) {
    // If the user presses nothing, then deselect the entity.
    selected_entity_ = kNullEntity;
    DisableIndicators();
  } else if (entity != selected_entity_ && entity != dummy_entity_) {
    // Any other entity will be selected with Translation manipulators by
    // default.
    DisableIndicators();
    EnableIndicators(kTranslation, entity);
    auto* transform_system = registry_->Get<TransformSystem>();
    original_sqt_ = *transform_system->GetSqt(entity);
    selected_entity_ = entity;
    UpdateDummyPosition();
  }
}

void ManipulatorManager::AdvanceFrame(Clock::duration delta_time) {
  if (selected_entity_ == kNullEntity) {
    return;
  }
  // Check if there is a collision with a manipulator, if there is, then
  // override the current collision while the primary button is not pressed.
  auto* input_processor = registry_->Get<InputProcessor>();
  auto* input_focus =
      input_processor->GetInputFocus(input_processor->GetPrimaryDevice());
  const Ray& focus_collision_ray = input_focus->collision_ray;
  if (selected_indicator_ == kNoIndicatorSelected) {
    // If the user is not dragging, then find what manipular indicator
    // the cursor is colliding with, if any and override any other collisions.
    float closest_distance = std::numeric_limits<float>::max();
    size_t indicator_index =
        CheckRayCollidingManipulatorIndicator(&closest_distance);
    auto* standard_input_pipeline = registry_->Get<StandardInputPipeline>();
    if (indicator_index != kNoIndicatorSelected) {
      standard_input_pipeline->StartManualCollision(dummy_entity_,
                                                    closest_distance);
    } else {
      standard_input_pipeline->StopManualCollision();
    }
  } else {
    // Update the entity based on movement if the user is
    // currently pressing an indicator.
    mathfu::vec3 grab_pos = mathfu::kZeros3f;
    const Plane plane(selected_entity_input_data_.origin_location,
                      selected_entity_input_data_.plane_normal);
    if (ComputeRayPlaneCollision(focus_collision_ray, plane, &grab_pos)) {
      manipulators_[current_manipulator_]->ApplyManipulator(
          selected_entity_, selected_entity_input_data_.press_location,
          grab_pos, selected_indicator_);
      selected_entity_input_data_.press_location = grab_pos;
    }
  }
  // Update the indicators' transforms.
  auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::mat4* entity_world_from_object =
      transform_system->GetWorldFromEntityMatrix(selected_entity_);
  manipulators_[current_manipulator_]->UpdateIndicatorsTransform(
      *entity_world_from_object);
  UpdateDummyPosition();
}

Entity ManipulatorManager::GetSelectedEntity() const {
  return selected_entity_;
}

void ManipulatorManager::UpdateDummyPosition() {
  if (selected_entity_ == kNullEntity ||
      selected_indicator_ == kNoIndicatorSelected) {
    return;
  }
  auto* transform_system = registry_->Get<TransformSystem>();
  // Update the dummy entity's position to that of the selected indicator
  auto& manipulator = manipulators_[current_manipulator_];
  const mathfu::vec3 dummy_pos =
      manipulator->GetDummyPosition(selected_indicator_);
  transform_system->SetLocalTranslation(dummy_entity_, dummy_pos);
}

void ManipulatorManager::Render(Span<RenderView> views) {
  if (current_manipulator_ == kNumManipulators) {
    return;
  }
  manipulators_[current_manipulator_]->Render(views);
}
}  // namespace lull
