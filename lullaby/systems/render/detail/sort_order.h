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

#ifndef LULLABY_SYSTEMS_RENDER_DETAIL_SORT_ORDER_H_
#define LULLABY_SYSTEMS_RENDER_DETAIL_SORT_ORDER_H_

#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/registry.h"

namespace lull {
namespace detail {

// A helper class to manage sort orders.  This stores the offsets of all known
// entities, even those that don't have render components.
//
// Sort orders are calculated from offsets at every level of a hierarchy.  If an
// entity doesn't have an offset or its offset is 0, then it uses a default
// value based on its inheritance.
class SortOrderManager {
 public:
  using SortOrder = RenderSystem::SortOrder;
  using SortOrderOffset = RenderSystem::SortOrderOffset;

  static constexpr SortOrderOffset kUseDefaultOffset = 0;

  explicit SortOrderManager(Registry* registry) : registry_(registry) {}

  // Removes |entity_id_pair|'s data.
  void Destroy(EntityIdPair entity_id_pair);

  // Returns |entity_id_pair|'s sort order offset, or kUseDefaultOffset if it's
  // not known.
  SortOrderOffset GetOffset(EntityIdPair entity_id_pair) const;

  // Sets |entity_id_pair|'s sort order offset without recalculating its sort
  // order.  An offset of kUseDefaultOffset signifies that a default,
  // auto-calculated value be used when determining the sort order.
  void SetOffset(EntityIdPair entity_id_pair, SortOrderOffset offset);

  // Returns the sort order for |entity_id_pair| based on its offset and
  // hierarchy.  For entities that have a render component, prefer the cached
  // value in the component.
  SortOrder CalculateSortOrder(EntityIdPair entity_id_pair);

  // Calculates |entity_id_pair|'s sort order, stores it in its render component
  // (if it has one), and recurses through its children.
  template <typename GetComponentFn>
  void UpdateSortOrder(EntityIdPair entity_id_pair,
                       const GetComponentFn& get_component);

 private:
  // Returns the sibling offset of |entity_id_pair|.  Result is undefined if
  // |parent| is kNullEntity.
  SortOrderOffset CalculateSiblingOffset(EntityIdPair entity_id_pair,
                                         Entity parent) const;

  // Calculates the sort order for root-level |entity_id_pair|.  If
  // |entity_id_pair| does not yet have an offset assigned, this will assign
  // one.
  SortOrder CalculateRootSortOrder(EntityIdPair entity_id_pair);

  // Calculates the sort order for |entity|, also returning its hierarchical
  // depth.  If |entity_id_pair| has no parent, this will assign it a rolling
  // offset if one does not exist.
  std::pair<SortOrder, int> CalculateSortOrderAndDepth(
      EntityIdPair entity_id_pair);

  // Registry of shared systems, owned by the app.
  Registry* registry_;

  // Per-entity offsets requested via SetOffset.
  std::unordered_map<EntityIdPair, SortOrderOffset, EntityIdPairHash>
      requested_offset_map_;

  // Offsets assigned to root level entities, which need to remain consistent
  // across hierarchy changes and calls to SetOffset.
  std::unordered_map<EntityIdPair, SortOrderOffset, EntityIdPairHash>
      root_offset_map_;

  // Offset to use for the next root-level entity to be registered.
  SortOrderOffset next_root_offset_ = 1;
};

template <typename GetComponentFn>
void SortOrderManager::UpdateSortOrder(EntityIdPair entity_id_pair,
                                       const GetComponentFn& get_component) {
  auto* component = get_component(entity_id_pair);
  if (component) {
    component->sort_order = CalculateSortOrder(entity_id_pair);
  }

  const auto* transform_system = registry_->Get<TransformSystem>();
  const std::vector<Entity>* children =
      transform_system->GetChildren(entity_id_pair.entity);
  if (children) {
    for (const auto& child : *children) {
      UpdateSortOrder(child, get_component);
    }
  }
}

}  // namespace detail
}  // namespace lull

#endif  // LULLABY_SYSTEMS_RENDER_DETAIL_SORT_ORDER_H_
