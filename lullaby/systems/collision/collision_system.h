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

#ifndef LULLABY_SYSTEMS_COLLISION_COLLISION_SYSTEM_H_
#define LULLABY_SYSTEMS_COLLISION_COLLISION_SYSTEM_H_

#include <unordered_map>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"

namespace lull {

// The CollisionSystem can be used to provide Entities with collision
// information that can be used to for raycast tests.
class CollisionSystem : public System {
 public:
  explicit CollisionSystem(Registry* registry);

  void Initialize() override;

  // Associate collision data with the Entity using the specified Def.
  void Create(Entity entity, HashValue type, const Def* def) override;

  // Disassociate collision from the Entity.
  void Destroy(Entity entity) override;

  // Structure to store the result of collision tests.
  struct CollisionResult {
    Entity entity;
    float distance;
  };

  // Cast the specified |ray| and return the closest Entity that is hit (if any)
  // and the distance to the hit point from the ray's origin.
  CollisionResult CheckForCollision(const Ray& ray);

  // Returns a vector of entities that a point lies within
  std::vector<Entity> CheckForPointCollisions(const mathfu::vec3& point);

  // Disables |entity|'s collision.
  void DisableCollision(Entity entity);

  // Enables |entity|'s collision.
  void EnableCollision(Entity entity);

  // Returns whether or not collision is enabled for |entity|.
  bool IsCollisionEnabled(Entity entity) const;

  // Disables |entity|'s interactivity.
  void DisableInteraction(Entity entity);

  // Enables |entity|'s interactivity.
  void EnableInteraction(Entity entity);

  // Disables |entity|'s default interactivity.
  void DisableDefaultInteraction(Entity entity);

  // Enables |entity|'s default interactivity.
  void EnableDefaultInteraction(Entity entity);

  // Updates the |entity|'s interactivity based on its "default" interactivity.
  void RestoreInteraction(Entity entity);

  // Returns whether or interactivity is enabled for |entity|.
  bool IsInteractionEnabled(Entity entity) const;

  // Disables |entity|'s and its children's interactivity.
  void DisableInteractionDescendants(Entity entity);

  // Restores |entity|'s and its children's interactivity to their "default"
  // interactivity.
  void RestoreInteractionDescendants(Entity entity);

  // Disables |entity|'s clipping, which means all collisions are allowed.
  void DisableClipping(Entity entity);

  // Enables |entity|'s clipping, which will only allow collisions inside the
  // nearest ancestor's bound's aabb if one can be found.
  void EnableClipping(Entity entity);

 private:
  Entity GetContainingBounds(Entity entity) const;
  bool IsCollisionClipped(Entity entity, const mathfu::vec3& point) const;

  TransformSystem* transform_system_;
  TransformSystem::TransformFlags collision_flag_;
  TransformSystem::TransformFlags on_exit_flag_;
  TransformSystem::TransformFlags interaction_flag_;
  TransformSystem::TransformFlags default_interaction_flag_;
  TransformSystem::TransformFlags clip_flag_;
  std::unordered_map<Entity, Aabb> clip_bounds_;

  CollisionSystem(const CollisionSystem&) = delete;
  CollisionSystem& operator=(const CollisionSystem&) = delete;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::CollisionSystem);

#endif  // LULLABY_SYSTEMS_COLLISION_COLLISION_SYSTEM_H_
