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

#include "lullaby/contrib/visibility/visibility_system.h"

#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/util/trace.h"

namespace lull {

namespace {
const HashValue kWindowGroupDefHashValue =
    ConstHash("VisibilityWindowGroupDef");
const HashValue kContentDefHashValue = ConstHash("VisibilityContentDef");
}  // namespace

VisibilitySystem::VisibilitySystem(Registry* registry)
    : System(registry), groups_(2), contents_(16) {
  RegisterDef<VisibilityWindowGroupDefT>(this);
  RegisterDef<VisibilityContentDefT>(this);

  RegisterDependency<TransformSystem>(this);

  auto* dispatcher = registry->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const ParentChangedEvent& event) {
    OnParentChanged(event.target);
  });
}

VisibilitySystem::~VisibilitySystem() {
  // The Dispatcher might be destroyed before the VisibilitySystem, so we need
  // to check the pointer before using it.
  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->DisconnectAll(this);
  }
}

void VisibilitySystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type == kWindowGroupDefHashValue) {
    const auto* data = ConvertDef<VisibilityWindowGroupDef>(def);
    if (!data->windows() || data->windows()->size() == 0) {
      return;
    }

    WindowGroup* group = groups_.Emplace(entity);
    for (const VisibilityWindowDef* window_def : *data->windows()) {
      Window window;
      window.on_enter_events = window_def->on_enter_events();
      window.on_exit_events = window_def->on_exit_events();
      window.on_exit_top_events = window_def->on_exit_top_events();
      window.on_exit_bottom_events = window_def->on_exit_bottom_events();
      window.on_exit_left_events = window_def->on_exit_left_events();
      window.on_exit_right_events = window_def->on_exit_right_events();
      AabbFromFbAabb(window_def->bounds(), &window.bounds);
      window.collision_axes = window_def->collision_axes();
      group->windows.emplace_back(window);
    }
  } else if (type == kContentDefHashValue) {
    const auto* data = ConvertDef<VisibilityContentDef>(def);
    Content* content = contents_.Emplace(entity);
    content->on_enter_events = data->on_enter_events();
    content->on_exit_events = data->on_exit_events();
    content->starting_state = data->starting_state();
  }
}

void VisibilitySystem::Destroy(Entity entity) {
  Content* content = contents_.Get(entity);
  if (content) {
    WindowGroup* group = groups_.Get(content->group);
    if (group) {
      RemoveContentFromGroup(group, entity);
    }
    contents_.Destroy(entity);
  }

  WindowGroup* group = groups_.Get(entity);
  if (group) {
    for (const Entity iter : group->contents) {
      Content* content = contents_.Get(iter);
      content->group = kNullEntity;
    }
    groups_.Destroy(entity);
  }
}

void VisibilitySystem::ResetWindow(Entity entity) {
  WindowGroup* group = groups_.Get(entity);
  if (!group) {
    return;
  }

  for (Window& window : group->windows) {
    window.states.clear();
  }
}

VisibilitySystem::WindowGroup* VisibilitySystem::GetContainingGroup(
    Entity entity) {
  auto* transform_system = registry_->Get<TransformSystem>();
  Entity parent = transform_system->GetParent(entity);
  while (parent != kNullEntity) {
    WindowGroup* group = groups_.Get(parent);
    if (group) {
      return group;
    }
    parent = transform_system->GetParent(parent);
  }
  return nullptr;
}

void VisibilitySystem::RemoveContentFromGroup(WindowGroup* group,
                                              Entity entity) {
  auto iter = std::find(group->contents.begin(), group->contents.end(), entity);
  if (iter != group->contents.end()) {
    group->contents.erase(iter);
    for (Window& window : group->windows) {
      window.states.erase(entity);
    }
  }
}

void VisibilitySystem::UpdateContentWithGroup(Content* content,
                                              WindowGroup* new_group) {
  WindowGroup* old_group = groups_.Get(content->group);
  if (old_group == new_group) {
    return;
  }

  if (old_group) {
    RemoveContentFromGroup(old_group, content->GetEntity());
    content->group = kNullEntity;
  }

  if (new_group) {
    content->group = new_group->GetEntity();
    new_group->contents.emplace_back(content->GetEntity());
  }
}

void VisibilitySystem::OnParentChangedRecursive(Entity target,
                                                WindowGroup* new_group) {
  Content* content = contents_.Get(target);
  if (content) {
    UpdateContentWithGroup(content, new_group);
  }
  if (!groups_.Get(target)) {
    // Groups should own all contents below them, so stop when we encounter a
    // new child group because it should own all of its own descendants. They
    // should have been assigned the correct group when they got parented.
    const auto* transform_system = registry_->Get<TransformSystem>();
    const auto* children = transform_system->GetChildren(target);
    if (children) {
      for (Entity child : *children) {
        OnParentChangedRecursive(child, new_group);
      }
    }
  }
}

void VisibilitySystem::OnParentChanged(Entity target) {
  WindowGroup* new_group = GetContainingGroup(target);
  OnParentChangedRecursive(target, new_group);
}

void VisibilitySystem::UpdateContentState(Window* window, Entity target,
                                          const mathfu::vec3& position) {
  auto it = window->states.find(target);
  if (it == window->states.end()) {
    Content* content = contents_.Get(target);
    if (content) {
      it = window->states.emplace(target, content->starting_state).first;
    } else {
      LOG(DFATAL) << "Content (" << target << ") not found for window.";
      return;
    }
  }

  bool is_inside;
  switch (window->collision_axes) {
    case CollisionAxes_XY: {
      is_inside = position.x <= window->bounds.max.x &&
                  position.x >= window->bounds.min.x &&
                  position.y <= window->bounds.max.y &&
                  position.y >= window->bounds.min.y;
      break;
    }
    case CollisionAxes_XYZ: {
      is_inside = position.x <= window->bounds.max.x &&
                  position.x >= window->bounds.min.x &&
                  position.y <= window->bounds.max.y &&
                  position.y >= window->bounds.min.y &&
                  position.z <= window->bounds.max.z &&
                  position.z >= window->bounds.min.z;
      break;
    }
  }
  const VisibilityContentState state = is_inside
                                           ? VisibilityContentState_Inside
                                           : VisibilityContentState_Outside;

  if (state != it->second) {
    it->second = state;

    Content* content = contents_.Get(target);
    if (content) {
      if (state == VisibilityContentState_Inside) {
        SendEventDefs(registry_, target, window->on_enter_events);
        SendEventDefs(registry_, target, content->on_enter_events);
      } else {
        SendEventDefs(registry_, target, window->on_exit_events);
        if (position.y > window->bounds.max.y) {
          SendEventDefs(registry_, target, window->on_exit_top_events);
        } else if (position.y < window->bounds.min.y) {
          SendEventDefs(registry_, target, window->on_exit_bottom_events);
        }
        if (position.x < window->bounds.min.x) {
          SendEventDefs(registry_, target, window->on_exit_left_events);
        } else if (position.x > window->bounds.max.x) {
          SendEventDefs(registry_, target, window->on_exit_right_events);
        }
        SendEventDefs(registry_, target, content->on_exit_events);
      }
    } else {
      LOG(DFATAL) << "Content (" << target << ") not found for window.";
    }
  }
}

void VisibilitySystem::UpdateGroup(WindowGroup* group) {
  auto* transform_system = registry_->Get<TransformSystem>();
  if (!transform_system->IsEnabled(group->GetEntity())) {
    return;
  }

  const mathfu::mat4* world_from_window_matrix =
      transform_system->GetWorldFromEntityMatrix(group->GetEntity());
  if (!world_from_window_matrix) {
    return;
  }

  const mathfu::mat4 window_from_world_matrix =
      world_from_window_matrix->Inverse();

  for (const Entity target : group->contents) {
    const mathfu::mat4* world_from_content_matrix =
        transform_system->GetWorldFromEntityMatrix(target);
    if (!world_from_content_matrix) {
      continue;
    }

    const mathfu::mat4 window_from_content_matrix =
        window_from_world_matrix * (*world_from_content_matrix);
    const mathfu::vec3 pos = window_from_content_matrix.TranslationVector3D();

    for (Window& window : group->windows) {
      UpdateContentState(&window, target, pos);
    }
  }
}

void VisibilitySystem::Update() {
  LULLABY_CPU_TRACE_CALL();
  groups_.ForEach([this](WindowGroup& group) { UpdateGroup(&group); });
}

}  // namespace lull
