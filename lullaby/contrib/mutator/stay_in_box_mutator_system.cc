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

#include "lullaby/contrib/mutator/stay_in_box_mutator_system.h"

#include "mathfu/io.h"
#include "lullaby/modules/ecs/system.h"
#include "lullaby/systems/transform/transform_system.h"
#include "lullaby/modules/flatbuffers/mathfu_fb_conversions.h"
#include "lullaby/generated/stay_in_box_mutator_def_generated.h"

namespace lull {

namespace {
constexpr HashValue kStayInBoxMutatorDef = ConstHash("StayInBoxMutatorDef");

float SoftClamp(float value, float min, float max, float negative_stretch,
                float positive_stretch) {
  DCHECK(negative_stretch <= 0 && positive_stretch >= 0)
      << "stretch values are offsets from min and max";
  if (value < min) {
    const float distance_past = value - min;
    const float stretch_factor = (1 - distance_past);
    return min + negative_stretch - negative_stretch / stretch_factor;
  } else if (value > max) {
    const float distance_past = value - max;
    const float stretch_factor = (1 + distance_past);
    return max + positive_stretch - positive_stretch / stretch_factor;
  }
  return value;
}

mathfu::vec3 SoftClampVector(const mathfu::vec3 target, const Aabb& hard,
                             const Aabb& stretch) {
  return mathfu::vec3(
      SoftClamp(target.x, hard.min.x, hard.max.x, stretch.min.x, stretch.max.x),
      SoftClamp(target.y, hard.min.y, hard.max.y, stretch.min.y, stretch.max.y),
      SoftClamp(target.z, hard.min.z, hard.max.z, stretch.min.z,
                stretch.max.z));
}

}  // namespace

StayInBoxMutatorSystem::StayInBoxMutatorSystem(Registry* registry)
    : System(registry) {
  RegisterDependency<TransformSystem>(this);
  RegisterDependency<MutatorSystem>(this);
  RegisterDef<StayInBoxMutatorDefT>(this);
}

void StayInBoxMutatorSystem::Create(Entity entity, HashValue type,
                                    const Def* def) {
  if (type != kStayInBoxMutatorDef) {
    LOG(DFATAL)
        << "Unrecognized component type in StayInBoxMutatorSystem.Create()";
    return;
  }
  const auto* data = ConvertDef<StayInBoxMutatorDef>(def);
  auto& mutator = mutators_.emplace(entity, Mutator{})->second;
  mutator.space = data->space();
  mutator.group = data->group();
  mutator.order = data->order();

  AabbFromFbAabb(data->box(), &mutator.box);
  AabbFromFbAabb(data->stretch(), &mutator.stretch);

  registry_->Get<MutatorSystem>()->RegisterSqtMutator(entity, mutator.group,
                                                      mutator.order, this);
}

void StayInBoxMutatorSystem::Mutate(Entity entity, HashValue group, int order,
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
    LOG(DFATAL) << "StayInBoxMutator not found: " << entity << ", " << group
                << ", " << order;
  }

  // If we're requiring valid, just set the stretch to the empty, which forces
  // the sqt to be actually inside the bounds.
  const Aabb stretch = require_valid ? Aabb() : mutator->stretch;
  switch (mutator->space) {
    case MutateSpace_Parent: {
      // Constrain the sqt's position within the parent's space.
      mutate->translation =
          SoftClampVector(mutate->translation, mutator->box, stretch);
      break;
    }
    case MutateSpace_World: {
      // Constrain the sqt's position within world space.
      auto* transform_system = registry_->Get<TransformSystem>();
      Entity parent = transform_system->GetParent(entity);
      if (parent == kNullEntity) {
        // no parent - Sqt is already in world space.
        mutate->translation =
            SoftClampVector(mutate->translation, mutator->box, stretch);
        break;
      }

      // Entity has ancestors, so need to work in world space:
      const mathfu::mat4* mat =
          transform_system->GetWorldFromEntityMatrix(parent);
      if (mat != nullptr) {
        // Get the point in world space:
        mathfu::vec3 mutated_world_pos = *mat * mutate->translation;
        // Clamp it:
        mutated_world_pos =
            SoftClampVector(mutated_world_pos, mutator->box, stretch);
        // Put the clamped position back into local space.
        mutate->translation = mat->Inverse() * mutated_world_pos;
      }
      break;
    }
    default:
      LOG(DFATAL) << "StayInBoxMutator only supports Parent and World spaces.";
  }
}

}  // namespace lull
