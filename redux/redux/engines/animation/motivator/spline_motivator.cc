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

#include "redux/engines/animation/motivator/spline_motivator.h"

#include "redux/engines/animation/common.h"
#include "redux/engines/animation/processor/spline_processor.h"

namespace redux {

float SplineMotivator::Value() const { return Processor().Values(index_)[0]; }

float SplineMotivator::Velocity() const {
  float out = 0.f;
  Processor().Velocities(index_, 1, &out);
  return out;
}

absl::Duration SplineMotivator::TimeRemaining() const {
  return Processor().TimeRemaining(index_, 1);
}

void SplineMotivator::SetSpline(const CompactSpline& spline,
                                const AnimationPlayback& playback) {
  Processor().SetSplines(index_, 1, &spline, playback);
}

void SplineMotivator::SetTarget(float value, absl::Duration time) {
  SetTarget(value, 0.f, time);
}

void SplineMotivator::SetTarget(float value, float velocity,
                                absl::Duration time) {
  Processor().SetTargets(index_, 1, &value, &velocity, time);
}

void SplineMotivator::SetRepeating(bool repeat) {
  const int dimensions = Processor().Dimensions(index_);
  Processor().SetSplineRepeating(index_, dimensions, repeat);
}

void SplineMotivator::SetPlaybackRate(float playback_rate) {
  const int dimensions = Processor().Dimensions(index_);
  Processor().SetSplinePlaybackRate(index_, dimensions, playback_rate);
}

bool SplineMotivator::Settled(float max_difference, float max_velocity) const {
  const int dimensions = Processor().Dimensions(index_);
  return Processor().Settled(index_, dimensions, max_difference, max_velocity);
}

SplineProcessor& SplineMotivator::Processor() {
  return *static_cast<SplineProcessor*>(processor_);
}

const SplineProcessor& SplineMotivator::Processor() const {
  return *static_cast<const SplineProcessor*>(processor_);
}

}  // namespace redux
