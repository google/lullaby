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

#include <fbxsdk.h>
#include <algorithm>
#include <unordered_map>
#include "lullaby/util/filename.h"
#include "lullaby/util/logging.h"
#include "lullaby/util/math.h"
#include "lullaby/util/optional.h"
#include "lullaby/tools/anim_pipeline/animation.h"
#include "lullaby/tools/anim_pipeline/import_options.h"
#include "lullaby/tools/common/fbx_base_importer.h"

namespace lull {
namespace tool {

// Anonymous namespace to avoid collisions with the model_pipeline FbxImporter.
namespace {

// Strips namespaces that are added to bone node names on export from Maya.
const char* BoneBaseName(const std::string& name) {
  const size_t colon = name.find_last_of(':');
  const size_t base_idx = colon == std::string::npos ? 0 : colon + 1;
  return &name[base_idx];
}

// Reads animation data from a FbxProperty taken from an FBX animation node.
class FbxAnimationReader {
  // Helper type that mimics an iterator style begin/end pair, but using
  // integer indices (as required by the API)
  struct SegmentRange {
    const int begin_index;
    const int end_index;
  };

 public:
  FbxAnimationReader(FbxPropertyT<FbxDouble3>* property, Tolerances tolerances,
                     motive::MatrixOperationType base_op, bool invert,
                     FbxAnimStack* anim_stack)
      : property_(property),
        tolerances_(tolerances),
        base_op_(base_op),
        invert_(invert) {
    // Find the right FbxAnimCurveNode.
    const int count = property->GetSrcObjectCount<FbxAnimCurveNode>();
    for (int i = 0; i < count; ++i) {
      FbxAnimCurveNode* candidate = property->GetSrcObject<FbxAnimCurveNode>(i);
      if (!anim_stack) {
        // no anim stack to determine context, the first is as good as any.
        anim_node_ = candidate;
        break;
      } else {
        int layer_count = candidate->GetDstObjectCount<FbxAnimLayer>();
        for (int layer_index = 0; layer_index < layer_count; layer_index++) {
          FbxAnimLayer* layer =
              candidate->GetDstObject<FbxAnimLayer>(layer_index);
          if (layer->GetDstObjectCount<FbxAnimStack>() > 0 &&
              layer->GetDstObject<FbxAnimStack>(0) == anim_stack) {
            anim_node_ = candidate;
            break;
          }
        }
      }
    }
    // If there are nodes, we should find one.
    assert(anim_node_ || !count);

    // Ensure we have three channels (x, y, z).
    if (anim_node_ != nullptr && anim_node_->GetChannelsCount() != 3) {
      LOG(ERROR) << "Animation property " << property->GetNameAsCStr()
                 << " has " << anim_node_->GetChannelsCount()
                 << " channels instead of 3";
    }

    for (int c = 0; c < 3; ++c) {
      const_channels_[c] = RetrieveConstValue(c);
    }
  }

  // Returns true if all channels are const.
  bool AllChannelsConst() const {
    return const_channels_[0] && const_channels_[1] && const_channels_[2];
  }

  // Returns a floating-point value if the animation associated with the channel
  // is a constant operation, otherwise returns NullOpt. The value is pre-cached
  // at construction time.
  const Optional<float>& GetConstValue(int channel) const {
    return const_channels_[channel];
  }

  FbxTimeSpan GetSegmentSpan(FbxAnimCurve* curve,
                             const Optional<FbxTimeSpan>& clip_span,
                             int segment_index) const {
    FbxTimeSpan segment_span(curve->KeyGetTime(segment_index),
                             curve->KeyGetTime(segment_index + 1));
    return clip_span ? clip_span->Intersect(segment_span) : segment_span;
  }

  // An empty optional, or, in the event of overlap, the begin/end indices.
  Optional<SegmentRange> GetOverlappingSegmentRange(
      FbxAnimCurve* curve, const Optional<FbxTimeSpan>& clip_span) const {
    if (!clip_span) {
      return SegmentRange{0, curve->KeyGetCount() - 1};
    }

    const int key_count = curve->KeyGetCount();

    if (curve->KeyGetTime(0) >= clip_span->GetStop() ||
        curve->KeyGetTime(key_count - 1) <= clip_span->GetStart()) {
      // trivial non-overlap
      return {};
    }

    int first_overlap = -1;
    int segment_index = 0;
    const int segment_count = key_count - 1;
    for (; segment_index < segment_count; segment_index++) {
      FbxTimeSpan segment_span =
          GetSegmentSpan(curve, clip_span, segment_index);
      if (segment_span.GetDuration().Get() > 0) {
        if (first_overlap == -1) {
          // first segment that overlaps
          first_overlap = segment_index;
        }
      } else if (first_overlap != -1) {
        // We were previously overlapping; this is the 'end' segment.
        break;
      }
    }
    return SegmentRange{first_overlap, segment_index};
  }

  // Returns a floating-point value if the animation associated with the channel
  // is a constant operation, otherwise returns NullOpt. The value is fetched
  // from the actual animation data and should be cached for future accesses.
  Optional<float> RetrieveConstValue(int channel) const {
    const unsigned int num_channels =
        anim_node_ ? anim_node_->GetChannelsCount() : 0;
    if (num_channels == 0) {
      // If there is no animation, return the "const value" directly from the
      // property.
      return property_ ? ConvertValue(property_->Get()[channel]) : 0.f;
    }

    const float channel_value =
        ConvertValue(anim_node_->GetChannelValue(channel, 0.0f));

    // If there is no animation curve or the curve has no keys, then return the
    // "const value" directly from the channel in the animation.
    FbxAnimCurve* curve = anim_node_->GetCurve(channel);
    const int num_keys = curve ? curve->KeyGetCount() : 0;
    if (num_keys <= 0) {
      return channel_value;
    }

    // The first value may be different from the value at time 0.
    // The value at time 0 may actually be the end value, if the first key
    // doesn't start at time 0 and the channel cycles.
    const float first_value = ConvertValue(curve->KeyGetValue(0));

    const float derivative_tolerance = tolerances_.derivative_angle;
    float op_tolerance = 0.1f;
    if (motive::RotateOp(base_op_)) {
      op_tolerance = tolerances_.rotate;
    } else if (motive::TranslateOp(base_op_)) {
      op_tolerance = tolerances_.translate;
    } else if (motive::ScaleOp(base_op_)) {
      op_tolerance = tolerances_.scale;
    }

    // Scan the entire curve for anything that indicates that it is a non-const
    // curve.
    for (int i = 0; i < num_keys - 1; ++i) {
      // A value in the curve differs from the initial value, so the curve is
      // not constant.
      const float value = ConvertValue(curve->KeyGetValue(i + 1));
      if (fabs(value - first_value) > op_tolerance) {
        return NullOpt;
      }

      // The left derivative is non-zero, so the curve is not constant.
      const float left_derivative =
          ConvertDerivative(curve->KeyGetLeftDerivative(i));
      if (fabs(DerivativeAngle(left_derivative)) > derivative_tolerance) {
        return NullOpt;
      }

      // The right derivative is non-zero, so the curve is not constant.
      const float right_derivative =
          ConvertDerivative(curve->KeyGetRightDerivative(i));
      if (fabs(DerivativeAngle(right_derivative)) > derivative_tolerance) {
        return NullOpt;
      }
    }

    // The curve appears to hold a single value for its entire duration, so
    // just return the first value in the curve.
    return first_value;
  }

  // Returns the segment of an animation curve sampled between start and end
  // times.
  AnimCurve GetCurveSegment(FbxAnimCurve* curve, FbxTime start_time,
                            FbxTime end_time) const {
    // Oversample the original curve to verify its cubic validity. Nearly all
    // of the oversampled points won't result in nodes in the final curves.
    static const int kNumPointsPerSegment = 16;
    AnimCurve segment(base_op_, kNumPointsPerSegment);
    const FbxTime delta_time =
        (end_time - start_time) / (kNumPointsPerSegment - 1);
    int last_index = 0;
    FbxTime time = start_time;
    for (int i = 0; i < kNumPointsPerSegment; ++i) {
      const float value = curve->Evaluate(time, &last_index);
      const float derivative =
          (i == 0) ? curve->EvaluateRightDerivative(time, &last_index)
                   : curve->EvaluateLeftDerivative(time, &last_index);
      segment.AddNode(ConvertTime(time), ConvertValue(value),
                      ConvertDerivative(derivative));
      time += delta_time;
    }
    return segment;
  }

  // Returns the curve associated with the channel as a list of curve
  // segments.
  std::vector<AnimCurve> GetCurveSegments(
      int channel, const Optional<FbxTimeSpan>& clip_span) const {
    std::vector<AnimCurve> segments;
    if (anim_node_ == nullptr) {
      LOG(ERROR) << "No animation node. How did this happen?";
      return segments;
    }

    // For simplicitly, we will process only the first curve. If we run into
    // animations with multiple curves, we should add extra logic here.
    const int num_curves = anim_node_->GetCurveCount(channel);
    if (num_curves > 1) {
      LOG(WARNING) << property_->GetNameAsCStr() << " has " << num_curves
                   << " curves. Only using the first one.";
    }

    FbxAnimCurve* curve = anim_node_->GetCurve(channel);

    Optional<SegmentRange> overlapping_segment_range =
        GetOverlappingSegmentRange(curve, clip_span);

    if (overlapping_segment_range) {
      segments.reserve(overlapping_segment_range->end_index -
                       overlapping_segment_range->begin_index);
      for (int segment_index = overlapping_segment_range->begin_index;
           segment_index < overlapping_segment_range->end_index;
           segment_index++) {
        FbxTimeSpan segment_span =
            GetSegmentSpan(curve, clip_span, segment_index);
        segments.emplace_back(GetCurveSegment(curve, segment_span.GetStart(),
                                              segment_span.GetStop()));
      }
    } else {
      segments.reserve(1);
      segments.emplace_back(
          GetCurveSegment(curve, clip_span->GetStart(), clip_span->GetStop()));
    }

    return segments;
  }

  // Returns the Euler rotation curves associated with a list of channels as a
  // list of curve segments per channel. Applies Euler filtering to each segment
  // to prevent large Euler angle changes mid-animation. |channel_order|
  // indicates the order to process channels in.
  void GetRotationCurveSegments(const int* channel_order,
                                const Optional<FbxTimeSpan>& clip_span,
                                std::vector<AnimCurve>* out_curves) const {
    // Retrieve curves for each channel, if possible.
    FbxAnimCurve* in_curves[3] = {nullptr, nullptr, nullptr};
    std::set<FbxTime> sorted_keys;
    for (int c = 0; c < 3; ++c) {
      const int channel = channel_order[c];
      auto const_channel_value = GetConstValue(channel);
      if (!const_channel_value) {
        // For simplicitly, we will process only the first curve. If we run into
        // animations with multiple curves, we should add extra logic here.
        const int num_curves = anim_node_->GetCurveCount(channel);
        if (num_curves > 1) {
          LOG(WARNING) << property_->GetNameAsCStr() << " has " << num_curves
                       << " curves. Only using the first one.";
        }
        in_curves[channel] = anim_node_->GetCurve(channel);
        assert(in_curves[channel]);

        // Find the set of keyframes required by the original set of curves.
        // These should be the same for every curve but might not be depending
        // on how the asset was exported.
        const Optional<SegmentRange> overlapping_range =
            GetOverlappingSegmentRange(in_curves[channel], clip_span);
        if (overlapping_range) {
          for (int segment_index = overlapping_range->begin_index;
               segment_index < overlapping_range->end_index; segment_index++) {
            FbxTimeSpan segment_span =
                GetSegmentSpan(in_curves[channel], clip_span, segment_index);
            sorted_keys.emplace(segment_span.GetStart());
            if (segment_index == overlapping_range->end_index - 1) {
              // otherwise, adding the segment end is redundant.
              sorted_keys.emplace(segment_span.GetStop());
            }
          }
        } else {
          sorted_keys.emplace(clip_span->GetStart());
          sorted_keys.emplace(clip_span->GetStop());
        }
      }
      out_curves[channel].clear();
    }

    // Gather curve samples between each of the required keyframes.
    std::vector<FbxTime> keys(sorted_keys.size());
    keys.assign(sorted_keys.begin(), sorted_keys.end());
    const int num_keys = keys.size();
    assert(num_keys > 1);

    // Reserve space for all the output segments.
    out_curves[0].reserve(num_keys);
    out_curves[1].reserve(num_keys);
    out_curves[2].reserve(num_keys);

    for (int k = 0; k < num_keys - 1; ++k) {
      const FbxTime& start_time = keys[k];
      const FbxTime& end_time = keys[k + 1];

      // Before sampling the curve, check for Euler flips. To do so, get the
      // start and end values for this curve segment.
      mathfu::vec3 start_sample;
      mathfu::vec3 end_sample;
      for (int c = 0; c < 3; ++c) {
        const int channel = channel_order[c];
        auto const_channel_value = GetConstValue(channel);
        if (!const_channel_value) {
          start_sample[channel] =
              ConvertValue(in_curves[channel]->Evaluate(start_time));
          end_sample[channel] =
              ConvertValue(in_curves[channel]->Evaluate(end_time));
        } else {
          start_sample[channel] = const_channel_value.value();
          end_sample[channel] = const_channel_value.value();
        }
      }

      // If the Euler-filtered rotation is not equal to the curve-sampled
      // rotation, add a curve with only two samples that bridges the "bad"
      // transition. Ordinarily, consecutive keys in a curve produces duplicate
      // nodes that are de-duplicated later. The filtered `end_sample` won't
      // match the first node of the next curve segment, resulting in two nodes
      // at the same time value. This discontinuity is handled at runtime.
      const mathfu::vec3 filtered = EulerFilter(end_sample, start_sample);
      if (end_sample != filtered) {
        for (int c = 0; c < 3; ++c) {
          const int channel = channel_order[c];
          auto const_channel_value = GetConstValue(channel);

          // Create a curve segment that holds exactly two nodes.
          AnimCurve segment(
              static_cast<motive::MatrixOperationType>(base_op_ + channel), 2);

          // Constant curves have 0 derivatives. Non-constant curves must be
          // evaluated.
          float start_derivative = 0.f;
          float end_derivative = 0.f;
          if (!const_channel_value) {
            // Evaluate the left derivative of the start and right derivative
            // of the end to cut out the "bad" part of the curve.
            FbxAnimCurve* curve = in_curves[channel];
            start_derivative =
                ConvertDerivative(curve->EvaluateLeftDerivative(start_time));
            end_derivative =
                ConvertDerivative(curve->EvaluateRightDerivative(end_time));
          }

          // Add nodes at the beginning and end of segment.
          segment.AddNode(ConvertTime(start_time), start_sample[channel],
                          start_derivative);
          segment.AddNode(ConvertTime(end_time), filtered[channel],
                          end_derivative);
          out_curves[channel].emplace_back(std::move(segment));
        }
      } else {
        // Otherwise, the curves can be sampled one-at-a-time for this interval.
        for (int c = 0; c < 3; ++c) {
          const int channel = channel_order[c];
          auto const_channel_value = GetConstValue(channel);
          if (!const_channel_value) {
            out_curves[channel].emplace_back(
                GetCurveSegment(in_curves[channel], start_time, end_time));
          } else {
            const float value = const_channel_value.value();
            AnimCurve segment(
                static_cast<motive::MatrixOperationType>(base_op_ + channel),
                2);
            segment.AddNode(ConvertTime(start_time), value, 0.f);
            segment.AddNode(ConvertTime(end_time), value, 0.f);
            out_curves[channel].emplace_back(std::move(segment));
          }
        }
      }
    }
  }

 private:
  // Returns the time converted into milliseconds.
  float ConvertTime(FbxTime time) const {
    return time.GetSecondDouble() * 1000.f;
  }

  float ConvertValue(double value) const {
    const float tmp = motive::RotateOp(base_op_)
                          ? static_cast<float>(FBXSDK_DEG_TO_RAD * value)
                          : static_cast<float>(value);
    return !invert_ ? tmp : motive::ScaleOp(base_op_) ? 1.0f / tmp : -tmp;
  }

  float ConvertDerivative(float d) const {
    // The FBX derivative appears to be in units of seconds.
    // The FlatBuffer file format is in units of milliseconds.
    const float time_scaled_derivative = d / 1000.0f;
    return ConvertValue(time_scaled_derivative);
  }

  FbxPropertyT<FbxDouble3>* property_ = nullptr;
  FbxAnimCurveNode* anim_node_ = nullptr;
  Tolerances tolerances_;
  motive::MatrixOperationType base_op_ = motive::kInvalidMatrixOperation;
  Optional<float> const_channels_[3];
  bool invert_ = false;
};

class FbxImporter : public FbxBaseImporter {
 public:
  FbxImporter() {}
  std::vector<Animation> Load(const std::string& filename,
                              const ImportOptions& opts);

 private:
  void BuildBoneAnimation(FbxNode* node, int bone_index,
                          FbxAnimStack* anim_stack);
  void LoadAnimation(FbxAnimStack* anim_stack, bool no_uniform_scale,
                     bool sqt_animations);
  // Gathers animations curves from a reader one at a time. Creates
  // constant-value curves when possible. This function should be called when
  // the curves for a property can be gathered completely independently, which
  // happens when:
  // 1. They are all constant OR,
  // 2. They are not rotation curves (which have Euler filtering applied).
  // |channel_order| indicates the order to process channels in.
  void ReadCurvesOneAtATime(const FbxAnimationReader& reader, int bone_index,
                            motive::MatrixOpId base_id,
                            motive::MatrixOperationType base_op,
                            const int* channel_order,
                            const Optional<FbxTimeSpan>& clip_span);
  // Gathers animations curves associated with a rotation property from a reader
  // all at once. Applies Euler filtering to the input curves to ensure the
  // output curves do not feature rapid changes in Euler rotations.
  // |channel_order| indicates the order to process channels in.
  void ReadRotationCurves(const FbxAnimationReader& reader, int bone_index,
                          const int* channel_order, motive::MatrixOpId base_id,
                          const Optional<FbxTimeSpan>& clip_span);

  void ReadAnimation(FbxNode* node, int bone_index,
                     FbxPropertyT<FbxDouble3>* property,
                     motive::MatrixOpId base_id,
                     motive::MatrixOperationType base_op, bool invert,
                     FbxAnimStack* anim_stack);
  Animation* anim_ = nullptr;
};

std::vector<Animation> FbxImporter::Load(const std::string& filename,
                                         const ImportOptions& opts) {
  std::vector<Animation> anims;

  FbxBaseImporter::Options fbx_opts;
  fbx_opts.axis_system = opts.axis_system;
  fbx_opts.cm_per_unit = opts.cm_per_unit;
  fbx_opts.scale_multiplier = opts.scale_multiplier;

  const bool success = LoadScene(filename, fbx_opts);
  if (success) {
    if (opts.import_clips) {
      ForEachAnimationStack([&](FbxAnimStack* anim_stack) {
        anims.push_back(Animation{anim_stack->GetName(), opts.tolerances,
                                  opts.sqt_animations});
        anim_ = &anims.back();
        LoadAnimation(anim_stack, opts.no_uniform_scale, opts.sqt_animations);
        anim_ = nullptr;
      });
    } else {
      anims.push_back(
          Animation{RemoveDirectoryAndExtensionFromFilename(filename),
                    opts.tolerances, opts.sqt_animations});
      anim_ = &anims.back();
      LoadAnimation(nullptr, opts.no_uniform_scale, opts.sqt_animations);
      anim_ = nullptr;
    }
  }

  return anims;
}

void FbxImporter::LoadAnimation(FbxAnimStack* anim_stack, bool no_uniform_scale,
                                bool sqt_animations) {
  std::unordered_map<FbxNode*, int> node_to_bone_map;
  ForEachBone([&](FbxNode* node, FbxNode* parent) {
    auto iter = node_to_bone_map.find(parent);
    const int parent_index = iter != node_to_bone_map.end() ? iter->second : -1;
    const int bone_index =
        anim_->RegisterBone(BoneBaseName(node->GetName()), parent_index);
    node_to_bone_map[node] = bone_index;
  });
  ForEachBone([&](FbxNode* node, FbxNode* parent) {
    auto iter = node_to_bone_map.find(node);
    if (iter == node_to_bone_map.end()) {
      return;
    }
    BuildBoneAnimation(node, iter->second, anim_stack);
    if (sqt_animations) {
      anim_->BakeSqtAnimations();
    }
    anim_->PruneChannels(no_uniform_scale);
  });
}

const int* ChannelOrder(const FbxNode* node, motive::MatrixOperationType op,
                        bool invert) {
  static const int kDefaultChannelOrder[] = {0, 1, 2};
  // x, y, z order is significant only for rotations.
  if (!motive::RotateOp(op)) {
    return kDefaultChannelOrder;
  }

  // For rotations, we output the last channel first, since they're applied in
  // reverse order in the motive runtime.
  FbxEuler::EOrder rotation_order;
  node->GetRotationOrder(FbxNode::eSourcePivot, rotation_order);
  assert(0 <= rotation_order &&
         rotation_order < fbxsdk::FbxEuler::eOrderSphericXYZ);

  if (!invert) {
    static const int kRotationOrderToChannelOrder[7][3] = {
        {2, 1, 0},  // eOrderXYZ,
        {2, 0, 1},  // eOrderXZY,
        {1, 0, 2},  // eOrderYZX,
        {1, 2, 0},  // eOrderYXZ,
        {0, 2, 1},  // eOrderZXY,
        {0, 1, 2},  // eOrderZYX,
        {2, 1, 0},  // eOrderSphericXYZ
    };
    return kRotationOrderToChannelOrder[rotation_order];
  } else {
    static const int kRotationOrderToChannelOrderInverted[7][3] = {
        {0, 1, 2},  // eOrderXYZ,
        {0, 2, 1},  // eOrderXZY,
        {1, 2, 0},  // eOrderYZX,
        {1, 0, 2},  // eOrderYXZ,
        {2, 0, 1},  // eOrderZXY,
        {2, 1, 0},  // eOrderZYX,
        {0, 1, 2},  // eOrderSphericXYZ
    };
    return kRotationOrderToChannelOrderInverted[rotation_order];
  }
}

void FbxImporter::ReadCurvesOneAtATime(
    const FbxAnimationReader& reader, int bone_index,
    motive::MatrixOpId base_id, motive::MatrixOperationType base_op,
    const int* channel_order, const Optional<FbxTimeSpan>& clip_span) {
  for (int i = 0; i < 3; ++i) {
    const int channel = channel_order[i];
    auto const_channel_value = reader.GetConstValue(channel);

    // Calculate the actual operation type and id for this channel.
    const motive::MatrixOperationType op =
        static_cast<motive::MatrixOperationType>(base_op + channel);
    const motive::MatrixOpId id = static_cast<motive::MatrixOpId>(base_id + i);

    if (const_channel_value) {
      // Do not record a const value if it is the same as the default value.
      if (anim_->IsDefaultValue(op, const_channel_value.value())) {
        continue;
      }

      // Record constant value for this channel.
      const FlatChannelId channel_id = anim_->AllocChannel(bone_index, op, id);
      anim_->AddConstant(channel_id, const_channel_value.value());
    } else {
      const std::vector<AnimCurve> segments =
          reader.GetCurveSegments(channel, clip_span);
      if (segments.empty()) {
        continue;
      }

      const FlatChannelId channel_id = anim_->AllocChannel(bone_index, op, id);
      for (const AnimCurve& segment : segments) {
        anim_->AddCurve(channel_id, segment);
      }
      // Remove redundant nodes from the final curve.
      anim_->PruneNodes(channel_id);
    }
  }
}

void FbxImporter::ReadRotationCurves(const FbxAnimationReader& reader,
                                     int bone_index, const int* channel_order,
                                     motive::MatrixOpId base_id,
                                     const Optional<FbxTimeSpan>& clip_span) {
  std::vector<AnimCurve> segments[3];
  reader.GetRotationCurveSegments(channel_order, clip_span, segments);

  for (int i = 0; i < 3; ++i) {
    const int channel = channel_order[i];

    // Calculate the actual operation type and id for this channel.
    const motive::MatrixOperationType op =
        static_cast<motive::MatrixOperationType>(motive::kRotateAboutX +
                                                 channel);
    const motive::MatrixOpId id = static_cast<motive::MatrixOpId>(base_id + i);

    const FlatChannelId channel_id = anim_->AllocChannel(bone_index, op, id);
    for (const AnimCurve& segment : segments[channel]) {
      anim_->AddCurve(channel_id, segment);
    }
    // Remove redundant nodes from the final curve.
    anim_->PruneNodes(channel_id);
  }
}

void FbxImporter::ReadAnimation(FbxNode* node, int bone_index,
                                FbxPropertyT<FbxDouble3>* property,
                                motive::MatrixOpId base_id,
                                motive::MatrixOperationType base_op,
                                bool invert, FbxAnimStack* anim_stack) {
  Optional<FbxTimeSpan> clip_span =
      anim_stack ? anim_stack->GetReferenceTimeSpan() : Optional<FbxTimeSpan>{};

  FbxAnimationReader reader(property, anim_->GetTolerances(), base_op, invert,
                            anim_stack);

  // Loop through each channel, reading the animation data. We cannot simply
  // iterate through channels (ie. 1, 2, 3). Instead, we use the index order
  // as returned by ChannelOrder.
  const int* channel_order = ChannelOrder(node, base_op, invert);

  // If processing a non-rotation op, create constants and curves one at a
  // time. If processing a rotation op, only process curves one at a time
  // if all curves were const. If processing a rotation op with at least one
  // non-const curve, process them all at the same time.
  if (!motive::RotateOp(base_op) || reader.AllChannelsConst()) {
    ReadCurvesOneAtATime(reader, bone_index, base_id, base_op, channel_order,
                         clip_span);
  } else {
    ReadRotationCurves(reader, bone_index, channel_order, base_id, clip_span);
  }
}

void FbxImporter::BuildBoneAnimation(FbxNode* node, int bone_index,
                                     FbxAnimStack* anim_stack) {
  // The FBX tranform format is defined as below (see
  // http://help.autodesk.com/view/FBX/2016/ENU/?guid=__files_GUID_10CDD63C_79C1_4F2D_BB28_AD2BE65A02ED_htm):
  //
  // WorldTransform = ParentWorldTransform * T * Roff * Rp * Rpre * R *
  //                  Rpost_inv * Rp_inv * Soff * Sp * S * Sp_inv
  //
  ReadAnimation(node, bone_index, &node->LclTranslation, 0, motive::kTranslateX,
                false, anim_stack);
  ReadAnimation(node, bone_index, &node->RotationOffset, 0, motive::kTranslateX,
                false, anim_stack);
  ReadAnimation(node, bone_index, &node->RotationPivot, 0, motive::kTranslateX,
                false, anim_stack);
  ReadAnimation(node, bone_index, &node->PreRotation, 3, motive::kRotateAboutX,
                false, anim_stack);
  ReadAnimation(node, bone_index, &node->LclRotation, 6, motive::kRotateAboutX,
                false, anim_stack);
  ReadAnimation(node, bone_index, &node->PostRotation, 9, motive::kRotateAboutX,
                true, anim_stack);
  ReadAnimation(node, bone_index, &node->RotationPivot, 12, motive::kTranslateX,
                true, anim_stack);
  ReadAnimation(node, bone_index, &node->ScalingOffset, 12, motive::kTranslateX,
                false, anim_stack);
  ReadAnimation(node, bone_index, &node->ScalingPivot, 12, motive::kTranslateX,
                false, anim_stack);
  ReadAnimation(node, bone_index, &node->LclScaling, 15, motive::kScaleX, false,
                anim_stack);
  ReadAnimation(node, bone_index, &node->ScalingPivot, 19, motive::kTranslateX,
                true, anim_stack);
}

}  // namespace

std::vector<Animation> ImportFbx(const std::string& filename,
                                 const ImportOptions& opts) {
  FbxImporter importer;
  return importer.Load(filename, opts);
}

}  // namespace tool
}  // namespace lull
