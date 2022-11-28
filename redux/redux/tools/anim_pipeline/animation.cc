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

#include "redux/tools/anim_pipeline/animation.h"

#include <cmath>

#include "redux/engines/animation/common.h"
#include "redux/modules/math/matrix.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"
#include "redux/tools/anim_pipeline/tolerances.h"

namespace redux::tool {

Animation::Animation(const Tolerances& tolerances) : tolerances_(tolerances) {}

BoneIndex Animation::RegisterBone(const char* bone_name,
                                  BoneIndex parent_bone_index) {
  const BoneIndex bone_index = static_cast<BoneIndex>(bones_.size());
  bones_.emplace_back(bone_name, parent_bone_index);
  return bone_index;
}

BoneIndex Animation::BoneParent(BoneIndex bone_idx) const {
  CHECK(bone_idx != kInvalidBoneIdx);
  CHECK(bone_idx < bones_.size());
  return bones_[bone_idx].parent_bone_index;
}

void Animation::FinishBone(BoneIndex bone_index) {
  AnimBone& anim = GetMutableBone(bone_index);

  // First, ensure that the bones are in correct "channel" order. This makes
  // pruning curves easier.
  std::sort(anim.curves.begin(), anim.curves.end(),
            [](const AnimCurve& lhs, const AnimCurve& rhs) {
              return lhs.Type() < rhs.Type();
            });

  // Remove any "constant value" channels that have the default value.
  for (auto iter = anim.curves.begin(); iter != anim.curves.end();) {
    bool is_default = false;
    auto maybe_const_value = iter->GetConstValue(tolerances_);
    if (maybe_const_value) {
      const float const_value = maybe_const_value.value();
      const float default_value = ChannelDefaultValue(iter->Type());
      const float tolerance = ToleranceForOp(tolerances_, iter->Type());
      is_default = std::fabs(const_value - default_value) < tolerance;
    }
    if (is_default) {
      iter = anim.curves.erase(iter);
    } else {
      ++iter;
    }
  }
}

void Animation::ShiftTime(float time_offset) {
  if (time_offset != 0) {
    for (AnimBone& bone : bones_) {
      for (AnimCurve& curve : bone.curves) {
        curve.ShiftTime(time_offset);
      }
    }
  }
}

void Animation::ExtendChannelsToTime(float end_time) {
  for (AnimBone& bone : bones_) {
    for (AnimCurve& curve : bone.curves) {
      curve.ExtendToTime(end_time);
    }
  }
}

bool Animation::IsDefaultValue(AnimChannelType op, float value) const {
  return std::fabs(value - ChannelDefaultValue(op)) <
         ToleranceForOp(tolerances_, op);
}

float Animation::MaxAnimatedTimeMs() const {
  float max_time = std::numeric_limits<float>::min();
  for (const AnimBone& bone : bones_) {
    for (const AnimCurve& curve : bone.curves) {
      // Only consider channels with more than one keyframe (non-constant).
      if (curve.Nodes().size() > 1) {
        max_time = std::max(max_time, curve.Nodes().back().time_ms);
      }
    }
  }
  return max_time == std::numeric_limits<float>::min() ? 0.0f : max_time;
}

float Animation::MinAnimatedTimeMs() const {
  float min_time = std::numeric_limits<float>::max();
  for (const AnimBone& bone : bones_) {
    for (const AnimCurve& curve : bone.curves) {
      // Only consider channels with more than one keyframe (non-constant).
      if (curve.Nodes().size() > 1) {
        min_time = std::min(min_time, curve.Nodes().front().time_ms);
      }
    }
  }
  return min_time == std::numeric_limits<float>::max() ? 0.0f : min_time;
}

BoneIndex Animation::FirstNonRepeatingBone() const {
  for (BoneIndex bone_idx = 0; bone_idx < bones_.size(); ++bone_idx) {
    const AnimBone& bone = bones_[bone_idx];
    for (const AnimCurve& curve : bone.curves) {
      // Get deltas for the start and end of the channel.
      const auto& start = curve.Nodes().front();
      const auto& end = curve.Nodes().back();
      const float diff_val = fabs(start.value - end.value);
      const float diff_derivative_angle =
          fabs(DerivativeAngle(start.derivative - end.derivative));

      // Return false unless the start and end of the channel are the same.
      const float tolerance = ToleranceForOp(tolerances_, curve.Type());
      const bool same =
          diff_val < tolerance &&
          diff_derivative_angle < tolerances_.repeat_derivative_angle;
      if (!same) {
        return bone_idx;
      }
    }
  }
  return kInvalidBoneIdx;
}

bool Animation::Repeat(RepeatPreference repeat_preference) const {
  if (repeat_preference == kNeverRepeat) {
    return false;
  } else if (repeat_preference == kAlwaysRepeat) {
    return true;
  } else {
    const BoneIndex bone_idx = FirstNonRepeatingBone();
    return bone_idx == kInvalidBoneIdx;
  }
}

}  // namespace redux::tool
