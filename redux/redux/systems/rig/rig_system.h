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

#ifndef REDUX_SYSTEMS_RIG_RIG_SYSTEM_H_
#define REDUX_SYSTEMS_RIG_RIG_SYSTEM_H_

#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/types/span.h"
#include "redux/modules/ecs/system.h"
#include "redux/modules/math/matrix.h"

namespace redux {

// Stores the skeleton and current pose of an Entity.
//
// A skeleton is a collection of bones arranged in a tree-like hierarchy. A
// pose is a set of transforms that can be applied to each bone in a skeleton.
// Poses are stored in "Entity-space"; they are relative to the Entity's
// transform.
class RigSystem : public System {
 public:
  explicit RigSystem(Registry* registry);

  using BoneIndex = std::uint16_t;
  static constexpr BoneIndex kInvalidBoneIndex =
      std::numeric_limits<BoneIndex>::max();

  // Creates a skeleton for an Entity. An Entity must have a skeleton before it
  // can have a pose assigned to it.
  void SetSkeleton(Entity entity, absl::Span<const std::string_view> bones,
                   absl::Span<const BoneIndex> hierarchy);

  // Sets the current pose for the Entity.
  void UpdatePose(Entity entity, absl::Span<const mat4> pose);

  // Returns the current pose of the Entity, or an empty span if no pose.
  absl::Span<const mat4> GetPose(Entity entity) const;

  // Returns the transform of the given bone of the Entity.
  mat4 GetBonePose(Entity entity, HashValue bone) const;

 private:
  void OnDestroy(Entity entity) override;

  struct Rig {
    absl::flat_hash_map<HashValue, BoneIndex> bone_map;
    std::vector<HashValue> bones;
    std::vector<BoneIndex> hierarchy;
    std::vector<mat4> pose;
  };

  absl::flat_hash_map<Entity, Rig> rigs_;
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::RigSystem);

#endif  // REDUX_SYSTEMS_RIG_RIG_SYSTEM_H_
