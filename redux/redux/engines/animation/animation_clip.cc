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

#include "redux/engines/animation/animation_clip.h"

#include "redux/modules/base/logging.h"

namespace redux {

static AnimationChannel ReadChannelAssetDef(const AnimChannelAssetDef* def) {
  const AnimChannelType type = def->type();

  if (def->data_type() ==
      AnimChannelDataAssetDef::AnimChannelConstValueAssetDef) {
    const float value = def->data_as_AnimChannelConstValueAssetDef()->value();
    return AnimationChannel(type, value);
  } else if (def->data_type() ==
             AnimChannelDataAssetDef::AnimChannelSplineAssetDef) {
    const AnimChannelSplineAssetDef* spline_def =
        def->data_as_AnimChannelSplineAssetDef();
    CHECK(spline_def);

    const int num_nodes = spline_def->nodes()->size();
    CompactSplinePtr spline = CompactSpline::Create(num_nodes);
    const Interval y_range(spline_def->y_range_start(),
                           spline_def->y_range_end());
    spline->Init(y_range, spline_def->x_granularity());
    for (int i = 0; i < num_nodes; ++i) {
      const AnimSplineNodeAssetDef* node = spline_def->nodes()->Get(i);
      spline->AddNodeVerbatim(node->x(), node->y(), node->angle());
    }
    return AnimationChannel(type, std::move(spline));
  } else {
    return AnimationChannel(type);
  }
}

AnimationClip::AnimationClip(DataContainer data) {
  Initialize(std::move(data));
}

void AnimationClip::Initialize(DataContainer data) {
  data_ = std::move(data);
  if (data_.GetNumBytes() == 0) {
    return;
  }

  def_ = flatbuffers::GetRoot<AnimAssetDef>(data_.GetBytes());
  if (def_->bone_anims()) {
    anims_.resize(def_->bone_anims()->size());
    for (std::size_t i = 0; i < def_->bone_anims()->size(); ++i) {
      const BoneAnimAssetDef* bone_anim_def = def_->bone_anims()->Get(i);
      if (bone_anim_def->ops()) {
        for (std::size_t j = 0; j < bone_anim_def->ops()->size(); ++j) {
          const AnimChannelAssetDef* def = bone_anim_def->ops()->Get(j);
          AnimationChannel channel = ReadChannelAssetDef(def);
          anims_[i].push_back(std::move(channel));
        }
      }
    }
  }
  repeat_ = def_->repeat();
  duration_ = absl::Seconds(def_->length_in_seconds());
}

void AnimationClip::Finalize() {
  if (data_.GetNumBytes() > 0) {
    ready_ = true;
    for (auto& cb : on_ready_callbacks_) {
      cb();
    }
    on_ready_callbacks_.clear();
  }
}

const AnimationClip::BoneAnimation& AnimationClip::GetBoneAnimation(
    BoneIndex idx) const {
  CHECK(idx < anims_.size());
  return anims_[idx];
}

const char* AnimationClip::BoneName(BoneIndex idx) const {
  CHECK(ready_);
  CHECK(def_->bone_names());
  CHECK(def_->bone_names()->size() > idx);
  return def_->bone_names()->GetAsString(idx)->c_str();
}

absl::Span<const BoneIndex> AnimationClip::BoneParents() const {
  CHECK(ready_);
  CHECK(def_->bone_parents());
  return {def_->bone_parents()->data(), NumBones()};
}

void AnimationClip::OnReady(const std::function<void()>& callback) {
  if (ready_) {
    callback();
  } else {
    on_ready_callbacks_.push_back(callback);
  }
}
}  // namespace redux
