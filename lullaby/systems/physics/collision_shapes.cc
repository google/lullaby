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

#include "lullaby/systems/physics/collision_shapes.h"

#include "lullaby/systems/physics/bullet_utils.h"

namespace lull {

/// Common CollisionShape functions.

btVector3 CollisionShape::CalculateLocalInertia(float mass) {
  btVector3 local_inertia(0.f, 0.f, 0.f);
  btCollisionShape& shape = GetBtShape();
  shape.calculateLocalInertia(mass, local_inertia);
  return local_inertia;
}

std::unique_ptr<CollisionShape> CollisionShape::CreateCollisionShape(
    const PhysicsShapePart* part) {
  const Sqt shape_sqt = GetShapeSqt(part);
  std::unique_ptr<btCollisionShape> bt_shape = CreateBtShape(part);

  // If no local transforms are applied, make this shape the primary shape and
  // avoid using a compound shape altogether.
  if (shape_sqt.translation == mathfu::kZeros3f &&
      AreNearlyEqual(shape_sqt.rotation, mathfu::quat::identity)) {
    return MakeUnique<SingleCollisionShape>(std::move(bt_shape),
                                            shape_sqt.scale);
  } else {
    // Otherwise, create a compound shape to encapsulate the single shape with
    // a transform.
    auto collision_shape =
        MakeUnique<CompoundCollisionShape>(/* num_shapes= */ 1);
    collision_shape->AddSubShape(std::move(bt_shape), shape_sqt);
    return std::move(collision_shape);
  }
}

/// SingleCollisionShape functions.

SingleCollisionShape::SingleCollisionShape(
    std::unique_ptr<btCollisionShape> shape, const mathfu::vec3& scale)
    : shape_(std::move(shape)), local_scale_(scale) {
  shape_->setLocalScaling(BtVectorFromMathfu(local_scale_));
}

bool SingleCollisionShape::ApplyEntityScale(const mathfu::vec3& scale) {
  // Only reset local scaling if necessary, since it may be an expensive
  // operation.
  const btVector3 new_scale = BtVectorFromMathfu(scale * local_scale_);
  if (new_scale != shape_->getLocalScaling()) {
    shape_->setLocalScaling(new_scale);
    return true;
  }
  return false;
}

/// CompoundCollisionShape functions.

bool CompoundCollisionShape::ApplyEntityScale(const mathfu::vec3& scale) {
  // Only reset local scaling if necessary, since it may be an expensive
  // operation.
  const btVector3 new_scale = BtVectorFromMathfu(scale);
  if (new_scale != compound_->getLocalScaling()) {
    compound_->setLocalScaling(new_scale);
    return true;
  }
  return false;
}

void CompoundCollisionShape::AddSubShape(
    std::unique_ptr<btCollisionShape> shape, const Sqt& sqt) {
  const btTransform transform(BtQuatFromMathfu(sqt.rotation),
                              BtVectorFromMathfu(sqt.translation));
  compound_->addChildShape(transform, shape.get());
  shape->setLocalScaling(BtVectorFromMathfu(sqt.scale));

  shapes_.emplace_back(std::move(shape));
}

/// AabbCollisionShape functions.

AabbCollisionShape::AabbCollisionShape()
    : container_(MakeUnique<btCompoundShape>(/* dynamic AABB tree= */ true,
                                             /* size= */ 1)),
      box_(MakeUnique<btBoxShape>(btVector3(0.5f, 0.5f, 0.5f))) {
  // Create a unit box and place it in a compound to handle asymmetrical AABBs.
  // The transform and scale of the box will be updated by UpdateShape().
  container_->addChildShape(btTransform::getIdentity(), box_.get());
}

bool AabbCollisionShape::ApplyEntityScale(const mathfu::vec3& scale) {
  // Only reset local scaling if necessary, since it may be an expensive
  // operation.
  const btVector3 new_scale = BtVectorFromMathfu(scale);
  if (new_scale != container_->getLocalScaling()) {
    container_->setLocalScaling(new_scale);
    return true;
  }
  return false;
}

void AabbCollisionShape::UpdateShape(const Aabb& aabb) {
  // Match the box's size to the AABB, even if asymmetrical.
  box_->setLocalScaling(BtVectorFromMathfu(aabb.Size()));

  // Locally translate the box within the compound to handle asymmetry.
  const mathfu::vec3 translation = (aabb.min + aabb.max) / 2.f;
  const btTransform transform(btQuaternion::getIdentity(),
                              BtVectorFromMathfu(translation));
  container_->updateChildTransform(0, transform);
}

}  // namespace lull
