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

#ifndef LULLABY_SYSTEMS_DEFORM_DEFORM_SYSTEM_H_
#define LULLABY_SYSTEMS_DEFORM_DEFORM_SYSTEM_H_

#include <unordered_map>

#include "lullaby/generated/deform_def_generated.h"
#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/events/entity_events.h"
#include "lullaby/util/math.h"
#include "lullaby/util/optional.h"
#include "lullaby/util/string_view.h"

namespace lull {

// Handles cylindrical deformation logic for transform and render systems.
// Deformation is applied in two parts:
//
//  1) Transform - Every time a deformed entity is moved or reparented in the
//     transform system, the deform system will recalculate the correct deformed
//     world_from_entity matrix for that entity.  Moving an object in the x axis
//     will be re-interpreted to a movement along the circumference of the
//     deformer's cylinder.
//  2) Render - When a mesh is first created, the vertices of that mesh will be
//     deformed according to the entity's current position and deformer.  This
//     mesh will not be updated unless it is recreated / reloaded by the render
//     system.
//
// Deformation should be accomplished by adding a DeformerDef component to
// the root object of the deformation, and a DeformedDef component to any
// children that should deform.  If a DeformedDef Entity is a child of an
// undeformed parent, it will not apply any deformation.
class DeformSystem : public System {
 public:
  // Note: DeformMode enum is in deform_def.fbs

  explicit DeformSystem(Registry* registry);

  ~DeformSystem() override;

  // Sets and entity to be deformed.
  void Create(Entity e, HashValue type, const Def* def) override;

  // Sets an entity to be deformed (not a deformer).
  void SetAsDeformed(Entity entity, string_view path_id = "");

  // Disassociate data from the Entity.
  void Destroy(Entity e) override;

  // Returns true if |entity| is set as deformed.
  bool IsSetAsDeformed(Entity entity) const;

  // Returns true if the entity should be deformed (it's both set as deformed
  // and has a deformer).
  bool IsDeformed(Entity e) const;

  // Returns the cylindrical deformation radius for the Entity or 0 if no
  // deform has been set for that Entity.
  float GetDeformRadius(Entity e) const;

  // Returns the deformation mode the entity is currently using or kNone if
  // the entity does not have a deformed component.
  DeformMode GetDeformMode(Entity e) const;

  // Returns the bounding box of the entity before deformation was applied.
  const Aabb* UndeformedBoundingBox(Entity entity) const;

 private:
  // All the data for one Waypoint in waypoint deformation mode.
  struct Waypoint {
    // The original position in deformer's coordinate system that matches this
    // waypoint.
    mathfu::vec3 original_position;
    // The position of the deformed entity at this waypoint.
    mathfu::vec3 remapped_position;
    // The base rotation of the deformed entity at this waypoint.
    mathfu::vec3 remapped_rotation;

    // Normalized coordinates representing a point in the Deformed's aabb that
    // will match with original_position. (0,0,0) is the left, bottom, far
    // corner, and (1,1,1) is the right, top, near corner.
    // Ignored if use_aabb_anchor is false.
    mathfu::vec3 original_aabb_anchor;
    // Normalized coordinates representing a point in the Deformed's aabb that
    // will match with remapped_position. (0,0,0) is the left, bottom, far
    // corner, and (1,1,1) is the right, top, near corner.
    // Ignored if use_aabb_anchor is false.
    mathfu::vec3 remapped_aabb_anchor;
  };

  // A set of waypoints that define a deformation along a path.
  struct WaypointPath {
    // The unique ID of this path. Only deformed entities with a matching
    // path_id will be deformed by this path.
    HashValue path_id;
    // The set of deformed positions and rotations along this path.
    std::vector<Waypoint> waypoints;
    // A set of scalars representing the parameterized values of the points to
    // be mapped. For each input point, this is simply the dot product of that
    // point along the parameterization axis.
    std::vector<float> parameterization_values;
    // The axis along which to parameterize the input points.
    mathfu::vec3 parameterization_axis;

    // True if any of the waypoints in the path use_aabb_anchor.
    bool use_aabb_anchor = false;
  };

  struct Deformer : Component {
    explicit Deformer(Entity e)
        : Component(e),
          radius(0.f),
          mode(DeformMode::DeformMode_GlobalCylinder) {}
    Deformer(Entity e, const Deformer& prototype)
        : Component(e), radius(prototype.radius), mode(prototype.mode) {}
    float radius;
    DeformMode mode;
    float clamp_angle;
    std::unordered_map<HashValue, WaypointPath> paths;
  };

  struct Deformed : Component {
    explicit Deformed(Entity e) : Component(e), deformer(kNullEntity) {}
    Deformed(Entity e, const Deformed& prototype)
        : Component(e), deformer(prototype.deformer) {}
    mutable mathfu::mat4 deformer_from_entity_undeformed_space;
    Entity deformer;
    Aabb undeformed_aabb;
    HashValue path_id;

    // If the path that path_id points uses aabb anchors, Deformed needs to
    // keep a cached version that offsets based on its entity's aabb.
    std::unique_ptr<WaypointPath> anchored_path;
  };

  // Builds a WaypointPath from its def.
  Optional<WaypointPath> BuildWaypointPath(
      const lull::WaypointPath& waypoint_path_def) const;

  // Calculates the parameterization axis for a path by finding the unit vector
  // pointing to the last point in the path from the first point. Also
  // calculates the values for each point in the path.
  void CalculateWaypointParameterization(WaypointPath* path) const;

  // Determines the world from entity transformation function and applies it to
  // the deformed entity in the transform system. This does not update the mesh
  // deformation function.
  void ApplyDeform(Entity e, const Deformer* deformer);

  // Deforms the mesh for the given entity. If deformer_entity is null then this
  // function will always recalculate the deformer based on the current
  // transform hierarchy. It should not be called more than once for each
  // entity.
  void DeformMesh(Entity e, float* data, size_t len, size_t stride);

  // Calculates the deformed world from entity transformation matrix for the
  // given entity. We expect this function to be called frequently by the
  // transformation system.
  mathfu::mat4 CalculateMatrixCylinderBend(
      Entity e, const Sqt& local_sqt,
      const mathfu::mat4* world_from_parent_mat) const;

  // Calculates the deformed world from a series of input and output positions
  // and rotations.
  mathfu::mat4 CalculateWaypointTransformMatrix(
      Entity e, const Sqt& local_sqt,
      const mathfu::mat4* world_from_parent_mat) const;

  // Deforms the given deformed component entity's mesh according to the radius
  // and position of the given deformer entity. We expect this function to be
  // called whenever someone updates the mesh of the deformed component.
  void CylinderBendDeformMesh(const Deformed& deformed,
                              const Deformer& deformer, float* data, size_t len,
                              size_t stride) const;

  void OnParentChanged(const ParentChangedEvent& ev);

  // Recursively sets the deformer for all child entity's deformed components in
  // the transform hierarchy. If there is any entity that does not have a
  // deformed component, then none of its children, grandchildren, etc will be
  // deformed even if they have the component.
  void SetDeformerRecursive(Deformed* deformed, const Deformer* deformer);

  // Sets |entity|'s deformation function in the render system.  This should be
  // done as early as possible so the render system can defer mesh creation if
  // necessary.
  void SetDeformationFunction(Entity entity);

  // Grabs the deformer for an entity if it exists and sets
  // deformer_from_entity_undeformed_space. Returns true if the deformed entity
  // exists and should be deformed.
  bool PrepDeformerFromEntityUndeformedSpace(const Entity& e,
                                             const Sqt& local_sqt,
                                             const Deformer* deformer,
                                             const Deformed* deformed) const;

  // If |entity| is a Deformed with a waypoint path that uses aabb anchors,
  // recalculate the cached path.
  void RecalculateAnchoredPath(Entity entity);

  // DEPRECATED DO NOT USE
  Entity GetDeformerDeprecated(Entity e) const;

  ComponentPool<Deformer> deformers_;
  ComponentPool<Deformed> deformed_;

  DeformSystem(const DeformSystem&);
  DeformSystem& operator=(const DeformSystem&);
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::DeformSystem);

#endif  // LULLABY_SYSTEMS_DEFORM_DEFORM_SYSTEM_H_
