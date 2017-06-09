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

#ifndef LULLABY_SYSTEMS_ANIMATION_PLAYBACK_PARAMETERS_H_
#define LULLABY_SYSTEMS_ANIMATION_PLAYBACK_PARAMETERS_H_

#include <stdlib.h>

namespace lull {

// Parameters that control the playback of animations.
struct PlaybackParameters {
  // Whether or not to repeat the animation.
  bool looping = false;

  // Playback speed, multiplies the animation timestep.
  float speed = 1.0;

  // Time (in seconds) to delay the start of the playback.
  float start_delay_s = 0.f;

  // Duration (in seconds) to blend from previous animation.
  float blend_time_s = 0.f;

  // Values added to each dimension of the animation.
  const float* offsets = nullptr;
  size_t num_offsets = 0;

  // Values multiplied on each dimension of the animation.
  const float* multipliers = nullptr;
  size_t num_multipliers = 0;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_ANIMATION_PLAYBACK_PARAMETERS_H_
