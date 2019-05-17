/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_TOOLS_ANIM_PIPELINE_ANIM_BONE_H_
#define LULLABY_TOOLS_ANIM_PIPELINE_ANIM_BONE_H_

#include <string>
#include <vector>
#include "motive/matrix_op.h"

namespace lull {
namespace tool {

// Represents a single point on an animation spline.
struct SplineNode {
  SplineNode() {}
  SplineNode(int time, float val, float derivative)
      : time(time), val(val), derivative(derivative) {}

  bool operator==(const SplineNode& rhs) const {
    return time == rhs.time && val == rhs.val && derivative == rhs.derivative;
  }
  bool operator!=(const SplineNode& rhs) const { return !operator==(rhs); }

  int time = 0;
  float val = 0.f;
  float derivative = 0.f;
};

// Represents a single "channel" of animating data.
//
// A channel has a unique id, the type of data being animated, and the curve
// along which it is animating.
struct AnimChannel {
  AnimChannel() {}
  AnimChannel(motive::MatrixOpId id, motive::MatrixOperationType op)
      : id(id), op(op) {}

  motive::MatrixOpId id = motive::kInvalidMatrixOpId;
  motive::MatrixOperationType op = motive::kInvalidMatrixOperation;
  std::vector<SplineNode> nodes;
};

// Represents a single bone in a skeleton and all the animation curves to be
// played on that bone for a single animation.
struct AnimBone {
  AnimBone(std::string name, int parent_bone_index)
      : name(std::move(name)), parent_bone_index(parent_bone_index) {
    // There probably won't be more than one of each op type.
    channels.reserve(motive::kNumMatrixOperationTypes);
  }

  int MaxAnimatedTime() const {
    int max_time = std::numeric_limits<int>::min();
    for (auto ch = channels.begin(); ch != channels.end(); ++ch) {
      // Only consider channels with more than one keyframe (non-constant).
      if (ch->nodes.size() > 1) {
        max_time = std::max(max_time, ch->nodes.back().time);
      }
    }
    return max_time == std::numeric_limits<int>::min() ? 0 : max_time;
  }

  int MinAnimatedTime() const {
    int min_time = std::numeric_limits<int>::max();
    for (auto ch = channels.begin(); ch != channels.end(); ++ch) {
      // Only consider channels with more than one keyframe (non-constant).
      if (ch->nodes.size() > 1) {
        min_time = std::min(min_time, ch->nodes[0].time);
      }
    }
    return min_time == std::numeric_limits<int>::max() ? 0 : min_time;
  }

  std::string name;
  int parent_bone_index = -1;
  std::vector<AnimChannel> channels;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_ANIM_PIPELINE_ANIM_BONE_H_
