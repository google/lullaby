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

#include "lullaby/contrib/slider/slider_system.h"

#include <utility>

#include "lullaby/events/input_events.h"
#include "lullaby/modules/animation_channels/transform_channels.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/input_processor/input_processor.h"
#include "lullaby/systems/collision/collision_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/util/time.h"
#include "lullaby/util/trace.h"

namespace lull {
namespace {

constexpr const char* kGutterUniform = "slider_value";

const HashValue kSliderDefHash = ConstHash("SliderDef");

}  // namespace

SliderSystem::SliderSystem(Registry* registry)
    : System(registry), components_(8) {
  RegisterDef<SliderDefT>(this);
  RegisterDependency<DispatcherSystem>(this);
  RegisterDependency<RenderSystem>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<InputProcessor>(this);
}

void SliderSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type != kSliderDefHash) {
    LOG(DFATAL) << "Invalid def.  Expecting SliderDef.";
    return;
  }

  SliderComponent* slider = components_.Emplace(entity);
  if (slider == nullptr) {
    LOG(DFATAL) << "Could not create slider for entity " << entity;
    return;
  }

  if (def == nullptr) {
    LOG(DFATAL) << "Create Def file invalid.";
    return;
  }

  auto* data = ConvertDef<SliderDef>(def);
  if (data == nullptr) {
    LOG(DFATAL) << "Failed to convert to slider.";
    return;
  }

  MathfuVec3FromFbVec3(data->min_position(), &(slider->min_position));
  MathfuVec3FromFbVec3(data->max_position(), &(slider->max_position));
  MathfuVec2FromFbVec2(data->value_range(), &(slider->value_range));
  slider->current_value = mathfu::Clamp(
      data->default_value(), slider->value_range[0], slider->value_range[1]);

  auto* transform_system = registry_->Get<TransformSystem>();
  if (data->gutter_blueprint() != nullptr) {
    slider->gutter = transform_system->CreateChild(
        entity, data->gutter_blueprint()->c_str());
  }
  if (data->gutter_uniform_name() != nullptr) {
    slider->gutter_uniform_name = data->gutter_uniform_name()->c_str();
  }

  // Create knob object from slider->handle.
  if (data->knob_blueprint() != nullptr) {
    slider->knob =
        transform_system->CreateChild(entity, data->knob_blueprint()->c_str());
    UpdateKnobSqt(entity);
  }

  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Connect(entity, this, [this](const ClickEvent& evt) {
    StartSliding(evt.target);
  });
  dispatcher_system->Connect(
      entity, this, [this](const ClickReleasedEvent& evt) { StopSliding(); });
}

void SliderSystem::Destroy(Entity entity) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  dispatcher_system->Disconnect<StartHoverEvent>(entity, this);
  dispatcher_system->Disconnect<StopHoverEvent>(entity, this);
  components_.Destroy(entity);
}

void SliderSystem::AdvanceFrame(const Clock::duration& delta_time) {
  LULLABY_CPU_TRACE_CALL();
  if (active_entity_ != kNullEntity) {
    // While pressed, move the slider to the closest point on the line.
    UpdateActiveEntity(active_entity_);
    active_entity_ = kNullEntity;
  }
}

Optional<float> SliderSystem::GetValue(Entity entity) const {
  const SliderComponent* slider = components_.Get(entity);
  if (slider == nullptr) {
    return Optional<float>();
  }
  return slider->current_value;
}

Optional<float> SliderSystem::GetValuePercentage(Entity entity) const {
  const SliderComponent* slider = components_.Get(entity);
  if (slider == nullptr) {
    return Optional<float>();
  }
  return CalculatePercentageFromValue(*slider, slider->current_value);
}

void SliderSystem::SetValue(Entity entity, float value) {
  SliderComponent* slider = components_.Get(entity);
  if (slider == nullptr) {
    return;
  }
  AssignValue(slider, value);
  UpdateKnobSqt(entity);
}

void SliderSystem::SetValuePercentage(Entity entity, float percentage) {
  SliderComponent* slider = components_.Get(entity);
  if (slider == nullptr) {
    return;
  }
  AssignValue(slider, CalculateValueFromPercentage(*slider, percentage));
  UpdateKnobSqt(entity);
}

Optional<mathfu::vec2> SliderSystem::GetValueRange(Entity entity) const {
  const SliderComponent* slider = components_.Get(entity);
  if (slider == nullptr) {
    return Optional<mathfu::vec2>();
  }
  return slider->value_range;
}

void SliderSystem::AssignValue(SliderComponent* slider, float value) {
  const float old_value = slider->current_value;
  slider->current_value = value;

  if (!slider->gutter_uniform_name.empty()) {
    auto* render_system = registry_->Get<RenderSystem>();
    render_system->SetUniform(slider->gutter,
                              slider->gutter_uniform_name.c_str(),
                              &(slider->current_value), 1, 1);
  }

  SliderChangedEvent event(slider->GetEntity(), old_value, value);
  SendEvent(registry_, slider->GetEntity(), event);
}

float SliderSystem::CalculateValueFromPercentage(const SliderComponent& slider,
                                                 float percentage) {
  return slider.value_range[0] +
         percentage * (slider.value_range[1] - slider.value_range[0]);
}

float SliderSystem::CalculatePercentageFromValue(const SliderComponent& slider,
                                                 float value) {
  return (value - slider.value_range[0]) /
         (slider.value_range[1] - slider.value_range[0]);
}

void SliderSystem::StartSliding(Entity entity) {
  const SliderComponent* slider = components_.Get(entity);
  if (slider == nullptr) {
    return;
  }
  active_entity_ = entity;
}

void SliderSystem::StopSliding() { active_entity_ = kNullEntity; }

void SliderSystem::UpdateActiveEntity(Entity entity) {
  SliderComponent* slider = components_.Get(active_entity_);
  if (slider == nullptr) {
    return;
  }
  const auto* input_processor = registry_->Get<InputProcessor>();
  const auto* transform_system = registry_->Get<TransformSystem>();
  const mathfu::mat4* entity_from_world_mat =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (!entity_from_world_mat) {
    return;
  }

  const InputFocus* focus =
      input_processor->GetInputFocus(input_processor->GetPrimaryDevice());
  if (!focus) {
    return;
  }

  // While pressed, move the knob to the closest point on the line.
  const mathfu::vec3 local_hit_pos =
      entity_from_world_mat->Inverse() * focus->cursor_position;

  const float percentage_along_line = GetPercentageOfLineClosestToPoint(
      slider->min_position, slider->max_position, local_hit_pos);
  const float clamped_percentage =
      mathfu::Clamp(percentage_along_line, 0.0f, 1.0f);
  SetValuePercentage(entity, clamped_percentage);
}

void SliderSystem::UpdateKnobSqt(Entity entity) {
  SliderComponent* slider = components_.Get(entity);
  if (slider == nullptr || slider->knob == kNullEntity) {
    return;
  }
  const float percentage_along_line =
      CalculatePercentageFromValue(*slider, slider->current_value);
  const mathfu::vec3 true_target =
      slider->min_position +
      (percentage_along_line * (slider->max_position - slider->min_position));

  auto* transform_system = registry_->Get<TransformSystem>();
  const Sqt* sqt = transform_system->GetSqt(slider->knob);
  if (sqt != nullptr) {
    Sqt new_sqt = *sqt;
    new_sqt.translation = true_target;
    transform_system->SetSqt(slider->knob, new_sqt);
  }
}

}  // namespace lull
