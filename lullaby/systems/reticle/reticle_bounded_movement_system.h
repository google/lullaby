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

#ifndef LULLABY_SYSTEMS_RETICLE_RETICLE_BOUNDED_MOVEMENT_SYSTEM_H_
#define LULLABY_SYSTEMS_RETICLE_RETICLE_BOUNDED_MOVEMENT_SYSTEM_H_

#include "lullaby/generated/reticle_boundary_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/math.h"

namespace lull {

/// Extends the ReticleSystem to support relative reticle movement in a certain
/// bounded area.
/// The current use case for this system is that during standalone device setup
/// flow, users have no idea about recentering. Therefore there need to be a
/// boundary that can deal with controller drifting issue. Considering the
/// controller would only drift in horizontal direction (x-axis) and vertical
/// direction (y-axis) is always accurate, this system also supports
/// horizontal-only-boundary with vertical direction unbounded.
class ReticleBoundedMovementSystem : public System {
 public:
  explicit ReticleBoundedMovementSystem(Registry* registry);

  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  void Destroy(Entity entity) override;

  /// Enable the reticle boundary on this entity.
  void Enable(Entity entity);
  /// Disable all reticle boundaries.
  void Disable();

  /// Set the horizontal boundary.
  void SetReticleHorizontalBoundary(Entity entity,
                                    const mathfu::vec2& horizontal);
  /// Set the vertical boundary.
  void SetReticleVerticalBoundary(Entity entity, const mathfu::vec2& vertical);

  /// Manually set the waiting frames for reticle to stabilize.
  void SetStabilizationFrames(int frames);

 private:
  struct ReticleBoundedMovement : Component {
    explicit ReticleBoundedMovement(Entity entity);

    // The boundary (min, max) is defined with (vec2.x, vec2.y)
    mathfu::vec2 horizontal, vertical;
    bool is_horizontal_only;

    mathfu::vec2 reticle_2d_position_last_frame;
    // Only track yaw(x) and pitch(y) value in world space. Ignore roll value.
    mathfu::vec2 input_orientation_last_frame;
  };
  void ResetReticlePosition(Entity entity);
  void ResetStabilizationCounter();
  std::unordered_map<Entity, ReticleBoundedMovement> reticle_movement_map_;
  int stabilization_counter_;
  int stabilization_frames_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ReticleBoundedMovementSystem);

#endif  // LULLABY_SYSTEMS_RETICLE_RETICLE_BOUNDED_MOVEMENT_SYSTEM_H_
