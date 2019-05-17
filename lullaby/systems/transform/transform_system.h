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

#ifndef LULLABY_SYSTEMS_TRANSFORM_TRANSFORM_SYSTEM_H_
#define LULLABY_SYSTEMS_TRANSFORM_TRANSFORM_SYSTEM_H_

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/bits.h"
#include "lullaby/util/math.h"
#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

/// The TransformSystem provide Entities with position, rotation, scale and
/// volume (via an AABB).  It also allows entities to be attached to each other
/// to create a scene graph hierarchy.
class TransformSystem : public System {
 public:
  /// TransformFlags are custom flags defined by applications that help group
  /// transform components together to do operation on. For example a collision
  /// system maybe defined a unique flag for collidables, while a render system
  /// may flag renderables. This can be used to pull and operate on all the
  /// relevant entities with the flag.
  using TransformFlags = uint32_t;
  static const TransformFlags kInvalidFlag;
  static const TransformFlags kAllFlags;

  /// Control the behavior of AddChild and RemoveParent.
  enum ModifyParentChildMode {
    /// Keep the local transform and update the world transform (Default).
    kPreserveParentToEntityTransform,
    /// Keep the world transform and update the local transform.
    kPreserveWorldToEntityTransform,
  };

  /// A function which takes as input the local sqt and the parent's
  /// world_from_entity_matrix, and returns a world_from_entity_matrix.  Used
  /// to override the normal local to world transform logic by systems like
  /// DeformSystem.
  typedef std::function<mathfu::mat4(const Sqt&, const mathfu::mat4*)>
      CalculateWorldFromEntityMatrixFunc;
  /// The inverse of the above.  Takes the new world_from_entity_matrix and the
  /// parent's world_from_entity_matrix, and calculates a local sqt.
  typedef std::function<Sqt(const mathfu::mat4&, const mathfu::mat4*)>
      CalculateLocalSqtFunc;

  explicit TransformSystem(Registry* registry);

  ~TransformSystem() override;

  /// Checks whether an entity has a transform.
  bool HasTransform(Entity e) const;

  /// Adds a transform to the Entity using the specified ComponentDef.
  void Create(Entity e, HashValue type, const Def* def) override;

  /// Adds a transform to |e| using |sqt|.  If |e| already has a transform, just
  /// sets sqt.
  void Create(Entity e, const Sqt& sqt);

  /// Performs post creation initialization.
  void PostCreateInit(Entity e, HashValue type, const Def* def) override;

  /// Removes the transform from the Entity.
  void Destroy(Entity e) override;

  /// Sets the specified transform to be included when calling foreach with the
  /// provided flag.
  void SetFlag(Entity e, TransformFlags flag);

  /// Removes the specified transform from being included when calling foreach
  /// with the provided flag.
  void ClearFlag(Entity e, TransformFlags flag);

  /// Checks whether an entity has a flag.
  bool HasFlag(Entity e, TransformFlags flag) const;

  /// Sets the Aabb for the specified transform.
  void SetAabb(Entity e, Aabb box);

  /// Gets the (padded) Aabb for the specified transform.
  const Aabb* GetAabb(Entity e) const;

  /// Sets the padding for the specified entity. This adds the padding aabb to
  /// any Aabbs set to this entity.
  void SetAabbPadding(Entity e, const Aabb& padding);

  /// Gets the padding for the specified entity.
  const Aabb* GetAabbPadding(Entity e) const;

  /// Enables an Entity and all its children.  OnEnabledEvent will be dispatched
  /// when this is called if the entity was previously disabled.
  void Enable(Entity e);

  /// Disables an Entity and all its children.  Disabled entities will not be
  /// rendered or collided against, and will not be included in
  /// transform_system.ForEach() calls.  OnDisabledEvent will be dispatched if
  /// the entity was previously enabled.
  void Disable(Entity e);

  /// Gets the inherited enabled status for an entity. Returns true if no entity
  /// is found.
  bool IsEnabled(Entity e) const;

  /// Gets the local enabled status for an entity.  This will only return false
  /// if Disable has been called on this specific entity.  Returns true if no
  /// entity is found.
  bool IsLocallyEnabled(Entity e) const;

  /// Set the specified entity to the given position, rotation, and scale.
  void SetSqt(Entity e, const Sqt& sqt);

  /// Gets the SQT for the specified entity (or NULL if it does not have a
  /// transform).
  const Sqt* GetSqt(Entity e) const;

  /// Adds the translation and multiplies the rotation and scale into the
  /// existing local_sqt.
  void ApplySqt(Entity e, const Sqt& sqt);

  /// Sets the local translation of the entity.
  void SetLocalTranslation(Entity e, const mathfu::vec3& translation);

  /// Gets the local translation of the entity (or (0, 0, 0) if it does not have
  /// a transform).
  mathfu::vec3 GetLocalTranslation(Entity e) const;

  /// Sets the local rotation of the entity.
  void SetLocalRotation(Entity e, const mathfu::quat& rotation);

  /// Gets the local rotation of the entity (or (0, 0, 0, 0) if it does not have
  /// a transform).
  mathfu::quat GetLocalRotation(Entity e) const;

  /// Sets the local scale of the entity.
  void SetLocalScale(Entity e, const mathfu::vec3& scale);

  /// Gets the local scale of the entity (or (0, 0, 0) if it does not have a
  /// transform).
  mathfu::vec3 GetLocalScale(Entity e) const;

  /// Sets the world matrix of an entity.  Local sqt values will be derived from
  /// based on the parent's current world_transform.
  void SetWorldFromEntityMatrix(Entity e,
                                const mathfu::mat4& world_from_entity_mat);

  /// Gets the world matrix for the specified entity (or NULL if it does not
  /// have a transform).
  const mathfu::mat4* GetWorldFromEntityMatrix(Entity e) const;

  /// Overrides the default math for calculating local->world and world->local
  /// transforms.
  void SetWorldFromEntityMatrixFunction(
      Entity e, const CalculateWorldFromEntityMatrixFunc& func,
      const CalculateLocalSqtFunc* inverse_func = nullptr);

  /// Returns the parent Entity, if it exists.
  Entity GetParent(Entity child) const;

  /// Returns the Entity at the root of the hierarchy including the provided
  /// entity, or kNullEntity if the entity does not have a valid transform.
  Entity GetRoot(Entity entity) const;

  /// Establish a parent/child relationship between two Entities
  void AddChild(Entity parent, Entity child,
                ModifyParentChildMode mode =
                    ModifyParentChildMode::kPreserveParentToEntityTransform);

  /// Create an entity and add it as a child.  Shortcut to EntityFactory::Create
  /// followed by TransformSystem::AddChild that avoids an extra call to
  /// UpdateTransform().
  Entity CreateChild(Entity parent, const std::string& name);

  /// Populates the specified |child| entity with the component data specified
  /// in the blueprint |name| and adds it as a child to the |parent| entity.
  /// Shortcut to EntityFactory::Create followed by TransformSystem::AddChild
  /// that avoids an extra call to UpdateTransform().
  Entity CreateChildWithEntity(Entity parent, Entity child,
                               const std::string& name);

  /// Inserts a child at the specific index in the parent's list of children.
  /// Negative indices will insert from the back of the list. For example, a
  /// |child| inserted at an |index| of '-1' would become the last element in
  /// the list, at '-2' it would be the second to last, etc. Out-of-range
  /// |index| values are clamped so as not to exceed the number of children in
  /// the list.
  void InsertChild(Entity parent, Entity child, int index = -1);

  /// Moves a child entity to the given index in its parent's list of children.
  /// If the |child| has no parent, this has no effect. The |index| behaves the
  /// same as described in InsertChild().
  void MoveChild(Entity child, int index);

  /// Break a child's connection to its parent.
  void RemoveParent(
      Entity child,
      ModifyParentChildMode mode =
          ModifyParentChildMode::kPreserveParentToEntityTransform);

  /// Retrieve the list of children of an Entity
  const std::vector<Entity>* GetChildren(Entity parent) const;

  /// Destroys all child entities of the given parent entity. If |parent| has
  /// no children, this has no effect.
  void DestroyChildren(Entity parent);

  /// Returns the number of children belonging to the |parent| entity.
  size_t GetChildCount(Entity parent) const;

  /// Returns the index of the |child| entity within its parent's list of
  /// children. Returns 0 if the |child| has no parent.
  size_t GetChildIndex(Entity child) const;

  /// Returns true if |ancestor| is in the parent chain of |target|.
  bool IsAncestorOf(Entity ancestor, Entity target) const;

  /// Returns a unique flag that can be used to iterate via ForEach.
  TransformFlags RequestFlag();

  /// Releases a flag.  After calling this, you should set all references to
  /// kInvalidFlag.
  void ReleaseFlag(TransformFlags flag);

  /// Calls the provided function with every Transform and provides the
  /// TransformFlags.
  template <typename Fn>
  void ForAll(Fn fn) const {
    world_transforms_.ForEach([&](const WorldTransform& transform) {
      fn(transform.GetEntity(), transform.world_from_entity_mat, transform.box,
         transform.flags);
    });
  }

  /// Calls the provided function with a Transform for every Entity which has
  /// the provided flag.
  ///
  /// For example:
  /// @code
  /// transform_system->ForEach(
  ///     kSomeFlag,
  ///      [this](Entity e, const mathfu::mat4& world_from_entity_mat,
  ///             const Aabb& box) {
  ///       DoSomethingToEntityAt(e, world_from_entity_mat);
  ///   });
  /// @endcode
  template <typename Fn>
  void ForEach(TransformFlags flag, Fn fn) const {
    if (flag == kAllFlags) {
      ForAll([&](Entity e, const mathfu::mat4& world_from_entity_mat,
                 const Aabb& box, Bits) { fn(e, world_from_entity_mat, box); });
    } else {
      ForAll([&](Entity e, const mathfu::mat4& world_from_entity_mat,
                 const Aabb& box, Bits flags) {
        if (CheckBit(flags, flag)) fn(e, world_from_entity_mat, box);
      });
    }
  }

  /// Calls the provided function on the provided entity and all of it's
  /// descendants.
  template <typename Fn>
  void ForAllDescendants(Entity parent, Fn fn) const {
    fn(parent);
    const auto* const children = GetChildren(parent);
    if (children) {
      for (const lull::Entity& child : *children) {
        ForAllDescendants(child, fn);
      }
    }
  }

  // Returns the Graphviz representation of the entity tree.
  // Protip: These trees tend to be shallow and wide, so try other layout
  // engines than the default, like fdp.
  std::string GetEntityTreeDebugString(bool enabled_only = true) const;

  // Recalculates the WorldFromEntityMatrix function for the given entity and
  // all of its children. Potentially expensive, so should be called sparingly.
  void RecalculateWorldFromEntityMatrix(Entity child);

  // Calculates the world_from_entity_matrix for the given local sqt and
  // world_from_parent_matrix.
  static mathfu::mat4 CalculateWorldFromEntityMatrix(
      const Sqt& local_sqt, const mathfu::mat4* world_from_parent_mat);

 private:
  struct GraphNode : Component {
    explicit GraphNode(Entity e)
        : Component(e),
          local_sqt(mathfu::kZeros3f, mathfu::quat::identity, mathfu::kOnes3f),
          parent(kNullEntity),
          enable_self(true) {}

    Sqt local_sqt;
    Aabb aabb_padding;
    CalculateWorldFromEntityMatrixFunc world_from_entity_matrix_function;
    CalculateLocalSqtFunc local_sqt_function;
    std::vector<Entity> children;
    Entity parent;
    bool enable_self;
  };

  struct WorldTransform : Component {
    // This struct should be kept as small as possible to reduce cache misses
    // when iterating.
    explicit WorldTransform(Entity e) : Component(e), flags(0) {}
    Bits flags;
    mathfu::mat4 world_from_entity_mat;
    Aabb box;
  };

  static Sqt CalculateLocalSqt(const mathfu::mat4& world_from_entity_mat,
                               const mathfu::mat4* world_from_parent_mat);
  void SetEnabled(Entity e, bool enabled);
  void UpdateEnabled(Entity e, bool parent_enabled);
  const WorldTransform* GetWorldTransform(Entity e) const;
  WorldTransform* GetWorldTransform(Entity e);

  // Break a child's connection to its parent without sending any events.
  void RemoveParentNoEvent(Entity child);

  // Establish a parent/child relationship between two Entities without
  // sending any events.
  bool AddChildNoEvent(Entity parent, Entity child, ModifyParentChildMode mode);

  ComponentPool<GraphNode> nodes_;
  ComponentPool<WorldTransform> world_transforms_;
  ComponentPool<WorldTransform> disabled_transforms_;
  uint32_t reserved_flags_;

  // A map of parent/child relationships requested by CreateChild, which need to
  // be handled during Create().
  std::unordered_map<Entity, Entity> pending_children_;

  TransformSystem(const TransformSystem&);
  TransformSystem& operator=(const TransformSystem&);
};

struct CreateChildEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&child, ConstHash("child"));
    archive(&parent, ConstHash("parent"));
    archive(&blueprint, ConstHash("blueprint"));
  }

  Entity child = kNullEntity;
  Entity parent = kNullEntity;
  std::string blueprint;
};

struct InsertChildEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&child, ConstHash("child"));
    archive(&index, ConstHash("index"));
  }

  Entity entity = kNullEntity;
  Entity child = kNullEntity;
  int index = -1;
};

struct MoveChildEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&index, ConstHash("index"));
  }

  Entity entity = kNullEntity;
  int index = -1;
};

struct RemoveChildEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&child, ConstHash("child"));
  }

  Entity entity = kNullEntity;
  Entity child = kNullEntity;
};

struct SetWorldFromEntityMatrixEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&transform, ConstHash("transform"));
  }

  Entity entity = kNullEntity;
  mathfu::mat4 transform = mathfu::mat4::Identity();
};

struct SetPositionEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&position, ConstHash("position"));
  }

  Entity entity = kNullEntity;
  mathfu::vec3 position = mathfu::kZeros3f;
};

struct SetRotationEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&rotation, ConstHash("rotation"));
  }

  Entity entity = kNullEntity;
  mathfu::quat rotation = mathfu::quat::identity;
};

struct SetScaleEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&scale, ConstHash("scale"));
  }

  Entity entity = kNullEntity;
  mathfu::vec3 scale = mathfu::kZeros3f;
};

struct SetAabbEvent {
  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&entity, ConstHash("entity"));
    archive(&min, ConstHash("min"));
    archive(&max, ConstHash("max"));
  }

  Entity entity = kNullEntity;
  mathfu::vec3 min = mathfu::kZeros3f;
  mathfu::vec3 max = mathfu::kZeros3f;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::TransformSystem);
LULLABY_SETUP_TYPEID(lull::CreateChildEvent);
LULLABY_SETUP_TYPEID(lull::InsertChildEvent);
LULLABY_SETUP_TYPEID(lull::MoveChildEvent);
LULLABY_SETUP_TYPEID(lull::RemoveChildEvent);
LULLABY_SETUP_TYPEID(lull::SetAabbEvent);
LULLABY_SETUP_TYPEID(lull::SetPositionEvent);
LULLABY_SETUP_TYPEID(lull::SetRotationEvent);
LULLABY_SETUP_TYPEID(lull::SetScaleEvent);
LULLABY_SETUP_TYPEID(lull::SetWorldFromEntityMatrixEvent);

#endif  // LULLABY_SYSTEMS_TRANSFORM_TRANSFORM_SYSTEM_H_
