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

#ifndef LULLABY_EVENTS_RENDER_EVENTS_H_
#define LULLABY_EVENTS_RENDER_EVENTS_H_

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Dispatched when a texture that is being loaded asynchronously is finished.
struct TextureReadyEvent {
  TextureReadyEvent() {}
  TextureReadyEvent(Entity e, int texture_unit)
      : target(e), texture_unit(texture_unit) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, Hash("target"));
    archive(&texture_unit, Hash("texture_unit"));
  }

  Entity target = kNullEntity;
  int texture_unit = 0;
};

// Dispatched when all assets for an entity are finished loading.
struct ReadyToRenderEvent {
  ReadyToRenderEvent() {}
  explicit ReadyToRenderEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

// Dispatched when an entity is hidden (via RenderSystem::Hide).
struct HiddenEvent {
  HiddenEvent() {}
  explicit HiddenEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

// Dispatched when an entity is unhidden (via RenderSystem::Show).
struct UnhiddenEvent {
  UnhiddenEvent() {}
  explicit UnhiddenEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

// Invokes RenderSystem::Hide(entity)
struct HideEvent {
  HideEvent() {}
  explicit HideEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

// Invokes RenderSystem::Show(entity)
struct ShowEvent {
  ShowEvent() {}
  explicit ShowEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("entity"));
  }

  Entity entity = kNullEntity;
};

// Dispatched when an entity's mesh had been changed.
struct MeshChangedEvent {
  MeshChangedEvent() {}
  MeshChangedEvent(Entity entity, HashValue component_id)
      : entity(entity), component_id(component_id) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, Hash("target"));
    archive(&component_id, Hash("component_id"));
  }

  Entity entity = kNullEntity;
  HashValue component_id = 0;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ReadyToRenderEvent);
LULLABY_SETUP_TYPEID(lull::TextureReadyEvent);
LULLABY_SETUP_TYPEID(lull::HiddenEvent);
LULLABY_SETUP_TYPEID(lull::UnhiddenEvent);
LULLABY_SETUP_TYPEID(lull::HideEvent);
LULLABY_SETUP_TYPEID(lull::ShowEvent);
LULLABY_SETUP_TYPEID(lull::MeshChangedEvent);

#endif  // LULLABY_EVENTS_RENDER_EVENTS_H_
