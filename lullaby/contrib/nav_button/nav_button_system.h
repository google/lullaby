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

#ifndef LULLABY_CONTRIB_NAV_BUTTON_NAV_BUTTON_SYSTEM_H_
#define LULLABY_CONTRIB_NAV_BUTTON_NAV_BUTTON_SYSTEM_H_

#include "lullaby/events/input_events.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/render/texture.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// The NavButtonSystem supports easy creation of the standard design for
// navigation buttons.
class NavButtonSystem : public System {
 public:
  explicit NavButtonSystem(Registry* registry);

  void Create(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;
  Entity GetLabelEntity(Entity entity);

 private:
  struct NavButton : Component {
    explicit NavButton(Entity entity) : Component(entity) {}
    Entity icon = kNullEntity;
    Entity label = kNullEntity;
    Entity background = kNullEntity;
    mathfu::vec4 label_color = mathfu::kOnes4f;
    mathfu::vec4 label_hover_color = mathfu::kOnes4f;
    mathfu::vec4 background_color = mathfu::kOnes4f;
    mathfu::vec4 background_hover_color = mathfu::kOnes4f;
    mathfu::vec4 icon_color = mathfu::kOnes4f;
    mathfu::vec4 icon_hover_color = mathfu::kOnes4f;
    std::chrono::milliseconds start_hover_duration;
    std::chrono::milliseconds stop_hover_duration;
  };

  // Animate the various children Entities towards the viewer.
  void OnStartHover(const StartHoverEvent& event);

  // Animate the various children Entities back to their original positions.
  void OnStopHover(const StopHoverEvent& event);

  ComponentPool<NavButton> buttons_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::NavButtonSystem);

#endif  // LULLABY_CONTRIB_NAV_BUTTON_NAV_BUTTON_SYSTEM_H_
