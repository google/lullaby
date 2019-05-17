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
  using BoneIndices = Span<uint8_t>;

  // A pose is defined by a transform for each bone in the rig.
  using Pose = Span<mathfu::AffineTransform>;

  explicit RigSystem(Registry* registry, bool use_ubo = false);

  // Initializes the "rig" animation channel to pass pose information from
  // the AnimationSystem to the RenderSystem.
  void Initialize() override;

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

  // Returns the array of bone names associated with |entity|.
  Span<std::string> GetBoneNames(Entity entity) const;

  // Returns the array of parent bone indices associated with |entity|.
  BoneIndices GetBoneParentIndices(Entity entity) const;

  // Returns the array of default bone transform inverses (AKA inverse bind-pose
  // matrices) associated with |entity|.
  Pose GetDefaultBoneTransformInverses(Entity entity) const;

  // Returns the array of bone transforms representing the current pose
  // associated with |entity|.
  Pose GetPose(Entity entity) const;

  // Whether to use Uniform Buffer Objects for bone transforms. Allows more
  // bones to be used without exceeding driver limits, but requires the skinning
  // shaders to accept the transforms as UBOs rather than plain uniforms.
  bool UseUbo() const { return use_ubo_; }

 private:
  using AffineMatrixAllocator = mathfu::simd_allocator<mathfu::AffineTransform>;

  struct RigComponent {
    // The number of elements represents the number of bones in the rig, and
    // each element refers to the parent bone for the bone at that index. A
    // value of kInvalidBoneIdx (0xff or 255) indicates a root bone with no
    // parent.
    std::vector<uint8_t> parent_indices;

    // An (optional) list of bone names useful for debugging.
    std::vector<std::string> bone_names;

    // The current pose of the Entity represented by the local transform of each
    // bone. Typically updated once per-frame by a rig animation.
    std::vector<mathfu::AffineTransform, AffineMatrixAllocator> pose;

    // The default inverse bind pose of each bone in the rig. These matrices
    // transform vertices into the space of each bone so that skinning can be
    // applied. They are mulitplied with the individual bone pose transforms to
    // generate the final flattened pose that is sent to skinning shaders.
    std::vector<mathfu::AffineTransform, AffineMatrixAllocator>
        inverse_bind_pose;

    // Maps a bone to a given uniform index in the skinning shader. Since not
    // all bones are required for skinning and may just be necessary for
    // computing their descendants' transforms, we only upload bones used for
    // skinning to the shader. Each value in this vector is an index into
    // |pose|, |inverse_bind_pose|, and |root_indices| to get the matrices
    // necessary for the final |shader_pose|.
    std::vector<uint8_t> shader_indices;

    // The flattened pose data passed to the shader for skinning. See the
    // comments in UpdateShaderTransforms() for an explanation of how these are
    // computed.
    std::vector<mathfu::AffineTransform, AffineMatrixAllocator> shader_pose;
  };

  void UpdateShaderTransforms(Entity entity, RigComponent* rig);

  const bool use_ubo_;
  std::unordered_map<Entity, RigComponent> rigs_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::RigSystem);

#endif  // LULLABY_SYSTEMS_RIG_RIG_SYSTEM_H_
