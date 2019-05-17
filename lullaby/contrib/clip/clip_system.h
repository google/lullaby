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

#ifndef LULLABY_CONTRIB_CLIP_CLIP_SYSTEM_H_
#define LULLABY_CONTRIB_CLIP_CLIP_SYSTEM_H_

#include "lullaby/generated/clip_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/events/entity_events.h"

namespace lull {

// The ClipSystem manages clip regions and their children.  Note that this
// requires the app to have a stencil buffer!  Normally this is done by
// overriding GetDepthFormat() in your app's header.
//
// A clip region is defined by adding a ClipDef to an entity with a RenderDef.
// When this is done, all of the entity's descendents in the scene graph will
// be visible only where they overlap the clip region's mesh (as defined by its
// RenderDef).
//
// This is done in screenspace using a stencil buffer and assigning
// a unique stencil reference value to clip regions. A region is drawn with a
// reference value and then all its children are drawn. During children draw
// stage a stencil check is assigned against the reference value and only pixels
// that intersect the assigned reference value are written.
// https://www.opengl.org/wiki/Stencil_Test
//
// Current limitations:
// - Dependent on the relative draw order of the region and its descendents.
//   The region needs to be drawn before its contents.
// - Overlapping clip regions do not work.
class ClipSystem : public System {
 public:
  explicit ClipSystem(Registry* registry);
  ~ClipSystem() override;

  void Initialize() override;

  // Registers a clip region.
  void Create(Entity entity, HashValue type, const Def* def) override;

  // Performs post creation initialization.
  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  // Registers a clip region for a given |entity| using automatically
  // incremented reference value.
  void Create(Entity entity);

  // Sets the stencil reference value for clip region |entity|.
  void SetReferenceValue(Entity entity, const int value);

  // Deregisters a clip region.
  void Destroy(Entity entity) override;

  // Disables |entity|'s clipping behavior, but does not fully disable |entity|
  // (use TransformSystem::Disable for that).
  void DisableStencil(Entity entity);

  // Enables |entity|'s clipping behavior, but does not fully enable |entity|
  // (use TransformSystem::Enable for that).
  void EnableStencil(Entity entity);

  // Returns the clip region that contains e, or kNullEntity.
  Entity GetRegion(Entity entity) const;

 private:
  struct ClipRegion : Component {
    explicit ClipRegion(Entity entity) : Component(entity) {}

    int stencil_value = 0;
    bool enabled = true;
  };

  struct ClipTarget : Component {
    explicit ClipTarget(Entity entity) : Component(entity) {}

    Entity region = kNullEntity;
  };

  using RegionPool = ComponentPool<ClipRegion>;
  using TargetPool = ComponentPool<ClipTarget>;

  void AddTarget(Entity target_entity, Entity region_entity);
  void RemoveTarget(Entity entity);

  void OnParentChanged(const ParentChangedImmediateEvent& event);

  RegionPool regions_;
  TargetPool targets_;
  int next_stencil_value_;
  int auto_stencil_start_value_;

  ClipSystem(const ClipSystem&) = delete;
  ClipSystem& operator=(const ClipSystem&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ClipSystem);

#endif  // LULLABY_CONTRIB_CLIP_CLIP_SYSTEM_H_
