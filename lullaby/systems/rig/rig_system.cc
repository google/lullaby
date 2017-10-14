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

#include "lullaby/systems/rig/rig_system.h"
#include "lullaby/systems/render/render_system.h"

namespace lull {

RigSystem::RigSystem(Registry* registry) : System(registry) {}

void RigSystem::SetRig(Entity entity, BoneIndices parent_indices,
                       Pose inverse_bind_pose, BoneIndices shader_indices,
                       std::vector<std::string> bone_names) {
  const size_t num_bones = parent_indices.size();
  if (num_bones == 0) {
    return;
  } else if (num_bones != inverse_bind_pose.size()) {
    return;
  }

  auto res = rigs_.emplace(entity, RigComponent());
  if (!res.second) {
    return;
  }
  RigComponent& rig = res.first->second;
  rig.parent_indices = std::move(parent_indices);
  rig.inverse_bind_pose = std::move(inverse_bind_pose);
  rig.shader_indices = std::move(shader_indices);
  rig.bone_names = std::move(bone_names);
  UpdateShaderTransforms(entity, &rig);
}

void RigSystem::Destroy(Entity entity) { rigs_.erase(entity); }

size_t RigSystem::GetNumBones(Entity entity) const {
  auto iter = rigs_.find(entity);
  if (iter != rigs_.end()) {
    return iter->second.parent_indices.size();
  }
  return 0;
}

Span<uint8_t> RigSystem::GetBoneParentIndices(Entity entity) const {
  auto iter = rigs_.find(entity);
  if (iter != rigs_.end()) {
    return iter->second.parent_indices;
  }
  return {};
}

Span<std::string> RigSystem::GetBoneNames(Entity entity) const {
  auto iter = rigs_.find(entity);
  if (iter != rigs_.end()) {
    return iter->second.bone_names;
  }
  return {};
}

void RigSystem::SetPose(Entity entity, Pose pose) {
  auto iter = rigs_.find(entity);
  if (iter == rigs_.end()) {
    return;
  }

  RigComponent& rig = iter->second;
  if (pose.size() != rig.parent_indices.size()) {
    LOG(DFATAL) << "Bone mismatch.";
    return;
  }

  rig.pose = std::move(pose);
  UpdateShaderTransforms(entity, &rig);
}

void RigSystem::UpdateShaderTransforms(Entity entity, RigComponent* rig) {
  if (rig->pose.empty() || rig->parent_indices.empty()) {
    return;
  }

  const size_t num_bones = rig->shader_indices.size();
  rig->shader_pose.resize(num_bones);
  for (size_t i = 0; i < num_bones; ++i) {
    const uint8_t bone_index = rig->shader_indices[i];
    CHECK(bone_index < rig->parent_indices.size());

    const mathfu::AffineTransform& transform = rig->pose[bone_index];
    const mathfu::AffineTransform& inverse = rig->inverse_bind_pose[bone_index];

    rig->shader_pose[i] = mathfu::mat4::ToAffineTransform(
        mathfu::mat4::FromAffineTransform(transform) *
        mathfu::mat4::FromAffineTransform(inverse));
  }

  constexpr int kDimension = 4;
  constexpr int kNumVec4sInAffineTransform = 3;
  constexpr const char* kUniform = "bone_transforms";
  const float* data = &rig->shader_pose[0][0];
  const int count = kNumVec4sInAffineTransform * static_cast<int>(num_bones);

  auto* render_system = registry_->Get<RenderSystem>();
  render_system->SetUniform(entity, kUniform, data, kDimension, count);
}

}  // namespace lull
