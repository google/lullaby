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

#ifndef REDUX_ENGINES_PHYSICS_BULLET_BULLET_UTILS_H_
#define REDUX_ENGINES_PHYSICS_BULLET_BULLET_UTILS_H_

#include "absl/base/casts.h"
#include "LinearMath/btQuaternion.h"
#include "LinearMath/btTransform.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/transform.h"
#include "redux/modules/math/vector.h"

namespace redux {

inline btVector3 ToBullet(const vec3& in) {
  return btVector3(in.x, in.y, in.z);
}

inline vec3 FromBullet(const btVector3& in) {
  return vec3(in.x(), in.y(), in.z());
}

inline btQuaternion ToBullet(const quat& in) {
  return btQuaternion(in.x, in.y, in.z, in.w);
}

inline quat FromBullet(const btQuaternion& in) {
  return quat(in.x(), in.y(), in.z(), in.w());
}

inline btTransform ToBullet(const Transform& in) {
  return btTransform(ToBullet(in.rotation), ToBullet(in.translation));
}

inline int EntityToBulletUserIndex(Entity entity) {
  return absl::bit_cast<int>(entity.get());
}

inline Entity EntityFromBulletUserIndex(int index) {
  return Entity(absl::bit_cast<Entity::Rep>(index));
}

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_BULLET_BULLET_UTILS_H_
