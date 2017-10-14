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

#ifndef LULLABY_SYSTEMS_SCROLL_SCROLL_SNAP_TO_GRID_SYSTEM_H_
#define LULLABY_SYSTEMS_SCROLL_SCROLL_SNAP_TO_GRID_SYSTEM_H_

#include <unordered_map>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"

namespace lull {

// Extends the ScrollSystem such that scrolling will be automatically snapped
// to a grid.
class ScrollSnapToGridSystem : public System {
 public:
  explicit ScrollSnapToGridSystem(Registry* registry);

  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  // Deregisters the grid interval data.
  void Destroy(Entity entity) override;

  // Returns the grid interval.
  mathfu::vec2 GetGridInterval(Entity entity) const;

  // Flings that move the snap position.
  static const float kFlingMultiplier;

 private:
  std::unordered_map<Entity, mathfu::vec2> grid_interval_map_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScrollSnapToGridSystem);

#endif  // LULLABY_SYSTEMS_SCROLL_SCROLL_SNAP_TO_GRID_SYSTEM_H_
