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

#include "lullaby/systems/physics/bullet_utils.h"

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/util/make_unique.h"

namespace lull {

bool MatrixAlmostOrthogonal(const mathfu::mat3& m, float tolerance) {
  for (int col1 = 0; col1 < 3; ++col1) {
    auto column = m.GetColumn(col1);
    // Test for pairwise orthogonality of column vectors.
    for (int col2 = col1 + 1; col2 < 3; ++col2) {
      if (mathfu::vec3::DotProduct(column, m.GetColumn(col2)) > tolerance) {
        return false;
      }
    }
    // Test for unit length.
    if (std::fabs(column.LengthSquared() - 1.f) > tolerance) {
      return false;
    }
  }
  return true;
}

Sqt GetShapeSqt(const PhysicsShapePart* part) {
  Sqt sqt;
  MathfuVec3FromFbVec3(part->translation(), &sqt.translation);
  MathfuQuatFromFbVec3(part->rotation(), &sqt.rotation);
  MathfuVec3FromFbVec3(part->scale(), &sqt.scale);
  return sqt;
}

std::unique_ptr<btCollisionShape> CreateCollisionShape(
    const PhysicsShapePart* part) {
  std::unique_ptr<btCollisionShape> shape;
  const auto type = part->shape_type();
  const void* shape_data = part->shape();

  if (type == PhysicsShapePrimitive_PhysicsBoxShape) {
    const auto* box = static_cast<const PhysicsBoxShape*>(shape_data);
    mathfu::vec3 half_dimensions;
    MathfuVec3FromFbVec3(box->half_dimensions(), &half_dimensions);

    shape = MakeUnique<btBoxShape>(BtVectorFromMathfu(half_dimensions));
  } else if (type == PhysicsShapePrimitive_PhysicsSphereShape) {
    const auto* sphere = static_cast<const PhysicsSphereShape*>(shape_data);
    const float radius = sphere->radius();
    btVector3 position(0.f, 0.f, 0.f);

    // The btSphereShape only supports uniform scale, but the btMultiSphereShape
    // supports ellipsoids. Because the scale of this shape is affected by the
    // Entity's scale (which may change at any time), always use a
    // btMultiSphereShape.
    // TODO(b/64492155): add a "PhysicsUniformSphereShape" to handle this case.
    shape = MakeUnique<btMultiSphereShape>(&position, &radius, 1);
  } else {
    LOG(DFATAL) << "Unsupported shape type";
  }
  return shape;
}

}  // namespace lull
