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

#include "lullaby/systems/skin/skin_system.h"

#include "lullaby/modules/script/function_binder.h"
#include "lullaby/systems/render/render_system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/util/math.h"

namespace lull {

SkinSystem::SkinSystem(Registry* registry, bool use_ubo)
    : System(registry), use_ubo_(use_ubo) {
  RegisterDependency<TransformSystem>(this);

  FunctionBinder* binder = registry->Get<FunctionBinder>();
  if (binder) {
    binder->RegisterMethod("lull.Skin.AdvanceFrame",
                           &lull::SkinSystem::AdvanceFrame);
  }
}

SkinSystem::~SkinSystem() {
  FunctionBinder* binder = registry_->Get<FunctionBinder>();
  if (binder) {
    binder->UnregisterFunction("lull.Skin.AdvanceFrame");
  }
}

void SkinSystem::AdvanceFrame() {
  for (auto iter = skins_.begin(); iter != skins_.end(); ++iter) {
    UpdateShaderTransforms(iter->first, &iter->second);
  }
}

void SkinSystem::Destroy(Entity entity) {
  skins_.erase(entity);
}

void SkinSystem::SetSkin(Entity entity, Skin skin, Pose inverse_bind_pose) {
  if (skin.empty()) {
    LOG(DFATAL) << "Ignoring empty skin.";
    return;
  }
  if (skin.size() > kMaxNumBones) {
    LOG(DFATAL) << "Skins must have no more than "
                << static_cast<int>(kMaxNumBones) << " bones.";
    return;
  }

  if (skin.size() != inverse_bind_pose.size()) {
    LOG(DFATAL) << "Each bone requires an inverse bind pose.";
    return;
  }

  auto res = skins_.emplace(entity, SkinComponent());
  if (!res.second) {
    LOG(ERROR) << "Cannot replace a skin.";
    return;
  }

  SkinComponent& skin_component = res.first->second;
  skin_component.bones.assign(skin.begin(), skin.end());
  skin_component.inverse_bind_pose.assign(inverse_bind_pose.begin(),
                                          inverse_bind_pose.end());

  UpdateShaderTransforms(entity, &skin_component);
}

size_t SkinSystem::GetNumBones(Entity entity) const {
  auto iter = skins_.find(entity);
  if (iter != skins_.end()) {
    return iter->second.bones.size();
  }
  return 0;
}

SkinSystem::Pose SkinSystem::GetInverseBindPose(Entity entity) const {
  auto iter = skins_.find(entity);
  if (iter != skins_.end()) {
    return iter->second.inverse_bind_pose;
  }
  return {};
}

void SkinSystem::UpdateShaderTransforms(Entity entity, SkinComponent* skin) {
  auto* transform_system = registry_->Get<TransformSystem>();

  const mathfu::mat4 skin_from_world =
      transform_system->GetWorldFromEntityMatrix(entity)->Inverse();

  const size_t num_bones = skin->bones.size();
  skin->shader_pose.resize(num_bones);
  for (size_t i = 0; i < num_bones; ++i) {
    const auto* world_from_bone =
        transform_system->GetWorldFromEntityMatrix(skin->bones[i]);
    // The shader_pose matrix transforms a vertex from "unskinned world space"
    // to "world space". As a formula:
    //
    // V_world_skinned = M_world_from_skin *
    //                   M_skin_from_world *
    //                   M_world_skinned_from_bone *
    //                   M_bone_from_mesh *
    //                   V_mesh
    //
    // In reverse order:
    // 1. V_mesh is the original vertex in mesh space. These are specified in
    //    assets and copied into the RenderSystem as-is.
    // 2. M_bone_from_mesh is the "inverse bind pose matrix" of the bone. It
    //    transforms vertices into the space of the bone so that the bone may
    //    "influence" the vertices.
    // 3. M_world_skinned_from_bone is the world-from-entity matrix of the bone.
    //    It applies the "influence" of each bone for a particular vertex, which
    //    is typically relative to the skeleton root, but also includes the
    //    remaining transforms to get the vertex into world space.
    // 4. M_skin_from_world is the inverse of the skinned Entity's
    //    world-from-entity matrix. See the next comment.
    // 5. M_world_from_skin is the skinned Entity's world-from-entity matrix.
    //    Skinned Entities replace the mesh Entity's world-from-entity matrix
    //    with the skeleton root's world-from-entity matrix. Since the skeleton
    //    root's matrix is included in 3) and this matrix is in all of our
    //    vertex shaders, we use 4) to clobber this matrix.
    // 6. V_world_skinned is the skinned world space vertex.
    //
    // We combine 2 through 4 into a single "shader pose" matrix to transform
    // vertices from "mesh space" to "skinned world space".
    skin->shader_pose[i] = mathfu::mat4::ToAffineTransform(
        skin_from_world *
        *world_from_bone *
        mathfu::mat4::FromAffineTransform(skin->inverse_bind_pose[i]));
  }

  constexpr int kDimension = 4;
  constexpr int kNumVec4sInAffineTransform = 3;
  constexpr const char* kUniform = "bone_transforms";
  const float* data = &skin->shader_pose[0][0];
  const int count = kNumVec4sInAffineTransform * static_cast<int>(num_bones);
  auto* render_system = registry_->Get<RenderSystem>();
  if (UseUbo()) {
    render_system->SetUniform(entity, kUniform,
                              lull::ShaderDataType_BufferObject,
                              {reinterpret_cast<const uint8_t*>(data),
                               kDimension * count * sizeof(data[0])});
  } else {
    render_system->SetUniform(entity, kUniform, data, kDimension, count);
  }
}

}  // namespace lull
