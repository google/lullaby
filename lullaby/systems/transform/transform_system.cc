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

#include "lullaby/systems/transform/transform_system.h"

#include <algorithm>

#include "lullaby/events/entity_events.h"
#include "lullaby/modules/dispatcher/dispatcher.h"
#include "lullaby/modules/ecs/entity_factory.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/dispatcher/event.h"
#include "lullaby/util/logging.h"
#include "lullaby/generated/transform_def_generated.h"

namespace {
// Given an |index| into a list containing |list_size| elements, this transforms
// |index| into a valid offset. Allows for negative indices, where '-1' would
// map to the last position, '-2', the second to last, etc. Clamps to the valid
// range of the |list_size|.
size_t RoundAndClampIndex(int index, size_t list_size) {
  if (index < 0) {
    if (static_cast<unsigned int>(index * -1) > list_size) {
      return 0;
    } else {
      return list_size + index;
    }
  } else {
    if (static_cast<unsigned int>(index) >= list_size) {
      return list_size - 1;
    } else {
      return static_cast<size_t>(index);
    }
  }
}
}  // namespace

namespace lull {

const TransformSystem::TransformFlags TransformSystem::kInvalidFlag = 0;
const TransformSystem::TransformFlags TransformSystem::kAllFlags = ~0;
const HashValue kTransformDefHash = Hash("TransformDef");

TransformSystem::TransformSystem(Registry* registry)
    : System(registry),
      nodes_(16),
      world_transforms_(16),
      disabled_transforms_(16),
      reserved_flags_(0) {
  RegisterDef(this, kTransformDefHash);

  EntityFactory* entity_factory = registry_->Get<EntityFactory>();
  if (entity_factory) {
    entity_factory->SetCreateChildFn(
        [this, entity_factory](Entity parent, BlueprintTree* bpt) -> Entity {
          if (parent == kNullEntity) {
            LOG(DFATAL) << "Attempted to create a child for a null parent. "
                        << "Creating child as a parentless entity instead";
            return entity_factory->Create(bpt);
          }

          const Entity child = entity_factory->Create();
          pending_children_[child] = parent;
          const Entity created_child = entity_factory->Create(child, bpt);
          pending_children_.erase(child);
          return created_child;
        });
  }

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.Transform.Enable",
                           &lull::TransformSystem::Enable);
    binder->RegisterMethod("lull.Transform.Disable",
                           &lull::TransformSystem::Disable);
    binder->RegisterMethod("lull.Transform.IsEnabled",
                           &lull::TransformSystem::IsEnabled);
    binder->RegisterMethod("lull.Transform.IsLocallyEnabled",
                           &lull::TransformSystem::IsLocallyEnabled);
    binder->RegisterMethod("lull.Transform.SetLocalTranslation",
                           &lull::TransformSystem::SetLocalTranslation);
    binder->RegisterMethod("lull.Transform.SetLocalRotation",
                           &lull::TransformSystem::SetLocalRotation);
    binder->RegisterMethod("lull.Transform.SetLocalScale",
                           &lull::TransformSystem::SetLocalScale);
    binder->RegisterMethod("lull.Transform.GetLocalTranslation",
                           &lull::TransformSystem::GetLocalTranslation);
    binder->RegisterMethod("lull.Transform.GetLocalRotation",
                           &lull::TransformSystem::GetLocalRotation);
    binder->RegisterMethod("lull.Transform.GetLocalScale",
                           &lull::TransformSystem::GetLocalScale);
    binder->RegisterMethod("lull.Transform.SetWorldFromEntityMatrix",
                           &lull::TransformSystem::SetWorldFromEntityMatrix);
    binder->RegisterFunction(
        "lull.Transform.GetWorldFromEntityMatrix",
        [this](Entity e) { return *GetWorldFromEntityMatrix(e); });
    binder->RegisterMethod("lull.Transform.GetParent",
                           &lull::TransformSystem::GetParent);
    binder->RegisterFunction("lull.Transform.AddChild", [this](Entity parent,
                                                               Entity child) {
      AddChild(parent, child, AddChildMode::kPreserveParentToEntityTransform);
    });
    binder->RegisterFunction(
        "lull.Transform.AddChildPreserveWorldToEntityTransform",
        [this](Entity parent, Entity child) {
          AddChild(parent, child,
                   AddChildMode::kPreserveWorldToEntityTransform);
        });
    binder->RegisterMethod("lull.Transform.CreateChild",
                           &lull::TransformSystem::CreateChild);
    binder->RegisterMethod("lull.Transform.CreateChildWithEntity",
                           &lull::TransformSystem::CreateChildWithEntity);
    binder->RegisterMethod("lull.Transform.RemoveParent",
                           &lull::TransformSystem::RemoveParent);
    binder->RegisterFunction(
        "lull.Transform.GetChildren",
        [this](Entity parent) { return *GetChildren(parent); });
    binder->RegisterMethod("lull.Transform.IsAncestorOf",
                           &lull::TransformSystem::IsAncestorOf);
  }

  auto* dispatcher = registry->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->Connect(
        this, [this](const EnableEvent& event) { Enable(event.entity); });
    dispatcher->Connect(
        this, [this](const DisableEvent& event) { Disable(event.entity); });
    dispatcher->Connect(this, [this](const AddChildEvent& event) {
      AddChild(event.entity, event.child,
               AddChildMode::kPreserveParentToEntityTransform);
    });
    dispatcher->Connect(
        this, [this](const AddChildPreserveWorldToEntityTransformEvent& event) {
          AddChild(event.entity, event.child,
                   AddChildMode::kPreserveWorldToEntityTransform);
        });
  }
}

TransformSystem::~TransformSystem() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Transform.Enable");
    binder->UnregisterFunction("lull.Transform.Disable");
    binder->UnregisterFunction("lull.Transform.IsEnabled");
    binder->UnregisterFunction("lull.Transform.IsLocallyEnabled");
    binder->UnregisterFunction("lull.Transform.SetLocalTranslation");
    binder->UnregisterFunction("lull.Transform.SetLocalRotation");
    binder->UnregisterFunction("lull.Transform.SetLocalScale");
    binder->UnregisterFunction("lull.Transform.GetLocalTranslation");
    binder->UnregisterFunction("lull.Transform.GetLocalRotation");
    binder->UnregisterFunction("lull.Transform.GetLocalScale");
    binder->UnregisterFunction("lull.Transform.SetWorldFromEntityMatrix");
    binder->UnregisterFunction("lull.Transform.GetWorldFromEntityMatrix");
    binder->UnregisterFunction("lull.Transform.GetParent");
    binder->UnregisterFunction("lull.Transform.AddChild");
    binder->UnregisterFunction("lull.Transform.CreateChild");
    binder->UnregisterFunction("lull.Transform.CreateChildWithEntity");
    binder->UnregisterFunction("lull.Transform.RemoveParent");
    binder->UnregisterFunction("lull.Transform.GetChildren");
    binder->UnregisterFunction("lull.Transform.IsAncestorOf");
  }
  auto* dispatcher = registry_->Get<Dispatcher>();
  if (dispatcher) {
    dispatcher->DisconnectAll(this);
  }
}

void TransformSystem::Create(Entity e, HashValue type, const Def* def) {
  if (type != kTransformDefHash) {
    LOG(DFATAL) << "Invalid type passed to Create. Expecting TransformDef!";
    return;
  }
  auto data = ConvertDef<TransformDef>(def);

  auto node = nodes_.Emplace(e);
  if (!node) {
    LOG(DFATAL) << "Encountered null node!";
    return;
  }

  auto world_transform = world_transforms_.Emplace(e);
  if (!world_transform) {
    LOG(DFATAL) << "Encountered null world transform!";
    nodes_.Destroy(e);
    return;
  }

  node->enable_self = data->enabled();

  MathfuVec3FromFbVec3(data->position(), &node->local_sqt.translation);
  if (data->quaternion()) {
    MathfuQuatFromFbVec4(data->quaternion(), &node->local_sqt.rotation);
  } else {
    MathfuQuatFromFbVec3(data->rotation(), &node->local_sqt.rotation);
  }
  MathfuVec3FromFbVec3(data->scale(), &node->local_sqt.scale);
  AabbFromFbAabb(data->aabb(), &world_transform->box);
  node->world_from_entity_matrix_function = CalculateWorldFromEntityMatrix;

  if (data->aabb_padding()) {
    AabbFromFbAabb(data->aabb_padding(), &node->aabb_padding);

    world_transform->box.min += node->aabb_padding.min;
    world_transform->box.max += node->aabb_padding.max;
  }

  auto iter = pending_children_.find(e);
  if (iter != pending_children_.end()) {
    AddChildNoEvent(iter->second, e,
                    AddChildMode::kPreserveParentToEntityTransform);
  } else {
    UpdateTransforms(e);
    UpdateEnabled(e, IsEnabled(node->parent));
  }
}

void TransformSystem::Create(Entity e, const Sqt& sqt) {
  GraphNode* node = nodes_.Get(e);
  if (!node) {
    node = nodes_.Emplace(e);
    world_transforms_.Emplace(e);
    node->world_from_entity_matrix_function = CalculateWorldFromEntityMatrix;
  }

  node->local_sqt = sqt;
  UpdateTransforms(e);
}

void TransformSystem::PostCreateInit(Entity e, HashValue type, const Def* def) {
  if (type != kTransformDefHash) {
    LOG(DFATAL)
        << "Invalid type passed to PostCreateInit. Expecting TransformDef!";
    return;
  }

  auto iter = pending_children_.find(e);
  if (iter != pending_children_.end()) {
    const Entity parent = iter->second;
    SendEvent(registry_, parent, ChildAddedEvent(parent, e));
    SendEvent(registry_, e, ParentChangedEvent(e, kNullEntity, parent));
    SendEventImmediately(registry_, e,
                         ParentChangedImmediateEvent(e, kNullEntity, parent));
  }

  auto data = ConvertDef<TransformDef>(def);

  if (data->children()) {
    for (auto child = data->children()->begin();
         child != data->children()->end(); ++child) {
      CreateChild(e, child->c_str());
    }
  }
}

void TransformSystem::Destroy(Entity e) {
  // First destroy any children.
  auto children = GetChildren(e);
  if (children) {
    EntityFactory* entity_factory = registry_->Get<EntityFactory>();
    // Make a local copy to avoid re-entrancy problems.
    std::vector<Entity> local_children(*children);
    for (auto& child : local_children) {
      entity_factory->Destroy(child);
    }
  }

  auto node = nodes_.Get(e);
  if (node) {
    Entity parent = GetParent(e);
    RemoveParentNoEvent(e);

    // Only send out the global events in Destroy since this entity's local
    // dispatcher (and possibly the parent's) could already be destroyed.
    auto* dispatcher = registry_->Get<Dispatcher>();
    if (dispatcher && parent != kNullEntity) {
      dispatcher->Send(ChildRemovedEvent(parent, e));
      dispatcher->Send(ParentChangedEvent(e, parent, kNullEntity));
      dispatcher->SendImmediately(
          ParentChangedImmediateEvent(e, parent, kNullEntity));
    }

    nodes_.Destroy(e);
  }
  world_transforms_.Destroy(e);
  disabled_transforms_.Destroy(e);
}

void TransformSystem::SetFlag(Entity e, TransformFlags flag) {
  auto transform = GetWorldTransform(e);
  if (transform) {
    transform->flags = SetBit(transform->flags, flag);
  }
}

void TransformSystem::ClearFlag(Entity e, TransformFlags flag) {
  auto transform = GetWorldTransform(e);
  if (transform) {
    transform->flags = ClearBit(transform->flags, flag);
  }
}

bool TransformSystem::HasFlag(Entity e, TransformFlags flag) const {
  auto transform = GetWorldTransform(e);
  return transform ? CheckBit(transform->flags, flag) : false;
}

void TransformSystem::SetAabb(Entity e, Aabb box) {
  auto transform = GetWorldTransform(e);
  if (transform) {
    transform->box = box;

    const auto node = nodes_.Get(e);
    if (node) {
      transform->box.min += node->aabb_padding.min;
      transform->box.max += node->aabb_padding.max;
    }
  }

  SendEvent(registry_, e, AabbChangedEvent(e));
}

const Aabb* TransformSystem::GetAabb(Entity e) const {
  auto transform = GetWorldTransform(e);
  return transform ? &transform->box : nullptr;
}

void TransformSystem::SetAabbPadding(Entity e, const Aabb& padding) {
  auto node = nodes_.Get(e);
  if (!node) {
    return;
  }

  auto transform = GetWorldTransform(e);
  if (transform) {
    transform->box.min += -node->aabb_padding.min + padding.min;
    transform->box.max += -node->aabb_padding.max + padding.max;
  }

  node->aabb_padding = padding;
}

const Aabb* TransformSystem::GetAabbPadding(Entity e) const {
  const auto node = nodes_.Get(e);
  return node ? &node->aabb_padding : nullptr;
}

void TransformSystem::Enable(Entity e) { SetEnabled(e, true); }

void TransformSystem::Disable(Entity e) { SetEnabled(e, false); }

bool TransformSystem::IsEnabled(Entity e) const {
  // We want to return true if the entity is enabled OR if it doesn't exist.
  // Thus, we only need to check if the entity exists in the disabled pool.
  const auto transform = disabled_transforms_.Get(e);
  return transform == nullptr;
}

bool TransformSystem::IsLocallyEnabled(Entity e) const {
  const auto node = nodes_.Get(e);
  return node ? node->enable_self : true;
}

void TransformSystem::SetSqt(Entity e, const Sqt& sqt) {
  auto node = nodes_.Get(e);
  if (node) {
    node->local_sqt = sqt;
    UpdateTransforms(e);
  }
}

const Sqt* TransformSystem::GetSqt(Entity e) const {
  auto node = nodes_.Get(e);
  return node ? &node->local_sqt : nullptr;
}

void TransformSystem::ApplySqt(Entity e, const Sqt& modifier) {
  auto node = nodes_.Get(e);
  if (node) {
    node->local_sqt.translation += modifier.translation;
    node->local_sqt.rotation = node->local_sqt.rotation * modifier.rotation;
    node->local_sqt.scale *= modifier.scale;
    UpdateTransforms(e);
  }
}

void TransformSystem::SetLocalTranslation(Entity e,
                                          const mathfu::vec3& translation) {
  auto node = nodes_.Get(e);
  if (node) {
    node->local_sqt.translation = translation;
    UpdateTransforms(e);
  }
}

mathfu::vec3 TransformSystem::GetLocalTranslation(Entity e) const {
  const Sqt* sqt = GetSqt(e);
  return sqt ? sqt->translation : mathfu::vec3(0, 0, 0);
}

void TransformSystem::SetLocalRotation(Entity e, const mathfu::quat& rotation) {
  auto node = nodes_.Get(e);
  if (node) {
    node->local_sqt.rotation = rotation;
    UpdateTransforms(e);
  }
}

mathfu::quat TransformSystem::GetLocalRotation(Entity e) const {
  const Sqt* sqt = GetSqt(e);
  return sqt ? sqt->rotation : mathfu::quat(0, 0, 0, 0);
}

void TransformSystem::SetLocalScale(Entity e, const mathfu::vec3& scale) {
  auto node = nodes_.Get(e);
  if (node) {
    node->local_sqt.scale = scale;
    UpdateTransforms(e);
  }
}

mathfu::vec3 TransformSystem::GetLocalScale(Entity e) const {
  const Sqt* sqt = GetSqt(e);
  return sqt ? sqt->scale : mathfu::vec3(0, 0, 0);
}

void TransformSystem::SetWorldFromEntityMatrix(
    Entity e, const mathfu::mat4& world_from_entity_mat) {
  auto node = nodes_.Get(e);
  if (node == nullptr) {
    return;
  }

  mathfu::mat4 parent_from_local_mat(world_from_entity_mat);
  auto parent = GetWorldTransform(node->parent);
  if (parent) {
    const auto parent_from_world_mat = parent->world_from_entity_mat.Inverse();
    parent_from_local_mat = parent_from_world_mat * world_from_entity_mat;
  }

  node->local_sqt = CalculateSqtFromMatrix(parent_from_local_mat);

  UpdateTransforms(e);
}

const mathfu::mat4* TransformSystem::GetWorldFromEntityMatrix(Entity e) const {
  auto transform = GetWorldTransform(e);
  return transform ? &transform->world_from_entity_mat : nullptr;
}

void TransformSystem::SetWorldFromEntityMatrixFunction(
    Entity e, CalculateWorldFromEntityMatrixFunc func) {
  auto node = nodes_.Get(e);
  if (node) {
    node->world_from_entity_matrix_function =
        func ? func : CalculateWorldFromEntityMatrix;
    UpdateTransforms(e);
  }
}

Entity TransformSystem::GetParent(Entity child) const {
  const auto* node = nodes_.Get(child);
  return node ? node->parent : kNullEntity;
}

bool TransformSystem::AddChildNoEvent(Entity parent, Entity child,
                                      AddChildMode mode) {
  if (parent == kNullEntity) {
    LOG(INFO) << "Cannot add a child to a null parent.";
    return false;
  }
  if (parent == child) {
    LOG(DFATAL) << "Cannot make an entity its own child.";
    return false;
  }
  if (IsAncestorOf(child, parent)) {
    LOG(DFATAL) << "Cannot make a node a parent of one of its ancestors.";
    return false;
  }
  auto child_node = nodes_.Get(child);
  if (!child_node) {
    LOG(DFATAL) << "Invalid - the child entity doesn't exist.";
    return false;
  }
  if (child_node->parent == parent) {
    LOG(DFATAL) << "Parent-child relationship already established.";
    return false;
  }
  if (child_node->parent) {
    RemoveParentNoEvent(child);
  }
  const mathfu::mat4* matrix_ptr = nullptr;
  if (mode == AddChildMode::kPreserveWorldToEntityTransform) {
    matrix_ptr = GetWorldFromEntityMatrix(child);
    if (!matrix_ptr) {
      LOG(DFATAL) << "No world from entity matrix to keep.";
      return false;
    }
  }
  auto parent_node = nodes_.Get(parent);
  if (!parent_node) {
    // Do nothing if parent doesn't exist.
    return true;
  }

  parent_node->children.emplace_back(child);
  child_node->parent = parent;
  if (mode == AddChildMode::kPreserveWorldToEntityTransform) {
    // This will call UpdateTransforms().
    SetWorldFromEntityMatrix(child, *matrix_ptr);
  } else {
    UpdateTransforms(child);
  }
  const bool parent_enabled = IsEnabled(parent);
  UpdateEnabled(child, parent_enabled);

  return true;
}

void TransformSystem::AddChild(Entity parent, Entity child, AddChildMode mode) {
  auto child_node = nodes_.Get(child);
  if (!child_node) {
    LOG(DFATAL) << "Invalid - the child entity doesn't exist.";
    return;
  }

  auto old_parent = child_node->parent;

  if (AddChildNoEvent(parent, child, mode)) {
    SendEvent(registry_, parent, ChildAddedEvent(parent, child));
    SendEvent(registry_, child, ParentChangedEvent(child, old_parent, parent));
    SendEventImmediately(
        registry_, child,
        ParentChangedImmediateEvent(child, old_parent, parent));
  }
}

Entity TransformSystem::CreateChild(Entity parent, const std::string& name) {
  EntityFactory* entity_factory = registry_->Get<EntityFactory>();
  const Entity child = entity_factory->Create();
  return CreateChildWithEntity(parent, child, name);
}

Entity TransformSystem::CreateChildWithEntity(Entity parent, Entity child,
                                              const std::string& name) {
  EntityFactory* entity_factory = registry_->Get<EntityFactory>();

  if (child == kNullEntity) {
    LOG(INFO) << "Attempted to create child using a null entity.";
    return child;
  }

  if (parent == kNullEntity) {
    LOG(INFO) << "Attempted to create a child for a null parent. Creating"
              << " child as a parentless entity instead";
    return entity_factory->Create(child, name);
  }

  const auto node = nodes_.Get(child);
  if (node) {
    LOG(INFO) << "Child already has a Transform component.";
    return child;
  }

  pending_children_[child] = parent;
  const Entity created_child = entity_factory->Create(child, name);
  pending_children_.erase(child);
  return created_child;
}

void TransformSystem::InsertChild(Entity parent, Entity child, int index) {
  if (!IsAncestorOf(parent, child)) {
    AddChild(parent, child);
  }
  MoveChild(child, index);
}

void TransformSystem::MoveChild(Entity child, int index) {
  auto child_node = nodes_.Get(child);
  if (!child_node || !child_node->parent) {
    return;
  }
  auto parent_node = nodes_.Get(child_node->parent);
  if (!parent_node) {
    return;
  }

  auto& children = parent_node->children;
  const size_t num_children = children.size();

  // Get iterator to child's current position.
  const auto source = std::find(children.begin(), children.end(), child);
  if (source == children.end()) {
    LOG(DFATAL) << "Child entity not found in its parent's list of children.";
    return;
  }

  const size_t new_index = RoundAndClampIndex(index, num_children);
  const auto destination = children.begin() + new_index;
  if (source >= destination) {
    std::rotate(destination, source, source + 1);
  } else {
    std::rotate(source, source + 1, destination + 1);
  }
}

void TransformSystem::RemoveParentNoEvent(Entity child) {
  auto child_node = nodes_.Get(child);
  if (child_node && child_node->parent) {
    auto parent_node = nodes_.Get(child_node->parent);
    if (parent_node) {
      parent_node->children.erase(
          std::remove(parent_node->children.begin(),
                      parent_node->children.end(), child),
          parent_node->children.end());
    }
    child_node->parent = kNullEntity;
  }
}

void TransformSystem::RemoveParent(Entity child) {
  auto parent = GetParent(child);
  if (parent != kNullEntity) {
    RemoveParentNoEvent(child);
    SendEvent(registry_, parent, ChildRemovedEvent(parent, child));
    SendEvent(registry_, child, ParentChangedEvent(child, parent, kNullEntity));
    SendEventImmediately(
        registry_, child,
        ParentChangedImmediateEvent(child, parent, kNullEntity));
  }
}

void TransformSystem::DestroyChildren(Entity parent) {
  const std::vector<Entity>* children = GetChildren(parent);
  if (!children) {
    return;
  }

  auto* entity_factory = registry_->Get<EntityFactory>();

  // Make a local copy of the children to avoid re-entrancy problems.
  const std::vector<Entity> children_copy(*children);
  for (const auto child : children_copy) {
    entity_factory->Destroy(child);
  }
}

const std::vector<Entity>* TransformSystem::GetChildren(Entity parent) const {
  auto node = nodes_.Get(parent);
  if (node) {
    return &node->children;
  }
  return nullptr;
}

size_t TransformSystem::GetChildCount(Entity parent) const {
  auto node = nodes_.Get(parent);
  if (node) {
    return node->children.size();
  }
  return 0;
}

size_t TransformSystem::GetChildIndex(Entity child) const {
  auto child_node = nodes_.Get(child);
  if (child_node == nullptr) {  // Child has no TransformDef.
    LOG(DFATAL) << "GetChildIndex called on entity with no TransformDef.";
    return 0;
  }

  Entity parent = GetParent(child);
  auto parent_node = nodes_.Get(parent);
  if (parent_node == nullptr) {  // Child has no parent.
    return 0;
  }

  const auto& children = parent_node->children;
  auto found = std::find(children.begin(), children.end(), child);
  if (found == children.end()) {
    return 0;
  } else {
    return static_cast<size_t>(found - children.begin());
  }
}

bool TransformSystem::IsAncestorOf(Entity ancestor, Entity target) const {
  auto node = nodes_.Get(target);
  while (node && node->parent != kNullEntity) {
    if (node->parent == ancestor) {
      return true;
    }
    node = nodes_.Get(node->parent);
  }
  return false;
}

mathfu::mat4 TransformSystem::CalculateWorldFromEntityMatrix(
    const Sqt& local_sqt, const mathfu::mat4* world_from_parent_mat) {
  mathfu::mat4 parent_from_local_mat = CalculateTransformMatrix(local_sqt);
  if (world_from_parent_mat) {
    return (*world_from_parent_mat) * parent_from_local_mat;
  } else {
    return parent_from_local_mat;
  }
}

void TransformSystem::UpdateTransforms(Entity child) {
  auto node = nodes_.Get(child);
  auto world_transform = GetWorldTransform(child);

  world_transform->world_from_entity_mat =
      node->world_from_entity_matrix_function(
          node->local_sqt, GetWorldFromEntityMatrix(node->parent));
  for (auto& grand_child : node->children) {
    UpdateTransforms(grand_child);
  }
}

void TransformSystem::SetEnabled(Entity e, bool enabled) {
  auto node = nodes_.Get(e);
  if (node && node->enable_self != enabled) {
    node->enable_self = enabled;

    const bool parent_enabled = IsEnabled(node->parent);
    UpdateEnabled(e, parent_enabled);
  }
}

void TransformSystem::UpdateEnabled(Entity e, bool parent_enabled) {
  auto graph_node = nodes_.Get(e);
  if (!graph_node) {
    return;
  }
  const bool enabled = graph_node->enable_self;
  auto transform = world_transforms_.Get(e);
  bool changed = false;
  if (transform) {
    if (!enabled || !parent_enabled) {
      changed = true;
      disabled_transforms_.Emplace(std::move(*transform));
      world_transforms_.Destroy(e);
      SendEvent(registry_, e, OnDisabledEvent(e));
    }
  } else {
    transform = disabled_transforms_.Get(e);
    if (transform) {
      if (enabled && parent_enabled) {
        changed = true;
        world_transforms_.Emplace(std::move(*transform));
        disabled_transforms_.Destroy(e);
        SendEvent(registry_, e, OnEnabledEvent(e));
      }
    }
  }

  if (changed) {
    for (auto& child : graph_node->children) {
      UpdateEnabled(child, enabled && parent_enabled);
    }
  }
}

const TransformSystem::WorldTransform* TransformSystem::GetWorldTransform(
    Entity e) const {
  auto transform = world_transforms_.Get(e);
  if (!transform) {
    transform = disabled_transforms_.Get(e);
  }
  return transform;
}

TransformSystem::WorldTransform* TransformSystem::GetWorldTransform(Entity e) {
  auto transform = world_transforms_.Get(e);
  if (!transform) {
    transform = disabled_transforms_.Get(e);
  }
  return transform;
}

TransformSystem::TransformFlags TransformSystem::RequestFlag() {
  const int kNumBits = 8 * sizeof(TransformFlags);
  for (TransformFlags i = 0; i < kNumBits; ++i) {
    TransformFlags flag = 1 << i;
    if (!CheckBit(reserved_flags_, flag)) {
      reserved_flags_ = SetBit(reserved_flags_, flag);
      return flag;
    }
  }
  CHECK(false) << "Ran out of flags";
  return kInvalidFlag;
}

void TransformSystem::ReleaseFlag(TransformFlags flag) {
  if (flag == kInvalidFlag) {
    LOG(DFATAL) << "Cannot release invalid flag.";
    return;
  }
  reserved_flags_ = ClearBit(reserved_flags_, flag);
}

}  // namespace lull
