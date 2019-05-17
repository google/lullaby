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

#include "lullaby/modules/animation_channels/deform_channels.h"

#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/contrib/deform/deform_system.h"
#include "lullaby/util/logging.h"

namespace lull {

const HashValue DeformStrengthChannel::kChannelName =
    ConstHash("deform-strength");

DeformStrengthChannel::DeformStrengthChannel(Registry* registry,
                                             size_t pool_size)
    : AnimationChannel(registry, 1, pool_size),
      deform_system_(registry->Get<DeformSystem>()) {}

void DeformStrengthChannel::Setup(Registry* registry, size_t pool_size) {
  auto animation_system = registry->Get<AnimationSystem>();
  auto deform_system = registry->Get<DeformSystem>();
  if (animation_system == nullptr || deform_system == nullptr) {
    LOG(DFATAL) << "Failed to setup DeformStrengthChannel.";
  }
  AnimationChannelPtr ptr(new DeformStrengthChannel(registry, pool_size));
  animation_system->AddChannel(kChannelName, std::move(ptr));
}

bool DeformStrengthChannel::Get(Entity entity, float* values,
                                size_t length) const {
  Optional<float> strength = deform_system_->GetDeformStrength(entity);
  if (!strength || length < 1) {
    return false;
  }
  values[0] = *strength;
  return true;
}

void DeformStrengthChannel::Set(Entity entity, const float* values,
                                size_t length) {
  if (length < 1) {
    return;
  }
  deform_system_->SetDeformStrength(entity, values[0]);
}

}  // namespace lull
