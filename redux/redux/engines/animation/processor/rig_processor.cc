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

#include "redux/engines/animation/processor/rig_processor.h"

#include "absl/time/time.h"
#include "redux/engines/animation/animation_clip.h"
#include "redux/engines/animation/animation_engine.h"
#include "redux/modules/base/logging.h"

namespace redux {

RigMotivator RigProcessor::AllocateMotivator(int dimensions) {
  RigMotivator motivator;
  const Motivator::Index index =
      AllocateMotivatorIndices(&motivator, dimensions);
  data_[index].end_time = time_;
  return motivator;
}

void RigProcessor::AdvanceFrame(absl::Duration delta_time) {
  Defragment();
  for (auto& data : data_) {
    data.UpdateGlobalTransforms();
  }
  // Update our global time. It shouldn't matter if this wraps around, since we
  // only calculate times relative to it.
  time_ += delta_time;
}

void RigProcessor::RigData::Reset() {
  animation = nullptr;
  end_time = absl::ZeroDuration();
  motivators.clear();
  global_transforms.clear();
}

// Traverse hierarchy, converting local transforms from `motivators_` into
// global transforms. The `parents` are layed out such that the parent
// always come before the child.
// TODO OPT: optimize `parents` layout so that we can parallelize this call.
void RigProcessor::RigData::UpdateGlobalTransforms() {
  const absl::Span<const BoneIndex> parents = animation->BoneParents();
  const int num_bones = animation->NumBones();
  for (int i = 0; i < num_bones; ++i) {
    const int parent_idx = parents[i];
    const TransformMotivator& motivator = motivators[i];
    const Transform local_transform =
        motivator.Valid() ? motivator.Value() : Transform();

    const mat4 local_matrix = TransformMatrix(local_transform);
    if (parent_idx == kInvalidBoneIdx) {
      global_transforms[i] = local_matrix;
    } else {
      CHECK_GT(i, parent_idx);
      global_transforms[i] = global_transforms[parent_idx] * local_matrix;
    }
  }

  // TODO: We should let go of the animation once we've reached the end and no
  // longer need to hold on to the splines.
}

void RigProcessor::BlendToAnim(Motivator::Index index,
                               const AnimationClipPtr& anim,
                               const AnimationPlayback& playback) {
  RigData& data = Data(index);
  data.end_time = time_ + anim->Duration();

  // When animation has only one bone, or mesh has only one bone,
  // we simply animate the root node only.
  data.animation = anim;
  const int num_bones = anim->NumBones();

  data.motivators.resize(num_bones);
  data.global_transforms.resize(num_bones);

  // Update the motivators to blend to our new values.
  for (BoneIndex i = 0; i < num_bones; ++i) {
    TransformMotivator& motivator = data.motivators[i];
    if (!motivator.Valid()) {
      motivator = Engine()->AcquireMotivator<TransformMotivator>();
    }
    motivator.BlendTo(anim->GetBoneAnimation(i), playback);
  }
}

void RigProcessor::SetPlaybackRate(Motivator::Index index,
                                   float playback_rate) {
  RigData& data = Data(index);
  for (size_t i = 0; i < data.motivators.size(); ++i) {
    data.motivators[i].SetPlaybackRate(playback_rate);
  }
}

void RigProcessor::SetRepeating(Motivator::Index index, bool repeat) {
  RigData& data = Data(index);
  for (size_t i = 0; i < data.motivators.size(); ++i) {
    data.motivators[i].SetRepeating(repeat);
  }
}

const AnimationClipPtr& RigProcessor::CurrentAnimationClip(
    Motivator::Index index) const {
  return Data(index).animation;
}

absl::Span<const mat4> RigProcessor::GlobalTransforms(
    Motivator::Index index) const {
  return Data(index).global_transforms;
}

absl::Duration RigProcessor::TimeRemaining(Motivator::Index index) const {
  const RigData& data = Data(index);
  if (data.end_time == absl::InfiniteDuration()) {
    return absl::InfiniteDuration();
  }

  absl::Duration time = absl::ZeroDuration();
  for (size_t i = 0; i < data.motivators.size(); ++i) {
    time = std::max(time, data.motivators[i].TimeRemaining());
  }
  return time;
}

void RigProcessor::SetNumIndices(Motivator::Index num_indices) {
  data_.resize(num_indices);
}

void RigProcessor::ResetIndices(Motivator::Index index, int dimensions) {
  for (Motivator::Index i = index; i < index + dimensions; ++i) {
    data_[i].Reset();
  }
}

void RigProcessor::MoveIndices(Motivator::Index old_index,
                               Motivator::Index new_index, int dimensions) {
  using std::swap;
  for (int i = 0; i < dimensions; ++i) {
    swap(data_[new_index + i], data_[old_index + i]);
    data_[old_index + i].Reset();
  }
}

RigProcessor::RigData& RigProcessor::Data(Motivator::Index index) {
  CHECK(ValidIndex(index));
  return data_[index];
}

const RigProcessor::RigData& RigProcessor::Data(Motivator::Index index) const {
  CHECK(ValidIndex(index));
  return data_[index];
}
}  // namespace redux
