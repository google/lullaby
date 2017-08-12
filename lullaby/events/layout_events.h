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

#ifndef LULLABY_EVENTS_LAYOUT_EVENTS_H_
#define LULLABY_EVENTS_LAYOUT_EVENTS_H_

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Event which is triggered when the layout of an entity is updated (i.e., the
// LayoutSystem has moved its children into position).
struct LayoutChangedEvent {
  LayoutChangedEvent() {}
  explicit LayoutChangedEvent(Entity e) : target(e) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
  }

  Entity target = kNullEntity;
};

// Used by the LayoutBoxSystem to notify when the original box is changed.
// See LayoutBoxSystem documentation for more details on how this event should
// be used.
struct OriginalBoxChangedEvent {
  OriginalBoxChangedEvent() {}
  explicit OriginalBoxChangedEvent(Entity e) : target(e) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
  }

  Entity target = kNullEntity;
};

// Used by the LayoutBoxSystem to notify when the desired size is changed.
// See LayoutBoxSystem documentation for more details on how this event should
// be used.
struct DesiredSizeChangedEvent {
  DesiredSizeChangedEvent() {}
  explicit DesiredSizeChangedEvent(Entity e, Entity source,
                                   const Optional<float>& x,
                                   const Optional<float>& y,
                                   const Optional<float>& z)
      : target(e), source(source), x(x), y(y), z(z) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&source, Hash("source"));
    archive(&x, Hash("x"));
    archive(&y, Hash("y"));
    archive(&z, Hash("z"));
  }

  Entity target = kNullEntity;
  Entity source = kNullEntity;
  Optional<float> x;
  Optional<float> y;
  Optional<float> z;
};

// Used by the LayoutBoxSystem to notify when the actual box is changed.
// See LayoutBoxSystem documentation for more details on how this event should
// be used.
struct ActualBoxChangedEvent {
  ActualBoxChangedEvent() {}
  explicit ActualBoxChangedEvent(Entity e, Entity source)
      : target(e), source(source) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&source, Hash("source"));
  }

  Entity target = kNullEntity;
  Entity source = kNullEntity;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::LayoutChangedEvent);
LULLABY_SETUP_TYPEID(lull::OriginalBoxChangedEvent);
LULLABY_SETUP_TYPEID(lull::DesiredSizeChangedEvent);
LULLABY_SETUP_TYPEID(lull::ActualBoxChangedEvent);

#endif  // LULLABY_EVENTS_LAYOUT_EVENTS_H_
