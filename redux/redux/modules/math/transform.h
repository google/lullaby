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

#ifndef REDUX_MODULES_MATH_TRANSFORM_H_
#define REDUX_MODULES_MATH_TRANSFORM_H_

#include "redux/modules/base/hash.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Compound math type that consists of a translation, rotation, and scale.
struct Transform {
  Transform() = default;

  Transform(const vec3& translation, const quat& rotation, const vec3& scale)
      : translation(translation), rotation(rotation), scale(scale) {}

  explicit Transform(const mat4& mat);

  explicit Transform(const mat34& mat);

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&translation, ConstHash("translation"));
    archive(&rotation, ConstHash("rotation"));
    archive(&scale, ConstHash("scale"));
  }

  vec3 translation = vec3::Zero();
  quat rotation = quat::Identity();
  vec3 scale = vec3::One();
};

inline Transform::Transform(const mat4& mat) {
  const vec3 c0 = mat.Column(0).xyz();
  const vec3 c1 = mat.Column(1).xyz();
  const vec3 c2 = mat.Column(2).xyz();
  const float scale_x = c0.Length();
  const float scale_y = c1.Length();
  const float scale_z = c2.Length();
  const float inv_x = 1.0f / scale_x;
  const float inv_y = 1.0f / scale_y;
  const float inv_z = 1.0f / scale_z;
  // clang-format off
  const mat3 rot(c0.x * inv_x, c1.x * inv_y, c2.x * inv_z,
                 c0.y * inv_x, c1.y * inv_y, c2.y * inv_z,
                 c0.z * inv_x, c1.z * inv_y, c2.z * inv_z);
  // clang-format on
  translation = mat.Column(3).xyz();
  rotation = QuaternionFromRotationMatrix(rot);
  scale = vec3(scale_x, scale_y, scale_z);
}

inline Transform::Transform(const mat34& mat) {
  const vec3 c0 = mat.Column(0);
  const vec3 c1 = mat.Column(1);
  const vec3 c2 = mat.Column(2);
  const float scale_x = c0.Length();
  const float scale_y = c1.Length();
  const float scale_z = c2.Length();
  const float inv_x = 1.f / scale_x;
  const float inv_y = 1.f / scale_y;
  const float inv_z = 1.f / scale_z;
  // clang-format off
  const mat3 rot(c0.x * inv_x, c1.x * inv_y, c2.x * inv_z,
                 c0.y * inv_x, c1.y * inv_y, c2.y * inv_z,
                 c0.z * inv_x, c1.z * inv_y, c2.z * inv_z);
  // clang-format on
  translation = mat.Column(3);
  rotation = QuaternionFromRotationMatrix(rot);
  scale = vec3(scale_x, scale_y, scale_z);
}

// Converts a position, rotation, and scale into a mat4.
inline mat4 TransformMatrix(const vec3& position, const quat& rotation,
                            const vec3& scale) {
  const mat3 rm = RotationMatrixFromQuaternion(rotation);
  const vec4 c0(scale.x * rm(0, 0), scale.x * rm(1, 0), scale.x * rm(2, 0), 0);
  const vec4 c1(scale.y * rm(0, 1), scale.y * rm(1, 1), scale.y * rm(2, 1), 0);
  const vec4 c2(scale.z * rm(0, 2), scale.z * rm(1, 2), scale.z * rm(2, 2), 0);
  const vec4 c3(position, 1);
  return MatrixFromColumns(c0, c1, c2, c3);
}

// Converts a Transform into a mat4.
inline mat4 TransformMatrix(const Transform& transform) {
  return TransformMatrix(transform.translation, transform.rotation,
                         transform.scale);
}

}  // namespace redux

REDUX_SETUP_TYPEID(redux::Transform);

#endif  // REDUX_MODULES_MATH_TRANSFORM_H_
