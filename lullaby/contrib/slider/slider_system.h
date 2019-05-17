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

#ifndef LULLABY_CONTRIB_SLIDER_SLIDER_SYSTEM_H_
#define LULLABY_CONTRIB_SLIDER_SLIDER_SYSTEM_H_

#include <unordered_set>

#include "lullaby/generated/slider_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"

namespace lull {

struct SliderChangedEvent {
  SliderChangedEvent() {}
  SliderChangedEvent(Entity e, float previous_value, float next_value)
      : target(e), old_value(previous_value), new_value(next_value) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, ConstHash("target"));
    archive(&old_value, ConstHash("old_value"));
    archive(&new_value, ConstHash("new_value"));
  }

  Entity target = kNullEntity;
  float old_value = 0.0f;
  float new_value = 0.0f;
};

/// The SliderSystem provides Entities that represent a slider with an optional
/// knob that can be dragged. The slider represents a range specified in the Def
/// file.  The system can then report the value or obtain the percentage to the
/// end position the slider currently represents.
class SliderSystem : public System {
 public:
  explicit SliderSystem(Registry* registry);

  /// Adds a slider component to the Entity using the specified ComponentDef.
  void Create(Entity entity, HashValue type, const Def* def) override;

  /// Removes the slider from the Entity.
  void Destroy(Entity entity) override;

  /// Processes touch input to control scrolling.
  void AdvanceFrame(const Clock::duration& delta_time);
  /// Get the value in terms of the min_position and max_position of the slider
  /// entity. Method returns value if entity is a slider.
  Optional<float> GetValue(Entity entity) const;

  /// Get the percentage from start to end that the value is. The out parameter
  /// will be from 0.0f to 1.0f. Method returns value if the entity is a slider.
  Optional<float> GetValuePercentage(Entity entity) const;

  /// Set the value for the slider. The new value is used in comparison to the
  /// min_position and max_position from the Def.
  void SetValue(Entity entity, float value);

  /// Set the value based on the percentage from the value range start and end.
  void SetValuePercentage(Entity entity, float percentage);

  /// Retrieve the value range previously specified by the def file. Method
  /// returns value if entity is a slider.
  Optional<mathfu::vec2> GetValueRange(Entity entity) const;

 private:
  struct SliderComponent : Component {
    explicit SliderComponent(Entity entity) : Component(entity) {}

    /// The knob is the floating entity that is optional and is used to grab
    /// with the reticle and adjust the value of the slider.
    Entity knob = kNullEntity;
    Entity gutter = kNullEntity;
    std::string gutter_uniform_name;
    mathfu::vec3 min_position = mathfu::kZeros3f;
    mathfu::vec3 max_position = mathfu::kZeros3f;
    mathfu::vec2 value_range = mathfu::kZeros2f;
    /// Current value between value_range[0] and value_range[1].
    float current_value = 0.0f;
  };

  void AssignValue(SliderComponent* slider, float value);

  /// Given a percentage from start to end position, what is the corresponding
  /// value.
  static float CalculateValueFromPercentage(const SliderComponent& slider,
                                            float percentage);
  /// Given a value from value_range[0 - 1] return the percentage.
  static float CalculatePercentageFromValue(const SliderComponent& slider,
                                            float value);
  /// Track the entity as the current active slider.
  void StartSliding(Entity entity);
  /// Stop tracking the entity as the current active slider.
  void StopSliding();
  /// If there is an active slider, calculate its position and update its value
  /// relative to the reticle location.
  void UpdateActiveEntity(Entity entity);
  /// Given a slider entity, set its position using the value stored.
  void UpdateKnobSqt(Entity entity);

  ComponentPool<SliderComponent> components_;
  Entity active_entity_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::SliderSystem);
LULLABY_SETUP_TYPEID(lull::SliderChangedEvent);

#endif  // LULLABY_CONTRIB_SLIDER_SLIDER_SYSTEM_H_
