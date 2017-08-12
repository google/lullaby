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

#ifndef LULLABY_MODULES_ANIMATION_CHANNELS_AUDIO_CHANNELS_H_
#define LULLABY_MODULES_ANIMATION_CHANNELS_AUDIO_CHANNELS_H_

#include "lullaby/systems/animation/animation_channel.h"
#include "lullaby/systems/audio/audio_system.h"
#include "lullaby/util/registry.h"

namespace lull {

// Channel for animating volume.
class VolumeChannel : public AnimationChannel {
 public:
  static const HashValue kVolumeChannelName;

  static void Setup(Registry* registry, size_t pool_size);

  VolumeChannel(Registry* registry, size_t pool_size);

 private:
  static const int kNumDimensions;

  bool Get(Entity e, float* values, size_t len) const override;
  void Set(Entity e, const float* values, size_t len) override;

  AudioSystem* audio_system_;
};

}  // namespace lull

#endif  // LULLABY_MODULES_ANIMATION_CHANNELS_AUDIO_CHANNELS_H_
