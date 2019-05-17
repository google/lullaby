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

#ifndef LULLABY_CONTRIB_SHADER_CLIP_SHADER_CLIP_SYSTEM_H_
#define LULLABY_CONTRIB_SHADER_CLIP_SHADER_CLIP_SYSTEM_H_

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/util/math.h"

namespace lull {

// A system that performs clipping at a shader level. Entities can be registered
// as regions and targets in the blueprints. A region defines a clip region
// which is a box in the Model Space of the entity. Targets are entities that
// should only draw geometry inside a clip region. To add an entity and all its
// children as a target, the AddTarget function is used together with which
// region to clip against.
//
// All targets should use customized shaders that import the
// uber_vertex_common.glslh or uber_fragment_common.glslh headers, call the
// necessary functions, and compile with the CLIP_REGION flag.
class ShaderClipSystem : public System {
 public:
  explicit ShaderClipSystem(Registry* registry);
  ~ShaderClipSystem() override;
  ShaderClipSystem(const ShaderClipSystem&) = delete;
  ShaderClipSystem& operator=(const ShaderClipSystem&) = delete;

  // Adds |target| and all its children to be clipped by |region|. |target| must
  // use any of the clipping shaders mentioned in the class documentation.
  // |region| can be kNullEntity in which case the |target| will be inactive
  // until it gets a Region as an ancestor.
  void AddTarget(Entity region, Entity target);

  // Sets uniforms needed by the clipping shader. Needs to be called after the
  // Transform System update every frame.
  // Note: If another System manages multiple entities that are renderable, make
  // sure to Update() first, and then that System needs to propagate the
  // uniforms (e.g. FlatuiTextSystem::ProcessTasks()).
  void Update();

  // ---------------------------------------------------------------------------
  // System overrides.
  // ---------------------------------------------------------------------------
  void Create(Entity entity, HashValue type, const Def* def) override;
  void Destroy(Entity entity) override;

 private:
  // Defines the clip region of an entity, which is a box volume. Any target
  // added to this region will not have any geometry drawn outside of the
  // bounds defined by |min_in_clip_region_space| and
  // |max_in_clip_region_space|.
  struct ClipRegion : Component {
    explicit ClipRegion(Entity entity);
    mathfu::vec3 min_in_clip_region_space;
    mathfu::vec3 max_in_clip_region_space;

    // Cache of world_from_clip_region_matrix to reduce calculating the below
    // inverse.
    mathfu::mat4 world_from_clip_region_matrix;
    // Whether the cache changed this frame.
    bool world_from_clip_region_matrix_changed = true;
    // Inverse of the above to make it faster to compute the
    // |clip_region_from_model_space_matrix| per target per frame.
    mathfu::mat4 clip_region_from_world_matrix;
  };

  // A target will only draw geometry inside the box volume defined in |region|.
  // |region| must be a ClipRegion.
  struct ClipTarget : Component {
    explicit ClipTarget(Entity entity);
    Entity region;
    // This target was enabled through AddTarget() or ShaderClipTargetDef.
    bool manually_enabled;
    // Cache of world_from_model_matrix to reduce calculations and setting the
    // uniform.
    mathfu::mat4 world_from_model_matrix;
  };

  ClipTarget* GetTarget(Entity entity);
  const ClipTarget* GetTarget(Entity entity) const;
  Entity GetContainingRegion(Entity entity) const;
  void OnDisabled(Entity entity);
  void OnEnabled(Entity entity);
  void OnParentChanged(const ParentChangedImmediateEvent& event);
  void AddTargetRecursive(Entity region, Entity target);
  void TryAddTargetRecursive(Entity region, Entity target);
  void RemoveTarget(Entity entity);
  void ResetUniforms(Entity entity);
  void DestroyTarget(Entity entity);

  ComponentPool<ClipRegion> clip_regions_;
  ComponentPool<ClipTarget> clip_targets_;
  ComponentPool<ClipTarget> disabled_clip_targets_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(ShaderClipSystem);

#endif  // LULLABY_CONTRIB_SHADER_CLIP_SHADER_CLIP_SYSTEM_H_
