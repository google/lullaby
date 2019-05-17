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

#include "lullaby/contrib/mutator/face_point_mutator_system.h"

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

namespace {
constexpr HashValue kFacePointMutatorDef = ConstHash("FacePointMutatorDef");
constexpr float kMaxPitch = mathfu::kPi / 2;
}

FacePointMutatorSystem::FacePointMutatorSystem(Registry* registry)
    : System(registry) {
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<MutatorSystem>(this);
  RegisterDef<FacePointMutatorDefT>(this);
}

void FacePointMutatorSystem::Create(Entity entity, HashValue type,
                                    const Def* def) {
  if (type != kFacePointMutatorDef) {
    LOG(DFATAL)
        << "Invalid type passed to Create. Expecting FacePointMutatorDef!";
    return;
  }
  const auto* data = ConvertDef<FacePointMutatorDef>(def);
  auto& mutator = mutators_.emplace(entity, Mutator{})->second;
  mutator.group = data->group();
  mutator.order = data->order();
  MathfuVec3FromFbVec3(data->target_point(), &mutator.target_point);
  mutator.face_hmd = data->face_hmd();
  mutator.arctic_radian = data->arctic_degree() * mathfu::kPi / 180.f;

  registry_->Get<MutatorSystem>()->RegisterSqtMutator(entity, mutator.group,
                                                      mutator.order, this);
}

void FacePointMutatorSystem::Mutate(Entity entity, HashValue group, int order,
                                    Sqt* mutate, bool require_valid) {
  const Mutator* mutator = nullptr;
  auto mutators = mutators_.equal_range(entity);
  for (auto iter = mutators.first; iter != mutators.second; ++iter) {
    if (iter->second.group == group && iter->second.order == order) {
      mutator = &(iter->second);
      break;
    }
  }
  if (mutator == nullptr) {
    LOG(DFATAL) << "FacePointMutator not found: " << entity << ", " << group
                << ", " << order;
    return;
  }

  auto* transform_system = registry_->Get<TransformSystem>();
  mathfu::mat4 parent_from_hmd_mat =
      registry_->Get<InputManager>()->GetDofWorldFromObjectMatrix(
          InputManager::DeviceType::kHmd);
  Entity parent = transform_system->GetParent(entity);
  if (parent != kNullEntity) {
    parent_from_hmd_mat =
        transform_system->GetWorldFromEntityMatrix(parent)->Inverse() *
        parent_from_hmd_mat;
  }

  const mathfu::vec3 target_pos =
      mutator->face_hmd ? parent_from_hmd_mat.TranslationVector3D()
                        : mutator->target_point;

  const mathfu::vec3 entity_pos = transform_system->GetLocalTranslation(entity);
  mathfu::vec3 up = mathfu::kAxisY3f;
  // If arctic area is valid, blending +Y axis with HMD up direction as the
  // final up direction.
  if (mutator->arctic_radian < kMaxPitch) {
    const mathfu::vec3 entity_world_pos =
        transform_system->GetWorldFromEntityMatrix(entity)
            ->TranslationVector3D();
    const float abs_pitch =
        std::abs(asinf(entity_world_pos.y / entity_world_pos.Length()));
    if (abs_pitch > mutator->arctic_radian) {
      const mathfu::vec3 hmd_up_vector =
          mathfu::mat4::ToRotationMatrix(parent_from_hmd_mat) *
          mathfu::kAxisY3f;
      up = ((abs_pitch - mutator->arctic_radian) * hmd_up_vector +
            (kMaxPitch - abs_pitch) * mathfu::kAxisY3f)
               .Normalized();
    }
  }

  // Calculate the coordinate that its negative Z-axis points from
  // target_pos (target point or HMD) to entity_pos.
  const mathfu::mat4 lookat_mat =
      mathfu::mat4::LookAt(entity_pos, target_pos, up, 1);
  // The LookAt() function gives the coordinate transformation. We need its
  // inverse as the correct object transformation. In order to avoid
  // expensive computation, here we use the transpose of the rotation matrix to
  // get its inverse.
  mutate->rotation = mathfu::quat::FromMatrix(
      mathfu::mat4::ToRotationMatrix(lookat_mat).Transpose());
}

}  // namespace lull
