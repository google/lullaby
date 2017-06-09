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

#include "lullaby/systems/scroll/scroll_channels.h"

#include "lullaby/systems/animation/animation_system.h"
#include "lullaby/systems/scroll/scroll_system.h"
#include "lullaby/util/logging.h"

namespace lull {

const HashValue ScrollViewOffsetChannel::kChannelName =
    Hash("scroll-view-offset");

ScrollViewOffsetChannel::ScrollViewOffsetChannel(Registry* registry,
                                                 size_t pool_size)
    : AnimationChannel(registry, 2, pool_size) {}

void ScrollViewOffsetChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  if (animation_system) {
    AnimationChannelPtr ptr(new ScrollViewOffsetChannel(registry, pool_size));
    animation_system->AddChannel(kChannelName, std::move(ptr));
  } else {
    LOG(DFATAL) << "Failed to register ScrollViewOffsetChannel";
  }
}

bool ScrollViewOffsetChannel::Get(Entity e, float* values, size_t len) const {
  const auto* scroll_system = registry_->Get<ScrollSystem>();
  const mathfu::vec2 offset = scroll_system->GetViewOffset(e);
  values[0] = offset.x;
  values[1] = offset.y;
  return true;
}

void ScrollViewOffsetChannel::Set(Entity e, const float* values, size_t len) {
  auto* scroll_system = registry_->Get<ScrollSystem>();
  const mathfu::vec2 offset(values[0], values[1]);
  scroll_system->ActuallySetViewOffset(e, offset);
}

}  // namespace lull
