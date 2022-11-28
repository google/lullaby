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

#include <fbxsdk.h>

#include "absl/container/flat_hash_map.h"
#include "redux/modules/base/filepath.h"
#include "redux/modules/math/transform.h"
#include "redux/tools/anim_pipeline/animation.h"
#include "redux/tools/anim_pipeline/import_options.h"
#include "redux/tools/common/fbx_utils.h"

namespace redux::tool {

// Strips namespaces that are added to bone node names on export from Maya.
static const char* BoneBaseName(const std::string& name) {
  const size_t colon = name.find_last_of(':');
  const size_t base_idx = colon == std::string::npos ? 0 : colon + 1;
  return &name[base_idx];
}

class FbxImporter : public FbxBaseImporter {
 public:
  FbxImporter() {}
  AnimationPtr Import(std::string_view uri, const ImportOptions& opts);

 private:
  struct Keyframe {
    float time_s = 0.0f;
    Transform transform;
  };

  void LoadAnimation(FbxAnimStack* anim_stack);
  void BuildBoneAnimation(FbxNode* node, BoneIndex bone_index);
  Keyframe ReadKeyframe(FbxNode* node, double time);

  template <AnimChannelType Key>
  void AddChannel(const std::vector<Keyframe>& keyframes, BoneIndex bone);

  AnimationPtr anim_ = nullptr;
};

AnimationPtr FbxImporter::Import(std::string_view uri,
                                 const ImportOptions& opts) {
  FbxBaseImporter::Options fbx_opts;
  fbx_opts.axis_system = opts.axis_system;
  fbx_opts.cm_per_unit = opts.cm_per_unit;
  fbx_opts.scale_multiplier = opts.scale_multiplier;

  const bool success = LoadScene(std::string(uri), fbx_opts);
  if (!success) {
    return nullptr;
  }

  anim_ = std::make_shared<Animation>(opts.tolerances);
  LoadAnimation(nullptr);
  return anim_;
}

FbxImporter::Keyframe FbxImporter::ReadKeyframe(FbxNode* node, double time) {
  FbxTime fbx_time;
  fbx_time.SetSecondDouble(time);
  FbxAMatrix fbx_transform = node->EvaluateLocalTransform(fbx_time);
  Keyframe keyframe;
  keyframe.time_s = static_cast<float>(time);
  keyframe.transform.translation = Vec4FromFbx(fbx_transform.GetT()).xyz();
  keyframe.transform.rotation = QuatFromFbx(fbx_transform.GetQ());
  keyframe.transform.scale = Vec4FromFbx(fbx_transform.GetS()).xyz();
  return keyframe;
}

void FbxImporter::LoadAnimation(FbxAnimStack* anim_stack) {
  absl::flat_hash_map<FbxNode*, BoneIndex> node_to_bone_map;
  ForEachBone([&](FbxNode* node, FbxNode* parent) {
    auto iter = node_to_bone_map.find(parent);
    const BoneIndex parent_index =
        iter != node_to_bone_map.end() ? iter->second : kInvalidBoneIdx;
    const BoneIndex bone_index =
        anim_->RegisterBone(BoneBaseName(node->GetName()), parent_index);
    node_to_bone_map[node] = bone_index;
  });

  ForEachBone([&](FbxNode* node, FbxNode* parent) {
    auto iter = node_to_bone_map.find(node);
    if (iter == node_to_bone_map.end()) {
      return;
    }
    BuildBoneAnimation(node, iter->second);
    anim_->FinishBone(iter->second);
  });
}

template <AnimChannelType Key>
static float ExtractValue(const Transform& transform) {
  if constexpr (Key == AnimChannelType::TranslateX) {
    return transform.translation.x;
  } else if constexpr (Key == AnimChannelType::TranslateY) {
    return transform.translation.y;
  } else if constexpr (Key == AnimChannelType::TranslateZ) {
    return transform.translation.z;
  } else if constexpr (Key == AnimChannelType::QuaternionX) {
    return transform.rotation.x;
  } else if constexpr (Key == AnimChannelType::QuaternionY) {
    return transform.rotation.y;
  } else if constexpr (Key == AnimChannelType::QuaternionZ) {
    return transform.rotation.z;
  } else if constexpr (Key == AnimChannelType::QuaternionW) {
    return transform.rotation.w;
  } else if constexpr (Key == AnimChannelType::ScaleX) {
    return transform.scale.x;
  } else if constexpr (Key == AnimChannelType::ScaleY) {
    return transform.scale.y;
  } else if constexpr (Key == AnimChannelType::ScaleZ) {
    return transform.scale.z;
  }
}

template <AnimChannelType Key>
void FbxImporter::AddChannel(const std::vector<Keyframe>& keyframes,
                             BoneIndex bone) {
  const float kSecondsToMilliseconds = 1000.0f;
  AnimCurve curve(Key, keyframes.size());
  for (const Keyframe& keyframe : keyframes) {
    curve.AddNode(keyframe.time_s * kSecondsToMilliseconds,
                  ExtractValue<Key>(keyframe.transform));
  }
  curve.Finish(anim_->GetTolerances());
  AnimBone& anim_bone = anim_->GetMutableBone(bone);
  anim_bone.curves.emplace_back(std::move(curve));
}

void FbxImporter::BuildBoneAnimation(FbxNode* node, BoneIndex bone_index) {
  const double kHz = 1.0 / 120.0;

  FbxTimeSpan span;
  const bool ok = node->GetAnimationInterval(span);
  if (!ok) {
    return;
  }
  const FbxTime start = span.GetStart().Get();
  const FbxTime end = span.GetStop();

  std::vector<Keyframe> keyframes;
  keyframes.reserve((end.GetSecondDouble() - start.GetSecondDouble()) / kHz);

  double cur_time = start.GetSecondDouble();
  while (cur_time < end.GetSecondDouble()) {
    keyframes.emplace_back(ReadKeyframe(node, cur_time));
    cur_time += kHz;
  }
  keyframes.emplace_back(ReadKeyframe(node, end.GetSecondDouble()));

  AddChannel<AnimChannelType::TranslateX>(keyframes, bone_index);
  AddChannel<AnimChannelType::TranslateY>(keyframes, bone_index);
  AddChannel<AnimChannelType::TranslateZ>(keyframes, bone_index);
  AddChannel<AnimChannelType::QuaternionX>(keyframes, bone_index);
  AddChannel<AnimChannelType::QuaternionY>(keyframes, bone_index);
  AddChannel<AnimChannelType::QuaternionZ>(keyframes, bone_index);
  AddChannel<AnimChannelType::QuaternionW>(keyframes, bone_index);
  AddChannel<AnimChannelType::ScaleX>(keyframes, bone_index);
  AddChannel<AnimChannelType::ScaleY>(keyframes, bone_index);
  AddChannel<AnimChannelType::ScaleZ>(keyframes, bone_index);
}

AnimationPtr ImportFbx(std::string_view uri, const ImportOptions& opts) {
  FbxImporter importer;
  return importer.Import(uri, opts);
}

}  // namespace redux::tool
