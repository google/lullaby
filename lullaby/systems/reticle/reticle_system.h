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

#ifndef LULLABY_SYSTEMS_RETICLE_RETICLE_SYSTEM_H_
#define LULLABY_SYSTEMS_RETICLE_RETICLE_SYSTEM_H_

#include <deque>

#include "lullaby/generated/reticle_behaviour_def_generated.h"
#include "lullaby/generated/reticle_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"
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
  using ReticleMovementFn = std::function<Sqt(InputManager::DeviceType)>;

  /// The reticle smoothing function takes the current controller direction
  /// and frame interval as parameters and returns the reticle direction.
  using ReticleSmoothingFn = std::function<mathfu::vec3(
      mathfu::vec3 direction, const Clock::duration& delta_time)>;

  explicit ReticleSystem(Registry* registry);

  void Initialize() override;

  void Create(Entity entity, HashValue type, const Def* def) override;

  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  void Destroy(Entity entity) override;

  void AdvanceFrame(const Clock::duration& delta_time);

  /// Sets the distance for the reticle when there is no collision.
  void SetNoHitDistance(float distance);

  /// Gets the distance for the reticle when there is no collision.
  float GetNoHitDistance() const;

  /// Gets the current reticle entity.
  Entity GetReticle() const;

  /// Gets the current target under the reticle.
  Entity GetTarget() const;

  /// Gets the ray representing the target direction for the reticle.
  Ray GetCollisionRay() const;

  /// Returns the type of the device currently used to position the reticle.
  InputManager::DeviceType GetActiveDevice() const;

  /// Returns the reticle ergo angle offset.
  float GetReticleErgoAngleOffset() const;

  /// Set a preset reticle movement function if the default reticle movement is
  /// not applicable in some cases.
  void SetReticleMovementFn(ReticleMovementFn fn);

  /// Set a reticle smoothing function.
  void SetReticleSmoothingFn(ReticleSmoothingFn fn);

  /// Lock the reticle to an entity.  For the duration of the lock, the reticle
  /// will maintain a constant offset from the target entity's world location.
  /// Pass kNullEntity to return ReticleSystem to normal behavior.
  void LockOn(Entity entity, const mathfu::vec3& offset);

  /// Sets the reticle collision behaviour for |entity|.
  void SetReticleCollisionBehaviour(Entity entity,
      ReticleCollisionBehaviour collision_behaviour);

 private:
  struct Reticle : Component {
    explicit Reticle(Entity entity);
    // The current entity target hit by the raycast from the reticle.
    Entity target_entity = kNullEntity;

    // An entity that was pressed by the primary input. This is the same entity
    // that receives a ClickEvent, and later a ClickReleasedEvent.
    Entity pressed_entity = kNullEntity;

    // The amount of time between press and release, reported in
    // ClickPressedAndReleasedEvent.
    int64_t ms_since_press = 0;

    // The ray representing the direction that the reticle is pointing. This is
    // used for collision checking.
    Ray collision_ray;

    float no_hit_distance = 0.f;
    float ergo_angle_offset = 0.f;
    float ring_active_diameter = 0.f;
    float ring_inactive_diameter = 0.f;
    mathfu::vec4 hit_color;
    mathfu::vec4 no_hit_color;
    std::vector<InputManager::DeviceType> device_preference;

    ReticleMovementFn movement_fn = nullptr;
    ReticleSmoothingFn smoothing_fn = nullptr;

    // An entity that (if set), the reticle is forced to target.
    Entity locked_target = kNullEntity;

    // The world space offset from the locked_entity's position.
    mathfu::vec3 lock_offset = mathfu::kZeros3f;
  };

  struct ReticleBehaviour : Component {
    explicit ReticleBehaviour(Entity entity)
        : Component(entity), hover_start_dead_zone(mathfu::kZeros3f) {}

    // The amount to shrink this entity's Aabb by when checking for a hover
    // start event. The dead zone is applied on both sides.
    mathfu::vec3 hover_start_dead_zone;

    // How this entity should handle collisions.
    ReticleCollisionBehaviour collision_behaviour;
  };

  void CreateReticle(Entity entity, const ReticleDef* data);

  void CreateReticleBehaviour(Entity entity, const ReticleBehaviourDef* data);

  // If the target has a ReticleBehavior, apply that behavior and return the
  // actual target.
  Entity HandleReticleBehavior(Entity targeted_entity);

  // Place the reticle at the desired location, rotate it to face the camera,
  // and scale it to maintain constant visual size.
  void SetReticleTransform(Entity reticle_entity,
                           const mathfu::vec3& reticle_world_pos,
                           const mathfu::vec3& camera_world_pos);

  // Calculate where the reticle should be based on input (assuming no actual
  // collisions take place).
  mathfu::vec3 CalculateReticleNoHitPosition(Entity reticle_entity,
                                             InputManager::DeviceType device,
                                             const Clock::duration& delta_time);

  // Checks if |collided_entity| has a hover start dead zone. If it does and the
  // reticle is currently within the dead zone, return true. Otherwise, return
  // false.
  bool IsInsideEntityDeadZone(Entity collided_entity) const;
  std::unique_ptr<Reticle> reticle_;
  ComponentPool<ReticleBehaviour> reticle_behaviours_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::ReticleSystem);

#endif  // LULLABY_SYSTEMS_RETICLE_RETICLE_SYSTEM_H_
