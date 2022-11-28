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

#ifndef REDUX_TOOLS_ANIM_PIPELINE_ANIMATION_H_
#define REDUX_TOOLS_ANIM_PIPELINE_ANIMATION_H_

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "redux/engines/animation/common.h"
#include "redux/tools/anim_pipeline/anim_curve.h"
#include "redux/tools/anim_pipeline/tolerances.h"

namespace redux::tool {

// Represents a single bone in a skeleton and all the animation curves
// associated with that bone.
struct AnimBone {
  AnimBone(std::string name, BoneIndex parent_bone_index)
      : name(std::move(name)), parent_bone_index(parent_bone_index) {}

  std::string name;
  BoneIndex parent_bone_index = kInvalidBoneIdx;
  std::vector<AnimCurve> curves;
};

// Contains all the necessary information to represent an animation.
//
// Different importers for different formats will return an instance of this
// class which will then be exported into a redux animation binary file.
class Animation {
 public:
  enum RepeatPreference { kRepeatIfRepeatable, kAlwaysRepeat, kNeverRepeat };

  explicit Animation(const Tolerances& tolerances);

  // Adds a bone to the animation's skeleton.
  BoneIndex RegisterBone(const char* bone_name,
                         BoneIndex parent_bone_index = kInvalidBoneIdx);

  // Returns the number of bones in the animation's skeleton.
  size_t NumBones() const { return bones_.size(); }

  // Returns the given bone's parent index.
  BoneIndex BoneParent(BoneIndex bone_idx) const;

  // Returns all the data associated with the bone at the specified index.
  AnimBone& GetMutableBone(size_t index) { return bones_[index]; }

  // Returns all the data associated with the bone at the specified index.
  const AnimBone& GetBone(size_t index) const { return bones_[index]; }

  // Determines if the animation should repeat back to start after it reaches
  // the end.
  bool Repeat(RepeatPreference repeat_preference = kRepeatIfRepeatable) const;

  // Collapse multiple channels into one, when possible.
  void FinishBone(BoneIndex bone_index);

  // Shift all times in all channels by a time offset.
  void ShiftTime(float time_offset);

  // For each channel that ends before `end_time`, extend it at its current
  // value to `end_time`. If already longer, or has no nodes to begin with, do
  // nothing.
  void ExtendChannelsToTime(float end_time);

  // Returns true if the specified value is the default value for the given
  // matrix operation (ie. 0 for translation and rotation, 1 for scale).
  bool IsDefaultValue(AnimChannelType op, float value) const;

  // Returns the set of tolerance values for this animation.
  const Tolerances& GetTolerances() const { return tolerances_; }

  // Return the time of the channel that requires the most time.
  float MaxAnimatedTimeMs() const;

  // Return the time of the channel that starts the earliest. Could be a
  // negative time.
  float MinAnimatedTimeMs() const;

 private:
  // Return the first channel of the first bone that isn't repeatable. If all
  // channels are repeatable, return kInvalidBoneIdx. A channel is repeatable
  // if its start and end values and derivatives are within `tolerances_`.
  BoneIndex FirstNonRepeatingBone() const;

  Tolerances tolerances_;
  std::vector<AnimBone> bones_;
};

using AnimationPtr = std::shared_ptr<Animation>;

}  // namespace redux::tool

#endif  // REDUX_TOOLS_ANIM_PIPELINE_ANIMATION_H_
