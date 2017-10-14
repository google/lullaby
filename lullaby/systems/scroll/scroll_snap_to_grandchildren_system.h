/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_SYSTEMS_SCROLL_SCROLL_SNAP_TO_GRANDCHILDREN_SYSTEM_H_
#define LULLABY_SYSTEMS_SCROLL_SCROLL_SNAP_TO_GRANDCHILDREN_SYSTEM_H_

#include "lullaby/generated/scroll_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/entity.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/scroll/scroll_system.h"

namespace lull {

// Extends the ScrollSystem such that scrolling will be automatically snapped
// to the position of children elements of the scroll content.
class ScrollSnapToGrandchildrenSystem : public System {
 public:
  explicit ScrollSnapToGrandchildrenSystem(Registry* registry);

  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  void Destroy(Entity entity) override;

  // Returns the grandchild that was last snapped to for the entity with the
  // scroll component. Returns kNullEntity if one has not been snapped to.
  Entity GetLastSnappedGrandchild(Entity scroll) const;

  // Returns the configured flingmultiplier for the input |scroll| entity.
  float GetFlingMultiplier(Entity scroll) const;

 private:
  struct LastSnapped : Component {
    explicit LastSnapped(Entity scroll) : Component(scroll) {}
    Entity grandchild = kNullEntity;
  };

  // Callback set on ScrollSystem to determine how the offset is set
  mathfu::vec2 SnapOffset(Entity entity, const float fling_multiplier,
                          const mathfu::vec2& position,
                          InputManager::GestureDirection gesture,
                          const Aabb& bounds, ScrollSystem::SnapCallType type);
  // Callback set on ScrollSystem to handle snapping by a specific delta.
  // Returns an Optional set with the expected xy translation needed to snap
  // the scroll view by |delta| grandchildren if the entity is valid,
  // unset otherwise
  Optional<mathfu::vec2> SnapByDelta(Entity entity, int delta) const;

  ComponentPool<LastSnapped> last_snapped_;
  std::unordered_map<Entity, float> fling_multiplier_map_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScrollSnapToGrandchildrenSystem);

#endif  // LULLABY_SYSTEMS_SCROLL_SCROLL_SNAP_TO_GRANDCHILDREN_SYSTEM_H_
