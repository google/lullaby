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

#include "redux/systems/constraint/constraint_system.h"

#include "redux/modules/base/choreographer.h"
#include "redux/modules/math/transform.h"
#include "redux/systems/constraint/events.h"

namespace redux {

ConstraintSystem::ConstraintSystem(Registry* registry)
    : System(registry), fns_(registry) {
  RegisterDependency<RigSystem>(this);
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<EntityFactory>(this);
}

void ConstraintSystem::OnRegistryInitialize() {
  GetEntityFactory();
  rig_system_ = registry_->Get<RigSystem>();
  transform_system_ = registry_->Get<TransformSystem>();
  dispatcher_system_ = registry_->Get<DispatcherSystem>();

  auto* choreo = registry_->Get<Choreographer>();
  if (choreo) {
    choreo->Add<&ConstraintSystem::UpdateTransforms>(
        Choreographer::Stage::kPostPhysics);
  }
}

Entity ConstraintSystem::GetParent(Entity child) const {
  auto data = constraints_.Find<kParent>(child);
  return data ? *data : kNullEntity;
}

Entity ConstraintSystem::GetRoot(Entity entity) const {
  Entity root = entity;

  auto data = constraints_.Find<kParent>(root);
  while (data && *data != kNullEntity) {
    root = *data;
    data = constraints_.Find<kParent>(root);
  }
  return root;
}

bool ConstraintSystem::IsAncestorOf(Entity ancestor, Entity entity) const {
  auto data = constraints_.Find<kParent>(entity);
  while (data) {
    if (*data == ancestor) {
      return true;
    }
    data = constraints_.Find<kParent>(*data);
  }
  return false;
}

void ConstraintSystem::AttachChild(Entity parent, Entity child,
                                   const AttachParams& params) {
  CHECK(parent != child) << "Cannot make an entity its own child.";
  CHECK(!IsAncestorOf(child, parent))
      << "Cannot make Entity a parent of one of its ancestors.";

  const auto [old_parent, added] = TryCreateLink(parent, child, params);
  if (added) {
    Notify(child, old_parent, parent, false);
  }
}

std::pair<Entity, bool> ConstraintSystem::TryCreateLink(
    Entity parent, Entity child, const AttachParams& params) {
  Constraints::Row child_row = constraints_.TryEmplace(child);
  const Entity old_parent = child_row.Get<kParent>();
  if (old_parent == parent) {
    // Already parented.
    return std::make_pair(old_parent, false);
  }

  // Remove child from previous parent.
  if (old_parent != kNullEntity) {
    const auto [_, removed] = TryRemoveLink(child);
    CHECK(removed) << "Unable to remove child from parent.";
  }

  Constraints::Row parent_row = constraints_.TryEmplace(parent);
  child_row.Get<kParent>() = parent;
  child_row.Get<kPrevSibling>() = kNullEntity;
  child_row.Get<kNextSibling>() = parent_row.Get<kFirstChild>();
  parent_row.Get<kFirstChild>() = child;

  child_row.Get<kOffset>() = params.local_offset;
  child_row.Get<kBone>() = params.child_bone;
  child_row.Get<kParentBone>() = params.parent_bone;
  child_row.Get<kIgnoreParentScale>() = params.ignore_parent_scale;

  if (!entity_factory_->IsEnabled(parent)) {
    entity_factory_->DisableIndirectly(child);
    SetEnabled(child, false);
  }

  transform_system_->LockTransform(child, this);
  ApplyConstraint(child_row);

  return std::make_pair(old_parent, true);
}

void ConstraintSystem::DetachFromParent(Entity child) {
  const auto [old_parent, removed] = TryRemoveLink(child);
  if (removed) {
    Notify(child, old_parent, kNullEntity, false);
  }
}

std::pair<Entity, bool> ConstraintSystem::TryRemoveLink(Entity child,
                                                        bool preserve) {
  auto child_row = constraints_.FindRow(child);
  if (!child_row) {
    return std::make_pair(kNullEntity, false);
  }

  const Entity old_parent = child_row.Get<kParent>();
  if (old_parent == kNullEntity) {
    // Child has no parent so it is a root Entity (i.e. there is no node).
    return std::make_pair(old_parent, false);
  }

  auto parent_row = constraints_.FindRow(old_parent);
  CHECK(parent_row) << "Child has parent, but parent doesn't exist.";

  transform_system_->UnlockTransform(child, this);

  // Remove connection between nodes.
  if (parent_row.Get<kFirstChild>() == child) {
    parent_row.Get<kFirstChild>() = child_row.Get<kNextSibling>();
  }
  if (child_row.Get<kPrevSibling>() != kNullEntity) {
    auto prev_sibling_row = constraints_.FindRow(child_row.Get<kPrevSibling>());
    prev_sibling_row.Get<kNextSibling>() = child_row.Get<kNextSibling>();
  }
  if (child_row.Get<kNextSibling>() != kNullEntity) {
    auto next_sibling_row = constraints_.FindRow(child_row.Get<kNextSibling>());
    next_sibling_row.Get<kPrevSibling>() = child_row.Get<kPrevSibling>();
  }

  child_row.Get<kParent>() = kNullEntity;
  child_row.Get<kPrevSibling>() = kNullEntity;
  child_row.Get<kNextSibling>() = kNullEntity;

  if (!entity_factory_->IsEnabled(child)) {
    entity_factory_->Disable(child);
  }
  entity_factory_->ClearIndirectDisable(child);

  // Check to see if either the parent or child are isolated. If so, remove them
  // from the constraints_. Do the checks before the erases to prevent any
  // iterator invalidation.
  const bool child_isolated = (child_row.Get<kParent>() == kNullEntity &&
                               child_row.Get<kFirstChild>() == kNullEntity);
  const bool parent_isolated = (parent_row.Get<kParent>() == kNullEntity &&
                                parent_row.Get<kFirstChild>() == kNullEntity);
  if (child_isolated) {
    constraints_.Erase(child);
  }

  if (parent_isolated) {
    constraints_.Erase(old_parent);
  }

  return std::make_pair(old_parent, true);
}

void ConstraintSystem::OnDestroy(Entity entity) {
  auto row = constraints_.FindRow(entity);
  if (!row) {
    return;
  }

  // First destroy all the entity's children. We create a list of children
  // to destroy, and then destroy them, to prevent any re-entrancy problems.
  std::vector<Entity> children;
  auto child = constraints_.FindRow(row.Get<kFirstChild>());
  while (child) {
    children.push_back(child.Get<kEntity>());
    child = constraints_.FindRow(child.Get<kNextSibling>());
  }

  for (const Entity child : children) {
    // We call DestroyNow because we are already in the middle of Entity
    // destruction.
    entity_factory_->DestroyNow(child);
  }

  // Then remove the entity from its own parent. Doing this should cause the
  // entity constraint to be removed entirely.
  const Entity parent = GetParent(entity);
  if (parent != kNullEntity) {
    const auto [_, removed] = TryRemoveLink(entity, false);
    CHECK(removed) << "Could not remove parent?";
    Notify(entity, parent, kNullEntity, true);
  }

  CHECK(!constraints_.Contains(entity));
}

void ConstraintSystem::OnEnable(Entity entity) { SetEnabled(entity, true); }

void ConstraintSystem::OnDisable(Entity entity) { SetEnabled(entity, false); }

void ConstraintSystem::SetEnabled(Entity entity, bool enable) {
  auto data = constraints_.Find<kFirstChild>(entity);
  if (!data) {
    // We don't know about this entity, so ignore it.
    return;
  }

  // Refresh the enable-state for all children.
  auto child_data = constraints_.Find<kEntity, kNextSibling>(*data);
  while (child_data) {
    const Entity child = child_data.Get<kEntity>();
    if (enable) {
      entity_factory_->ClearIndirectDisable(child);
    } else {
      entity_factory_->DisableIndirectly(child);
    }

    child_data = constraints_.Find<kEntity, kNextSibling>(
        child_data.Get<kNextSibling>());
  }
}

void ConstraintSystem::Notify(Entity child, Entity old_parent,
                              Entity new_parent, bool on_destroy) {
  if (dispatcher_system_ == nullptr) {
    return;
  }

  if (child != kNullEntity) {
    dispatcher_system_->SendToEntity(
        child, ParentConstraintChangedEvent{child, old_parent, new_parent});
  }
  if (old_parent != kNullEntity) {
    dispatcher_system_->SendToEntity(
        old_parent, ChildConstraintRemovedEvent{old_parent, child});
  }
  if (new_parent != kNullEntity) {
    dispatcher_system_->SendToEntity(
        new_parent, ChildConstraintAddedEvent{new_parent, child});
  }
}

Transform ConstraintSystem::GetBoneTransform(Entity entity,
                                             HashValue bone) const {
  if (entity != kNullEntity && bone != HashValue()) {
    return Transform(rig_system_->GetBonePose(entity, bone));
  }
  return Transform();
}

void ConstraintSystem::ApplyConstraint(const Constraints::Row& row) {
  const Entity entity = row.Get<kEntity>();
  const Entity parent = row.Get<kParent>();
  const HashValue child_bone = row.Get<kBone>();
  const HashValue parent_bone = row.Get<kParentBone>();

  mat4 parent_transform = transform_system_->GetWorldTransformMatrix(parent);
  if (parent_bone != HashValue()) {
    parent_transform *= rig_system_->GetBonePose(parent, parent_bone);
  }

  mat4 child_offset = TransformMatrix(row.Get<kOffset>());
  if (child_bone != HashValue()) {
    child_offset *= rig_system_->GetBonePose(entity, child_bone);
  }

  if (row.Get<kIgnoreParentScale>()) {
    Transform tmp(parent_transform);
    tmp.scale = vec3::One();
    parent_transform = TransformMatrix(tmp);
  }

  transform_system_->SetTransform(
      entity, Transform(parent_transform * child_offset), this);
}

template <typename Fn>
void ConstraintSystem::ForEachRowInHierarchy(Entity root, const Fn& fn) {
  if (root == kNullEntity) {
    return;
  }
  const Constraints::Row row = constraints_.FindRow(root);
  if (!row) {
    return;
  }

  fn(row);
  ForEachRowInHierarchy(row.Get<kFirstChild>(), fn);
  ForEachRowInHierarchy(row.Get<kNextSibling>(), fn);
}

void ConstraintSystem::UpdateTransforms() {
  // Awful, awful performance, completely negating the benefits of using a
  // structure-of-arrays data container. But, it works.
  for (auto row : constraints_) {
    if (row.Get<kParent>() == kNullEntity) {
      const Entity root = row.Get<kEntity>();
      ForEachRowInHierarchy(root, [&](const Constraints::Row& row) {
        if (row.Get<kParent>() != kNullEntity) {
          ApplyConstraint(row);
        }
      });
    }
  }
}

}  // namespace redux
