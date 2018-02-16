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

#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/util/make_unique.h"

namespace lull {

class RigChannel : public AnimationChannel {
 public:
  RigChannel(Registry* registry, size_t pool_size)
      : AnimationChannel(registry, 0, pool_size) {
    rig_system_ = registry->Get<RigSystem>();
  }

  bool IsRigChannel() const override { return true; }

  void Set(Entity e, const float* values, size_t len) override {
    LOG(DFATAL) << "SetRig should be called for rig channels.";
  }

  void SetRig(Entity entity, const mathfu::AffineTransform* values,
              size_t len) override {
    rig_system_->SetPose(entity, {values, len});
  }

  RigSystem* rig_system_;
};

RigSystem::RigSystem(Registry* registry) : System(registry) {}

void RigSystem::Initialize() {
  auto* animation_system = registry_->Get<AnimationSystem>();
  if (animation_system) {
    AnimationChannelPtr ptr = MakeUnique<RigChannel>(registry_, 8);
    animation_system->AddChannel(ConstHash("rig"), std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup RigChannel.";
  }
}

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
  rig.parent_indices.assign(parent_indices.begin(), parent_indices.end());

#if 0
  rig.inverse_bind_pose.assign(inverse_bind_pose.begin(),
                               inverse_bind_pose.end());
#else
  // TODO(b/72828343): The data in the inverse_bind_pose may not be aligned
  // correctly, so manually copy the floats into the rig's inverse_bind_pose.
  const size_t num_bind_bones = inverse_bind_pose.size();
  rig.inverse_bind_pose.reserve(num_bind_bones);
  for (size_t i = 0; i < num_bind_bones; ++i) {
    const auto& m = inverse_bind_pose[i];
    rig.inverse_bind_pose.emplace_back(m[0], m[1], m[2], m[3],
                                       m[4], m[5], m[6], m[7],
                                       m[8], m[9], m[10], m[11]);
  }
#endif

  rig.shader_indices.assign(shader_indices.begin(), shader_indices.end());
  rig.bone_names = std::move(bone_names);

  // Clear out any previous pose.
  rig.pose.resize(num_bones);
  for (size_t i = 0; i < rig.pose.size(); ++i) {
    rig.pose[i] = mathfu::mat4::ToAffineTransform(mathfu::mat4::Identity());
  }
  for (size_t i = 0; i < rig.shader_indices.size(); ++i) {
    const uint8_t bone_index = rig.shader_indices[i];
    CHECK(bone_index < rig.parent_indices.size());
    const mathfu::mat4 transform =
        mathfu::mat4::FromAffineTransform(rig.inverse_bind_pose[bone_index]);
    rig.pose[bone_index] = mathfu::mat4::ToAffineTransform(transform.Inverse());
  }
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

RigSystem::Pose RigSystem::GetDefaultBoneTransformInverses(
    Entity entity) const {
  auto iter = rigs_.find(entity);
  if (iter != rigs_.end()) {
    return iter->second.inverse_bind_pose;
  }
  return {};
}

RigSystem::Pose RigSystem::GetPose(Entity entity) const {
  auto iter = rigs_.find(entity);
  if (iter != rigs_.end()) {
    return iter->second.pose;
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
    LOG(DFATAL) << "Bone count mismatch. Expected " << rig.parent_indices.size()
                << " got " << pose.size() << ".";
    return;
  }

  rig.pose.assign(pose.begin(), pose.end());
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
