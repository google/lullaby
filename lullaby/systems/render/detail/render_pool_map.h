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

#ifndef LULLABY_SYSTEMS_RENDER_DETAIL_RENDER_POOL_MAP_H_
#define LULLABY_SYSTEMS_RENDER_DETAIL_RENDER_POOL_MAP_H_

#include <unordered_map>

#include "lullaby/generated/render_def_generated.h"
#include "lullaby/systems/render/detail/render_pool.h"
#include "lullaby/util/registry.h"

namespace lull {
namespace detail {

// RenderPoolMaps maintain a 1:1 relationship between render passes and render
// pools.  Components can be moved between render passes via MoveToPool, but can
// never be in more than one pass/pool at once.
template <typename Component>
class RenderPoolMap {
 public:
  using ComponentPool = RenderPool<Component>;

  explicit RenderPoolMap(Registry* registry) : registry_(registry) {}

  // Returns |e|'s component or nullptr if one does not exist.
  Component* GetComponent(Entity e) {
    for (auto& entry : map_) {
      Component* component = entry.second.GetComponent(e);
      if (component) {
        return component;
      }
    }
    return nullptr;
  }

  // Returns |e|'s component or nullptr if one does not exist.
  const Component* GetComponent(Entity e) const {
    for (const auto& entry : map_) {
      const Component* component = entry.second.GetComponent(e);
      if (component) {
        return component;
      }
    }
    return nullptr;
  }

  // Emplaces |e|'s component at the end of the render pool for |pass| and
  // returns a reference to the newly created component.
  Component& EmplaceComponent(Entity e, RenderPass pass) {
    return GetPool(pass).EmplaceComponent(Component(e));
  }

  // Destroys |e|'s component, regardless of which pool it's stored in. Ignores
  // entities without a component.
  void DestroyComponent(Entity e) {
    for (auto& entry : map_) {
      entry.second.DestroyComponent(e);
    }
  }

  // Returns the render pool for |pass|. If the pool has not already been
  // created then this function will do so.
  ComponentPool& GetPool(RenderPass pass) {
    static const int kInitialPoolSize = 16;
    auto existing = map_.find(pass);
    if (existing != map_.end()) {
      return existing->second;
    }
    return map_.emplace(pass, ComponentPool(registry_, kInitialPoolSize))
        .first->second;
  }

  // Returns the render pool for |pass| or nullptr.
  const ComponentPool* GetExistingPool(RenderPass pass) const {
    const auto existing = map_.find(pass);
    return (existing != map_.end()) ? &existing->second : nullptr;
  }

  // Moves |e|'s component into |pass|'s pool.  This can invalidate |e|'s
  // component,
  // so take care not to continue using any pointers to it. Ignores entities
  // without a component.
  void MoveToPool(Entity e, RenderPass pass) {
    if (pass == RenderPass_Debug) {
      LOG(DFATAL) << "Cannot move to pool in Render Debug Pass.";
      return;
    }
    for (auto& entry : map_) {
      if (entry.first == pass) {
        continue;
      }

      ComponentPool& old_pool = entry.second;
      auto* component = old_pool.GetComponent(e);
      if (component) {
        ComponentPool& new_pool = GetPool(pass);
        new_pool.EmplaceComponent(std::move(*component));
        old_pool.DestroyComponent(e);
        return;
      }
    }
  }

 private:
  std::unordered_map<int, ComponentPool> map_;
  Registry* registry_;
};

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_DETAIL_RENDER_POOL_MAP_H_
