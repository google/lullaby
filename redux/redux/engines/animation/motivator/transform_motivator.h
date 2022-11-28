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

#ifndef REDUX_ENGINES_ANIMATION_MOTIVATOR_TRANSFORM_MOTIVATOR_H_
#define REDUX_ENGINES_ANIMATION_MOTIVATOR_TRANSFORM_MOTIVATOR_H_

#include "redux/engines/animation/animation_clip.h"
#include "redux/engines/animation/animation_playback.h"
#include "redux/engines/animation/motivator/motivator.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/math/transform.h"

namespace redux {

class TransformProcessor;

// Drives a Transform (ie. translation, rotation, and scale) using data from a
// series of animating channels each of which drives a single scalar value.
class TransformMotivator : public Motivator {
 public:
  using ProcessorT = TransformProcessor;

  TransformMotivator() = default;

  // Returns the current Transform value of the Motivator.
  const Transform& Value() const;

  // Returns the time remaining in the current animation.
  absl::Duration TimeRemaining() const;

  // Smoothly transitions to the given `animation` as defined by a collection
  // of individual channels of animations for scalar components. Information
  // about the transition and other options are provided in the playback params.
  void BlendTo(const std::vector<AnimationChannel>& animation,
               const AnimationPlayback& playback);

  // Instantly changes the playback speed of this animation.
  void SetPlaybackRate(float playback_rate);

  // Instantly changes the repeat state of this animation. If the current
  // animation is done playing, then this call has no effect.
  void SetRepeating(bool repeat);

 private:
  TransformProcessor& Processor();
  const TransformProcessor& Processor() const;
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::TransformMotivator);

#endif  // REDUX_ENGINES_ANIMATION_MOTIVATOR_TRANSFORM_MOTIVATOR_H_
