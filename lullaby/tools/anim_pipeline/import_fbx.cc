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

#include <fbxsdk.h>
#include "lullaby/util/logging.h"
#include "lullaby/tools/anim_pipeline/animation.h"
#include "lullaby/tools/common/fbx_utils.h"

namespace lull {
namespace tool {

// Represents a segment of a curve.
struct AnimationCurveSegment {
  static const int kNumPoints = 16;
  int start_time = -1;
  int end_time = -1;
  float values[kNumPoints];
  float derivatives[kNumPoints];
};

// Reads animation data from a FbxProperty taken from an FBX animation node.
class FbxAnimationReader {
 public:
  FbxAnimationReader(FbxPropertyT<FbxDouble3>* property, Tolerances tolerances,
                     motive::MatrixOpId base_id,
                     motive::MatrixOperationType base_op, bool invert)
      : property(property),
        tolerances(tolerances),
        base_id(base_id),
        base_op(base_op),
        invert(invert) {
    // Get the anim_node from the property.
    const int count = property->GetSrcObjectCount();
    for (int i = 0; i < count; ++i) {
      FbxObject* obj = property->GetSrcObject(i);
      if (obj->GetClassId() == FbxAnimCurveNode::ClassId) {
        anim_node = static_cast<FbxAnimCurveNode*>(obj);
        break;
      }
    }

    // Ensure we have three channels (x, y, z).
    if (anim_node != nullptr && anim_node->GetChannelsCount() != 3) {
      LOG(ERROR) << "Animation property " << property->GetNameAsCStr()
                 << " has " << anim_node->GetChannelsCount()
                 << " channels instead of 3";
    }
  }

  // Returns a floating-point value if the animation associated with the channel
  // is a constant operation, otherwise returns NullOpt.
  Optional<float> GetConstValue(int channel) {
    const unsigned int num_channels =
        anim_node ? anim_node->GetChannelsCount() : 0;
    if (num_channels == 0) {
      // If there is no animation, return the "const value" directly from the
      // property.
      return property ? ConvertValue(property->Get()[channel]) : 0.f;
    }

    const float channel_value =
        ConvertValue(anim_node->GetChannelValue(channel, 0.0f));

    // If there is no animation curve or the curve has no keys, then return the
    // "const value" directly from the channel in the animation.
    FbxAnimCurve* curve = anim_node->GetCurve(channel);
    const int num_keys = curve ? curve->KeyGetCount() : 0;
    if (num_keys <= 0) {
      return channel_value;
    }

    // The first value may be different from the value at time 0.
    // The value at time 0 may actually be the end value, if the first key
    // doesn't start at time 0 and the channel cycles.
    const float first_value = curve->KeyGetValue(0);

    const float derivative_tolerance = tolerances.derivative_angle;
    float op_tolerance = 0.1f;
    if (motive::RotateOp(base_op)) {
      op_tolerance = tolerances.rotate;
    } else if (motive::TranslateOp(base_op)) {
      op_tolerance = tolerances.translate;
    } else if (motive::ScaleOp(base_op)) {
      op_tolerance = tolerances.scale;
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

  // Returns the curve associated with the channel.
  std::vector<AnimationCurveSegment> GetCurveSegments(int channel) {
    if (anim_node == nullptr) {
      LOG(ERROR) << "No animation node.  How did this happen?";
      return {};
    }

    // For simplicitly, we will process only the first curve. If we run into
    // animations with multiple curves, we should add extra logic here.
    const int num_curves = anim_node->GetCurveCount(channel);
    if (num_curves > 1) {
      LOG(WARNING) << property->GetNameAsCStr() << ", "
                   << motive::MatrixOpName(base_op) << " has " << num_curves
                   << " curves. Only using the first one.";
    }

    FbxAnimCurve* curve = anim_node->GetCurve(channel);
    const int num_keys = curve->KeyGetCount();

    std::vector<AnimationCurveSegment> segments(num_keys - 1);
    for (int k = 0; k < num_keys - 1; ++k) {
      const FbxTime start_time = curve->KeyGetTime(k);
      const FbxTime end_time = curve->KeyGetTime(k + 1);
      const FbxTime delta_time =
          (end_time - start_time) / (AnimationCurveSegment::kNumPoints - 1);

      AnimationCurveSegment& s = segments[k];
      s.start_time = ConvertTime(start_time);
      s.end_time = ConvertTime(end_time);

      int last_index = 0;
      FbxTime time = start_time;
      for (int i = 0; i < AnimationCurveSegment::kNumPoints; ++i) {
        const float value = curve->Evaluate(time, &last_index);
        const float derivative =
            (i == 0) ? curve->EvaluateRightDerivative(time, &last_index)
                     : curve->EvaluateLeftDerivative(time, &last_index);
        s.values[i] = ConvertValue(value);
        s.derivatives[i] = ConvertDerivative(derivative);
        time += delta_time;
      }
    }
    return segments;
  }

 private:
  int ConvertTime(FbxTime time) const {
    const FbxLongLong milliseconds = time.GetMilliSeconds();
    assert(milliseconds <= std::numeric_limits<int>::max());
    return static_cast<int>(milliseconds);
  }

  float ConvertValue(double value) const {
    const float tmp = motive::RotateOp(base_op)
                          ? static_cast<float>(FBXSDK_DEG_TO_RAD * value)
                          : static_cast<float>(value);
    return !invert ? tmp : motive::ScaleOp(base_op) ? 1.0f / tmp : -tmp;
  }

  float ConvertDerivative(float d) const {
    // The FBX derivative appears to be in units of seconds.
    // The FlatBuffer file format is in units of milliseconds.
    const float time_scaled_derivative = d / 1000.0f;
    return ConvertValue(time_scaled_derivative);
  }

  FbxPropertyT<FbxDouble3>* property = nullptr;
  FbxAnimCurveNode* anim_node = nullptr;
  Tolerances tolerances;
  motive::MatrixOpId base_id = motive::kInvalidMatrixOpId;
  motive::MatrixOperationType base_op = motive::kInvalidMatrixOperation;
  bool invert = false;
};

class FbxImporter : public FbxBaseImporter {
 public:
  FbxImporter(Tolerances tolerances, bool no_uniform_scale)
      : tolerances_(tolerances), no_uniform_scale_(no_uniform_scale) {}

  Animation Load(const std::string& filename, AxisSystem axis_system,
                 float distance_unit_scale);

 private:
  void BuildAnimationRecursive(FbxNode* node);
  void BuildBoneAnimation(FbxNode* node);
  void ReadAnimation(FbxNode* node, FbxPropertyT<FbxDouble3>* property,
                     motive::MatrixOpId base_id,
                     motive::MatrixOperationType base_op, bool invert);

  Animation* anim_ = nullptr;
  Tolerances tolerances_;
  bool no_uniform_scale_ = false;
  std::unordered_map<FbxNode*, int> node_to_bone_map_;
};

Animation FbxImporter::Load(const std::string& filename, AxisSystem axis_system,
                            float distance_unit_scale) {
  Animation anim(filename, tolerances_);

  const bool success = LoadScene(filename);
  if (success) {
    ConvertFbxScale(distance_unit_scale);
    ConvertFbxAxes(axis_system);

    anim_ = &anim;
    const std::vector<BoneInfo> bones = BuildBoneList();
    for (const BoneInfo& bone : bones) {
      node_to_bone_map_[bone.node] =
          anim_->RegisterBone(bone.node->GetName(), bone.parent_index);
    }
    BuildAnimationRecursive(GetRootNode());
    anim_ = nullptr;
  }

  return anim;
}

void FbxImporter::BuildAnimationRecursive(FbxNode* node) {
  if (node == nullptr) {
    return;
  }

  // The root node cannot have a transform applied to it, so we do not
  // export it as a bone.
  if (node != GetRootNode()) {
    // We're only interested in nodes that contain meshes or are part of a
    // skeleton. If a node and all nodes under it have neither, we early out.
    auto iter = node_to_bone_map_.find(node);
    if (iter == node_to_bone_map_.end()) {
      return;
    }
    BuildBoneAnimation(node);
  }

  // Recursively traverse each node in the scene.
  for (int i = 0; i < node->GetChildCount(); i++) {
    BuildAnimationRecursive(node->GetChild(i));
  }
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

void FbxImporter::ReadAnimation(FbxNode* node,
                                FbxPropertyT<FbxDouble3>* property,
                                motive::MatrixOpId base_id,
                                motive::MatrixOperationType base_op,
                                bool invert) {
  auto iter = node_to_bone_map_.find(node);
  if (iter == node_to_bone_map_.end()) {
    return;
  }
  const int bone = iter->second;

  FbxAnimationReader reader(property, tolerances_, base_id, base_op, invert);

  // Loop through each channel, reading the animation data. We cannot simply
  // iterate through channels (ie. 1, 2, 3). Instead, we use the index order
  // as returned by ChannelOrder.
  const int* channel_order = ChannelOrder(node, base_op, invert);
  for (int channel_idx = 0; channel_idx < 3; ++channel_idx) {
    const int channel = channel_order[channel_idx];

    // Calculate the actual operation type and id for this channel.
    const motive::MatrixOperationType op =
        static_cast<motive::MatrixOperationType>(base_op + channel);
    const motive::MatrixOpId id =
        static_cast<motive::MatrixOpId>(base_id + channel_idx);

    Optional<float> const_value = reader.GetConstValue(channel);
    if (const_value) {
      // Do not record a const value if it is the same as the default value.
      if (anim_->IsDefaultValue(op, *const_value)) {
        continue;
      }

      // Record constant value for this channel.
      const FlatChannelId channel_id = anim_->AllocChannel(bone, op, id);
      anim_->AddConstant(channel_id, *const_value);
    } else {
      // There is no curve data to record.
      const std::vector<AnimationCurveSegment> segments =
          reader.GetCurveSegments(channel);
      if (segments.empty()) {
        continue;
      }

      const FlatChannelId channel_id = anim_->AllocChannel(bone, op, id);
      for (const AnimationCurveSegment& segment : segments) {
        anim_->AddCurve(channel_id, segment.start_time, segment.end_time,
                        segment.values, segment.derivatives,
                        AnimationCurveSegment::kNumPoints);
      }
      // Remove redundant nodes from the final curve.
      anim_->PruneNodes(channel_id);
    }
  }
}

void FbxImporter::BuildBoneAnimation(FbxNode* node) {
  // The FBX tranform format is defined as below (see
  // http://help.autodesk.com/view/FBX/2016/ENU/?guid=__files_GUID_10CDD63C_79C1_4F2D_BB28_AD2BE65A02ED_htm):
  //
  // WorldTransform = ParentWorldTransform * T * Roff * Rp * Rpre * R *
  //                  Rpost_inv * Rp_inv * Soff * Sp * S * Sp_inv
  //
  ReadAnimation(node, &node->LclTranslation, 0, motive::kTranslateX, false);
  ReadAnimation(node, &node->RotationOffset, 0, motive::kTranslateX, false);
  ReadAnimation(node, &node->RotationPivot, 0, motive::kTranslateX, false);
  ReadAnimation(node, &node->PreRotation, 3, motive::kRotateAboutX, false);
  ReadAnimation(node, &node->LclRotation, 6, motive::kRotateAboutX, false);
  ReadAnimation(node, &node->PostRotation, 9, motive::kRotateAboutX, true);
  ReadAnimation(node, &node->RotationPivot, 12, motive::kTranslateX, true);
  ReadAnimation(node, &node->ScalingOffset, 12, motive::kTranslateX, false);
  ReadAnimation(node, &node->ScalingPivot, 12, motive::kTranslateX, false);
  ReadAnimation(node, &node->LclScaling, 15, motive::kScaleX, false);
  ReadAnimation(node, &node->ScalingPivot, 19, motive::kTranslateX, true);
  anim_->PruneChannels(no_uniform_scale_);
}

struct ImportArgs {
  /// Amount output curves can deviate from input.
  Tolerances tolerances;
  /// If true, never collapse scale channels.
  bool no_uniform_scale = false;
  /// Which axes are up, front, left.
  AxisSystem axis_system = AxisSystem_Unspecified;
  /// This number of cm is set to one unit.
  float distance_unit_scale = -1.f;
};

Animation ImportFbx(const std::string& filename) {
  ImportArgs args;
  FbxImporter importer(args.tolerances, args.no_uniform_scale);
  return importer.Load(filename, args.axis_system, args.distance_unit_scale);
}

}  // namespace tool
}  // namespace lull
