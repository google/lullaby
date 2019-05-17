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

#ifndef LULLABY_CONTRIB_BACKDROP_BACKDROP_SYSTEM_H_
#define LULLABY_CONTRIB_BACKDROP_BACKDROP_SYSTEM_H_

#include "mathfu/constants.h"
#include "lullaby/generated/backdrop_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/events/layout_events.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/optional.h"

namespace lull {

// The BackdropSystem creates an entity with a RenderModel that acts as a
// backdrop to the other children of the entity that owns a backdrop component.
// The system automatically positions and scales that RenderModel to cover the
// axis-aligned bound box of the children.

class BackdropSystem : public System {
 public:
  explicit BackdropSystem(Registry* registry);

  ~BackdropSystem() override;

  void Initialize() override;

  // Creates the backdrop component for this entity.
  void Create(Entity e, HashValue type, const Def* def) override;

  void PostCreateInit(lull::Entity e, lull::HashValue type,
                      const Def* def) override;

  // Destroys the backdrop component for this entity.
  void Destroy(Entity e) override;

  // Checks to see if this entity is registered in the BackdropSystem.
  bool HasBackdrop(Entity e) const;

  // Returns the renderable entity (i.e. the child of the entity registered with
  // the BackdropSystem). If the given entity is not registered with the system
  // we log a warning and return the null entity.
  Entity GetBackdropRenderableEntity(Entity e) const;

  // Updates the Quad on the backdrop entity, since the renderable entity does
  // not have a quad. The BackdropSystem actively manages the size of the quad,
  // so any changes to size will be ignored, but other values can be modified.
  void SetBackdropQuad(Entity e, const RenderSystem::Quad& quad);

  // Get the merged aabb of all entities this backdrop covers, in the local
  // space of the backdrop entity. Will return nullptr if no backdrop on this
  // entity.
  const Aabb* GetBackdropAabb(Entity entity) const;

  // Set the merged aabb of all entities this backdrop covers, in the local
  // space of the backdrop entity.
  void SetBackdropAabb(Entity entity, const Aabb& aabb);

  // Get the duration of the aabb animation for the backdrop.
  Clock::duration GetBackdropAabbAnimationDuration(Entity entity);

  // Set the duration of the backdrop's aabb animation.
  void SetBackdropAabbAnimationDuration(Entity entity,
                                        Clock::duration duration);

 private:
  enum class RenderableType {
    kQuad,
    kNinePatch,
  };

  struct Backdrop : Component {
    explicit Backdrop(Entity e) : Component(e) {}

    float offset = 0.0f;
    mathfu::vec2 bottom_left_margin = mathfu::kZeros2f;
    mathfu::vec2 top_right_margin = mathfu::kZeros2f;
    BackdropAabbBehavior aabb_behavior = BackdropAabbBehavior_Backdrop;
    bool is_empty = true;
    Optional<Aabb> aabb;
    Clock::duration animate_aabb_duration = Clock::duration::zero();
    RenderableType renderable_type = RenderableType::kQuad;

    // The entity which owns the RenderModel that represents the backdrop.
    Entity renderable = kNullEntity;
    // The quad to use as the geometry of the backdrop. If renderable isn't a
    // quad, this still stores the size for that renderable.
    RenderSystem::Quad quad;
  };

  // Applies the margin and offset in |backdrop| onto |aabb|.
  void CreateRenderableAabb(const Backdrop* backdrop, Aabb* aabb);

  // Re-sizes and re-positions the backdrop's renderable entity based on the
  // axis-aligned bounding boxes of the other children.
  void UpdateBackdrop(Backdrop* backdrop);

  // Triggers an update if a parent with a backdrop was involved.
  void OnParentChanged(const ParentChangedEvent& event);

  // Triggers an update if the child of an entity with a backdrop was involved.
  void OnEntityChanged(Entity entity);

  // Forwards the set desired size event to all children.
  void OnDesiredSizeChanged(const DesiredSizeChangedEvent& entity);

  // The Backdrop components for each entity.
  ComponentPool<Backdrop> backdrops_;

  // The set of entities with a BackdropExclusionDef component.
  std::unordered_set<lull::Entity> exclusions_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::BackdropSystem);

#endif  // LULLABY_CONTRIB_BACKDROP_BACKDROP_SYSTEM_H_
