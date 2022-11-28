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

#include "redux/engines/animation/motivator/rig_motivator.h"

#include "redux/engines/animation/processor/rig_processor.h"

namespace redux {

void RigMotivator::BlendToAnim(const AnimationClipPtr& animation,
                               const AnimationPlayback& playback) {
  Processor().BlendToAnim(index_, animation, playback);
}

void RigMotivator::SetPlaybackRate(float playback_rate) {
  Processor().SetPlaybackRate(index_, playback_rate);
}

void RigMotivator::SetRepeating(bool repeat) {
  Processor().SetRepeating(index_, repeat);
}

absl::Span<const mat4> RigMotivator::GlobalTransforms() const {
  return Processor().GlobalTransforms(index_);
}

const AnimationClipPtr& RigMotivator::CurrentAnimationClip() const {
  return Processor().CurrentAnimationClip(index_);
}

absl::Duration RigMotivator::TimeRemaining() const {
  return Processor().TimeRemaining(index_);
}

RigProcessor& RigMotivator::Processor() {
  return *static_cast<RigProcessor*>(processor_);
}

const RigProcessor& RigMotivator::Processor() const {
  return *static_cast<const RigProcessor*>(processor_);
}

}  // namespace redux
