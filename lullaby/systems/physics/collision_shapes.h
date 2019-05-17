/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef VR_INTERNAL_LULLABY_SRC_SYSTEMS_PHYSICS_COLLISION_SHAPES_H_
#define VR_INTERNAL_LULLABY_SRC_SYSTEMS_PHYSICS_COLLISION_SHAPES_H_

#include <utility>
#include <vector>

#include "btBulletCollisionCommon.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "lullaby/util/make_unique.h"
#include "lullaby/util/math.h"
#include "lullaby/generated/physics_shapes_generated.h"

namespace lull {

/// Common interface for all collision shape representations.
class CollisionShape {
 public:
  virtual ~CollisionShape() {}

  /// Get the single shape that represents this shape (or set of shapes). This
  /// function should never be used outside of constructing a btRigidBody.
  virtual btCollisionShape& GetBtShape() = 0;

  /// Apply the given |scale| as a multiplier of this shapes pre-defined scale
  /// for the owning Entity. Returns true if the overall scale changed, false
  /// otherwise.
  virtual bool ApplyEntityScale(const mathfu::vec3& scale) = 0;

  /// Calculate the local inertia of this shape with the given |mass|.
  btVector3 CalculateLocalInertia(float mass);

  /// Creates the appropriate CollisionShape for the given PhysicsShapePart.
  static std::unique_ptr<CollisionShape> CreateCollisionShape(
      const PhysicsShapePart* part);
};

/// Implementation of a CollisionShape that represents a single shape with no
/// local transforms applied. Shapes with local transforms are encapsulated
/// by CompoundCollisionShape.
class SingleCollisionShape : public CollisionShape {
 public:
  SingleCollisionShape(std::unique_ptr<btCollisionShape> shape,
                       const mathfu::vec3& scale);

  btCollisionShape& GetBtShape() override { return *shape_; }

  bool ApplyEntityScale(const mathfu::vec3& scale) override;

 private:
  std::unique_ptr<btCollisionShape> shape_;
  // Local scale must be stored separately or it will be lost when Entity scale
  // is applied.
  mathfu::vec3 local_scale_;
};

/// Implementation of a CollisionShape that represents one or more shapes with
/// local offsets encapsulated by a compound shape.
class CompoundCollisionShape : public CollisionShape {
 public:
  explicit CompoundCollisionShape(int num_shapes)
      : compound_(MakeUnique<btCompoundShape>(/* dynamic AABB tree= */ true,
                                              num_shapes)) {}

  btCollisionShape& GetBtShape() override { return *compound_; }

  bool ApplyEntityScale(const mathfu::vec3& scale) override;

  /// Add |shape| to this compound collision shape with the given local |sqt|.
  /// This CompoundCollisionShape will take ownership of |shape|.
  void AddSubShape(std::unique_ptr<btCollisionShape> shape, const Sqt& sqt);

 private:
  std::unique_ptr<btCompoundShape> compound_;
  std::vector<std::unique_ptr<btCollisionShape>> shapes_;
};

/// Implementation of a CollisionShape that can match a lull::Aabb.
class AabbCollisionShape : public CollisionShape {
 public:
  explicit AabbCollisionShape();

  btCollisionShape& GetBtShape() override { return *container_; }

  bool ApplyEntityScale(const mathfu::vec3& scale) override;

  /// Update the size and local transform of this shape.
  void UpdateShape(const Aabb& aabb);

 private:
  std::unique_ptr<btCompoundShape> container_;
  std::unique_ptr<btBoxShape> box_;
};

}  // namespace lull

#endif  // VR_INTERNAL_LULLABY_SRC_SYSTEMS_PHYSICS_COLLISION_SHAPES_H_
