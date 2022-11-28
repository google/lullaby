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

#include "redux/engines/animation/motivator/transform_motivator.h"

#include "redux/engines/animation/common.h"
#include "redux/engines/animation/processor/transform_processor.h"

namespace redux {

const Transform& TransformMotivator::Value() const {
  return Processor().Value(index_);
}

absl::Duration TransformMotivator::TimeRemaining() const {
  return Processor().TimeRemaining(index_);
}

void TransformMotivator::BlendTo(const std::vector<AnimationChannel>& animation,
                                 const AnimationPlayback& playback) {
  Processor().BlendTo(index_, animation, playback);
}

void TransformMotivator::SetPlaybackRate(float playback_rate) {
  Processor().SetPlaybackRate(index_, playback_rate);
}

void TransformMotivator::SetRepeating(bool repeat) {
  Processor().SetRepeating(index_, repeat);
}

TransformProcessor& TransformMotivator::Processor() {
  return *static_cast<TransformProcessor*>(processor_);
}

const TransformProcessor& TransformMotivator::Processor() const {
  return *static_cast<const TransformProcessor*>(processor_);
}

}  // namespace redux
