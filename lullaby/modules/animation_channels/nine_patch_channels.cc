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

#include "lullaby/modules/animation_channels/nine_patch_channels.h"

#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/nine_patch/nine_patch_system.h"

namespace lull {

const HashValue NinePatchSizeChannel::kChannelName =
    ConstHash("nine-patch-size");
const HashValue NinePatchOriginalSizeChannel::kChannelName =
    ConstHash("nine-patch-original-size");

NinePatchSizeChannel::NinePatchSizeChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 2, pool_size),
      nine_patch_system_(registry->Get<NinePatchSystem>()) {}

void NinePatchSizeChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* nine_patch_system = registry->Get<NinePatchSystem>();
  if (animation_system && nine_patch_system) {
    AnimationChannelPtr ptr(new NinePatchSizeChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup NinePatchSizeChannel.";
  }
}

bool NinePatchSizeChannel::Get(Entity e, float* values, size_t len) const {
  Optional<mathfu::vec2> size = nine_patch_system_->GetSize(e);
  if (size) {
    values[0] = size.value().x;
    values[1] = size.value().y;
    return true;
  }
  return false;
}

void NinePatchSizeChannel::Set(Entity e, const float* values, size_t len) {
  nine_patch_system_->SetSize(e, mathfu::vec2(values[0], values[1]));
}

NinePatchOriginalSizeChannel::NinePatchOriginalSizeChannel(Registry* registry,
                                                           size_t pool_size)
    : AnimationChannel(registry, 2, pool_size),
      nine_patch_system_(registry->Get<NinePatchSystem>()) {}

void NinePatchOriginalSizeChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* nine_patch_system = registry->Get<NinePatchSystem>();
  if (animation_system && nine_patch_system) {
    AnimationChannelPtr ptr(
        new NinePatchOriginalSizeChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to setup NinePatchOriginalSizeChannel.";
  }
}

bool NinePatchOriginalSizeChannel::Get(Entity e, float* values,
                                       size_t len) const {
  Optional<mathfu::vec2> size = nine_patch_system_->GetOriginalSize(e);
  if (size) {
    values[0] = size.value().x;
    values[1] = size.value().y;
    return true;
  }
  return false;
}

void NinePatchOriginalSizeChannel::Set(Entity e, const float* values,
                                       size_t len) {
  nine_patch_system_->SetOriginalSize(e, mathfu::vec2(values[0], values[1]));
}

}  // namespace lull
