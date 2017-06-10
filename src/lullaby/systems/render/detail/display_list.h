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

#ifndef LULLABY_SYSTEMS_RENDER_DETAIL_DISPLAY_LIST_H_
#define LULLABY_SYSTEMS_RENDER_DETAIL_DISPLAY_LIST_H_

#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "lullaby/systems/render/detail/render_pool.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/math.h"
#include "lullaby/util/trace.h"

namespace lull {
namespace detail {

template <typename Component>
class DisplayList {
 public:
  using View = RenderSystem::View;

  union SortKey {
    SortKey() : u64(0) {}

    uint64_t u64;
    float f32;
  };

  struct Entry {
    explicit Entry(Entity e) : entity(e), component(nullptr) {}

    Entity entity;
    const Component* component;
    mathfu::mat4 world_from_entity_matrix;
    SortKey sort_key;
  };

  explicit DisplayList(Registry* registry) : registry_(registry) {}

  // Returns a pointer to the list of drawables.
  const std::vector<Entry>* GetContents() const { return &list_; }

  // Populates the list using |pool|.  |views| is used for camera-based sort
  // modes.
  void Populate(const RenderPool<Component>& pool, const View* views,
                size_t num_views);

 private:
  void GetComponentsUnsorted(const RenderPool<Component>& pool);
  void GetComponentsWithSortOrder(const RenderPool<Component>& pool);
  void GetComponentsWithWorldSpaceZ(const RenderPool<Component>& pool);
  void GetComponentsWithAverageSpaceZ(const RenderPool<Component>& pool,
                                      const View* views, size_t num_views);

  void SortDecreasingFloat();
  void SortIncreasingFloat();
  void SortDecreasingUnsigned();
  void SortIncreasingUnsigned();

  static constexpr size_t kMaxViews = 2;

  Registry* registry_;
  std::vector<Entry> list_;
};

template <typename Component>
void DisplayList<Component>::GetComponentsUnsorted(
    const RenderPool<Component>& pool) {
  for (auto& info : list_) {
    info.component = pool.GetComponent(info.entity);
    if (!info.component) {
      LOG(DFATAL) << "Failed to get component.";
    }
  }
}

template <typename Component>
void DisplayList<Component>::GetComponentsWithSortOrder(
    const RenderPool<Component>& pool) {
  for (auto& info : list_) {
    info.component = pool.GetComponent(info.entity);
    if (!info.component) {
      LOG(DFATAL) << "Failed to get component.";
      return;
    }
    info.sort_key.u64 = info.component->sort_order;
  }
}

template <typename Component>
void DisplayList<Component>::GetComponentsWithAverageSpaceZ(
    const RenderPool<Component>& pool, const View* views, size_t num_views) {
  if (num_views <= 0) {
    LOG(DFATAL) << "Must have at least 1 view.";
    return;
  }
  mathfu::vec3 avg_pos(0, 0, 0);
  mathfu::vec3 avg_z(0, 0, 0);
  for (size_t i = 0; i < num_views; ++i) {
    avg_pos += views[i].world_from_eye_matrix.TranslationVector3D();
    avg_z += GetMatrixColumn3D(views[i].world_from_eye_matrix, 2);
  }
  avg_pos /= static_cast<float>(num_views);
  avg_z.Normalize();

  for (auto& info : list_) {
    info.component = pool.GetComponent(info.entity);
    if (!info.component) {
      LOG(DFATAL) << "Failed to get component.";
      return;
    }
    const mathfu::vec3 world_pos =
        info.world_from_entity_matrix.TranslationVector3D();
    info.sort_key.f32 = mathfu::vec3::DotProduct(world_pos - avg_pos, avg_z);
  }
}

template <typename Component>
void DisplayList<Component>::GetComponentsWithWorldSpaceZ(
    const RenderPool<Component>& pool) {
  for (auto& info : list_) {
    info.component = pool.GetComponent(info.entity);
    if (!info.component) {
      LOG(DFATAL) << "Failed to get component.";
      return;
    }
    info.sort_key.f32 = info.world_from_entity_matrix.TranslationVector3D().z;
  }
}

template <typename Component>
void DisplayList<Component>::SortDecreasingFloat() {
  std::sort(list_.begin(), list_.end(), [](const Entry& a, const Entry& b) {
    return a.sort_key.f32 > b.sort_key.f32;
  });
}

template <typename Component>
void DisplayList<Component>::SortIncreasingFloat() {
  std::sort(list_.begin(), list_.end(), [](const Entry& a, const Entry& b) {
    return a.sort_key.f32 < b.sort_key.f32;
  });
}

template <typename Component>
void DisplayList<Component>::SortDecreasingUnsigned() {
  std::sort(list_.begin(), list_.end(), [](const Entry& a, const Entry& b) {
    return a.sort_key.u64 > b.sort_key.u64;
  });
}

template <typename Component>
void DisplayList<Component>::SortIncreasingUnsigned() {
  std::sort(list_.begin(), list_.end(), [](const Entry& a, const Entry& b) {
    return a.sort_key.u64 < b.sort_key.u64;
  });
}

template <typename Component>
void DisplayList<Component>::Populate(const RenderPool<Component>& pool,
                                      const View* views, size_t num_views) {
  LULLABY_CPU_TRACE_CALL();

  list_.clear();
  list_.reserve(pool.Size());

  using CullMode = RenderSystem::CullMode;
  const CullMode cull_mode = pool.GetCullMode();

  const auto* transform_system = registry_->Get<TransformSystem>();

  if (cull_mode == CullMode::kNone) {
    transform_system->ForEach(
        pool.GetTransformFlag(),
        [&](Entity e, const mathfu::mat4& world_from_entity_mat,
            const Aabb& box) {
          // TODO(b/28213394) Don't copy transforms.
          Entry info(e);
          info.world_from_entity_matrix = world_from_entity_mat;
          list_.push_back(info);
        });
  } else {
    // Compute the view frustum for all views.
    mathfu::vec4 frustum_clipping_planes[kMaxViews][kNumFrustumPlanes];
    if (num_views > kMaxViews) {
      LOG(DFATAL) << "Cannot have more views than eyes.";
      return;
    }
    for (size_t i = 0; i < num_views; i++) {
      CalculateViewFrustum(views[i].clip_from_world_matrix,
                           frustum_clipping_planes[i]);
    }

    transform_system->ForEach(
        pool.GetTransformFlag(),
        [&](Entity e, const mathfu::mat4& world_from_entity_mat,
            const Aabb& box) {
          // TODO(b/28213394) Don't copy transforms.
          Entry info(e);
          info.world_from_entity_matrix = world_from_entity_mat;

          // Compute the bounding sphere from bounding box and transform it
          // to world space because the view's frustum is in world space.
          // TODO(b/30646608): This should be cached since most entities are
          // static in nature and will result in improved performance.
          const float radius = (box.max - box.min).Length() * 0.5f;
          const mathfu::vec3 center =
              world_from_entity_mat *
              mathfu::vec3::Lerp(box.min, box.max, 0.5f);

          // Add the entity to the display list, if it's bounding sphere
          // intersects at least one render view's frustum.
          for (size_t i = 0; i < num_views; i++) {
            if (CheckSphereInFrustum(center, radius,
                                     frustum_clipping_planes[i])) {
              list_.push_back(info);
              break;
            }
          }
        });
  }

  using SortMode = RenderSystem::SortMode;
  const SortMode sort_mode = pool.GetSortMode();

  if (sort_mode == SortMode::kSortOrderIncreasing) {
    GetComponentsWithSortOrder(pool);
    SortIncreasingUnsigned();
  } else if (sort_mode == SortMode::kSortOrderDecreasing) {
    GetComponentsWithSortOrder(pool);
    SortDecreasingUnsigned();
  } else if (sort_mode == SortMode::kWorldSpaceZBackToFront) {
    // -z is forward, so z decreases as distance in front of camera increases.
    GetComponentsWithWorldSpaceZ(pool);
    SortIncreasingFloat();
  } else if (sort_mode == SortMode::kWorldSpaceZFrontToBack) {
    GetComponentsWithWorldSpaceZ(pool);
    SortDecreasingFloat();
  } else if (sort_mode == SortMode::kAverageSpaceOriginBackToFront) {
    // -z is forward, so z decreases as distance in front of camera increases.
    GetComponentsWithAverageSpaceZ(pool, views, num_views);
    SortIncreasingFloat();
  } else if (sort_mode == SortMode::kAverageSpaceOriginFrontToBack) {
    GetComponentsWithAverageSpaceZ(pool, views, num_views);
    SortDecreasingFloat();
  } else {
    DCHECK(sort_mode == SortMode::kNone) << "Unsupported sort mode "
                                         << static_cast<int>(sort_mode);
    GetComponentsUnsorted(pool);
  }
}

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_DETAIL_DISPLAY_LIST_H_
