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

#ifndef REDUX_ENGINES_ANIMATION_MOTIVATOR_SPLINE_MOTIVATOR_H_
#define REDUX_ENGINES_ANIMATION_MOTIVATOR_SPLINE_MOTIVATOR_H_

#include "redux/engines/animation/animation_playback.h"
#include "redux/engines/animation/motivator/motivator.h"
#include "redux/engines/animation/spline/compact_spline.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/math/matrix.h"

namespace redux {

class SplineProcessor;

// Drives a scalar value using CompactSplines.
class SplineMotivator : public Motivator {
 public:
  using ProcessorT = SplineProcessor;

  SplineMotivator() = default;

  // Drives the motivator values using the provides spline.
  void SetSpline(const CompactSpline& spline,
                 const AnimationPlayback& playback);

  // Drives the motivator to the specified value and optional target velocity
  // over the given timeframe.
  void SetTarget(float value, absl::Duration time);
  void SetTarget(float value, float velocity, absl::Duration time);

  // Instantly changes whether or not the spline should be repeated when the
  // end is reached.
  void SetRepeating(bool repeat);

  // Instantly changes the speed at which the spline is followed.
  void SetPlaybackRate(float playback_rate);

  // Returns the value of the spline at the current time of the Motivator.
  float Value() const;

  // Returns the derivate of the spline at the current time of the Motivator.
  float Velocity() const;

  // Returns the amount of time left in the animation.
  absl::Duration TimeRemaining() const;

  // Returns true if the spline has reached its end state (within the given
  // tolerances).
  bool Settled(float max_difference = 0.f, float max_velocity = 0.f) const;

 private:
  SplineProcessor& Processor();
  const SplineProcessor& Processor() const;
};
}  // namespace redux

REDUX_SETUP_TYPEID(redux::SplineMotivator);

#endif  // REDUX_ENGINES_ANIMATION_MOTIVATOR_SPLINE_MOTIVATOR_H_
