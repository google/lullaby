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

#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "lullaby/base/entity.h"
#include "lullaby/base/typeid.h"

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

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScrollViewTargeted);
LULLABY_SETUP_TYPEID(lull::ScrollOffsetChanged);
LULLABY_SETUP_TYPEID(lull::ScrollVisibilityChanged);
LULLABY_SETUP_TYPEID(lull::ScrollSnappedToEntity);
LULLABY_SETUP_TYPEID(lull::ScrollSnapByDelta);

#endif  // LULLABY_EVENTS_SCROLL_EVENTS_H_
