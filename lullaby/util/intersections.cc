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

#include "lullaby/util/intersections.h"

namespace lull {

bool IntersectRayPlane(const mathfu::vec3& plane_normal, float plane_offset,
                       const mathfu::vec3& ray_position,
                       const mathfu::vec3& ray_direction,
                       mathfu::vec3* intersection_position) {
  DCHECK(AreNearlyEqual(plane_normal.Length(), 1.0f));
  DCHECK(AreNearlyEqual(ray_direction.Length(), 1.0f));

  const float denom = mathfu::vec3::DotProduct(plane_normal, ray_direction);
  if (IsNearlyZero(denom)) {
    // Ray and plane are parallel, collision cannot happen.
    return false;
  }

  const float intersection_distance =
      (plane_offset - mathfu::vec3::DotProduct(ray_position, plane_normal)) /
      denom;
  if (intersection_distance < 0.0f) {
    // Plane is behind the ray.
    return false;
  }

  if (intersection_position) {
    *intersection_position =
        ray_position + ray_direction * intersection_distance;
  }

  return true;
}

}  // namespace lull
