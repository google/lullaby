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

#ifndef LULLABY_UTIL_INTERSECTIONS_H_
#define LULLABY_UTIL_INTERSECTIONS_H_

#include "lullaby/util/math.h"

namespace lull {

// Tests for intersection between a ray and a plane. |plane_normal| and
// |ray_direction| must be normalized (length = 1.0). If |intersection_position|
// is supplied, the position of the intersection will be written to it on
// success.
bool IntersectRayPlane(const mathfu::vec3& plane_normal, float plane_offset,
                       const mathfu::vec3& ray_position,
                       const mathfu::vec3& ray_direction,
                       mathfu::vec3* intersection_position = nullptr);

}  // namespace lull

#endif  // LULLABY_UTIL_INTERSECTIONS_H_
