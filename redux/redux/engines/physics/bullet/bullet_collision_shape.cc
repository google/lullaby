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

#include "redux/engines/physics/bullet/bullet_collision_shape.h"

#include <utility>

#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h"
#include "BulletCollision/CollisionShapes/btUniformScalingShape.h"
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "redux/engines/physics/bullet/bullet_utils.h"

namespace redux {

BulletCollisionShape::BulletCollisionShape(std::shared_ptr<CollisionData> data)
    : data_(std::move(data)) {
  CHECK(data_);

  const int num_shapes = data_->GetNumParts();
  CHECK_GT(num_shapes, 0);

  bool single_shape_at_origin = true;
  if (num_shapes > 1) {
    single_shape_at_origin = false;
  } else if (data_->GetPosition(0) != vec3::Zero()) {
    single_shape_at_origin = false;
  } else if (data_->GetRotation(0) != quat::Identity()) {
    single_shape_at_origin = false;
  }

  btCompoundShape* compound = nullptr;
  if (!single_shape_at_origin) {
    compound = new btCompoundShape(true, num_shapes);
    shapes_.emplace_back(compound);
    bt_shape_ = compound;
  }

  for (int i = 0; i < num_shapes; ++i) {
    btCollisionShape* shape = nullptr;
    const auto type = data_->GetPartType(i);
    switch (type) {
      case CollisionData::kBox:
        shape = AddBoxShape(data_->GetBoxHalfExtents(i));
        break;
      case CollisionData::kSphere:
        shape = AddSphereShape(data_->GetSphereRadius(i));
        break;
      case CollisionData::kMesh:
        shape = AddMeshShape(data_->GetCollisionMesh(i));
        break;
      case CollisionData::kNone:
        LOG(FATAL) << "No shape.";
        break;
    }

    if (single_shape_at_origin) {
      bt_shape_ = shape;
    } else {
      const btVector3 bt_position = ToBullet(data_->GetPosition(i));
      const btQuaternion bt_rotation = ToBullet(data_->GetRotation(i));
      const btTransform bt_transform(bt_rotation, bt_position);
      compound->addChildShape(bt_transform, shape);
    }
  }
  if (compound) {
    compound->recalculateLocalAabb();
  }
}

btCollisionShape* BulletCollisionShape::AddBoxShape(const vec3& half_extents) {
  btVector3 bt_extents(half_extents.x, half_extents.y, half_extents.z);
  auto ptr = new btBoxShape(bt_extents);
  shapes_.emplace_back(ptr);
  return ptr;
}

btCollisionShape* BulletCollisionShape::AddSphereShape(float radius) {
  auto ptr = new btSphereShape(radius);
  shapes_.emplace_back(ptr);
  return ptr;
}

btCollisionShape* BulletCollisionShape::AddMeshShape(
    const CollisionMesh& mesh) {
  static constexpr size_t kVerticesPerTriangle = 3;
  static constexpr size_t kFloatsPerVertex = 3;
  static constexpr size_t kTriangleStride = kVerticesPerTriangle * sizeof(int);
  static constexpr size_t kVertexStride = kFloatsPerVertex * sizeof(float);

  const int* triangles = reinterpret_cast<const int*>(mesh.indices.GetBytes());
  const size_t num_triangles = mesh.indices.GetNumBytes() / kTriangleStride;
  const float* vertices =
      reinterpret_cast<const float*>(mesh.vertices.GetBytes());
  const size_t num_vertices = mesh.indices.GetNumBytes() / kVertexStride;

  btTriangleIndexVertexArray* vertex_array = new btTriangleIndexVertexArray(
      num_triangles, const_cast<int*>(triangles), kTriangleStride, num_vertices,
      const_cast<float*>(vertices), kVertexStride);
  vertices_.emplace_back(vertex_array);

  auto ptr = new btGImpactMeshShape(vertex_array);
  ptr->postUpdate();
  ptr->updateBound();
  shapes_.emplace_back(ptr);
  return ptr;
}
}  // namespace redux
