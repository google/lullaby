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

#ifndef REDUX_MODULES_MATH_RAY_H_
#define REDUX_MODULES_MATH_RAY_H_

#include "redux/modules/base/hash.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/vector.h"

namespace redux {

// A ray (starting point + direction) in 3D space. Can also be used to represent
// a Line.
struct Ray {
  Ray() : origin(vec3::Zero()), direction(-vec3::ZAxis()) {}

  Ray(const vec3& origin, const vec3& direction)
      : origin(origin), direction(direction) {}

  // Returns the point at parametric distance `t` along the Ray.
  vec3 GetPointAt(float t) const { return origin + t * direction; }

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&origin, ConstHash("origin"));
    archive(&direction, ConstHash("direction"));
  }

  vec3 origin;
  vec3 direction;  // user is responsible for ensuring this is of unit length
};

// Transforms a ray by the given matrix.
inline Ray TransformRay(const mat4& mat, const Ray& ray) {
  // Extend ray.direction with a fourth homogeneous coordinate of 0 in order to
  // perform a vector-like transformation rather than a point-like
  // transformation.
  const vec4 ray_dir = vec4(ray.direction, 0.0f);
  return Ray(mat * ray.origin, (mat * ray_dir).xyz());
}

// Finds the point on the ray nearest to given |point|.
inline vec3 ProjectPointOntoRay(const vec3& point, const Ray& ray) {
  const float distance = (point - ray.origin).Dot(ray.direction);
  return distance > 0.0f ? ray.GetPointAt(distance) : ray.origin;
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::Ray);

#endif  // REDUX_MODULES_MATH_RAY_H_
