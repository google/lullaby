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

#include "lullaby/systems/scroll/scroll_snap_to_grid_system.h"

#include "lullaby/generated/scroll_def_generated.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/scroll/scroll_system.h"
#include "lullaby/util/math.h"

namespace lull {
namespace {

const HashValue kScrollSnapToGridDefHash = Hash("ScrollSnapToGridDef");

mathfu::vec2 SnapPositionToGrid(const mathfu::vec2& position,
                                InputManager::GestureDirection gesture,
                                const Aabb& content_bounds,
                                ScrollSystem::SnapCallType type,
                                const mathfu::vec2& grid_interval) {
  // Determine the additional movement that should be added as a result of
  // a fling gesture.
  mathfu::vec2 delta = mathfu::kZeros2f;
  switch (gesture) {
    case InputManager::GestureDirection::kRight:
      delta = mathfu::vec2(-ScrollSnapToGridSystem::kFlingMultiplier, 0.f) *
              grid_interval;
      break;
    case InputManager::GestureDirection::kLeft:
      delta = mathfu::vec2(ScrollSnapToGridSystem::kFlingMultiplier, 0.f) *
              grid_interval;
      break;
    case InputManager::GestureDirection::kUp:
      delta = mathfu::vec2(0.f, ScrollSnapToGridSystem::kFlingMultiplier) *
              grid_interval;
      break;
    case InputManager::GestureDirection::kDown:
      delta = mathfu::vec2(0.f, -ScrollSnapToGridSystem::kFlingMultiplier) *
              grid_interval;
      break;
    case InputManager::GestureDirection::kNone:
      break;
  }

  const mathfu::vec2 pos = position + delta;
  const mathfu::vec2 cell_pos = pos / grid_interval;
  const mathfu::vec2 min = content_bounds.min.xy();
  const mathfu::vec2 max = content_bounds.max.xy();
  const mathfu::vec2 content_size = max - min;
  const mathfu::vec2 grid_size = content_size / grid_interval;
  mathfu::vec2 grid_max(std::floor(grid_size.x), std::floor(grid_size.y));
  if (type == ScrollSystem::SnapCallType::kSetBounds) {
    grid_max -= mathfu::kOnes2f;
  }
  const float grid_x = mathfu::Clamp(std::round(cell_pos.x), 0.0f, grid_max.x);
  const float grid_y = mathfu::Clamp(std::round(cell_pos.y), 0.0f, grid_max.y);
  const mathfu::vec2 result =
      mathfu::vec2(grid_x * grid_interval.x, grid_y * grid_interval.y);
  return min + result;
}
}  // namespace

// Flings should move the snap position by just over half the grid interval.
const float ScrollSnapToGridSystem::kFlingMultiplier = .51f;

ScrollSnapToGridSystem::ScrollSnapToGridSystem(Registry* registry)
    : System(registry) {
  RegisterDef(this, kScrollSnapToGridDefHash);
  RegisterDependency<ScrollSystem>(this);
}

void ScrollSnapToGridSystem::PostCreateInit(Entity entity, HashValue type,
                                            const Def* def) {
  if (type != kScrollSnapToGridDefHash) {
    LOG(DFATAL)
        << "Invalid type passed to Create. Expecting ScrollSnapToGridDef!";
    return;
  }
  const auto* data = ConvertDef<ScrollSnapToGridDef>(def);

  if (data->interval()) {
    mathfu::vec2 grid_interval;
    MathfuVec2FromFbVec2(data->interval(), &grid_interval);

    auto fn = [grid_interval](
        const mathfu::vec2& pos, InputManager::GestureDirection gesture,
        const Aabb& bounds, ScrollSystem::SnapCallType type) -> mathfu::vec2 {
      return SnapPositionToGrid(pos, gesture, bounds, type, grid_interval);
    };
    auto* scroll_system = registry_->Get<ScrollSystem>();
    scroll_system->SetSnapOffsetFn(entity, fn);
    grid_interval_map_[entity] = grid_interval;
  }
}

void ScrollSnapToGridSystem::Destroy(Entity entity) {
  grid_interval_map_.erase(entity);
}

mathfu::vec2 ScrollSnapToGridSystem::GetGridInterval(Entity entity) const {
  const auto entity_grid_interval = grid_interval_map_.find(entity);
  return entity_grid_interval != grid_interval_map_.end()
             ? entity_grid_interval->second
             : mathfu::kZeros2f;
}

}  // namespace lull
