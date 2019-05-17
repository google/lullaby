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

#ifndef LULLABY_SYSTEMS_ANIMATION_PLAYBACK_PARAMETERS_H_
#define LULLABY_SYSTEMS_ANIMATION_PLAYBACK_PARAMETERS_H_

#include <stdlib.h>
#include "lullaby/util/hash.h"
#include "lullaby/util/typeid.h"

namespace lull {

// Parameters that control the playback of animations.
struct PlaybackParameters {
  // Whether or not to repeat the animation.
  bool looping = false;

  // Playback speed, multiplies the animation timestep.
  float speed = 1.0;

  // Time (in seconds) to delay the start of the playback. Negative values will
  // start from the middle of the animation.
  float start_delay_s = 0.f;

  // Duration (in seconds) to blend from previous animation.
  float blend_time_s = 0.f;

  // Offset the spline is shifted by on the y-axis. The spline is scaled first
  // by |y_scale| and then shifted by |y_offset|.
  float y_offset = 0.f;

  // Factor the spline is scaled by on the y-axis. The spline is scaled first by
  // |y_scale| and then shifted by |y_offset|.
  float y_scale = 1.f;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&looping, ConstHash("looping"));
    archive(&speed, ConstHash("speed"));
    archive(&start_delay_s, ConstHash("start_delay_s"));
    archive(&blend_time_s, ConstHash("blend_time_s"));
    archive(&y_offset, ConstHash("y_offset"));
    archive(&y_scale, ConstHash("y_scale"));
  }
};

}  // namespace lull

LULLABY_SETUP_TYPEID(lull::PlaybackParameters);

#endif  // LULLABY_SYSTEMS_ANIMATION_PLAYBACK_PARAMETERS_H_
