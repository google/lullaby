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

#ifndef REDUX_SYSTEMS_CONSTRAINT_CONSTRAINT_SYSTEM_H_
#define REDUX_SYSTEMS_CONSTRAINT_CONSTRAINT_SYSTEM_H_

#include <functional>
#include <utility>

#include "absl/types/span.h"
#include "redux/engines/script/function_binder.h"
#include "redux/modules/base/data_table.h"
#include "redux/modules/ecs/system.h"
#include "redux/systems/dispatcher/dispatcher_system.h"
#include "redux/systems/rig/rig_system.h"
#include "redux/systems/transform/transform_system.h"

namespace redux {

// Constrains the Transform and Enabled-state of an Entity to another Entity.
class ConstraintSystem : public System {
 public:
  explicit ConstraintSystem(Registry* registry);

  void OnRegistryInitialize();

  struct AttachParams {
    // An additional transform applied between the parent and child.
    Transform local_offset;

    // The bone of the parent to use for constraint.
    HashValue parent_bone;

    // The bone of the child to use for constraint.
    HashValue child_bone;

    // Whether or not to apply the parent's scale to the child.
    bool ignore_parent_scale = true;
  };

  // Attaches a child to a parent such that the child's transform and lifecycle
  // (enable, disable, and destroy) are bound to the parents.
  void AttachChild(Entity parent, Entity child, const AttachParams& params);

  // Detaches a child from its parent.
  void DetachFromParent(Entity child);

  // Returns the immediate parent of an Entity, or kNullEntity if the given
  // Entity has no parent.
  Entity GetParent(Entity child) const;

  // Returns the top-most parent of an Entity (i.e. recursively traverses "up"
  // the hierarchy until it reaches the top). Returns the Entity itself if it
  // is the root.
  Entity GetRoot(Entity entity) const;

  // Returns true if `ancestor` is an ancesetor of `entity`.
  bool IsAncestorOf(Entity ancestor, Entity entity) const;

  // Iterates over all "child" Entities, updating their transforms based on
  // their parents and attachment properties.
  void UpdateTransforms();

 private:
  struct kEntity : DataColumn<Entity> {};
  struct kParent : DataColumn<Entity> {};
  struct kFirstChild : DataColumn<Entity> {};
  struct kNextSibling : DataColumn<Entity> {};
  struct kPrevSibling : DataColumn<Entity> {};
  struct kOffset : DataColumn<Transform> {};
  struct kBone : DataColumn<HashValue> {};
  struct kParentBone : DataColumn<HashValue> {};
  struct kIgnoreParentScale : DataColumn<uint8_t> {};

  using Constraints =
      DataTable<kEntity, kParent, kFirstChild, kNextSibling, kPrevSibling,
                kOffset, kBone, kParentBone, kIgnoreParentScale>;

  enum EnableState {};

  void OnEnable(Entity entity) override;
  void OnDisable(Entity entity) override;
  void OnDestroy(Entity entity) override;

  std::pair<Entity, bool> TryCreateLink(Entity parent, Entity child,
                                        const AttachParams& params);
  std::pair<Entity, bool> TryRemoveLink(Entity child, bool preserve = true);

  void SetEnabled(Entity entity, bool enable);
  void RefreshEnableState(Entity entity);
  void Notify(Entity child, Entity old_parent, Entity new_parent,
              bool on_destroy);

  Transform GetBoneTransform(Entity entity, HashValue bone) const;

  void ApplyConstraint(const Constraints::Row& row);

  template <typename Fn>
  void ForEachRowInHierarchy(Entity root, const Fn& fn);

  FunctionBinder fns_;
  Constraints constraints_;

  RigSystem* rig_system_ = nullptr;
  TransformSystem* transform_system_ = nullptr;
  DispatcherSystem* dispatcher_system_ = nullptr;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::ConstraintSystem);

#endif  // REDUX_SYSTEMS_CONSTRAINT_CONSTRAINT_SYSTEM_H_
