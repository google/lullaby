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

#ifndef LULLABY_SYSTEMS_RENDER_DETAIL_RENDER_POOL_H_
#define LULLABY_SYSTEMS_RENDER_DETAIL_RENDER_POOL_H_

#include "lullaby/modules/ecs/component.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {
namespace detail {

// RenderPools are containers for backend-specific RenderComponents, and are
// designed to exist 1:1 with a render pass.
template <typename Component>
class RenderPool {
 public:
  using SortMode = RenderSystem::SortMode;
  using CullMode = RenderSystem::CullMode;

  RenderPool(Registry* registry, size_t initial_size);

  RenderPool(const RenderPool& rhs) = delete;
  RenderPool& operator=(const RenderPool& rhs) = delete;

  RenderPool(RenderPool&& rhs)
      : registry_(rhs.registry_),
        components_(std::move(rhs.components_)),
        sort_mode_(rhs.sort_mode_),
        cull_mode_(rhs.cull_mode_),
        transform_flag_(rhs.transform_flag_) {
    rhs.transform_flag_ = TransformSystem::kInvalidFlag;
  }

  RenderPool& operator=(RenderPool&& rhs) = delete;

  ~RenderPool();

  // Returns |e|'s existing component, or nullptr if it doesn't have one.
  Component* GetComponent(Entity e) { return components_.Get(e); }
  const Component* GetComponent(Entity e) const { return components_.Get(e); }

  // Emplaces a component at the end of the pool's internal memory and returns a
  // pointer to it.  Check fails if there is already a component with the same
  // entity.
  template <typename... Args>
  Component& EmplaceComponent(Args&&... args) {
    Component* component = components_.Emplace(std::forward<Args>(args)...);
    CHECK(component != nullptr)
        << "Failed to emplace component into RenderPool.";

    if (GetTransformFlag() != TransformSystem::kInvalidFlag) {
      auto* transform_system = registry_->Get<TransformSystem>();
      transform_system->SetFlag(component->GetEntity(), transform_flag_);
    }

    return *component;
  }

  // Destroys |e|'s component.
  void DestroyComponent(Entity e);

  // Returns the number of components in the pool.
  size_t Size() const { return components_.Size(); }

  // Iterates over each component in the pool and passes it to |fn|.
  template <typename Fn>
  void ForEachComponent(Fn fn) const {
    components_.ForEach(fn);
  }

  // Returns the pool's cull mode.
  CullMode GetCullMode() const { return cull_mode_; }

  // Returns the transform flag, or TransformSystem::kInvalidFlag if not used.
  TransformSystem::TransformFlags GetTransformFlag() const {
    SyncTransformFlag();
    return transform_flag_;
  }

  // Sets the pool's cull mode to |cull_mode|.
  void SetCullMode(CullMode cull_mode);

  // Sets the pool's sort mode to |sort_mode|, and updates the transform flag
  // accordingly.
  void SetSortMode(SortMode sort_mode);

  // Returns the pool's sort mode.
  SortMode GetSortMode() const { return sort_mode_; }

 private:
  void SyncTransformFlag() const;

  Registry* registry_;
  ComponentPool<Component> components_;
  SortMode sort_mode_;
  CullMode cull_mode_;
  mutable TransformSystem::TransformFlags transform_flag_;
};

template <typename Component>
RenderPool<Component>::RenderPool(Registry* registry, size_t initial_size)
    : registry_(registry),
      components_(initial_size),
      sort_mode_(SortMode::kNone),
      cull_mode_(CullMode::kNone),
      transform_flag_(TransformSystem::kInvalidFlag) {}

template <typename Component>
RenderPool<Component>::~RenderPool() {
  auto* transform_system = registry_->Get<TransformSystem>();
  if (transform_system && transform_flag_ != TransformSystem::kInvalidFlag) {
    transform_system->ReleaseFlag(transform_flag_);
  }
}

template <typename Component>
void RenderPool<Component>::SetSortMode(SortMode mode) {
  if (mode != sort_mode_) {
    sort_mode_ = mode;
    SyncTransformFlag();
  }
}

template <typename Component>
void RenderPool<Component>::SetCullMode(CullMode mode) {
  cull_mode_ = mode;
}

template <typename Component>
void RenderPool<Component>::DestroyComponent(Entity e) {
  components_.Destroy(e);

  if (transform_flag_ != TransformSystem::kInvalidFlag) {
    auto* transform_system = registry_->Get<TransformSystem>();
    transform_system->ClearFlag(e, transform_flag_);
  }
}

template <typename Component>
void RenderPool<Component>::SyncTransformFlag() const {
  if (transform_flag_ != TransformSystem::kInvalidFlag) {
    return;
  }

  auto* transform_system = registry_->Get<TransformSystem>();
  if (transform_system) {
    transform_flag_ = transform_system->RequestFlag();

    components_.ForEach([=](const Component& component) {
      transform_system->SetFlag(component.GetEntity(), transform_flag_);
    });
  }
}

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_DETAIL_RENDER_POOL_H_
