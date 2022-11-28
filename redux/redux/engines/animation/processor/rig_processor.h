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

#ifndef REDUX_ENGINES_ANIMATION_PROCESSOR_RIG_PROCESSOR_H_
#define REDUX_ENGINES_ANIMATION_PROCESSOR_RIG_PROCESSOR_H_

#include <cstring>
#include <vector>

#include "redux/engines/animation/animation_clip.h"
#include "redux/engines/animation/common.h"
#include "redux/engines/animation/motivator/rig_motivator.h"
#include "redux/engines/animation/motivator/transform_motivator.h"
#include "redux/engines/animation/processor/anim_processor.h"
#include "redux/modules/base/typeid.h"
#include "redux/modules/math/matrix.h"

namespace redux {

class RigProcessor : public AnimProcessor {
 public:
  explicit RigProcessor(AnimationEngine* engine) : AnimProcessor(engine) {}

  RigMotivator AllocateMotivator(int dimensions = 1);

  int Priority() const override { return 3; }

  void AdvanceFrame(absl::Duration delta_time) override;

  // Return the animation that is currently playing.
  const AnimationClipPtr& CurrentAnimationClip(Motivator::Index index) const;

  // Returns an array of length `DefiningAnim.NumBones()`.
  // The i'th element of the array represents the transform from the root
  // bone to the bone-space on the i'th bone.
  absl::Span<const mat4> GlobalTransforms(Motivator::Index index) const;

  // Return the time remaining in the current rig animation.
  absl::Duration TimeRemaining(Motivator::Index index) const;

  // Smoothly transition to the animation in `anim`.
  void BlendToAnim(Motivator::Index index, const AnimationClipPtr& anim,
                   const AnimationPlayback& playback);

  // Instantly change the playback speed of this animation. If multiple
  // animations are running, set the playback rate for all of them.
  void SetPlaybackRate(Motivator::Index index, float playback_rate);

  // Instantly change the repeat state of this animation. If multiple
  // animations are running, set the repeat state for all of them. If no
  // animations are running, has no effect.
  void SetRepeating(Motivator::Index index, bool repeat);

 private:
  struct RigData {
    RigData() = default;
    RigData(RigData&&) noexcept = default;
    RigData& operator=(RigData&&) noexcept = default;

    void Reset();
    void UpdateGlobalTransforms();

    AnimationClipPtr animation = nullptr;

    // Time that the animation is expected to complete.
    absl::Duration end_time = absl::ZeroDuration();

    std::vector<TransformMotivator> motivators;
    std::vector<mat4> global_transforms;
  };

  void SetNumIndices(Motivator::Index num_indices) override;
  void MoveIndices(Motivator::Index old_index, Motivator::Index new_index,
                   int dimensions) override;
  void ResetIndices(Motivator::Index index, int dimensions) override;

  RigData& Data(Motivator::Index index);
  const RigData& Data(Motivator::Index index) const;

  std::vector<RigData> data_;
  absl::Duration time_ = absl::ZeroDuration();
};

}  // namespace redux

REDUX_SETUP_TYPEID(redux::RigProcessor);

#endif  // REDUX_ENGINES_ANIMATION_PROCESSOR_RIG_PROCESSOR_H_
