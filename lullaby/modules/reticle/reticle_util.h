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

#ifndef LULLABY_MODULES_RETICLE_RETICLE_UTIL_H_
#define LULLABY_MODULES_RETICLE_RETICLE_UTIL_H_

#include "lullaby/modules/ecs/entity.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/util/math.h"
#include "lullaby/util/registry.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Computes the intersection of a ray with the x-y plane (at z=0) of an
// Aabb. The collision point is returned relative to the Aabb size, where
// (0,0) represents the bottom left corner, and (1,1) represents the top right.
// The result is placed in *intersection_point and the function returns true if
// the ray intersects the plane. If the ray is nearly parallel to the plane,
// then the function returns false.
bool ComputeRayAabbXyIntersectionPoint(const Ray& ray, const Aabb& aabb,
                                       mathfu::vec2* intersection_point);

// Computes the intersection of the reticle (controlled by either the Controller
// or Hmd) with an entity's Aabb in the x-y plane where z=0.
// The collision point is returned relative to the Aabb size, where (0,0)
// represents the bottom left corner and (1,1) represents the top right.
// The result is placed in *intersection_point and the function returns true if
// the ray intersects the plane. If the ray is nearly parallel to the plane,
// then the function returns false.
bool GetReticleIntersectionPoint(Registry* registry, Entity entity,
                                 mathfu::vec2* intersection_point);


/// Computes the relative mathfu::vec3 that was touched by the reticle on the
/// passed in Entity.  Returns true if the registry contains a TransformSystem
/// and is passed non-null Entities for the reticle and target.
bool GetReticleRelativeHitPoint(Registry* registry,
                                Entity reticle,
                                Entity entity,
                                mathfu::vec3* out_vec3);

// Gets the current sqt of the device, using its position and orientation.
bool GetSqtForDevice(Registry* registry, InputManager::DeviceType device_type,
                     Sqt* out_sqt);

// Constructs an adjusted sqt from an existing sqt so that the direction points
// to the reticle's position.
Sqt AdjustSqtForReticle(Registry* registry, const Sqt& sqt);

}  // namespace lull

#endif  // LULLABY_MODULES_RETICLE_RETICLE_UTIL_H_
