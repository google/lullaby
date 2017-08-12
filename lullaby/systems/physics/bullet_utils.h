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

#ifndef LULLABY_SYSTEMS_PHYSICS_BULLET_UTILS_H_
#define LULLABY_SYSTEMS_PHYSICS_BULLET_UTILS_H_

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/physics_shapes_generated.h"

namespace lull {

inline btVector3 BtVectorFromMathfu(const mathfu::vec3& v) {
  return btVector3(v.x, v.y, v.z);
}

inline mathfu::vec3 MathfuVectorFromBt(const btVector3& v) {
  return mathfu::vec3(v.x(), v.y(), v.z());
}

// btQuaternions are represented x-y-z-w, while mathfu::quat are represented
// w-x-y-z. Additionally, Bullet provides no natural way of accessing the raw
// vector value, so the serialization functions below are used. All quaternions
// are normalized, so the raw data works fine.
inline btQuaternion BtQuatFromMathfu(const mathfu::quat& q) {
  btQuaternionFloatData data;
  data.m_floats[0] = q[1];
  data.m_floats[1] = q[2];
  data.m_floats[2] = q[3];
  data.m_floats[3] = q[0];

  btQuaternion output;
  output.deSerializeFloat(data);
  return output;
}

inline mathfu::quat MathfuQuatFromBt(const btQuaternion& q) {
  btQuaternionFloatData data;
  q.serializeFloat(data);
  return mathfu::quat(data.m_floats[3], data.m_floats[0],
                      data.m_floats[1], data.m_floats[2]);
}

bool MatrixAlmostOrthogonal(const mathfu::mat3& m, float tolerance);

// Get the local transform from |part|.
Sqt GetShapeSqt(const PhysicsShapePart* part);

// Create a collision shape from |part|.
std::unique_ptr<btCollisionShape> CreateCollisionShape(
    const PhysicsShapePart* part);

}  // namespace lull

#endif  // LULLABY_SYSTEMS_PHYSICS_BULLET_UTILS_H_
