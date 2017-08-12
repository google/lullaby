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

#ifndef LULLABY_SYSTEMS_SCROLL_SCROLL_CONTENT_LAYOUT_SYSTEM_H_
#define LULLABY_SYSTEMS_SCROLL_SCROLL_CONTENT_LAYOUT_SYSTEM_H_

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/math.h"

namespace lull {

// Extends the ScrollSystem such that the content bounds of a scroll view will
// be automatically updated using the child's Aabb.  This is useful if the
// scroll view content size is controlled by the LayoutSystem.
class ScrollContentLayoutSystem : public System {
 public:
  explicit ScrollContentLayoutSystem(Registry* registry);
  ~ScrollContentLayoutSystem() override;

  void Create(Entity entity, HashValue type, const Def* def) override;
  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;

 private:
  struct Content : Component {
    explicit Content(Entity entity) : Component(entity) {}

    mathfu::vec3 min_padding = mathfu::kZeros3f;
    mathfu::vec3 max_padding = mathfu::kZeros3f;
  };

  using ContentPool = ComponentPool<Content>;

  void UpdateScrollContentBounds(Entity entity);

  ContentPool contents_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ScrollContentLayoutSystem);

#endif  // LULLABY_SYSTEMS_SCROLL_SCROLL_CONTENT_LAYOUT_SYSTEM_H_
