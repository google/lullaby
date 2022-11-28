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

#include "redux/systems/rig/rig_system.h"

#include <cstddef>

#include "redux/modules/base/logging.h"

namespace redux {

RigSystem::RigSystem(Registry* registry) : System(registry) {}

void RigSystem::SetSkeleton(Entity entity,
                            absl::Span<const std::string_view> bones,
                            absl::Span<const BoneIndex> hierarchy) {
  const std::size_t num_bones = bones.size();
  CHECK_GT(num_bones, 0);
  CHECK_EQ(bones.size(), hierarchy.size());

  Rig& rig = rigs_[entity];
  // Clear out all the data in case we're reassigning a new skeleton.
  rig.bones.clear();
  rig.bone_map.clear();
  rig.hierarchy.clear();
  rig.pose.clear();

  rig.bones.reserve(num_bones);
  rig.hierarchy.reserve(num_bones);

  rig.pose.resize(num_bones, mat4::Identity());
  for (std::size_t i = 0; i < num_bones; ++i) {
    const HashValue bone_name = Hash(bones[i]);
    const BoneIndex parent_index = hierarchy[i];
    if (i == 0) {
      CHECK_EQ(parent_index, kInvalidBoneIndex) << "First bone must be root";
    } else {
      CHECK_LT(parent_index, i) << "Child bones must come after parent bones.";
    }

    rig.bones.push_back(bone_name);
    rig.bone_map[bone_name] = static_cast<BoneIndex>(i);
    rig.hierarchy.push_back(parent_index);
  }
}

void RigSystem::OnDestroy(Entity entity) { rigs_.erase(entity); }

void RigSystem::UpdatePose(Entity entity, absl::Span<const mat4> pose) {
  if (auto iter = rigs_.find(entity); iter != rigs_.end()) {
    Rig& rig = iter->second;
    CHECK_EQ(pose.size(), rig.bones.size()) << "Skeleton/pose size mismatch";
    rig.pose.assign(pose.begin(), pose.end());
  }
}

absl::Span<const mat4> RigSystem::GetPose(Entity entity) const {
  if (auto iter = rigs_.find(entity); iter != rigs_.end()) {
    return iter->second.pose;
  } else {
    return {};
  }
}

mat4 RigSystem::GetBonePose(Entity entity, HashValue bone) const {
  if (auto iter = rigs_.find(entity); iter != rigs_.end()) {
    auto& bone_map = iter->second.bone_map;
    if (auto bone_iter = bone_map.find(bone); bone_iter != bone_map.end()) {
      const std::size_t bone_index = bone_iter->second;
      return iter->second.pose[bone_index];
    }
  }
  return mat4::Identity();
}

}  // namespace redux
