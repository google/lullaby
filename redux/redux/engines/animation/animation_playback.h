/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#ifndef REDUX_ENGINES_ANIMATION_ANIMATION_PLAYBACK_H_
#define REDUX_ENGINES_ANIMATION_ANIMATION_PLAYBACK_H_

#include "absl/time/time.h"

namespace redux {

// Parameters to specify how an animation will be "played".
struct AnimationPlayback {
  // Scales `delta_time` of the update to allow for slower/faster playback.
  //   0.0 ==> paused
  //   0.5 ==> half speed (slow motion)
  //   1.0 ==> authored speed
  //   2.0 ==> double speed (fast forward)
  float playback_rate = 1.f;

  // If played "on top" of a previously playing animation, this specifies how
  // much longer in the future the animation will be 100% playing the new
  // animation. We create a smooth transition from the current state to the
  // animation that lasts for this duration.
  absl::Duration blend_time = absl::ZeroDuration();

  // The starting point from which to play an animation. Useful to start an
  // animation at an arbitrary start point.
  absl::Duration start_time = absl::ZeroDuration();

  // If true, start back at the beginning after we reach the end.
  bool repeat = false;

  // Shifts the values returned by the animation by the given amounts. The value
  // is first scaled by 'value_scale' and then shifted by 'value_offset'. This
  // will shift each dimension of the animation by the same amount.
  float value_offset = 0.f;
  float value_scale = 1.f;
};
}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_ANIMATION_PLAYBACK_H_
