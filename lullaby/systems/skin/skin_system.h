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

#ifndef LULLABY_SYSTEMS_SKIN_SKIN_SYSTEM_H_
#define LULLABY_SYSTEMS_SKIN_SKIN_SYSTEM_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "lullaby/modules/ecs/component.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/util/span.h"
#include "mathfu/glsl_mappings.h"

namespace lull {

/// Manages a skin per Entity.
///
/// The SkinSystem stores information to skin an Entity's mesh using other
/// Entities as the bones in the skin. Manipulations of the actual bones at
/// runtime should be done directly through those Entity's TransformSystem data.
class SkinSystem : public System {
 public:
  /// A skin is defined by a list of Entities that represent bones.
  using Skin = Span<Entity>;

  /// A pose is defined by a transform for each bone in the skin.
  using Pose = Span<mathfu::AffineTransform>;

  explicit SkinSystem(Registry* registry, bool use_ubo = false);
  ~SkinSystem() override;

  SkinSystem(const SkinSystem&) = delete;
  SkinSystem& operator=(const SkinSystem&) = delete;

  /// Removes the skinning information for an Entity.
  void Destroy(Entity entity) override;

  /// Update the skinning uniforms for all skinned Entities.
  void AdvanceFrame();

  /// Sets the skin defining |entity| to the list of Entities representing the
  /// |skin| and their |inverse_bind_pose|.
  void SetSkin(Entity entity, Skin skin, Pose inverse_bind_pose);

  /// Returns the number of bone Entities associated with |entity|.
  size_t GetNumBones(Entity entity) const;

  /// Returns the array of bone names associated with |entity|.
  Skin GetSkin(Entity entity) const;

  /// Returns the array of inverse bind-pose matrices associated with |entity|.
  Pose GetInverseBindPose(Entity entity) const;

  /// Whether to use Uniform Buffer Objects for bone transforms. Allows more
  /// bones to be used without exceeding driver limits, but requires the
  /// skinning shaders to accept the transforms as UBOs rather than plain
  /// uniforms.
  bool UseUbo() const { return use_ubo_; }

 private:
  using AffineMatrixAllocator = mathfu::simd_allocator<mathfu::AffineTransform>;
  struct SkinComponent {
    Entity skeleton_root;
    std::vector<Entity> bones;
    std::vector<mathfu::AffineTransform, AffineMatrixAllocator>
        inverse_bind_pose;
    std::vector<mathfu::AffineTransform, AffineMatrixAllocator> shader_pose;
  };

  void UpdateShaderTransforms(Entity entity, SkinComponent* skin);

  std::unordered_map<Entity, SkinComponent> skins_;
  const bool use_ubo_;
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::SkinSystem);

#endif  /// LULLABY_SYSTEMS_SKIN_SKIN_SYSTEM_H_
