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

#include "redux/engines/physics/collision_data.h"

#include <utility>

namespace redux {

void CollisionData::AddBox(const vec3& position, const quat& rotation,
                           const vec3& half_extents) {
  auto& shape = shape_parts_.emplace_back();
  shape.type = kBox;
  shape.position = position;
  shape.rotation = rotation;
  shape.extents = half_extents;
}

void CollisionData::AddSphere(const vec3& position, float radius) {
  auto& shape = shape_parts_.emplace_back();
  shape.type = kSphere;
  shape.position = position;
  shape.extents = vec3(radius);
}

void CollisionData::AddMesh(const vec3& position, const quat& rotation,
                            CollisionMesh mesh) {
  auto& shape = shape_parts_.emplace_back();
  shape.type = kMesh;
  shape.position = position;
  shape.rotation = rotation;
  shape.mesh = std::move(mesh);
}

CollisionData::PartType CollisionData::GetPartType(size_t index) const {
  CHECK(index < shape_parts_.size());
  return shape_parts_[index].type;
}

const vec3& CollisionData::GetPosition(size_t index) const {
  CHECK(index < shape_parts_.size());
  return shape_parts_[index].position;
}

const quat& CollisionData::GetRotation(size_t index) const {
  CHECK(index < shape_parts_.size());
  return shape_parts_[index].rotation;
}

vec3 CollisionData::GetBoxHalfExtents(size_t index) const {
  CHECK(index < shape_parts_.size());
  CHECK(shape_parts_[index].type == kBox);
  return shape_parts_[index].extents;
}

float CollisionData::GetSphereRadius(size_t index) const {
  CHECK(index < shape_parts_.size());
  CHECK(shape_parts_[index].type == kSphere);
  return shape_parts_[index].extents.x;
}

const CollisionMesh& CollisionData::GetCollisionMesh(size_t index) const {
  CHECK(index < shape_parts_.size());
  CHECK(shape_parts_[index].type == kMesh);
  return shape_parts_[index].mesh;
}

}  // namespace redux
