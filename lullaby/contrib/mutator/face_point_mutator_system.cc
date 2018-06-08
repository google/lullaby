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

#include "lullaby/contrib/mutator/face_point_mutator_system.h"

#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/modules/input/input_manager.h"
#include "lullaby/systems/transform/transform_system.h"

namespace lull {

namespace {
constexpr HashValue kFacePointMutatorDef = ConstHash("FacePointMutatorDef");
}

FacePointMutatorSystem::FacePointMutatorSystem(Registry* registry)
    : System(registry) {
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<MutatorSystem>(this);
  RegisterDef(this, kFacePointMutatorDef);
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
  mathfu::mat4 hmd_mat =
      registry_->Get<InputManager>()->GetDofWorldFromObjectMatrix(
          InputManager::DeviceType::kHmd);
  mathfu::vec3 target_pos;
  if (mutator->face_hmd) {
    target_pos = hmd_mat.TranslationVector3D();
  } else {
    target_pos = mutator->target_point;
  }
  mathfu::vec3 entity_pos =
      transform_system->GetWorldFromEntityMatrix(entity)->TranslationVector3D();
  // Make the entity's up vector the same as HMD's up vector.
  mathfu::vec3 up = mathfu::mat4::ToRotationMatrix(hmd_mat) * mathfu::kAxisY3f;

  // Calculate the coordinate that its negative Z-axis points from
  // target_pos (target point or HMD) to entity_pos.
  mathfu::mat4 lookat_mat = mathfu::mat4::LookAt(entity_pos, target_pos, up, 1);
  // The LookAt() function gives the coordinate transformation. We need its
  // inverse as the correct object transformation. In order to avoid
  // expensive computation, here we use the transpose of the rotation matrix to
  // get its inverse.
  mutate->rotation = mathfu::quat::FromMatrix(
      mathfu::mat4::ToRotationMatrix(lookat_mat).Transpose());
}

}  // namespace lull
