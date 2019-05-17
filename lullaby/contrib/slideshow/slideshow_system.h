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

#ifndef LULLABY_CONTRIB_SLIDESHOW_SLIDESHOW_SYSTEM_H_
#define LULLABY_CONTRIB_SLIDESHOW_SLIDESHOW_SYSTEM_H_

#include <unordered_map>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/util/clock.h"

namespace lull {

/// The SlideshowSystem manages slideshow entities that show one child at a time
/// among its children, and can transition between them in sequence. It also
/// animates its Aabb to match the shown child's Aabb. The first child will
/// automatically be shown at the start.
/// This system requires AabbMinChannel and AabbMaxChannel from
/// transform_channels.h.
class SlideshowSystem : public System {
 public:
  explicit SlideshowSystem(Registry* registry);
  ~SlideshowSystem() override;

  void Create(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;

  /// Shows the next child of |entity| and hides the current shown one.
  void ShowNextChild(Entity entity);

 private:
  struct Slideshow {
    Clock::duration show_next_transition;
    Entity showing_child = kNullEntity;
  };

  using SlideshowMap = std::unordered_map<Entity, Slideshow>;
  using SlideshowComponent = SlideshowMap::value_type;

  void DoShowNextChild(SlideshowComponent* widget, Clock::duration duration);
  void AnimateAabb(const SlideshowComponent& widget, const Aabb& aabb,
                   Clock::duration duration);
  void OnParentChanged(const ParentChangedEvent& event);
  void OnAabbChanged(Entity entity);

  SlideshowMap widgets_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::SlideshowSystem);

#endif  // LULLABY_CONTRIB_SLIDESHOW_SLIDESHOW_SYSTEM_H_
