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

#ifndef LULLABY_EVENTS_SCROLL_EVENTS_H_
#define LULLABY_EVENTS_SCROLL_EVENTS_H_

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/util/typeid.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

struct ScrollViewTargeted {
  ScrollViewTargeted() {}
  template <typename Archive>
  void Serialize(Archive archive) {}
};

struct ScrollOffsetChanged {
  ScrollOffsetChanged() {}
  ScrollOffsetChanged(Entity e, const mathfu::vec2& old_value,
                      const mathfu::vec2& new_value)
      : target(e), old_offset(old_value), new_offset(new_value) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&old_offset, Hash("old_offset"));
    archive(&new_offset, Hash("new_offset"));
  }

  Entity target = kNullEntity;
  mathfu::vec2 old_offset = mathfu::kZeros2f;
  mathfu::vec2 new_offset = mathfu::kZeros2f;
};

struct ScrollVisibilityChanged {
  ScrollVisibilityChanged() {}
  ScrollVisibilityChanged(Entity e, Entity scroll_entity, bool is_visible)
      : target(e), scroll_view(scroll_entity), visible(is_visible) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&scroll_view, Hash("scroll_view"));
    archive(&visible, Hash("visible"));
  }

  Entity target = kNullEntity;
  Entity scroll_view = kNullEntity;
  bool visible = false;
};

struct ScrollSnappedToEntity {
  ScrollSnappedToEntity() {}
  ScrollSnappedToEntity(Entity entity, Entity snapped_entity)
      : entity(entity), snapped_entity(snapped_entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
    archive(&snapped_entity, Hash("snapped_entity"));
  }

  Entity entity = kNullEntity;
  Entity snapped_entity = kNullEntity;
};

// Invokes ScrollSystem::Activate(entity)
struct ScrollActivateEvent {
  ScrollActivateEvent() {}
  explicit ScrollActivateEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

// Invokes ScrollSystem::Deactivate(entity)
struct ScrollDeactivateEvent {
  ScrollDeactivateEvent() {}
  explicit ScrollDeactivateEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

// Invokes ScrollSystem::SnapByDelta(entity, delta, time_ms)
struct ScrollSnapByDelta {
  ScrollSnapByDelta() {}
  ScrollSnapByDelta(Entity entity, int delta, float time_ms = -1.f)
      : entity(entity), delta(delta), time_ms(time_ms) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
    archive(&delta, Hash("delta"));
    archive(&time_ms, Hash("time_ms"));
  }

  Entity entity = kNullEntity;
  int delta = 0;
  float time_ms = -1;
};

// Invokes ScrollSystem::SetViewOffset(entity, offset, time_ms)
struct ScrollSetViewOffsetEvent {
  ScrollSetViewOffsetEvent() {}
  ScrollSetViewOffsetEvent(Entity entity, const mathfu::vec2& offset,
                           float time_ms)
      : entity(entity), offset(offset), time_ms(time_ms) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
    archive(&offset, Hash("offset"));
    archive(&time_ms, Hash("time_ms"));
  }

  Entity entity = kNullEntity;
  mathfu::vec2 offset = mathfu::kZeros2f;
  float time_ms = 0;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScrollViewTargeted);
LULLABY_SETUP_TYPEID(lull::ScrollOffsetChanged);
LULLABY_SETUP_TYPEID(lull::ScrollVisibilityChanged);
LULLABY_SETUP_TYPEID(lull::ScrollSnappedToEntity);
LULLABY_SETUP_TYPEID(lull::ScrollActivateEvent);
LULLABY_SETUP_TYPEID(lull::ScrollDeactivateEvent);
LULLABY_SETUP_TYPEID(lull::ScrollSnapByDelta);
LULLABY_SETUP_TYPEID(lull::ScrollSetViewOffsetEvent);

#endif  // LULLABY_EVENTS_SCROLL_EVENTS_H_
