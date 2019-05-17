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

#include "lullaby/contrib/slideshow/slideshow_system.h"

#include <algorithm>

#include "lullaby/generated/slideshow_def_generated.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/dispatcher/dispatcher_system.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/logging.h"
#include "lullaby/modules/animation_channels/transform_channels.h"

namespace lull {
namespace {
const HashValue kShowSlideshowHash = ConstHash("SlideshowShow");
const HashValue kHideSlideshowHash = ConstHash("SlideshowHide");
const HashValue kSlideshowDefHash = ConstHash("SlideshowDef");
}  // namespace

SlideshowSystem::SlideshowSystem(Registry* registry)
    : System(registry), widgets_(8) {
  RegisterDef<SlideshowDefT>(this);
  RegisterDependency<AnimationSystem>(this);
  RegisterDependency<DispatcherSystem>(this);
  RegisterDependency<TransformSystem>(this);

  Dispatcher* dispatcher = registry_->Get<Dispatcher>();
  dispatcher->Connect(this, [this](const ParentChangedEvent& event) {
    OnParentChanged(event);
  });
  dispatcher->Connect(this, [this](const AabbChangedEvent& event) {
    OnAabbChanged(event.target);
  });
}

SlideshowSystem::~SlideshowSystem() {
  registry_->Get<Dispatcher>()->DisconnectAll(this);
}

void SlideshowSystem::Create(Entity entity, HashValue type, const Def* def) {
  if (type != kSlideshowDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting SlideshowDef!";
    return;
  }
  auto* data = ConvertDef<SlideshowDef>(def);
  Slideshow widget;
  widget.show_next_transition =
      std::chrono::milliseconds(data->show_next_transition_ms());
  widgets_.emplace(entity, widget);

  auto response = [entity, this](const EventWrapper& event) {
    ShowNextChild(entity);
  };
  ConnectEventDefs(registry_, entity, data->show_next_events(),
                   response);
}

void SlideshowSystem::Destroy(Entity entity) { widgets_.erase(entity); }

void SlideshowSystem::ShowNextChild(Entity entity) {
  auto widget = widgets_.find(entity);
  if (widget == widgets_.end()) {
    return;
  }

  DoShowNextChild(&*widget, widget->second.show_next_transition);
}

void SlideshowSystem::DoShowNextChild(SlideshowComponent* widget,
                                      Clock::duration duration) {
  auto* dispatcher_system = registry_->Get<DispatcherSystem>();
  auto* transform_system = registry_->Get<TransformSystem>();
  auto* children = transform_system->GetChildren(widget->first);

  // Hide the previous child.
  if (widget->second.showing_child != kNullEntity) {
    dispatcher_system->Send(widget->second.showing_child,
                            EventWrapper(kHideSlideshowHash));
  }

  // Show the next child.
  if (children->empty()) {
    widget->second.showing_child = kNullEntity;
    AnimateAabb(*widget, Aabb(), duration);
  } else {
    auto iter = std::find(children->begin(), children->end(),
                           widget->second.showing_child);
    auto next = iter != children->end() ? iter + 1 : iter;
    widget->second.showing_child =
        next != children->end() ? *next : children->front();

    dispatcher_system->Send(widget->second.showing_child,
                            EventWrapper(kShowSlideshowHash));
    AnimateAabb(*widget,
                *transform_system->GetAabb(widget->second.showing_child),
                duration);
  }
}

// Animate Aabb to match showing child, or zeroed Aabb if no child.
void SlideshowSystem::AnimateAabb(const SlideshowComponent& widget,
                                  const Aabb& aabb, Clock::duration duration) {
  auto* animation_system = registry_->Get<AnimationSystem>();
  animation_system->SetTarget(widget.first,
      AabbMinChannel::kChannelName, &aabb.min.x, 3, duration);
  animation_system->SetTarget(widget.first,
      AabbMaxChannel::kChannelName, &aabb.max.x, 3, duration);
}

void SlideshowSystem::OnParentChanged(const ParentChangedEvent& event) {
  auto old_parent = widgets_.find(event.old_parent);
  if (old_parent != widgets_.end() &&
      old_parent->second.showing_child == event.target) {
    // Old slideshow needs a new showing child.
    DoShowNextChild(&*old_parent, old_parent->second.show_next_transition);
  }
  // If this was not the showing_child, then it was hidden so don't need to do
  // anything.

  auto new_parent = widgets_.find(event.new_parent);
  if (new_parent != widgets_.end() &&
      new_parent->second.showing_child == kNullEntity) {
    // First child, automatically show it with no aabb animation.
    DoShowNextChild(&*new_parent, Clock::duration(0));
  }
}

void SlideshowSystem::OnAabbChanged(Entity entity) {
  auto* transform_system = registry_->Get<TransformSystem>();
  auto parent = transform_system->GetParent(entity);
  if (parent == kNullEntity) {
    return;
  }

  auto widget = widgets_.find(parent);
  if (widget == widgets_.end() ||
      widget->second.showing_child != entity) {
    return;
  }

  // Whenever the showing_child's aabb changes, match it without animation.
  transform_system->SetAabb(parent, *transform_system->GetAabb(entity));
}

}  // namespace lull
