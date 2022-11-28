/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_MODULES_GRAPHICS_VERTEX_UTILS_H_
#define REDUX_MODULES_GRAPHICS_VERTEX_UTILS_H_

#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Returns an arbitrary vertex tangent for a given vertex normal.
inline vec4 CalculateTangent(const vec3& normal) {
  const vec3 axis = (std::fabs(Dot(normal, vec3::XAxis())) < 0.99f)
                        ? vec3::XAxis()
                        : vec3::YAxis();
  return vec4(Normalized(Cross(normal, axis)), 1.f);
}

// Returns the orientation of the vertex with the given normal and tangent.
// The orientation is a quaternion encoded as a vec4.
inline vec4 CalculateOrientation(const vec3& normal, const vec4& tangent) {
  const vec3 n = normal.Normalized();
  const vec3 t = tangent.xyz().Normalized();
  const vec3 b = Cross(n, t).Normalized();
  const mat3 m(t.x, b.x, n.x, t.y, b.y, n.y, t.z, b.z, n.z);
  quat q = QuaternionFromRotationMatrix(m).Normalized();
  // Align the sign bit of the orientation scalar to our handedness.
  if (std::signbit(tangent.w) != std::signbit(q.w)) {
    q = quat(-q.xyz(), -q.w);
  }
  return vec4(q.xyz(), q.w);
}

// Calculates and returns the orientation of a vertex given just its normal.
// An arbitrary tangent is calculated for the normal from which the orientation
// is derived.
inline vec4 CalculateOrientation(const vec3& normal) {
  const vec4 tangent = CalculateTangent(normal);
  return CalculateOrientation(normal, tangent);
}

}  // namespace redux

#endif  // REDUX_MODULES_GRAPHICS_VERTEX_UTILS_H_
