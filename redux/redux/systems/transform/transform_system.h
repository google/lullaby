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

#ifndef REDUX_SYSTEMS_TRANSFORM_TRANSFORM_SYSTEM_H_
#define REDUX_SYSTEMS_TRANSFORM_TRANSFORM_SYSTEM_H_

#include <limits>

#include "redux/engines/script/function_binder.h"
#include "redux/modules/base/bits.h"
#include "redux/modules/base/data_table.h"
#include "redux/modules/ecs/system.h"
#include "redux/modules/math/bounds.h"
#include "redux/modules/math/transform.h"
#include "redux/systems/transform/transform_def_generated.h"

namespace redux {

// Provides Entities with position, rotation, scale and box volume and supports
// spatial queries on this data.
class TransformSystem : public System {
 public:
  // Custom user-defined flags to help group Transform Components for spatial
  // queries.
  using TransformFlags = Bits32;

  explicit TransformSystem(Registry* registry);

  void OnRegistryInitialize();

  // Returns the translation of the Entity, or Zero() if the Entity has no
  // translation defined.
  vec3 GetTranslation(Entity entity) const;

  // Sets the translation of the Entity.
  void SetTranslation(Entity entity, const vec3& translation);

  // Returns the rotation of the Entity, or Identity() if the Entity has no
  // rotation defined.
  quat GetRotation(Entity entity) const;

  // Sets the rotation of the Entity.
  void SetRotation(Entity entity, const quat& rotation);

  // Returns the scale of the Entity, or One() if the Entity has no scale
  // defined.
  vec3 GetScale(Entity entity) const;

  // Sets the scale of the Entity.
  void SetScale(Entity entity, const vec3& scale);

  // Returns the transform for the Entity, or the default Transform if the
  // Entity has no transform defined.
  Transform GetTransform(Entity entity) const;

  // Returns the world transform matrix for the Entity, or Identity if the
  // Entity has no transform defined.
  mat4 GetWorldTransformMatrix(Entity entity) const;

  // Sets the transform for the Entity.
  void SetTransform(Entity entity, const Transform& transform);

  void SetTransform(Entity entity, const Transform& transform, void* owner);

  // Locks the Entity's transform to the given owner such that only this owner
  // can modify the Entity's transform.
  void LockTransform(Entity entity, void* owner);
  void UnlockTransform(Entity entity, void* owner);

  // Removes all transform data associated with the Entity.
  void RemoveTransform(Entity entity);

  // Sets a bounding box of the Entity.
  void SetBox(Entity entity, Box box);

  // Returns the bounding box for the Entity, or an empty for if the Entity has
  // no transform data. Note that this box will not have the Entity's scale
  // applied to it (in the same way that it also doesn't apply the Entity's
  // translation or rotation).
  Box GetEntityAlignedBox(Entity entity) const;

  // Returns the bounding box for the Entity, scaled and axis-aligned to the
  // world. Returns an empty box if the Entity has no transform data.
  Box GetWorldAlignedBox(Entity entity) const;

  // Reserves a flag that can be used to tag Entity transforms for improved
  // spatial queries.
  TransformFlags RequestFlag();

  // Releases a flag that had been previously requested.
  void ReleaseFlag(TransformFlags flag);

  // Associates a TransformFlag with the Entity.
  void SetFlag(Entity entity, TransformFlags flag);

  // Removes the TransformFlag from the Entity.
  void ClearFlag(Entity entity, TransformFlags flag);

  // Returns true if the Entity has the given TransformFlag, false otherwise.
  bool HasFlag(Entity entity, TransformFlags flag) const;

  // Sets the Entity's transformation data using the TransformDef.
  void SetFromTransformDef(Entity entity, const TransformDef& def);

 private:
  struct kEntity : DataColumn<Entity> {};
  struct kFlags : DataColumn<Bits32> {};
  struct kTranslation : DataColumn<vec3, &vec3::Zero> {};
  struct kRotation : DataColumn<quat, &quat::Identity> {};
  struct kScale : DataColumn<vec3, &vec3::One> {};
  struct kWorldMatrix : DataColumn<mat4, &mat4::Identity> {};
  struct kLocalBoundingBox : DataColumn<Box> {};
  struct kWorldBoundingBox : DataColumn<Box> {};
  struct kOwner : DataColumn<void*> {};

  // Store transform components data in a structure-of-arrays map to allow for
  // optimized spatial queries along a subset of the data.
  using Transforms =
      DataTable<kEntity, kFlags, kTranslation, kRotation, kScale, kWorldMatrix,
                kLocalBoundingBox, kWorldBoundingBox, kOwner>;

  void OnDestroy(Entity entity) override;
  void UpdateRow(Transforms::Row& data) const;

  FunctionBinder fns_;
  mutable Transforms transforms_;
  Bits32 dirty_flag_ = Bits32(0);
  uint32_t reserved_flags_ = 0;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::TransformSystem);

#endif  // REDUX_SYSTEMS_TRANSFORM_TRANSFORM_SYSTEM_H_
