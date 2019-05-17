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

#include "lullaby/contrib/backdrop/backdrop_channels.h"

#include "lullaby/contrib/backdrop/backdrop_system.h"
#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/util/logging.h"

namespace lull {

const HashValue BackdropAabbChannel::kChannelName = ConstHash("backdrop-aabb");

BackdropAabbChannel::BackdropAabbChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, 6, pool_size) {}

void BackdropAabbChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  if (animation_system) {
    AnimationChannelPtr ptr(new BackdropAabbChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to register BackdropAabbChannel";
  }
}

bool BackdropAabbChannel::Get(Entity entity, float* values, size_t len) const {
  const auto* backdrop_system = registry_->Get<BackdropSystem>();
  const Aabb* aabb = backdrop_system->GetBackdropAabb(entity);
  if (!aabb) {
    return false;
  }
  aabb->ToArray(values);
  return true;
}

void BackdropAabbChannel::Set(Entity entity, const float* values, size_t len) {
  auto* backdrop_system = registry_->Get<BackdropSystem>();
  const Aabb aabb(mathfu::vec3(values[0], values[1], values[2]),
                  mathfu::vec3(values[3], values[4], values[5]));
  backdrop_system->SetBackdropAabb(entity, aabb);
}

}  // namespace lull
