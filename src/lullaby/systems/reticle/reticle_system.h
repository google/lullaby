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

#include "mathfu/glsl_mappings.h"
#include "lullaby/generated/reticle_behaviour_def_generated.h"
#include "lullaby/generated/reticle_def_generated.h"
#include "lullaby/base/component.h"
#include "lullaby/base/input_manager.h"
#include "lullaby/base/system.h"
#include "lullaby/util/clock.h"
#include "lullaby/util/math.h"

namespace lull {

// The ReticleSystem updates the on-screen position of the reticle based on ray
// collision results.  It is also responsible for firing off reticle events
// (eg. HoverStart, HoverStop, Click, etc.).
class ReticleSystem : public System {
 public:
  static constexpr float kDefaultNoHitDistance = 2.0f;

  explicit ReticleSystem(Registry* registry);

  void Initialize() override;

  void Create(Entity entity, HashValue type, const Def* def) override;

  void PostCreateInit(Entity entity, HashValue type, const Def* def) override;

  void Destroy(Entity entity) override;

  void AdvanceFrame(const Clock::duration& delta_time);

  // Sets the distance for the reticle when there is no collision.
  void SetNoHitDistance(float distance);

  // Gets the current reticle entity.
  Entity GetReticle() const;

  // Gets the current target under the reticle.
  Entity GetTarget() const;

  // Gets the ray representing the target direction for the reticle.
  Ray GetCollisionRay() const;

  // Returns the type of the device currently used to position the reticle.
  InputManager::DeviceType GetActiveDevice() const;

  // Returns the reticle ergo angle offset.
  float GetReticleErgoAngleOffset() const;

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
    bool use_eye_collision_ray = false;
    std::vector<InputManager::DeviceType> device_preference;
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
