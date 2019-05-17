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

#include "lullaby/tools/anim_pipeline/animation.h"

#include <cmath>
#include <queue>

#include "lullaby/util/fixed_string.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/tools/common/file_utils.h"

namespace lull {
namespace tool {

namespace {

// Extracts the quaternion rotation from the transform matrix `m` using `scale`
// as the pre-computed scale component of the `m`.
mathfu::quat ExtractQuaternion(const mathfu::mat4 m,
                               const mathfu::vec3& scale) {
  // The math here must undo the math in mathfu::mat4::Transform().
  const mathfu::vec3 inv_scale = mathfu::kOnes3f / scale;
  const mathfu::mat3 rot(
      m(0, 0) * inv_scale.x, m(1, 0) * inv_scale.x, m(2, 0) * inv_scale.x,
      m(0, 1) * inv_scale.y, m(1, 1) * inv_scale.y, m(2, 1) * inv_scale.y,
      m(0, 2) * inv_scale.z, m(1, 2) * inv_scale.z, m(2, 2) * inv_scale.z);
  return mathfu::quat::FromMatrix(rot).Normalized();
}

}  // namespace

using LogString = FixedString<512>;

// Use these bitfields to find situations where scale x, y, and z occur, in
// any order, in a row.
static const uint32_t kScaleXBitfield = 1 << motive::kScaleX;
static const uint32_t kScaleYBitfield = 1 << motive::kScaleY;
static const uint32_t kScaleZBitfield = 1 << motive::kScaleZ;
static const uint32_t kScaleXyzBitfield =
    kScaleXBitfield | kScaleYBitfield | kScaleZBitfield;

Animation::Animation(std::string name, const Tolerances& tolerances,
                     bool sqt_anims)
    : name_(std::move(name)),
      tolerances_(tolerances),
      cur_bone_index_(-1),
      sqt_anims_(sqt_anims) {}

int Animation::RegisterBone(const char* bone_name, int parent_bone_index) {
  const int bone_index = static_cast<int>(bones_.size());
  bones_.emplace_back(bone_name, parent_bone_index);
  return bone_index;
}

FlatChannelId Animation::AllocChannel(int bone_index,
                                      motive::MatrixOperationType op,
                                      motive::MatrixOpId id) {
  assert(bone_index < bones_.size());
  cur_bone_index_ = bone_index;
  Channels& channels = CurChannels();
  channels.emplace_back(id, op);
  return static_cast<FlatChannelId>(channels.size() - 1);
}

void Animation::AddConstant(FlatChannelId channel_id, float const_val) {
  Channels& channels = CurChannels();
  Nodes& n = channels[channel_id].nodes;
  n.resize(0);
  n.emplace_back(0, const_val, 0.0f);
}

void Animation::AddCurve(FlatChannelId channel_id, const AnimCurve& curve) {
  AddCurve(channel_id, curve.times.data(), curve.values.data(),
           curve.derivatives.data(), curve.times.size());
}

void Animation::AddCurve(FlatChannelId channel_id, const float* times,
                         const float* vals, const float* derivatives,
                         size_t count) {
  // Break the curve down into segments and process them depth-first so that the
  // resulting nodes are in chronological order.
  std::vector<CurveSegment> segments;
  segments.emplace_back(times, vals, derivatives, count);

  while (!segments.empty()) {
    CurveSegment s = segments.back();
    segments.pop_back();

    // Create cubic that covers the entire range from time_start ~ time_end.
    // The cubic `c` is shifted to the left, to start at 0 instead of
    // time_start.
    // This is to maintain floating-point precision.
    const float time_start = s.times[0];
    const float time_end = s.times[s.count - 1];
    const float time_width = static_cast<float>(time_end - time_start);
    const motive::CubicCurve c(
        motive::CubicInit(s.vals[0], s.derivatives[0], s.vals[s.count - 1],
                          s.derivatives[s.count - 1], time_width));

    // Find the worst intermediate val in for this cubic.
    // That is, the index into `s.vals` where the cubic evaluation is most
    // inaccurate.
    float worst_diff = 0.0f;
    size_t worst_idx = 0;
    for (size_t i = 1; i < s.count - 1; ++i) {
      const float cubic_val = c.Evaluate(s.times[i] - time_start);
      const float curve_val = s.vals[i];
      const float diff_val = fabs(cubic_val - curve_val);
      if (diff_val > worst_diff) {
        worst_idx = i;
        worst_diff = diff_val;
      }
    }

    // If the cubic is off by a lot, divide the curve into two curves at the
    // worst time. Note that the recursion will end, at worst, when
    // s.count ==> 2.
    const float tolerance = Tolerance(channel_id);
    if (worst_idx > 0 && worst_diff > tolerance) {
      // Push the "end" segment on first so that the "start" segment is
      // processed first, resulting in a depth-first search.
      segments.emplace_back(&s.times[worst_idx], &s.vals[worst_idx],
                            &s.derivatives[worst_idx], s.count - worst_idx);
      segments.emplace_back(s.times, s.vals, s.derivatives, worst_idx + 1);
      continue;
    }

    // Otherwise, the generated cubic is good enough, so record it.
    const SplineNode start_node(time_start, s.vals[0], s.derivatives[0]);
    const SplineNode end_node(time_end, s.vals[s.count - 1],
                              s.derivatives[s.count - 1]);

    // Only push the start node if it differs from the previously pushed end
    // node. Most of the time it will be the same.
    Channels& channels = CurChannels();
    Nodes& n = channels[channel_id].nodes;
    const bool start_matches_prev = !n.empty() && n.back() == start_node;
    if (!start_matches_prev) {
      n.push_back(start_node);
    }
    n.push_back(end_node);
  }
}

size_t Animation::NumNodes(FlatChannelId channel_id) const {
  const Channels& channels = CurChannels();
  const Nodes& n = channels[channel_id].nodes;
  return n.size();
}

void Animation::PruneNodes(FlatChannelId channel_id) {
  const float tolerance = Tolerance(channel_id);

  // For every node try to prune as many redunant nodes that come after it.
  // A node is redundant if the spline evaluates to the same value even if
  // it doesn't exists (note: here "same value" means within `tolerances_`).
  Channels& channels = CurChannels();
  Nodes& n = channels[channel_id].nodes;
  std::vector<bool> prune(n.size(), false);
  for (size_t i = 0; i < n.size();) {
    size_t next_i = i + 1;
    for (size_t j = i + 2; j < n.size(); ++j) {
      const bool redundant =
          IntermediateNodesRedundant(&n[i], j - i + 1, tolerance);
      if (redundant) {
        prune[j - 1] = true;
        next_i = j;
      }
    }
    i = next_i;
  }

  // Compact to remove all pruned nodes.
  size_t write = 0;
  for (size_t read = 0; read < n.size(); ++read) {
    if (prune[read]) continue;
    if (write < read) {
      n[write] = n[read];
    }
    write++;
  }
  n.resize(write);

  // If value is constant for the entire time, remove the second node so that
  // we know to output a constant value in `OutputFlatBuffer()`.
  const bool is_const =
      n.size() == 2 && fabs(n[0].val - n[1].val) < tolerance &&
      fabs(DerivativeAngle(n[0].derivative)) < tolerances_.derivative_angle &&
      fabs(DerivativeAngle(n[1].derivative)) < tolerances_.derivative_angle;
  if (is_const) {
    n.resize(1);
  }
}

void Animation::PruneChannels(bool no_uniform_scale) {
  for (auto bone = bones_.begin(); bone != bones_.end(); ++bone) {
    // Iterate from the end to minimize the cost of the erase operations.
    Channels& channels = bone->channels;
    for (FlatChannelId ch = static_cast<FlatChannelId>(channels.size() - 1);
         ch >= 0; ch--) {
      // Collapse kScaleX,Y,Z into kScaleUniformly.
      const bool uniform_scale =
          (!no_uniform_scale && UniformScaleChannels(channels, ch));
      if (uniform_scale) {
        // log_.Log(fplutil::kLogVerbose,
        //          "  Collapsing scale x, y, z channels %d~%d into"
        //          " one scale-uniformly channel\n",
        //          ch, ch + 2);

        // Ids values are in consecutive order
        //   scale-X id, scale-Y id, scale-Z id, scale-uniformly id
        // the same as op values are in consecutive order
        //   kScaleX, kScaleY, kScaleZ, kScaleUniformly
        // but with a different initial value.
        //
        // So to convert from scale-? id to scale-uniformly id, we add on
        // the difference kScaleUniformly - kScale?.
        channels[ch].id += motive::kScaleUniformly -
                           static_cast<motive::MatrixOpId>(channels[ch].op);
        channels[ch].op = motive::kScaleUniformly;
        channels.erase(channels.begin() + (ch + 1),
                       channels.begin() + (ch + 3));
      }

      // Sum together channels that are adjacent, or separated only by
      // independent ops.
      const FlatChannelId summable_ch = SummableChannel(channels, ch);
      if (summable_ch >= 0) {
        // log_.Log(fplutil::kLogVerbose, "  Summing %s channels %d and %d\n",
        //          motive::MatrixOpName(channels[ch].op), ch, summable_ch);

        SumChannels(channels, ch, summable_ch);
        channels.erase(channels.begin() + summable_ch);
      }

      // Remove constant channels that have the default value.
      // Most of the time these won't be created, but it's possible that
      // of the collapse operations above (especially summing) will create
      // this situation.
      if (channels[ch].nodes.size() == 1 &&
          IsDefaultValue(channels[ch].op, channels[ch].nodes[0].val)) {
        // log_.Log(fplutil::kLogVerbose, "  Omitting constant %s channel %d\n",
        //          motive::MatrixOpName(channels[ch].op), ch);
        channels.erase(channels.begin() + ch);
      }
    }

    // Ensure that the channels remain in accending order of id.
    std::sort(channels.begin(), channels.end(),
              [](const AnimChannel& lhs, const AnimChannel& rhs) {
                return lhs.id < rhs.id;
              });
  }
}

void Animation::ShiftTime(int time_offset) {
  if (time_offset == 0) return;
  // log_.Log(fplutil::kLogImportant, "Shifting animation by %d ticks.\n",
  //          time_offset);

  for (auto bone = bones_.begin(); bone != bones_.end(); ++bone) {
    for (auto ch = bone->channels.begin(); ch != bone->channels.end(); ++ch) {
      for (auto n = ch->nodes.begin(); n != ch->nodes.end(); ++n) {
        n->time += time_offset;
      }
    }
  }
}

void Animation::ExtendChannelsToTime(int end_time) {
  for (auto bone = bones_.begin(); bone != bones_.end(); ++bone) {
    Channels& channels = bone->channels;
    for (auto ch = channels.begin(); ch != channels.end(); ++ch) {
      Nodes& n = ch->nodes;

      // Ignore empty or constant channels.
      if (n.size() <= 1) continue;

      // Ignore channels that are already long enough.
      const SplineNode back = n.back();
      if (back.time >= end_time) continue;

      // Append a point with 0 derivative at the back, if required.
      // This ensures that the extra segment is a flat line.
      if (back.derivative != 0) {
        n.push_back(SplineNode(back.time, back.val, 0.0f));
      }

      // Append a point at the end time, also with 0 derivative.
      n.push_back(SplineNode(end_time, back.val, 0.0f));
    }
  }
}

void Animation::BakeSqtAnimations() {
  if (cur_bone_index_ == -1) {
    return;
  }
  Channels& channels = CurChannels();
  if (channels.empty()) {
    return;
  }

  // Compute start and end times for this bone.
  const int start_time = bones_[cur_bone_index_].MinAnimatedTime();
  const int end_time = bones_[cur_bone_index_].MaxAnimatedTime();

  // Determine the sample rate and required number of samples.
  float sample_rate = 0.f;
  int num_samples = 1;

  // Length 0 animations only need a single sample.
  if (start_time != end_time) {
    // Otherwise, compute the duration and determine the number of samples
    // required at 120hz.
    const float duration = static_cast<float>(end_time - start_time);
    const float ideal_sample_rate = 1000.f / 120.f;
    num_samples = std::ceil(duration / ideal_sample_rate);

    // Ensure the actual sample rate will place the last node exactly on
    // `end_time`. Increment the number of samples so `end_time` is included.
    sample_rate = duration / num_samples;
    ++num_samples;
  }

  // Store over-sampled translation, rotation, and scale curves.
  AnimCurve curves[10] = {AnimCurve(motive::kTranslateX, num_samples),
                          AnimCurve(motive::kTranslateY, num_samples),
                          AnimCurve(motive::kTranslateZ, num_samples),
                          AnimCurve(motive::kQuaternionW, num_samples),
                          AnimCurve(motive::kQuaternionX, num_samples),
                          AnimCurve(motive::kQuaternionY, num_samples),
                          AnimCurve(motive::kQuaternionZ, num_samples),
                          AnimCurve(motive::kScaleX, num_samples),
                          AnimCurve(motive::kScaleY, num_samples),
                          AnimCurve(motive::kScaleZ, num_samples)};

  // Track the previous quaternion used to that neighboring quaternions lie in
  // the same 4-dimensional hemisphere since both q and -q represent the same
  // orientation.
  mathfu::quat last_rotation = mathfu::quat::identity;

  // Take the designated number of curve samples.
  float time = start_time;
  for (int i = 0; i < num_samples; ++i) {
    // Get a list of matrix operations to apply at this time.
    std::vector<motive::MatrixOperation> ops;
    for (auto ch = channels.begin(); ch != channels.end(); ++ch) {
      // If the time is outside the curve, insert an operation with the first
      // or last value.
      const SplineNode& front = ch->nodes.front();
      if (time <= static_cast<float>(front.time)) {
        ops.emplace_back(motive::MatrixOperationInit(ch->id, ch->op, front.val),
                         nullptr);
        continue;
      }
      const SplineNode& back = ch->nodes.back();
      if (time >= static_cast<float>(back.time)) {
        ops.emplace_back(motive::MatrixOperationInit(ch->id, ch->op, back.val),
                         nullptr);
        continue;
      }

      // Otherwise find where the time is within the curve.
      for (int j = 0; j < ch->nodes.size() - 1; ++j) {
        const SplineNode& start_node = ch->nodes[j];
        // Insert nearly-exact matches.
        if (AreNearlyEqual(start_node.time, time, 1e-5f)) {
          ops.emplace_back(
              motive::MatrixOperationInit(ch->id, ch->op, start_node.val),
              nullptr);
          break;
        } else if (static_cast<float>(start_node.time) < time) {
          // Ensure that `start_node` is the closest node before `time`.
          const SplineNode& end_node = ch->nodes[j + 1];
          if (static_cast<float>(end_node.time) < time) {
            continue;
          }
          // Create a cubic that covers the range between `start_node` and
          // `end_node`. The cubic `c` is shifted to the left, to start at 0
          // instead of `start_node.time`, to maintain floating-point
          // precision.
          const float time_width =
              static_cast<float>(end_node.time - start_node.time);
          const motive::CubicCurve c(
              motive::CubicInit(start_node.val, start_node.derivative,
                                end_node.val, end_node.derivative, time_width));

          // Shift the time of this sample into the cubic above, but cast it to
          // an integer since curve samples aren't floating point values and we
          // want the samples to be as accurate as possible
          const int shifted_time = time - static_cast<float>(start_node.time);
          ops.emplace_back(motive::MatrixOperationInit(
                               ch->id, ch->op, c.Evaluate(shifted_time)),
                           nullptr);
          break;
        }
      }
    }

    // Evaluate the transform matrix using Motive's function to maintain
    // consistency with how this frame *would* be computed at runtime.
    mathfu::vec3 scale;
    const mathfu::mat4 value = motive::MatrixOperation::CalculateResultMatrix(
        ops.data(), ops.size(), &scale);
    const mathfu::vec3 translation = value.TranslationVector3D();
    mathfu::quat rotation = ExtractQuaternion(value, scale);

    // Flip the quaternion if it lies in the opposite 4-dimensional hemisphere
    // as the previous quaternion to avoid huge changes in the individual
    // in subsequent component nodes.
    float dotprod = mathfu::quat::DotProduct(last_rotation, rotation);
    if (dotprod < 0.f) {
      rotation.set_scalar(-rotation.scalar());
      rotation.set_vector(-rotation.vector());
    }
    last_rotation = rotation;

    // Add new nodes to the sampled curves.
    curves[0].AddNode(time, translation.x);
    curves[1].AddNode(time, translation.y);
    curves[2].AddNode(time, translation.z);

    const mathfu::vec3 rotation_vector = rotation.vector();
    curves[3].AddNode(time, rotation.scalar());
    curves[4].AddNode(time, rotation_vector[0]);
    curves[5].AddNode(time, rotation_vector[1]);
    curves[6].AddNode(time, rotation_vector[2]);

    curves[7].AddNode(time, scale.x);
    curves[8].AddNode(time, scale.y);
    curves[9].AddNode(time, scale.z);

    // Update the sample time. If on the last sample, use exactly the end time.
    time += sample_rate;
    if (i == num_samples - 1) {
      time = end_time;
    }
  }

  // Re-allocate channels now that all the new curve data is prepared.
  channels.clear();
  for (int i = 0; i < 10; ++i) {
    curves[i].GenerateDerivatives();
    FlatChannelId id = AllocChannel(cur_bone_index_, curves[i].type, i);
    AddCurve(id, curves[i]);
    PruneNodes(id);
  }
}

float Animation::ToleranceForOp(motive::MatrixOperationType op) const {
  if (motive::RotateOp(op)) {
    return tolerances_.rotate;
  } else if (motive::TranslateOp(op)) {
    return tolerances_.translate;
  } else if (motive::ScaleOp(op)) {
    return tolerances_.scale;
  } else if (motive::QuaternionOp(op)) {
    return tolerances_.quaternion;
  }
  return 0.1f;
}

bool Animation::IsDefaultValue(motive::MatrixOperationType op,
                               float value) const {
  return fabs(value - DefaultOpValue(op)) < ToleranceForOp(op);
}

int Animation::MaxAnimatedTime() const {
  int max_time = std::numeric_limits<int>::min();
  for (auto bone = bones_.begin(); bone != bones_.end(); ++bone) {
    for (auto ch = bone->channels.begin(); ch != bone->channels.end(); ++ch) {
      // Only consider channels with more than one keyframe (non-constant).
      if (ch->nodes.size() > 1) {
        max_time = std::max(max_time, ch->nodes.back().time);
      }
    }
  }
  return max_time == std::numeric_limits<int>::min() ? 0 : max_time;
}

int Animation::MinAnimatedTime() const {
  int min_time = std::numeric_limits<int>::max();
  for (auto bone = bones_.begin(); bone != bones_.end(); ++bone) {
    for (auto ch = bone->channels.begin(); ch != bone->channels.end(); ++ch) {
      // Only consider channels with more than one keyframe (non-constant).
      if (ch->nodes.size() > 1) {
        min_time = std::min(min_time, ch->nodes[0].time);
      }
    }
  }
  return min_time == std::numeric_limits<int>::max() ? 0 : min_time;
}

Animation::Channels& Animation::CurChannels() {
  assert(static_cast<unsigned int>(cur_bone_index_) < bones_.size());
  return bones_[cur_bone_index_].channels;
}

const Animation::Channels& Animation::CurChannels() const {
  assert(static_cast<unsigned int>(cur_bone_index_) < bones_.size());
  return bones_[cur_bone_index_].channels;
}

float Animation::Tolerance(FlatChannelId channel_id) const {
  const Channels& channels = CurChannels();
  return ToleranceForOp(channels[channel_id].op);
}

motive::BoneIndex Animation::FirstNonRepeatingBone(
    FlatChannelId* first_channel_id) const {
  for (motive::BoneIndex bone_idx = 0; bone_idx < bones_.size(); ++bone_idx) {
    const AnimBone& bone = bones_[bone_idx];
    const Channels& channels = bone.channels;

    for (FlatChannelId channel_id = 0;
         channel_id < static_cast<FlatChannelId>(channels.size());
         ++channel_id) {
      const AnimChannel& channel = channels[channel_id];

      // Get deltas for the start and end of the channel.
      const SplineNode& start = channel.nodes.front();
      const SplineNode& end = channel.nodes.back();
      const float diff_val = fabs(start.val - end.val);
      const float diff_derivative_angle =
          fabs(DerivativeAngle(start.derivative - end.derivative));

      // Return false unless the start and end of the channel are the same.
      const float tolerance = ToleranceForOp(channel.op);
      const bool same =
          diff_val < tolerance &&
          diff_derivative_angle < tolerances_.repeat_derivative_angle;
      if (!same) {
        *first_channel_id = channel_id;
        return bone_idx;
      }
    }
  }
  return motive::kInvalidBoneIdx;
}

bool Animation::Repeat(RepeatPreference repeat_preference) const {
  if (repeat_preference == kNeverRepeat) return false;

  // Check to see if the animation is repeatable.
  FlatChannelId channel_id = 0;
  const motive::BoneIndex bone_idx = FirstNonRepeatingBone(&channel_id);
  const bool repeat = repeat_preference == kAlwaysRepeat ||
                      (repeat_preference == kRepeatIfRepeatable &&
                       bone_idx == motive::kInvalidBoneIdx);

  // Log repeat information.
  if (repeat_preference == kAlwaysRepeat) {
    if (bone_idx != motive::kInvalidBoneIdx) {
      // const Bone& bone = bones_[bone_idx];
      // const Channel& channel = bone.channels[channel_id];
      // log_.Log(fplutil::kLogWarning,
      //          "Animation marked as repeating (as requested),"
      //          " but it does not repeat on bone %s's"
      //          " `%s` channel\n",
      //          bone.name.c_str(), motive::MatrixOpName(channel.op));
    }
  } else if (repeat_preference == kRepeatIfRepeatable) {
    // log_.Log(fplutil::kLogVerbose,
    //         repeat ? "Animation repeats.\n" : "Animation does not
    //         repeat.\n");
  }

  return repeat;
}

bool Animation::UniformScaleChannels(const Channels& channels,
                                     FlatChannelId channel_id) const {
  if (channel_id + 2 >= static_cast<FlatChannelId>(channels.size()))
    return false;

  // Consider the three channels starting at `channel_id`.
  const AnimChannel& c0 = channels[channel_id];
  const AnimChannel& c1 = channels[channel_id + 1];
  const AnimChannel& c2 = channels[channel_id + 2];

  // The order is not important, but we need kScaleX, Y, and Z.
  const uint32_t op_bits = (1 << c0.op) | (1 << c1.op) | (1 << c2.op);
  if (op_bits != kScaleXyzBitfield) return false;

  // The sequence of values must also be identical.
  const Nodes& n0 = c0.nodes;
  const Nodes& n1 = c1.nodes;
  const Nodes& n2 = c2.nodes;
  const bool same_length = n0.size() == n1.size() && n0.size() == n2.size() &&
                           n1.size() == n2.size();
  if (!same_length) return false;

  // The splines must be equal.
  const float tolerance = tolerances_.scale;
  for (size_t i = 0; i < n0.size(); ++i) {
    const SplineNode v0 = n0[i];
    const SplineNode v1 = n1[i];
    const SplineNode v2 = n2[i];
    const bool are_equal =
        EqualNodes(v0, v1, tolerance, tolerances_.derivative_angle) &&
        EqualNodes(v0, v2, tolerance, tolerances_.derivative_angle) &&
        EqualNodes(v1, v2, tolerance, tolerances_.derivative_angle);
    if (!are_equal) return false;
  }

  return true;
}

FlatChannelId Animation::SummableChannel(const Channels& channels,
                                         FlatChannelId ch) const {
  const motive::MatrixOperationType ch_op = channels[ch].op;
  for (FlatChannelId id = ch + 1;
       id < static_cast<FlatChannelId>(channels.size()); ++id) {
    const motive::MatrixOperationType id_op = channels[id].op;

    // If we're adjacent to a similar op, we can combine by summing.
    if (id_op == ch_op) return id;

    // Rotate ops cannot have other ops inbetween them and still be combined.
    if (RotateOp(ch_op)) return -1;

    // Translate and scale ops can only have, respectively, other translate
    // and scale ops in between them.
    if (TranslateOp(ch_op) && !TranslateOp(id_op)) return -1;
    if (ScaleOp(ch_op) && !ScaleOp(id_op)) return -1;
  }
  return -1;
}

float Animation::EvaluateNodes(const Nodes& nodes, int time,
                               float* derivative) {
  assert(!nodes.empty());

  // Handle before and after curve cases.
  *derivative = 0.0f;
  if (time < nodes.front().time) return nodes.front().val;
  if (time >= nodes.back().time) return nodes.back().val;

  // Find first node after `time`.
  size_t i = 1;
  for (;; ++i) {
    assert(i < nodes.size());
    if (nodes[i].time >= time) break;
  }
  const SplineNode& pre = nodes[i - 1];
  const SplineNode& post = nodes[i];
  assert(pre.time <= time && time <= post.time);

  // Create a cubic from before time to after time, and interpolate values
  // with it.
  const float cubic_total_time = static_cast<float>(post.time - pre.time);
  const float cubic_time = static_cast<float>(time - pre.time);
  const motive::CubicCurve cubic(motive::CubicInit(
      pre.val, pre.derivative, post.val, post.derivative, cubic_total_time));
  *derivative = cubic.Derivative(cubic_time);
  return cubic.Evaluate(cubic_time);
}

bool Animation::GetValueAtTime(const Nodes& nodes,
                               const Nodes::const_iterator& node, int time,
                               float* value, float* derivative) const {
  if (node != nodes.end() && node->time == time) {
    *value = node->val;
    *derivative = node->derivative;
    return true;
  } else {
    *value = EvaluateNodes(nodes, time, derivative);
    return false;
  }
}

void Animation::SumChannels(Channels& channels, FlatChannelId ch_a,
                            FlatChannelId ch_b) const {
  const Nodes& nodes_a = channels[ch_a].nodes;
  const Nodes& nodes_b = channels[ch_b].nodes;
  Nodes sum;

  // TODO: The following assumes that the key on constant channels
  // is not significant to its evaluation. With pre/post infinities, single
  // key curves might not necessarily be "constant" curves. We should validate
  // if elsewhere that assumption is also made.
  //
  // If there is only one key, we ignore it because we can sample the curve
  // at any time, and don't want its key time to affect the resulting curve.
  auto node_iter_a = (nodes_a.size() == 1) ? nodes_a.end() : nodes_a.begin();
  auto node_iter_b = (nodes_b.size() == 1) ? nodes_b.end() : nodes_b.begin();

  // If both channels are constant, the curve should just contain a single
  // key with the sum.  Time and derivative are ignored in constant channels.
  if (nodes_a.size() == 1 && nodes_b.size() == 1) {
    sum.push_back(SplineNode(0, nodes_a[0].val + nodes_b[0].val, 0.0f));
  }

  while (node_iter_a != nodes_a.end() || node_iter_b != nodes_b.end()) {
    int time = std::numeric_limits<int>::max();
    if (node_iter_a != nodes_a.end()) {
      time = node_iter_a->time;
    }
    if (node_iter_b != nodes_b.end()) {
      time = std::min(time, node_iter_b->time);
    }

    float a, b;
    float da, db;
    if (GetValueAtTime(nodes_a, node_iter_a, time, &a, &da)) {
      ++node_iter_a;
    }
    if (GetValueAtTime(nodes_b, node_iter_b, time, &b, &db)) {
      ++node_iter_b;
    }
    sum.push_back(SplineNode(time, a + b, da + db));
  }

  channels[ch_a].nodes = sum;
}

motive::BoneIndex Animation::BoneParent(int bone_idx) const {
  const int parent_bone_index = bones_[bone_idx].parent_bone_index;
  return parent_bone_index < 0
             ? motive::kInvalidBoneIdx
             : static_cast<motive::BoneIndex>(parent_bone_index);
}

bool Animation::IntermediateNodesRedundant(const SplineNode* n, size_t len,
                                           float tolerance) const {
  // If the start and end nodes occur at the same time and are equal,
  // then ignore everything inbetween them.
  const SplineNode& start = n[0];
  const SplineNode& end = n[len - 1];
  if (EqualNodes(start, end, tolerance, tolerances_.derivative_angle))
    return true;

  // Construct cubic curve `c` that skips all the intermediate nodes.
  const float cubic_width = static_cast<float>(end.time - start.time);
  const motive::CubicCurve c(motive::CubicInit(
      start.val, start.derivative, end.val, end.derivative, cubic_width));

  // For each intermediate node, check if the cubic `c` is close.
  for (size_t i = 1; i < len - 1; ++i) {
    // Evaluate `c` at the time of `mid`.
    const SplineNode& mid = n[i];
    const float mid_time = static_cast<float>(mid.time - start.time);
    const float mid_val = c.Evaluate(mid_time);
    const float mid_derivative = c.Derivative(mid_time);

    // If the mid point is on the curve, it's redundant.
    const float derivative_angle_error =
        DerivativeAngle(mid_derivative - mid.derivative);
    const bool mid_on_c =
        fabs(mid_val - mid.val) < tolerance &&
        fabs(derivative_angle_error) < tolerances_.derivative_angle;
    if (!mid_on_c) return false;
  }

  // All mid points are redundant.
  return true;
}

bool Animation::EqualNodes(const SplineNode& a, const SplineNode& b,
                           float tolerance, float derivative_tolerance) {
  return a.time == b.time && fabs(a.val - b.val) < tolerance &&
         fabs(DerivativeAngle(a.derivative - b.derivative)) <
             derivative_tolerance;
}

float Animation::DefaultOpValue(motive::MatrixOperationType op) {
  return motive::OperationDefaultValue(op);
}

void Animation::LogChannel(FlatChannelId channel_id) const {
  const Channels& channels = CurChannels();
  const Nodes& n = channels[channel_id].nodes;
  for (size_t i = 0; i < n.size(); ++i) {
    const SplineNode& node = n[i];
    LOG(INFO) << "    flat, " << i << ", " << node.time << ", " << node.val
              << ", " << node.derivative;
  }
}

void Animation::LogAllChannels() const {
  LogString log;
  log.format("  %30s %16s  %9s   %s\n", "bone name", "operation", "time range",
             "values");
  LOG(INFO) << log;

  for (motive::BoneIndex bone_idx = 0; bone_idx < bones_.size(); ++bone_idx) {
    const AnimBone& bone = bones_[bone_idx];
    const Channels& channels = bone.channels;
    if (channels.empty()) {
      continue;
    }

    for (auto c = channels.begin(); c != channels.end(); ++c) {
      LogString tmp;
      log.clear();

      tmp.format("  %30s %16s   ", bone.name.c_str(), MatrixOpName(c->op));
      log.append(tmp);

      const char* format = motive::RotateOp(c->op)
                               ? "%.0f "
                               : motive::TranslateOp(c->op) ? "%.1f " : "%.2f ";
      const float factor =
          motive::RotateOp(c->op) ? motive::kRadiansToDegrees : 1.0f;

      const Nodes& n = c->nodes;
      if (n.size() <= 1) {
        tmp.format(" constant   ");
      } else {
        tmp.format("%4d~%4d   ", n[0].time, n[n.size() - 1].time);
      }
      log.append(tmp);

      for (size_t i = 0; i < n.size(); ++i) {
        tmp.format(format, factor * n[i].val);
        log.append(tmp);
      }
      LOG(INFO) << log;
    }
  }
}

bool Animation::GnuplotAllChannels(const std::string& gplot_dir) const {
  if (!CreateFolder(gplot_dir.c_str())) {
    return false;
  }

  for (motive::BoneIndex bone_idx = 0; bone_idx < NumBones(); ++bone_idx) {
    // Loop through bones; save data file for each, in turn.
    const AnimBone& bone = GetBone(bone_idx);
    const Channels& channels = bone.channels;
    if (channels.empty()) {
      continue;
    }
    std::string out_fullpath = gplot_dir + "/" + bone.name + ".dat";

    std::ostringstream os;
    std::string bone_name = bone.name;
    std::replace(bone_name.begin(), bone_name.end(), '_', '-');
    os << "# Run shell cmd below to visualize this file:\n#\n"
       << "# gnuplot -persist -e \""
       << "d = '" << out_fullpath.c_str() << "'; "
       << "set title '" << bone_name << "' "
       << "font '14' textcolor rgbcolor 'royalblue'; "
       << "set linetype 1; set pointsize 1; "
       << "plot ";

    std::set<int> key_times;
    for (auto it = channels.begin(); it != channels.end(); ++it) {
      // Build union set of all keytimes over all channels
      std::string c_name = MatrixOpName(it->op);
      std::replace(c_name.begin(), c_name.end(), ' ', '-');
      os << "d using 1:" << 2 + static_cast<int>(it - channels.begin()) << " "
         << "title '" << c_name << "' "
         << "with linespoints pointtype 7 pointsize .7, ";
      const Nodes& n = it->nodes;
      for (size_t i = 0; i < n.size(); ++i) {
        key_times.emplace(n[i].time);
      }
    }
    os << "\"\n#\n";

    for (auto it = key_times.begin(); it != key_times.end(); ++it) {
      // Each row of data is keytime + values for all channels
      int key_time = *it;
      float value, deriv;
      os << key_time << " ";
      for (auto c = channels.begin(); c != channels.end(); ++c) {
        const float factor =
            motive::RotateOp(c->op) ? motive::kRadiansToDegrees : 1.0f;
        value = EvaluateNodes(c->nodes, key_time, &deriv);
        os << factor * value << " ";
      }
      os << "\n";
    }

    const std::string& data = os.str();
    if (!SaveFile(data.c_str(), data.size(), out_fullpath.c_str(), false)) {
      return false;
    }
  }
  return true;
}

}  // namespace tool
}  // namespace lull
