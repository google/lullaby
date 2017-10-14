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

#ifndef LULLABY_SYSTEMS_RIG_RIG_SYSTEM_H_
#define LULLABY_SYSTEMS_RIG_RIG_SYSTEM_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/span.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

// Manages a skeletal rig per Entity.
//
// The RigSystem stores information about skeletal rigs and poses for use by
// both the Animation and RenderSystems. It allows for performing additional
// manipulations on the skeleton before sending the information to the
// RenderSystem for drawing.
class RigSystem : public System {
 public:
  // A list of bone indices.
  using BoneIndices = std::vector<uint8_t>;

  // A pose is defined by a transform for each bone in the rig.
  using Pose = std::vector<mathfu::AffineTransform>;

  explicit RigSystem(Registry* registry);

  RigSystem(const RigSystem&) = delete;
  RigSystem& operator=(const RigSystem&) = delete;

  // Removes the rig and poses from the Entity.
  void Destroy(Entity entity) override;

  // Sets the skeletal rig for the Entity.
  void SetRig(Entity entity, BoneIndices parent_indices, Pose inverse_bind_pose,
              BoneIndices shader_indices,
              std::vector<std::string> bone_names = {});

  // Sets the current pose for the Entity.
  void SetPose(Entity entity, Pose pose);

  // Returns the number of bones associated with |entity|.
  size_t GetNumBones(Entity entity) const;

  // Returns the array of bone indices associated with |entity|.
  Span<uint8_t> GetBoneIndices(Entity entity) const;

  // Returns the array of parent bone indices associated with |entity|.
  Span<uint8_t> GetBoneParentIndices(Entity entity) const;

  // Returns the array of bone names associated with |entity|.
  Span<std::string> GetBoneNames(Entity entity) const;

  // Returns the array of default bone transform inverses (AKA inverse bind-pose
  // matrices) associated with |entity|.
  Span<mathfu::AffineTransform> GetDefaultBoneTransformInverses(
      Entity entity) const;

 private:
  struct RigComponent {
    // The number of elements represents the number of bones in the rig, and
    // each element refers to the parent bone for the bone at that index.
    BoneIndices parent_indices;

    // An (optional) list of bone names useful for debugging.
    std::vector<std::string> bone_names;

    // The current pose of the Entity.
    Pose pose;

    // The default inverse bind pose of the rig.  These matrices are mulitplied
    // with the individual bone pose transforms to generate the final flattened
    // pose.
    Pose inverse_bind_pose;

    // Maps a bone to a given uniform index in the shader.
    BoneIndices shader_indices;

    // The flattened pose data passed to the shader.
    Pose shader_pose;
  };

  void UpdateShaderTransforms(Entity entity, RigComponent* rig);

  std::unordered_map<Entity, RigComponent> rigs_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RigSystem);

#endif  // LULLABY_SYSTEMS_RIG_RIG_SYSTEM_H_
