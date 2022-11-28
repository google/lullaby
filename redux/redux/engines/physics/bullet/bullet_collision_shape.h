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

#ifndef REDUX_ENGINES_PHYSICS_BULLET_BULLET_COLLISION_SHAPE_H_
#define REDUX_ENGINES_PHYSICS_BULLET_BULLET_COLLISION_SHAPE_H_

#include <functional>
#include <vector>

#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h"
#include "redux/engines/physics/collision_data.h"
#include "redux/engines/physics/collision_shape.h"

namespace redux {

class BulletCollisionShape : public CollisionShape {
 public:
  explicit BulletCollisionShape(std::shared_ptr<CollisionData> data);

  btCollisionShape* GetUnderlyingBtCollisionShape() { return bt_shape_; }

 private:
  btCollisionShape* AddBoxShape(const vec3& half_extents);
  btCollisionShape* AddSphereShape(float radius);
  btCollisionShape* AddMeshShape(const CollisionMesh& mesh);

  btCollisionShape* bt_shape_ = nullptr;
  std::shared_ptr<CollisionData> data_;
  std::vector<std::unique_ptr<btCollisionShape>> shapes_;
  std::vector<std::unique_ptr<btTriangleIndexVertexArray>> vertices_;
};

inline BulletCollisionShape* Upcast(CollisionShape* ptr) {
  return static_cast<BulletCollisionShape*>(ptr);
}

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_BULLET_BULLET_COLLISION_SHAPE_H_
