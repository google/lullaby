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

#ifndef LULLABY_EVENTS_ENTITY_EVENTS_H_
#define LULLABY_EVENTS_ENTITY_EVENTS_H_

#include "lullaby/base/entity.h"
#include "lullaby/base/typeid.h"

namespace lull {

// Invokes TransformSystem::Enable(entity)
struct EnableEvent {
  EnableEvent() {}
  explicit EnableEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

// Invokes TransformSystem::Disable(entity)
struct DisableEvent {
  DisableEvent() {}
  explicit DisableEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

struct OnDisabledEvent {
  OnDisabledEvent() {}
  explicit OnDisabledEvent(Entity entity) : target(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
  }

  Entity target = kNullEntity;
};

struct OnEnabledEvent {
  OnEnabledEvent() {}
  explicit OnEnabledEvent(Entity entity) : target(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
  }

  Entity target = kNullEntity;
};

struct ParentChangedEvent {
  ParentChangedEvent() {}
  ParentChangedEvent(Entity entity, Entity old_parent, Entity new_parent)
      : target(entity), old_parent(old_parent), new_parent(new_parent) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&old_parent, Hash("old_parent"));
    archive(&new_parent, Hash("new_parent"));
  }

  Entity target = kNullEntity;
  Entity old_parent = kNullEntity;
  Entity new_parent = kNullEntity;
};

struct ChildAddedEvent {
  ChildAddedEvent() {}
  ChildAddedEvent(Entity parent, Entity child)
      : target(parent), child(child) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&child, Hash("child"));
  }

  Entity target = kNullEntity;
  Entity child = kNullEntity;
};

struct ChildRemovedEvent {
  ChildRemovedEvent() {}
  ChildRemovedEvent(Entity parent, Entity child)
      : target(parent), child(child) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&child, Hash("child"));
  }

  Entity target = kNullEntity;
  Entity child = kNullEntity;
};

struct AabbChangedEvent {
  AabbChangedEvent() {}
  explicit AabbChangedEvent(Entity entity) : target(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
  }

  Entity target = kNullEntity;
};

struct OnInteractionDisabledEvent {
  OnInteractionDisabledEvent() {}
  explicit OnInteractionDisabledEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

struct OnInteractionEnabledEvent {
  OnInteractionEnabledEvent() {}
  explicit OnInteractionEnabledEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::AabbChangedEvent);
LULLABY_SETUP_TYPEID(lull::ChildAddedEvent);
LULLABY_SETUP_TYPEID(lull::ChildRemovedEvent);
LULLABY_SETUP_TYPEID(lull::DisableEvent);
LULLABY_SETUP_TYPEID(lull::EnableEvent);
LULLABY_SETUP_TYPEID(lull::OnDisabledEvent);
LULLABY_SETUP_TYPEID(lull::OnEnabledEvent);
LULLABY_SETUP_TYPEID(lull::OnInteractionDisabledEvent);
LULLABY_SETUP_TYPEID(lull::OnInteractionEnabledEvent);
LULLABY_SETUP_TYPEID(lull::ParentChangedEvent);

#endif  // LULLABY_EVENTS_ENTITY_EVENTS_H_
