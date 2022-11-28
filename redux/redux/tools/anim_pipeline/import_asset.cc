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

#include "absl/container/flat_hash_map.h"
#include "assimp/DefaultIOSystem.h"
#include "assimp/DefaultLogger.hpp"
#include "assimp/GltfMaterial.h"
#include "assimp/Importer.hpp"
#include "assimp/material.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "redux/modules/base/filepath.h"
#include "redux/tools/anim_pipeline/animation.h"
#include "redux/tools/anim_pipeline/import_options.h"
#include "redux/tools/common/assimp_utils.h"

namespace redux::tool {

// The number of intermediate samples to take between keyframes so that curve
// derivatives approximate how assimp computes intermediate values and the
// interpolation values for those intermediate samples.
constexpr std::array<float, 3> kSamplePercentages = {1.0f / 4.0f, 2.0f / 4.0f,
                                                     3.0f / 4.0f};

class AssetImporter : public AssimpBaseImporter {
 public:
  AssetImporter() {}
  AnimationPtr Import(std::string_view uri, const ImportOptions& opts);

 private:
  void BuildBoneAnimation(const aiAnimation* animation, const aiNode* bone,
                          BoneIndex index);

  void AddChannel(const AnimCurve& curve, int bone);

  template <AnimChannelType OpType, typename AiKey>
  void ReadCurve(const AiKey* keys, unsigned int num_keys, BoneIndex bone_index,
                 double time_to_ms);

  AnimationPtr anim_;
};

AnimationPtr AssetImporter::Import(std::string_view uri,
                                   const ImportOptions& opts) {
  AssimpBaseImporter::Options assimp_opts;
  assimp_opts.axis_system = opts.axis_system;
  assimp_opts.scale_multiplier = opts.scale_multiplier;
  assimp_opts.require_thread_safe = opts.desire_thread_safe;

  const bool success = LoadScene(std::string(uri), assimp_opts);
  if (!success) {
    return nullptr;
  }
  if (GetScene()->mNumAnimations == 0 ||
      GetScene()->mAnimations[0] == nullptr) {
    return nullptr;
  }

  anim_ = std::make_shared<Animation>(opts.tolerances);

  absl::flat_hash_map<const aiNode*, BoneIndex> node_to_bone_map;
  const aiAnimation* ai_animation = GetScene()->mAnimations[0];
  ForEachBone([&](const aiNode* bone, const aiNode* parent,
                  const aiMatrix4x4& transform) {
    auto iter = node_to_bone_map.find(parent);
    const BoneIndex parent_index =
        iter != node_to_bone_map.end() ? iter->second : kInvalidBoneIdx;
    const char* name = bone->mName.C_Str();
    const BoneIndex bone_index = anim_->RegisterBone(name, parent_index);
    node_to_bone_map[bone] = bone_index;
    BuildBoneAnimation(ai_animation, bone, bone_index);
    anim_->FinishBone(bone_index);
  });
  return anim_;
}

template <AnimChannelType Key, typename AiKey>
static float ExtractValue(const AiKey& key) {
  if constexpr (Key == AnimChannelType::TranslateX) {
    return key.mValue.x;
  } else if constexpr (Key == AnimChannelType::TranslateY) {
    return key.mValue.y;
  } else if constexpr (Key == AnimChannelType::TranslateZ) {
    return key.mValue.z;
  } else if constexpr (Key == AnimChannelType::QuaternionX) {
    return key.mValue.x;
  } else if constexpr (Key == AnimChannelType::QuaternionY) {
    return key.mValue.y;
  } else if constexpr (Key == AnimChannelType::QuaternionZ) {
    return key.mValue.z;
  } else if constexpr (Key == AnimChannelType::QuaternionW) {
    return key.mValue.w;
  } else if constexpr (Key == AnimChannelType::ScaleX) {
    return key.mValue.x;
  } else if constexpr (Key == AnimChannelType::ScaleY) {
    return key.mValue.y;
  } else if constexpr (Key == AnimChannelType::ScaleZ) {
    return key.mValue.z;
  }
}

template <AnimChannelType OpType, typename AiKey>
void AddCurveValue(AnimCurve* curve, const AiKey& key, double time_to_ms) {
  const float time = key.mTime * time_to_ms;
  curve->AddNode(time, ExtractValue<OpType>(key));
}

template <AnimChannelType OpType, typename AiKey>
void AssetImporter::ReadCurve(const AiKey* keys, unsigned int num_keys,
                              BoneIndex bone_index, double time_to_ms) {
  AnimCurve curve(OpType, num_keys);
  if (num_keys > 0) {
    AddCurveValue<OpType>(&curve, keys[0], time_to_ms);
    for (unsigned int i = 1; i < num_keys; ++i) {
      const AiKey& curr = keys[i];
      const AiKey& prev = keys[i - 1];
      const float dt = curr.mTime - prev.mTime;
      for (float percent : kSamplePercentages) {
        AiKey interp;
        interp.mTime = prev.mTime + (dt * percent);
        Assimp::Interpolator<AiKey>()(interp.mValue, prev, curr, percent);
        AddCurveValue<OpType>(&curve, interp, time_to_ms);
      }
      AddCurveValue<OpType>(&curve, curr, time_to_ms);
    }
    curve.Finish(anim_->GetTolerances());
  }

  AnimBone& bone = anim_->GetMutableBone(bone_index);
  bone.curves.push_back(std::move(curve));
}

void AssetImporter::BuildBoneAnimation(const aiAnimation* animation,
                                       const aiNode* bone,
                                       BoneIndex bone_index) {
  // assimp may split a bone's animation into multiple nodes and some bones may
  // not have one of their components animated.
  const aiNodeAnim* translation_node = nullptr;
  const aiNodeAnim* rotation_node = nullptr;
  const aiNodeAnim* scale_node = nullptr;
  for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
    const aiNodeAnim* node_anim = animation->mChannels[i];
    // When assimp splits a bone's animations, it will add suffixes to the name.
    if (!strncmp(bone->mName.C_Str(), node_anim->mNodeName.C_Str(),
                 bone->mName.length)) {
      // A node represents a position, rotation or scale animation if it has
      // more than one key of that type. The same node can represent all 3.
      if (node_anim->mNumPositionKeys > 1) {
        translation_node = node_anim;
      }
      if (node_anim->mNumRotationKeys > 1) {
        rotation_node = node_anim;
      }
      if (node_anim->mNumScalingKeys > 1) {
        scale_node = node_anim;
      }
    }
  }
  if (translation_node == nullptr && rotation_node == nullptr &&
      scale_node == nullptr) {
    return;
  }

  // If the source file specifies a framerate, assimp stores key times as
  // integral "tick" values instead of actual time values. In these cases,
  // dividing by aiAnimation->mTicksPerSecond gives the time value in seconds,
  // so multiplying by 1000 gives milliseconds. If no tick rate is specified,
  // the key times are correct as-is.
  const double ticks_per_second = animation->mTicksPerSecond;
  const double time_to_ms =
      ticks_per_second == 0.0 ? 1000.0 : 1000.0 / ticks_per_second;

  // Create a curve for each component of the transform, but only if the
  // appropriate node actually exists.
  if (translation_node != nullptr) {
    const unsigned int num_keys = translation_node->mNumPositionKeys;
    ReadCurve<AnimChannelType::TranslateX>(translation_node->mPositionKeys,
                                           num_keys, bone_index, time_to_ms);
    ReadCurve<AnimChannelType::TranslateY>(translation_node->mPositionKeys,
                                           num_keys, bone_index, time_to_ms);
    ReadCurve<AnimChannelType::TranslateZ>(translation_node->mPositionKeys,
                                           num_keys, bone_index, time_to_ms);
  }

  if (rotation_node != nullptr) {
    const unsigned int num_keys = rotation_node->mNumRotationKeys;
    ReadCurve<AnimChannelType::QuaternionX>(translation_node->mRotationKeys,
                                            num_keys, bone_index, time_to_ms);
    ReadCurve<AnimChannelType::QuaternionY>(translation_node->mRotationKeys,
                                            num_keys, bone_index, time_to_ms);
    ReadCurve<AnimChannelType::QuaternionZ>(translation_node->mRotationKeys,
                                            num_keys, bone_index, time_to_ms);
    ReadCurve<AnimChannelType::QuaternionW>(translation_node->mRotationKeys,
                                            num_keys, bone_index, time_to_ms);
  }

  if (scale_node != nullptr) {
    const unsigned int num_keys = scale_node->mNumScalingKeys;
    ReadCurve<AnimChannelType::ScaleX>(translation_node->mScalingKeys, num_keys,
                                       bone_index, time_to_ms);
    ReadCurve<AnimChannelType::ScaleY>(translation_node->mScalingKeys, num_keys,
                                       bone_index, time_to_ms);
    ReadCurve<AnimChannelType::ScaleZ>(translation_node->mScalingKeys, num_keys,
                                       bone_index, time_to_ms);
  }
}

AnimationPtr ImportAsset(std::string_view uri, const ImportOptions& opts) {
  AssetImporter importer;
  return importer.Import(uri, opts);
}

}  // namespace redux::tool
