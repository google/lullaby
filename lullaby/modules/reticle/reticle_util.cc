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

#include "lullaby/modules/reticle/reticle_util.h"

#include "lullaby/modules/reticle/reticle_provider.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

bool ComputeRayAabbXyIntersectionPoint(const Ray& ray, const Aabb& aabb,
                                       mathfu::vec2* intersection_point) {
  // If the ray is negative or only barely positive, then the ray is pointing
  // away from the plane.
  if (std::abs(ray.direction.z) < kDefaultEpsilon) {
    return false;
  }

  if (!intersection_point) {
    LOG(ERROR) <<
        "ComputeRayAabbXyIntersectionPoint without valid mathfu::vec2*.";
    return false;
  }

  // If the max and min are the same then the intersection point is 0,0.
  const mathfu::vec3 aabb_dif = aabb.max - aabb.min;
  if (aabb_dif.LengthSquared() < kDefaultEpsilon) {
    *intersection_point = mathfu::kZeros2f;
    return true;
  }

  const float lambda = -ray.origin.z / ray.direction.z;
  mathfu::vec3 delta = ray.origin + ray.direction * lambda;
  mathfu::vec3 relative_delta = (delta - aabb.min) / aabb_dif;
  *intersection_point = mathfu::vec2(relative_delta.x, relative_delta.y);
  return true;
}

bool GetReticleIntersectionPoint(Registry* registry, Entity entity,
                                 mathfu::vec2* intersection_point) {
  if (!registry) {
    LOG(DFATAL) << "GetReticleIntersectionPoint called without valid registry.";
    return false;
  }

  if (!intersection_point) {
    LOG(DFATAL) <<
        "GetReticleIntersectionPoint called without valid mathfu::vec2 *.";
    return false;
  }

  auto* transform_system = registry->Get<TransformSystem>();
  if (!transform_system) {
    LOG(DFATAL) << "Transform system missing from registry.";
    return false;
  }

  const mathfu::mat4* world_mat =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (!world_mat) {
    LOG(ERROR) << "Failed to get world matrix from entity matrix.";
    return false;
  }

  const Aabb* aabb = transform_system->GetAabb(entity);
  if (!aabb) {
    LOG(ERROR) << "Failed to get aabb from entity.";
    return false;
  }
  auto* reticle_provider = registry->Get<ReticleProvider>();
  if (!reticle_provider) {
    return false;
  }
  const Ray collision_ray = reticle_provider->GetCollisionRay();
  const Ray local_gaze = TransformRay(world_mat->Inverse(), collision_ray);
  return ComputeRayAabbXyIntersectionPoint(local_gaze, *aabb,
                                           intersection_point);
}

bool GetReticleRelativeHitPoint(Registry* registry,
                                Entity reticle,
                                Entity entity,
                                mathfu::vec3* out_vec3) {
  if (!out_vec3) {
    LOG(DFATAL) <<
        "GetReticleRelativeHitPoint called without valid mathfu::vec3 *.";
    return false;
  }

  auto* transform_system = registry->Get<TransformSystem>();
  if (!transform_system) {
    LOG(DFATAL) << "Transform system missing from registry.";
    return false;
  }

  const mathfu::mat4* entity_world_mat =
      transform_system->GetWorldFromEntityMatrix(entity);
  if (!entity_world_mat) {
    LOG(ERROR) << "Failed to get world matrix from entity matrix.";
    return false;
  }

  const mathfu::mat4* reticle_world_mat =
      transform_system->GetWorldFromEntityMatrix(reticle);
  if (!reticle_world_mat) {
    LOG(ERROR) << "Failed to get world matrix from reticle matrix.";
    return false;
  }

  const mathfu::mat4 relative_mat = CalculateRelativeMatrix(*entity_world_mat,
                                                            *reticle_world_mat);

  *out_vec3 = relative_mat.TranslationVector3D();

  return true;
}

bool GetSqtForDevice(Registry* registry, InputManager::DeviceType device_type,
                     Sqt* out_sqt) {
  if (!registry) {
    LOG(DFATAL) << "GetSqtForDevice called without valid registry.";
    return false;
  }

  if (!out_sqt) {
    LOG(DFATAL) << "GetSqtForDevice called without valid sqt pointer.";
    return false;
  }

  auto* input = registry->Get<InputManager>();

  if (!input) {
    LOG(DFATAL) << "Input manager missing from registry.";
    return false;
  }
  const bool input_connected = input->IsConnected(device_type);
  if (!input_connected) {
    return false;
  }

  if (input->HasRotationDof(device_type)) {
    out_sqt->rotation = input->GetDofRotation(device_type);
  }

  if (input->HasPositionDof(device_type)) {
    out_sqt->translation = input->GetDofPosition(device_type);
  }

  return true;
}

Sqt AdjustSqtForReticle(Registry* registry, const Sqt& sqt) {
  if (!registry) {
    LOG(DFATAL) << "AdjustSqtForReticle called without valid registry.";
    return sqt;
  }

  // Set sqt's offset angle to be the angle between the forward vector
  // and the direction vector to the reticle.
  auto* reticle_provider = registry->Get<ReticleProvider>();
  if (!reticle_provider) {
    return sqt;
  }

  auto* transform_system = registry->Get<TransformSystem>();
  if (!transform_system) {
    LOG(DFATAL) << "Transform system missing from registry.";
    return sqt;
  }

  const Sqt* reticle_sqt =
      transform_system->GetSqt(reticle_provider->GetReticleEntity());
  if (!reticle_sqt) {
    return sqt;
  }

  Sqt adjusted_sqt = sqt;
  mathfu::quat offset_angle = mathfu::quat::RotateFromTo(
      sqt.rotation * -mathfu::kAxisZ3f,
      (reticle_sqt->translation - sqt.translation).Normalized());

  adjusted_sqt.rotation = offset_angle * sqt.rotation;

  return adjusted_sqt;
}

}  // namespace lull
