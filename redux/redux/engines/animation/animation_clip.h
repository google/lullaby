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

#ifndef REDUX_ENGINES_ANIMATION_ANIMATION_CLIP_H_
#define REDUX_ENGINES_ANIMATION_ANIMATION_CLIP_H_

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "redux/engines/animation/common.h"
#include "redux/engines/animation/spline/compact_spline.h"
#include "redux/modules/base/data_container.h"

namespace redux {

// Contains the type and data for a single channel of an animation.
struct AnimationChannel {
  // An animation channel which has no data (so use the default value for this
  // channel).
  explicit AnimationChannel(AnimChannelType type) : type(type) {}

  // An animation channel that only contains a single value.
  AnimationChannel(AnimChannelType type, float const_value)
      : type(type), const_value(const_value) {}

  // An animation channel that will be animated along a spline.
  AnimationChannel(AnimChannelType type, CompactSplinePtr spline)
      : type(type), spline(std::move(spline)) {}

  AnimChannelType type = AnimChannelType::Invalid;
  std::optional<float> const_value;
  CompactSplinePtr spline;
};

// Drives a fully rigged model.
class AnimationClip {
 public:
  AnimationClip() = default;

  // An animation for a single bone is basically just a collection of data
  // streams.
  using BoneAnimation = std::vector<AnimationChannel>;

  // Reads the animation clip from an rx anim data blob.
  explicit AnimationClip(DataContainer data);
  void Initialize(DataContainer data);
  void Finalize();

  // Number of bones. Bones are arranged in an hierarchy. Each bone animates
  // a matrix. The matrix describes the transform of the bone from its parent.
  BoneIndex NumBones() const { return static_cast<BoneIndex>(anims_.size()); }

  // Returns the animations channels for a single bone.
  const BoneAnimation& GetBoneAnimation(BoneIndex idx) const;

  // Amount of time required by this animation. Time units are set by the
  // caller.
  absl::Duration Duration() const { return duration_; }

  // Animation is repeatable. That is, when the end of the animation is
  // reached, it can be started at the beginning again without glitching.
  // Generally, an animation is repeatable if it's curves have the same values
  // and derivatives at the start and end.
  bool Repeats() const { return repeat_; }

  bool IsReady() const { return ready_; }

  // For debugging. Very useful when an animation is applied to a mesh
  // that doesn't match: with the bone names you can determine whether the
  // mesh or the animation is out of date.
  const char* BoneName(BoneIndex idx) const;

  // Returns an array of length NumBones() representing the bone heirarchy.
  // `bone_parents()[i]` is the bone index of the ith bone's parent.
  // `bone_parents()[i]` < `bone_parents()[j]` for all i < j.
  // For bones at the root (i.e. no parent) value is kInvalidBoneIdx.
  absl::Span<const BoneIndex> BoneParents() const;

  void OnReady(const std::function<void()>& callback);

 private:
  std::vector<BoneAnimation> anims_;
  std::vector<std::function<void()>> on_ready_callbacks_;
  DataContainer data_;
  const AnimAssetDef* def_ = nullptr;
  absl::Duration duration_ = absl::ZeroDuration();
  bool repeat_ = false;
  bool ready_ = false;
};

using AnimationClipPtr = std::shared_ptr<AnimationClip>;

}  // namespace redux

#endif  // REDUX_ENGINES_ANIMATION_ANIMATION_CLIP_H_
