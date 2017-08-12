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

#include "lullaby/systems/scroll/scroll_snap_to_grandchildren_system.h"

#include <limits>

#include "lullaby/events/scroll_events.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/util/time.h"

namespace lull {
namespace {

constexpr int kPoolSize = 4;
const HashValue kScrollSnapToGrandchildrenDefHash =
    Hash("ScrollSnapToGrandchildrenDef");

mathfu::vec2 SnapPositionClosestEntity(
    const mathfu::vec2& position, InputManager::GestureDirection gesture,
    const Aabb& content_bounds, ScrollSystem::SnapCallType type,
    float fling_multiplier, TransformSystem* transform_system, Entity scroll,
    Entity* snapped_grandchild) {
  // If the entity has no children, return the original input position.
  auto* children = transform_system->GetChildren(scroll);
  if (!children) {
    return position;
  }

  // Adjust the target position based on the fling gesture.
  mathfu::vec2 target = position;
  switch (gesture) {
    case InputManager::GestureDirection::kRight:
      target += mathfu::vec2(-fling_multiplier, 0.f);
      break;
    case InputManager::GestureDirection::kLeft:
      target += mathfu::vec2(fling_multiplier, 0.f);
      break;
    case InputManager::GestureDirection::kUp:
      target += mathfu::vec2(0.f, -fling_multiplier);
      break;
    case InputManager::GestureDirection::kDown:
      target += mathfu::vec2(0.f, fling_multiplier);
      break;
    case InputManager::GestureDirection::kNone:
      break;
  }

  // Find the grandchild that is closest to the target position.
  mathfu::vec2 result = target;
  float dist_sq = std::numeric_limits<float>::max();

  for (const Entity child : *children) {
    auto* grand_children = transform_system->GetChildren(child);
    if (!grand_children) {
      continue;
    }

    for (const Entity grandchild : *grand_children) {
      const Sqt* sqt = transform_system->GetSqt(grandchild);
      if (!sqt) {
        continue;
      }

      const mathfu::vec2 diff = target - sqt->translation.xy();
      const float test_dist_sq = diff.LengthSquared();
      if (test_dist_sq < dist_sq) {
        *snapped_grandchild = grandchild;
        result = sqt->translation.xy();
        dist_sq = test_dist_sq;
      }
    }
  }
  return result;
}

}  // namespace

ScrollSnapToGrandchildrenSystem::ScrollSnapToGrandchildrenSystem(
    Registry* registry)
    : System(registry), last_snapped_(kPoolSize) {
  RegisterDef(this, kScrollSnapToGrandchildrenDefHash);
  RegisterDependency<ScrollSystem>(this);
}

void ScrollSnapToGrandchildrenSystem::PostCreateInit(Entity entity,
                                                     HashValue type,
                                                     const Def* def) {
  if (type != kScrollSnapToGrandchildrenDefHash) {
    LOG(DFATAL) << "Unsupported type: " << type;
    return;
  }

  last_snapped_.Emplace(entity);
  auto* scroll_system = registry_->Get<ScrollSystem>();
  const float fling_multiplier =
      ConvertDef<ScrollSnapToGrandchildrenDef>(def)->fling_multiplier();
  scroll_system->SetSnapOffsetFn(
      entity,
      [this, fling_multiplier, entity](
          const mathfu::vec2& position, InputManager::GestureDirection gesture,
          const Aabb& bounds, ScrollSystem::SnapCallType type) -> mathfu::vec2 {
        return SnapOffset(entity, fling_multiplier, position, gesture, bounds,
                          type);
      });

  scroll_system->SetSnapByDeltaFn(
      entity, [this, entity](int delta) -> Optional<mathfu::vec2> {
        return SnapByDelta(entity, delta);
      });
}

mathfu::vec2 ScrollSnapToGrandchildrenSystem::SnapOffset(
    Entity entity, const float fling_multiplier, const mathfu::vec2& position,
    InputManager::GestureDirection gesture, const Aabb& bounds,
    ScrollSystem::SnapCallType type) {
  LastSnapped* last_snapped = last_snapped_.Get(entity);
  if (last_snapped == nullptr) {
    return position;
  }

  last_snapped->grandchild = kNullEntity;
  auto* transform_system = registry_->Get<TransformSystem>();
  mathfu::vec2 snapped_offset = SnapPositionClosestEntity(
      position, gesture, bounds, type, fling_multiplier, transform_system,
      entity, &last_snapped->grandchild);

  ScrollSnappedToEntity event(entity, last_snapped->grandchild);
  SendEvent(registry_, entity, event);
  return snapped_offset;
}

Optional<mathfu::vec2> ScrollSnapToGrandchildrenSystem::SnapByDelta(
    Entity entity, int delta) const {
  const Entity current_entity = GetLastSnappedGrandchild(entity);
  if (current_entity == kNullEntity) {
    return Optional<mathfu::vec2>();
  }
  const auto* transform_system = registry_->Get<TransformSystem>();
  const std::vector<lull::Entity>* children =
      transform_system->GetChildren(entity);
  if (children == nullptr || children->size() != 1) {
    return Optional<mathfu::vec2>();
  }
  const std::vector<Entity>* grandchildren =
      transform_system->GetChildren(children->front());
  if (grandchildren == nullptr || grandchildren->size() < 2) {
    return Optional<mathfu::vec2>();
  }

  int current_index = -1;
  for (size_t i = 0; i < grandchildren->size(); ++i) {
    if (current_entity == (*grandchildren)[i]) {
      current_index = static_cast<int>(i);
      break;
    }
  }

  if (current_index == -1) {
    LOG(ERROR) << "Failed to find snapped grandchild";
    return Optional<mathfu::vec2>();
  }

  const int new_index = mathfu::Clamp(
      current_index + delta, 0, static_cast<int>(grandchildren->size() - 1));
  return Optional<mathfu::vec2>(
      transform_system->GetSqt((*grandchildren)[new_index])->translation.xy());
}

void ScrollSnapToGrandchildrenSystem::Destroy(Entity entity) {
  last_snapped_.Destroy(entity);
}

Entity ScrollSnapToGrandchildrenSystem::GetLastSnappedGrandchild(
    Entity scroll) const {
  const LastSnapped* last_snapped = last_snapped_.Get(scroll);
  return last_snapped != nullptr ? last_snapped->grandchild : kNullEntity;
}

}  // namespace lull
