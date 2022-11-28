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

#ifndef REDUX_ENGINES_PHYSICS_COLLISION_DATA_H_
#define REDUX_ENGINES_PHYSICS_COLLISION_DATA_H_

#include <memory>
#include <vector>

#include "redux/modules/base/data_container.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {

// Collision shape represented using a convex triangle mesh.
struct CollisionMesh {
  DataContainer vertices;  // `vec3` point array.
  DataContainer indices;   // `int` indices array, 3 indices for each triangle.
};

// Collision data from which a CollisionShape can be created.
//
// A CollisionShape itself may be composed of multiple parts, each of which
// is either a primitive shape (e.g. box, sphere, etc.) or a CollisionMesh.
class CollisionData {
 public:
  CollisionData() = default;

  enum PartType {
    kNone,
    kBox,
    kSphere,
    kMesh,
  };

  // Adds a sphere shape part to the collision data.
  void AddSphere(const vec3& position, float radius);

  // Adds a box shape part to the collision data.
  void AddBox(const vec3& position, const quat& rotation,
              const vec3& half_extents);

  // Adds a mesh shape part to the collision data.
  void AddMesh(const vec3& position, const quat& rotation, CollisionMesh mesh);

  // Returns the total number of parts in the collision data.
  size_t GetNumParts() const { return shape_parts_.size(); }

  // Returns the position of the shape part at the given `index` relative to the
  // collision data's origin.
  const vec3& GetPosition(size_t index) const;

  // Returns the rotation of the shape part at the given `index` relative to the
  // collision data's origin.
  const quat& GetRotation(size_t index) const;

  // Returns the PartType of the shape part at the given `index`.
  PartType GetPartType(size_t index) const;

  // Returns the box half extents of the part at the `index`. Will check-fail
  // if the part is not a box.
  vec3 GetBoxHalfExtents(size_t index) const;

  // Returns the sphere radius of the part at the `index`. Will check-fail if
  // the part is not a sphere.
  float GetSphereRadius(size_t index) const;

  // Returns the collision mesh of the part at the `index`. Will check-fail if
  // the part is not a mesh.
  const CollisionMesh& GetCollisionMesh(size_t index) const;

 private:
  struct ShapePart {
    PartType type = kNone;
    // Some values may not be set depending on the type.
    vec3 position = vec3::Zero();
    quat rotation = quat::Identity();
    vec3 extents = vec3::Zero();
    CollisionMesh mesh;
  };

  std::vector<ShapePart> shape_parts_;
};

using CollisionDataPtr = std::shared_ptr<CollisionData>;

}  // namespace redux

#endif  // REDUX_ENGINES_PHYSICS_COLLISION_DATA_H_
