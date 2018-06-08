/*
Copyright 2017 Google Inc. All Rights Reserved.

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

#ifndef LULLABY_TOOLS_ANIM_PIPELINE_ANIM_DATA_H_
#define LULLABY_TOOLS_ANIM_PIPELINE_ANIM_DATA_H_

#include <math.h>
#include <vector>
#include "lullaby/tools/anim_pipeline/anim_bone.h"
#include "lullaby/tools/anim_pipeline/tolerances.h"
#include "motive/matrix_op.h"

namespace lull {
namespace tool {

// Unique id identifying a single float curve being animated.
typedef int FlatChannelId;

/// Convert derivative to its angle in x/y space.
///  derivative 0 ==> angle 0
///  derivative 1 ==> angle 45 degrees
///  derivative +inf ==> angle 90 degrees
///  derivative -2 ==> angle -63.4 degrees
/// Returns Angle, in radians, >= -pi and <= pi
inline float DerivativeAngle(float derivative) { return atan(derivative); }

enum RepeatPreference { kRepeatIfRepeatable, kAlwaysRepeat, kNeverRepeat };

// Contains all the necessary information to represent an animation.
//
// Different importers for different formats will return an instance of this
// class which will then be exported into a lullanim binary file.
class Animation {
 public:
  Animation(std::string name, const Tolerances& tolerances);

  const std::string& GetName() const { return name_; }

  int RegisterBone(const char* bone_name, int parent_bone_index);

  FlatChannelId AllocChannel(int bone_index, motive::MatrixOperationType op,
                             motive::MatrixOpId id);

  void AddConstant(FlatChannelId channel_id, float const_val);

  void AddCurve(FlatChannelId channel_id, int time_start, int time_end,
                const float* vals, const float* derivatives, size_t count);

  size_t NumNodes(FlatChannelId channel_id) const;

  size_t NumBones() const { return bones_.size(); }
  const AnimBone& GetBone(size_t index) const { return bones_[index]; }
  motive::BoneIndex BoneParent(int bone_idx) const;

  // Determine if the animation should repeat back to start after it reaches
  // the end.
  bool Repeat(RepeatPreference repeat_preference) const;

  /// Remove redundant nodes from `channel_id`.
  void PruneNodes(FlatChannelId channel_id);

  /// Collapse multiple channels into one, when possible.
  void PruneChannels(bool no_uniform_scale);

  /// Shift all times in all channels by `time_offset`.
  void ShiftTime(int time_offset);

  /// For each channel that ends before `end_time`, extend it at its
  /// current value to `end_time`. If already longer, or has no nodes
  /// to begin with, do nothing.
  void ExtendChannelsToTime(int end_time);

  /// Returns the tolerance value for the specified matrix operation.
  float ToleranceForOp(motive::MatrixOperationType op) const;

  /// Returns true if the specified value is the default value for the given
  /// matrix operation (ie. 0 for translation and rotation, 1 for scale).
  bool IsDefaultValue(motive::MatrixOperationType op, float value) const;

  /// Return the time of the channel that requires the most time.
  int MaxAnimatedTime() const;

  /// Return the time of the channel that starts the earliest. Could be a
  /// negative time.
  int MinAnimatedTime() const;

  /// Logs information about the specified channel for debugging.
  void LogChannel(FlatChannelId channel_id) const;
  /// Logs information about all data channels for debugging.
  void LogAllChannels() const;

 private:
  using Nodes = std::vector<SplineNode>;
  using Channels = std::vector<AnimChannel>;

  Channels& CurChannels();
  const Channels& CurChannels() const;

  float Tolerance(FlatChannelId channel_id) const;

  /// Return the first channel of the first bone that isn't repeatable.
  /// If all channels are repeatable, return kInvalidBoneIdx.
  /// A channel is repeatable if its start and end values and derivatives
  /// are within `tolerances_`.
  motive::BoneIndex FirstNonRepeatingBone(
      FlatChannelId* first_channel_id) const;

  /// Return true if the three channels starting at `channel_id`
  /// can be replaced with a single kScaleUniformly channel.
  bool UniformScaleChannels(const Channels& channels,
                            FlatChannelId channel_id) const;

  FlatChannelId SummableChannel(const Channels& channels,
                                FlatChannelId ch) const;

  static float EvaluateNodes(const Nodes& nodes, int time, float* derivative);

  // Gets the value from the channel either at the exact time specified, or
  // interpolated from the surrounding nodes. This is intended to be called
  // with consecutively increasing time values.  Returns true if a node was
  // sampled directly, so caller can move to next node.
  bool GetValueAtTime(const Nodes& nodes, const Nodes::const_iterator& node,
                      int time, float* value, float* derivative) const;

  // Sum curves in ch_a and ch_b and put the result in ch_a.
  void SumChannels(Channels& channels, FlatChannelId ch_a,
                   FlatChannelId ch_b) const;

  /// Returns true if all nodes between the first and last in `n`
  /// can be deleted without noticable difference to the curve.
  bool IntermediateNodesRedundant(const SplineNode* n, size_t len,
                                  float tolerance) const;

  static bool EqualNodes(const SplineNode& a, const SplineNode& b,
                         float tolerance, float derivative_tolerance);
  static float DefaultOpValue(motive::MatrixOperationType op);

  std::string name_;
  std::vector<AnimBone> bones_;
  Tolerances tolerances_;
  int cur_bone_index_;
};

}  // namespace tool
}  // namespace lull

#endif  // LULLABY_TOOLS_ANIM_PIPELINE_ANIM_DATA_H_
