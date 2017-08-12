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

#include "lullaby/modules/animation_channels/audio_channels.h"

#include "lullaby/systems/animation/animation_system.h"

namespace lull {

const HashValue VolumeChannel::kVolumeChannelName = Hash("audio-volume");
const int VolumeChannel::kNumDimensions = 1;

VolumeChannel::VolumeChannel(Registry* registry, size_t pool_size)
    : AnimationChannel(registry, kNumDimensions, pool_size),
      audio_system_(registry->Get<AudioSystem>()) {}

void VolumeChannel::Setup(Registry* registry, size_t pool_size) {
  auto* animation_system = registry->Get<AnimationSystem>();
  auto* audio_system = registry->Get<AudioSystem>();
  if (animation_system && audio_system) {
    animation_system->AddChannel(
        kVolumeChannelName,
        AnimationChannelPtr(new VolumeChannel(registry, pool_size)));
  } else {
    LOG(DFATAL) << "Failed to setup VolumeChannel.";
  }
}

bool VolumeChannel::Get(Entity entity, float* values, size_t len) const {
  float volume = audio_system_->GetVolume(entity);
  if (volume >= 0) {
    values[0] = volume;
    return true;
  }
  return false;
}

void VolumeChannel::Set(Entity entity, const float* values, size_t len) {
  audio_system_->SetVolume(entity, values[0]);
}

}  // namespace lull
