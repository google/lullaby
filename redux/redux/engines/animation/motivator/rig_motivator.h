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

#ifndef REDUX_ENGINES_ANIMATION_MOTIVATOR_RIG_MOTIVATOR_H_
#define REDUX_ENGINES_ANIMATION_MOTIVATOR_RIG_MOTIVATOR_H_

#include "redux/engines/animation/animation_clip.h"
#include "redux/engines/animation/animation_playback.h"
#include "redux/engines/animation/motivator/motivator.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/math/matrix.h"

namespace redux {

class RigProcessor;

// Drives a "rig" (which is a hierarchical set of transforms) using data stored
// in an AnimationClip.
class RigMotivator : public Motivator {
 public:
  using ProcessorT = RigProcessor;

  RigMotivator() = default;

  // Blends from the currently playing animation to the given animation using
  // the provided set of playback parameters. If there is no currently playing
  // animation, blend_time will be treated as 0 which results in "snapping" to
  // the given animation.
  void BlendToAnim(const AnimationClipPtr& animation,
                   const AnimationPlayback& playback);

  // Instantly changes the playback speed of this animation.
  void SetPlaybackRate(float playback_rate);

  // Instantly changes the repeat state of this animation. If the current
  // animation is done playing, then this call has no effect.
  void SetRepeating(bool repeat);

  // Returns an array of matricies: one for each bone in the rig. The matrices
  // are all in the space of the root bone. That is, the bone hierarchy has been
  // flattened.
  absl::Span<const mat4> GlobalTransforms() const;

  // Returns the time remaining in the current animation.
  absl::Duration TimeRemaining() const;

  // Returns the currently playing animation clip driving this Motivator.
  const AnimationClipPtr& CurrentAnimationClip() const;

 private:
  RigProcessor& Processor();
  const RigProcessor& Processor() const;
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::RigMotivator);

#endif  // REDUX_ENGINES_ANIMATION_MOTIVATOR_RIG_MOTIVATOR_H_
