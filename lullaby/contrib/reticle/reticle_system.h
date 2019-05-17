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

#ifndef LULLABY_SYSTEMS_RETICLE_RETICLE_SYSTEM_H_
#define LULLABY_SYSTEMS_RETICLE_RETICLE_SYSTEM_H_

#include <deque>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/modules/reticle/standard_input_pipeline.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/reticle_behaviour_def_generated.h"
#include "lullaby/generated/reticle_def_generated.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

/// The ReticleSystem updates the on-screen position of the reticle based on ray
/// collision results.  It is also responsible for firing off reticle events
/// (eg. HoverStart, HoverStop, Click, etc.).
class ReticleSystem : public System {
 public:
  static constexpr float kDefaultNoHitDistance = 2.0f;

  /// The reticle movemt function takes the active input device as the parameter
  /// and returns an Sqt which contains the origin and direction of reticle ray.
  using ReticleMovementFn = std::function<Ray(InputManager::DeviceType)>;

  /// The reticle smoothing function takes the current controller direction
  /// and frame interval as parameters and returns the reticle direction.
  using ReticleSmoothingFn = std::function<mathfu::vec3(
      mathfu::vec3 direction, const Clock::duration& delta_time)>;

  explicit ReticleSystem(Registry* registry);

  void Create(Entity entity, HashValue type, const Def* def) override;

  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  void Destroy(Entity entity) override;

  void AdvanceFrame(const Clock::duration& delta_time);

  // TODO Remove this function.
  /// Deprecated - call CursorSystem::SetNoHitDistance instead.
  void SetNoHitDistance(float distance);

  /// Gets the current reticle entity.
  Entity GetReticle() const;

  /// Gets the current target under the reticle.
  Entity GetTarget() const;

  /// Gets the ray representing the target direction for the reticle.
  Ray GetCollisionRay() const;

  /// Returns the type of the device currently used to position the reticle.
  InputManager::DeviceType GetActiveDevice() const;

  /// Set a preset reticle movement function if the default reticle movement is
  /// not applicable in some cases.
  void SetReticleMovementFn(ReticleMovementFn fn);

  /// Set a reticle smoothing function.
  void SetReticleSmoothingFn(ReticleSmoothingFn fn);

  /// Lock the reticle to an entity.  For the duration of the lock, the reticle
  /// will maintain a constant offset from the target entity's world location.
  /// Pass kNullEntity to return ReticleSystem to normal behavior.
  void LockOn(Entity entity, const mathfu::vec3& offset);

 private:
  struct Reticle : Component {
    explicit Reticle(Entity entity);
    // The current entity target hit by the raycast from the reticle.
    Entity target_entity = kNullEntity;

    // An entity that was pressed by the primary input. This is the same entity
    // that receives a ClickEvent, and later a ClickReleasedEvent.
    Entity pressed_entity = kNullEntity;

    std::vector<InputManager::DeviceType> device_preference;

    ReticleMovementFn movement_fn = nullptr;
    ReticleSmoothingFn smoothing_fn = nullptr;
  };

  void CreateReticle(Entity entity, const ReticleDef* data);

  void CreateReticleBehaviour(Entity entity, const ReticleBehaviourDef* data);

  // Calculate the origin, collision_ray, and an ideal cursor_position for
  // where the cursor should be based on input (assuming no actual collisions
  // or collision ray modifications take place).
  void CalculateFocusPositions(Entity reticle_entity,
                               const Clock::duration& delta_time,
                               InputFocus* focus);

  std::unique_ptr<Reticle> reticle_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ReticleSystem);

#endif  // LULLABY_SYSTEMS_RETICLE_RETICLE_SYSTEM_H_
