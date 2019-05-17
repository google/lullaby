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

#include <unordered_map>

#include "lullaby/util/filename.h"
#include "lullaby/util/math.h"
#include "lullaby/tools/anim_pipeline/animation.h"
#include "lullaby/tools/anim_pipeline/import_options.h"
#include "lullaby/tools/common/assimp_base_importer.h"

namespace lull {
namespace tool {

namespace {

// The number of intermediate samples to take between keyframes so that curve
// derivatives approximate how assimp computes intermediate values and the
// interpolation values for those intermediate samples.
constexpr std::array<float, 3> kSamplePercentages = {1.f / 4.f, 2.f / 4.f,
                                                     3.f / 4.f};

}  // namespace

class AssetImporter : public AssimpBaseImporter {
 public:
  AssetImporter() {}
  Animation Import(const std::string& filename, const ImportOptions& opts);

 private:
  void BuildBoneAnimation(const aiAnimation* animation, const aiNode* bone,
                          int index);
  void AddChannel(const AnimCurve& curve, int bone, motive::MatrixOpId id);
  Animation* anim_ = nullptr;
};

Animation AssetImporter::Import(const std::string& filename,
                                const ImportOptions& opts) {
  Animation anim(RemoveDirectoryAndExtensionFromFilename(filename),
                 opts.tolerances, false);

  AssimpBaseImporter::Options assimp_opts;
  assimp_opts.axis_system = opts.axis_system;
  assimp_opts.scale_multiplier = opts.scale_multiplier;
  assimp_opts.require_thread_safe = opts.desire_thread_safe;

  const bool success = LoadScene(filename, assimp_opts);

  if (success) {
    if (GetScene()->mNumAnimations == 0 ||
        GetScene()->mAnimations[0] == nullptr) {
      return anim;
    }

    anim_ = &anim;

    std::unordered_map<const aiNode*, int> node_to_bone_map;
    const aiAnimation* ai_animation = GetScene()->mAnimations[0];
    ForEachBone([&](const aiNode* bone, const aiNode* parent,
                    const aiMatrix4x4& transform) {
      auto iter = node_to_bone_map.find(parent);
      const int parent_index =
          iter != node_to_bone_map.end() ? iter->second : -1;
      const char* name = bone->mName.C_Str();
      const int bone_index = anim_->RegisterBone(name, parent_index);
      node_to_bone_map[bone] = bone_index;
      BuildBoneAnimation(ai_animation, bone, bone_index);
      anim_->PruneChannels(opts.no_uniform_scale);
    });
    anim_ = nullptr;
  }

  return anim;
}

void AssetImporter::AddChannel(const AnimCurve& curve, int bone,
                               motive::MatrixOpId id) {
  Optional<float> const_value = curve.GetConstValue(anim_->GetTolerances());
  if (const_value) {
    if (anim_->IsDefaultValue(curve.type, *const_value)) {
      // Do not record a const value if it is the same as the default value.
      return;
    }
    const FlatChannelId channel = anim_->AllocChannel(bone, curve.type, id);
    anim_->AddConstant(channel, *const_value);
  } else {
    const FlatChannelId channel = anim_->AllocChannel(bone, curve.type, id);
    anim_->AddCurve(channel, curve);
    anim_->PruneNodes(channel);
  }
}

static void ReadCurveValue(AnimCurve* x, AnimCurve* y, AnimCurve* z,
                           const aiVectorKey& key, double assimp_time_to_ms) {
  const float time = key.mTime * assimp_time_to_ms;
  x->AddNode(time, key.mValue.x);
  y->AddNode(time, key.mValue.y);
  z->AddNode(time, key.mValue.z);
}

static mathfu::vec3 ReadCurveValue(AnimCurve* x, AnimCurve* y, AnimCurve* z,
                                   const aiQuatKey& key,
                                   const mathfu::vec3& prev,
                                   double assimp_time_to_ms) {
  const mathfu::quat rotation(key.mValue.w, key.mValue.x, key.mValue.y,
                              key.mValue.z);
  const mathfu::vec3 angles = EulerFilter(rotation.ToEulerAngles(), prev);
  const float time = key.mTime * assimp_time_to_ms;
  x->AddNode(time, angles.x);
  y->AddNode(time, angles.y);
  z->AddNode(time, angles.z);
  return angles;
}

void ReadVectorCurve(AnimCurve* x, AnimCurve* y, AnimCurve* z,
                     const aiVectorKey* keys, unsigned int num_keys,
                     double assimp_time_to_ms) {
  if (num_keys == 0) {
    return;
  }

  ReadCurveValue(x, y, z, keys[0], assimp_time_to_ms);

  for (unsigned int i = 1; i < num_keys; ++i) {
    const aiVectorKey& curr = keys[i];
    const aiVectorKey& prev = keys[i - 1];
    const float dt = curr.mTime - prev.mTime;
    for (float percent : kSamplePercentages) {
      aiVectorKey interp;
      interp.mTime = prev.mTime + (dt * percent);
      Assimp::Interpolator<aiVectorKey>()(interp.mValue, prev, curr, percent);
      ReadCurveValue(x, y, z, interp, assimp_time_to_ms);
    }
    ReadCurveValue(x, y, z, curr, assimp_time_to_ms);
  }

  x->GenerateDerivatives();
  y->GenerateDerivatives();
  z->GenerateDerivatives();
}

void ReadQuaternionCurve(AnimCurve* x, AnimCurve* y, AnimCurve* z,
                         const aiQuatKey* keys, unsigned int num_keys,
                         double assimp_time_to_ms) {
  if (num_keys == 0) {
    return;
  }

  mathfu::vec3 prev_node = mathfu::kZeros3f;
  prev_node = ReadCurveValue(x, y, z, keys[0], prev_node, assimp_time_to_ms);

  for (unsigned int i = 1; i < num_keys; ++i) {
    const aiQuatKey& curr = keys[i];
    const aiQuatKey& prev = keys[i - 1];
    const float dt = curr.mTime - prev.mTime;
    for (float percent : kSamplePercentages) {
      aiQuatKey interp;
      interp.mTime = prev.mTime + (dt * percent);
      Assimp::Interpolator<aiQuatKey>()(interp.mValue, prev, curr, percent);
      prev_node = ReadCurveValue(x, y, z, interp, prev_node, assimp_time_to_ms);
    }
    prev_node = ReadCurveValue(x, y, z, curr, prev_node, assimp_time_to_ms);
  }

  x->AdjustForModularAngles();
  y->AdjustForModularAngles();
  z->AdjustForModularAngles();

  x->GenerateDerivatives();
  y->GenerateDerivatives();
  z->GenerateDerivatives();
}

void AssetImporter::BuildBoneAnimation(const aiAnimation* animation,
                                       const aiNode* bone, int bone_index) {
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
  // so multiplying by 1000 gives milliseconds (motiveanim's unit). If no tick
  // rate is specified, the key times are correct as-is.
  const double ticks_per_second = animation->mTicksPerSecond;
  const double assimp_time_to_ms =
      ticks_per_second == 0.0 ? 1.0 : 1000.0 / ticks_per_second;

  // Create a curve for each component of the transform, but only if the
  // appropriate node actually exists.
  if (translation_node != nullptr) {
    const unsigned int num_keys = translation_node->mNumPositionKeys;
    AnimCurve tx(motive::kTranslateX, num_keys);
    AnimCurve ty(motive::kTranslateY, num_keys);
    AnimCurve tz(motive::kTranslateZ, num_keys);
    ReadVectorCurve(&tx, &ty, &tz, translation_node->mPositionKeys, num_keys,
                    assimp_time_to_ms);
    AddChannel(tx, bone_index, 0);
    AddChannel(ty, bone_index, 1);
    AddChannel(tz, bone_index, 2);
  }

  if (rotation_node != nullptr) {
    const unsigned int num_keys = rotation_node->mNumRotationKeys;
    AnimCurve rx(motive::kRotateAboutX, num_keys);
    AnimCurve ry(motive::kRotateAboutY, num_keys);
    AnimCurve rz(motive::kRotateAboutZ, num_keys);
    ReadQuaternionCurve(&rx, &ry, &rz, rotation_node->mRotationKeys, num_keys,
                        assimp_time_to_ms);

    // Rotations must be specified in z, y, x, order.
    AddChannel(rz, bone_index, 3);
    AddChannel(ry, bone_index, 4);
    AddChannel(rx, bone_index, 5);
  }

  if (scale_node != nullptr) {
    const unsigned int num_keys = scale_node->mNumScalingKeys;
    AnimCurve sx(motive::kScaleX, num_keys);
    AnimCurve sy(motive::kScaleY, num_keys);
    AnimCurve sz(motive::kScaleZ, num_keys);
    ReadVectorCurve(&sx, &sy, &sz, scale_node->mScalingKeys, num_keys,
                    assimp_time_to_ms);
    AddChannel(sx, bone_index, 6);
    AddChannel(sy, bone_index, 7);
    AddChannel(sz, bone_index, 8);
  }
}

std::vector<Animation> ImportAsset(const std::string& filename,
                                   const ImportOptions& opts) {
  AssetImporter importer;
  std::vector<Animation> result;
  result.push_back(importer.Import(filename, opts));
  return result;
}

}  // namespace tool
}  // namespace lull
