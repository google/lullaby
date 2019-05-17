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

#ifndef LULLABY_EVENTS_RENDER_EVENTS_H_
#define LULLABY_EVENTS_RENDER_EVENTS_H_

#include "lullaby/util/entity.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Dispatched when a texture that is being loaded asynchronously is finished.
struct TextureReadyEvent {
  TextureReadyEvent() {}
  TextureReadyEvent(Entity e, int texture_unit)
      : target(e), texture_unit(texture_unit) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&target, ConstHash("target"));
    archive(&texture_unit, ConstHash("texture_unit"));
  }

  Entity target = kNullEntity;
  int texture_unit = 0;
};

// Dispatched when all assets for an entity are finished loading.
struct ReadyToRenderEvent {
  ReadyToRenderEvent() {}
  explicit ReadyToRenderEvent(Entity entity) : entity(entity) {}
  explicit ReadyToRenderEvent(Entity entity, HashValue pass)
      : entity(entity), pass(pass) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&pass, ConstHash("pass"));
  }

  Entity entity = kNullEntity;
  HashValue pass = 0;
};

// Dispatched when an entity is hidden (via RenderSystem::Hide).
struct HiddenEvent {
  HiddenEvent() {}
  explicit HiddenEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
  }

  Entity entity = kNullEntity;
};

// Dispatched when an entity is unhidden (via RenderSystem::Show).
struct UnhiddenEvent {
  UnhiddenEvent() {}
  explicit UnhiddenEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
  }

  Entity entity = kNullEntity;
};

// Invokes RenderSystem::Hide(entity)
struct HideEvent {
  HideEvent() {}
  explicit HideEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
  }

  Entity entity = kNullEntity;
};

// Invokes RenderSystem::Show(entity)
struct ShowEvent {
  ShowEvent() {}
  explicit ShowEvent(Entity entity) : entity(entity) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
  }

  Entity entity = kNullEntity;
};

// Invokes RenderSystem::SetGroupId(entity, group_id)
struct SetRenderGroupIdEvent {
  SetRenderGroupIdEvent() {}
  explicit SetRenderGroupIdEvent(Entity entity, HashValue group_id)
      : entity(entity), group_id(group_id) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&group_id, ConstHash("group_id"));
  }

  Entity entity = kNullEntity;
  HashValue group_id = 0;
};

// Invokes RenderSystem::SetGroupParams(group_id, {sort_order_offset})
struct SetRenderGroupParamsEvent {
  SetRenderGroupParamsEvent() {}
  explicit SetRenderGroupParamsEvent(HashValue group_id, int sort_order_offset)
      : group_id(group_id), sort_order_offset(sort_order_offset) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&group_id, ConstHash("group_id"));
    archive(&sort_order_offset, ConstHash("sort_order_offset"));
  }

  HashValue group_id = 0;
  int sort_order_offset = 0;
};

/// Dispatched when an entity's mesh had been changed.
struct MeshChangedEvent {
  MeshChangedEvent() {}
  MeshChangedEvent(Entity entity, HashValue pass)
      : entity(entity), pass(pass) {}

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("target"));
    archive(&pass, ConstHash("pass"));
  }

  /// The entity whose mesh was changed.
  Entity entity = kNullEntity;
  /// The pass that the entity's mesh was changed in.
  HashValue pass = 0;
};

/// Sets the native window for the RenderSystem. As of 8/2018, only
/// Filament uses this, which is required to initialize its GL context. Also, it
/// needs to be resent every time a new window is created, for example on
/// Android when Activities are stopped and restarted. The type of native_window
/// depends on the platform:
///   Platform | native_window type
///   :--------|:----------------------------:
///   Android  | ANativeWindow*
///   OSX      | NSView*
///   IOS      | CAEAGLLayer*
///   X11      | Window
///   Windows  | HWND
/// Note: This event is currently not Serializable.
struct SetNativeWindowEvent {
  SetNativeWindowEvent() {}
  SetNativeWindowEvent(void* native_window) : native_window(native_window) {}

  void* native_window = nullptr;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ReadyToRenderEvent);
LULLABY_SETUP_TYPEID(lull::TextureReadyEvent);
LULLABY_SETUP_TYPEID(lull::HiddenEvent);
LULLABY_SETUP_TYPEID(lull::UnhiddenEvent);
LULLABY_SETUP_TYPEID(lull::HideEvent);
LULLABY_SETUP_TYPEID(lull::ShowEvent);
LULLABY_SETUP_TYPEID(lull::SetRenderGroupIdEvent);
LULLABY_SETUP_TYPEID(lull::SetRenderGroupParamsEvent);
LULLABY_SETUP_TYPEID(lull::MeshChangedEvent);
LULLABY_SETUP_TYPEID(lull::SetNativeWindowEvent);

#endif  // LULLABY_EVENTS_RENDER_EVENTS_H_
