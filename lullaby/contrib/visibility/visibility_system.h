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

#ifndef LULLABY_CONTRIB_VISIBILITY_VISIBILITY_SYSTEM_H_
#define LULLABY_CONTRIB_VISIBILITY_VISIBILITY_SYSTEM_H_

#include <unordered_map>
#include <unordered_set>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/visibility_def_generated.h"

namespace lull {

// Calculates the visibility of entities that have a VisibilityContentDef and
// are descendents of an entity which have a VisibilityWindowDef.  This can be
// used to hide content that moves out of the window, and show content that
// moves into it.
// Note: This system does not take deformations into account.
// TODO: add unit tests
class VisibilitySystem : public System {
 public:
  explicit VisibilitySystem(Registry* registry);
  ~VisibilitySystem() override;

  void Create(Entity entity, HashValue type, const Def* def) override;

  void Destroy(Entity entity) override;

  void Update();

  void ResetWindow(Entity entity);

 private:
  struct Window {
    Aabb bounds;
    std::unordered_map<Entity, VisibilityContentState> states;
    const EventDefArray* on_enter_events = nullptr;
    const EventDefArray* on_exit_events = nullptr;
    const EventDefArray* on_exit_top_events = nullptr;
    const EventDefArray* on_exit_bottom_events = nullptr;
    const EventDefArray* on_exit_left_events = nullptr;
    const EventDefArray* on_exit_right_events = nullptr;
    CollisionAxes collision_axes = CollisionAxes_XY;
  };

  struct WindowGroup : Component {
    explicit WindowGroup(Entity entity) : Component(entity) {}
    std::vector<Entity> contents;
    std::vector<Window> windows;
  };

  struct Content : Component {
    explicit Content(Entity entity) : Component(entity) {}

    Entity group = kNullEntity;
    const EventDefArray* on_enter_events = nullptr;
    const EventDefArray* on_exit_events = nullptr;
    VisibilityContentState starting_state = VisibilityContentState_Unknown;
  };

  WindowGroup* GetContainingGroup(Entity entity);
  void RemoveContentFromGroup(WindowGroup* group, Entity entity);
  void UpdateContentWithGroup(Content* content, WindowGroup* new_group);
  void OnParentChangedRecursive(Entity target, WindowGroup* new_group);
  void OnParentChanged(Entity target);
  void UpdateContentState(Window* window, Entity target,
                          const mathfu::vec3& position);
  void UpdateGroup(WindowGroup* group);

  ComponentPool<WindowGroup> groups_;
  ComponentPool<Content> contents_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::VisibilitySystem);

#endif  // LULLABY_CONTRIB_VISIBILITY_VISIBILITY_SYSTEM_H_
